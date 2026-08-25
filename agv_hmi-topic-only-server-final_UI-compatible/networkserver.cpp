#include "networkserver.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QHostAddress>
#include <QStringList>

NetworkServer::NetworkServer(QObject *parent)
    : QObject(parent),
      m_tcpServer(new QTcpServer(this)) {
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &NetworkServer::onNewConnection);
}

NetworkServer::~NetworkServer() {
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
}

void NetworkServer::startServer(quint16 port) {
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        qCritical() << "[NetworkServer] Failed to start:"
                    << m_tcpServer->errorString();
        return;
    }

    qInfo() << "[NetworkServer] Listening on 0.0.0.0:" << port;
    qInfo() << "[NetworkServer] Web HMI / Qt client API:"
            << "http://<server-ip>:" << port;
}

void NetworkServer::onNewConnection() {
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();

        m_buffers.insert(socket, QByteArray());

        connect(socket, &QTcpSocket::readyRead,
                this, &NetworkServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected,
                this, &NetworkServer::onDisconnected);
    }
}

void NetworkServer::onDisconnected() {
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }

    m_buffers.remove(socket);
    socket->deleteLater();
}

void NetworkServer::onReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }

    m_buffers[socket].append(socket->readAll());

    const QByteArray &buffer = m_buffers[socket];
    const int headerEnd = buffer.indexOf("\r\n\r\n");

    if (headerEnd < 0) {
        return;
    }

    const QByteArray header = buffer.left(headerEnd);
    const QByteArray body = buffer.mid(headerEnd + 4);

    const QList<QByteArray> lines = header.split('\n');

    int contentLength = 0;
    for (QByteArray line : lines) {
        line = line.trimmed();
        const QByteArray lowerLine = line.toLower();

        if (lowerLine.startsWith("content-length:")) {
            const int colon = line.indexOf(':');
            if (colon >= 0) {
                contentLength =
                    line.mid(colon + 1).trimmed().toInt();
            }
            break;
        }
    }

    if (body.size() < contentLength) {
        return;
    }

    const int requestSize = headerEnd + 4 + contentLength;
    const QByteArray request = buffer.left(requestSize);

    m_buffers[socket].remove(0, requestSize);
    processRequest(socket, request);
}

void NetworkServer::processRequest(QTcpSocket *socket,
                                   const QByteArray &requestData) {
    const int headerEnd = requestData.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return;
    }

    const QString headerText =
        QString::fromUtf8(requestData.left(headerEnd));

    const QByteArray body =
        requestData.mid(headerEnd + 4);

    const QStringList headerLines = headerText.split("\r\n");
    if (headerLines.isEmpty()) {
        return;
    }

    const QStringList requestLine =
        headerLines.first().split(' ', Qt::SkipEmptyParts);

    if (requestLine.size() < 2) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid HTTP request";
        sendHttpResponse(socket, 400, res);
        return;
    }

    const QString method = requestLine.at(0).toUpper();
    const QString path = requestLine.at(1);

    qInfo() << "[NetworkServer]" << method << path;

    if (method == "OPTIONS") {
        QJsonObject res;
        res["result"] = "success";
        sendHttpResponse(socket, 200, res);
        return;
    }

    if (method == "GET" && path == "/api/status") {
        handleGetStatus(socket);
        return;
    }

    if (method == "POST" && path == "/api/command") {
        handlePostCommand(socket, body);
        return;
    }

    if (method == "POST" && path == "/api/manual-mode") {
        handlePostManualMode(socket, body);
        return;
    }

    // 로봇 제어부/도착 감지부에서 실제 상태를 반영할 때 사용합니다.
    if (method == "POST" && path == "/api/status") {
        handlePostStatus(socket, body);
        return;
    }

    QJsonObject res;
    res["result"] = "fail";
    res["error"] = "Not Found";
    sendHttpResponse(socket, 404, res);
}

void NetworkServer::handlePostCommand(QTcpSocket *socket,
                                      const QByteArray &body) {
    QJsonParseError parseError;
    const QJsonDocument jsonDoc =
        QJsonDocument::fromJson(body, &parseError);

    if (parseError.error != QJsonParseError::NoError ||
        !jsonDoc.isObject()) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid JSON format";
        sendHttpResponse(socket, 400, res);
        return;
    }

    const QJsonObject request = jsonDoc.object();

    // 안내 취소: {"command":"cancel"}
    if (request.value("command").toString().trimmed() == "cancel") {
        m_robotStatus = "idle";

        QJsonObject res;
        res["result"] = "success";
        res["message"] = "Goal cancelled";
        res["status"] = m_robotStatus;
        sendHttpResponse(socket, 200, res);
        return;
    }

    // 자동주행 목적지: {"destination":"room_301"}
    if (!request.contains("destination")) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "destination is required";
        sendHttpResponse(socket, 400, res);
        return;
    }

    const QString destination =
        request.value("destination").toString().trimmed();

    if (destination.isEmpty() || !m_topicPublisher) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid destination or topic publisher unavailable";
        sendHttpResponse(socket, 400, res);
        return;
    }

    // 이 서버는 수동 주행(cmd_vel)을 처리하지 않습니다.
    // manual_mode=true일 때는 사용자 Web HMI의 목적지 명령만 차단합니다.
    if (m_manualMode) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "manual_mode is active";
        res["manual_mode"] = true;
        sendHttpResponse(socket, 409, res);
        return;
    }

    const bool accepted =
        m_topicPublisher->publishGoal(destination.toStdString());

    QJsonObject res;

    if (!accepted) {
        res["result"] = "fail";
        res["error"] = "등록되지 않았거나 좌표가 설정되지 않은 목적지입니다: " + destination;
        sendHttpResponse(socket, 400, res);
        return;
    }

    m_robotStatus = "moving";

    res["result"] = "success";
    res["message"] = "Goal topic published";
    res["destination"] = destination;
    res["status"] = m_robotStatus;

    sendHttpResponse(socket, 200, res);
}

void NetworkServer::handlePostManualMode(QTcpSocket *socket,
                                         const QByteArray &body) {
    QJsonParseError parseError;
    const QJsonDocument jsonDoc =
        QJsonDocument::fromJson(body, &parseError);

    if (parseError.error != QJsonParseError::NoError ||
        !jsonDoc.isObject()) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid JSON format";
        sendHttpResponse(socket, 400, res);
        return;
    }

    const QJsonObject request = jsonDoc.object();

    if (!request.contains("manual_mode") ||
        !request.value("manual_mode").isBool()) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "manual_mode boolean is required";
        sendHttpResponse(socket, 400, res);
        return;
    }

    m_manualMode = request.value("manual_mode").toBool();

    if (m_manualMode) {
        m_robotStatus = "idle";
    }

    QJsonObject res;
    res["result"] = "success";
    res["manual_mode"] = m_manualMode;
    res["status"] = m_robotStatus;

    qInfo() << "[NetworkServer] manual_mode =" << m_manualMode;

    sendHttpResponse(socket, 200, res);
}


void NetworkServer::handlePostStatus(QTcpSocket *socket,
                                     const QByteArray &body) {
    QJsonParseError parseError;
    const QJsonDocument jsonDoc =
        QJsonDocument::fromJson(body, &parseError);

    if (parseError.error != QJsonParseError::NoError ||
        !jsonDoc.isObject()) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid JSON format";
        sendHttpResponse(socket, 400, res);
        return;
    }

    const QString status =
        jsonDoc.object().value("status").toString().trimmed();

    if (status != "idle" &&
        status != "moving" &&
        status != "arrived") {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "status must be idle, moving, or arrived";
        sendHttpResponse(socket, 400, res);
        return;
    }

    m_robotStatus = status;

    QJsonObject res;
    res["result"] = "success";
    res["status"] = m_robotStatus;
    res["manual_mode"] = m_manualMode;

    qInfo() << "[NetworkServer] status =" << m_robotStatus;

    sendHttpResponse(socket, 200, res);
}

void NetworkServer::handleGetStatus(QTcpSocket *socket) {
    QJsonObject res;
    res["status"] = m_robotStatus;
    res["manual_mode"] = m_manualMode;
    res["wheel_rpm"] = m_wheelRpm;
    res["camera_url"] = m_cameraUrl;
    res["server"] = "agv_hmi_topic_only";

    sendHttpResponse(socket, 200, res);
}

void NetworkServer::sendHttpResponse(QTcpSocket *socket,
                                     int statusCode,
                                     const QJsonObject &jsonObj) {
    const QByteArray body =
        QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);

    QString statusText = "Error";

    switch (statusCode) {
    case 200:
        statusText = "OK";
        break;
    case 400:
        statusText = "Bad Request";
        break;
    case 404:
        statusText = "Not Found";
        break;
    case 409:
        statusText = "Conflict";
        break;
    default:
        break;
    }

    const QByteArray header =
        QString(
            "HTTP/1.1 %1 %2\r\n"
            "Content-Type: application/json; charset=utf-8\r\n"
            "Content-Length: %3\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Connection: close\r\n\r\n")
            .arg(statusCode)
            .arg(statusText)
            .arg(body.size())
            .toUtf8();

    socket->write(header);
    socket->write(body);
    socket->flush();
    socket->disconnectFromHost();
}
