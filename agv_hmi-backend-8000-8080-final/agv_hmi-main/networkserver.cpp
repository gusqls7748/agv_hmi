#include "networkserver.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QHostAddress>
#include <QStringList>
#include <QDebug>

NetworkServer::NetworkServer(QObject* parent)
    : QObject(parent),
      m_tcpServer_(new QTcpServer(this)) {
    connect(m_tcpServer_, &QTcpServer::newConnection,
            this, &NetworkServer::onNewConnection);
}

NetworkServer::~NetworkServer() {
    if (m_tcpServer_->isListening()) {
        m_tcpServer_->close();
    }
}

bool NetworkServer::startServer(quint16 port) {
    // Backend API is intentionally exposed on LAN for the remote Qt client.
    // The Web HMI itself stays on its own 8000 port and calls 127.0.0.1:8080
    // on the same robot/server PC.
    if (!m_tcpServer_->listen(QHostAddress::Any, port)) {
        qCritical() << "[NetworkServer] Failed to start:"
                    << m_tcpServer_->errorString();
        return false;
    }

    qInfo() << "[NetworkServer] Listening on 0.0.0.0:" << port;
    qInfo() << "[NetworkServer] Qt client/API address: http://<server-ip>:" << port;
    qInfo() << "[NetworkServer] Web HMI should call http://127.0.0.1:" << port
            << "from the same machine";
    return true;
}

void NetworkServer::onNewConnection() {
    while (m_tcpServer_->hasPendingConnections()) {
        QTcpSocket* socket = m_tcpServer_->nextPendingConnection();
        m_buffers_.insert(socket, QByteArray());

        connect(socket, &QTcpSocket::readyRead,
                this, &NetworkServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected,
                this, &NetworkServer::onDisconnected);
    }
}

void NetworkServer::onDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }
    m_buffers_.remove(socket);
    socket->deleteLater();
}

void NetworkServer::onReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    m_buffers_[socket].append(socket->readAll());
    const QByteArray& buffer = m_buffers_[socket];

    const int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return;
    }

    const QByteArray header = buffer.left(headerEnd);
    const QByteArray body = buffer.mid(headerEnd + 4);

    int contentLength = 0;
    const QList<QByteArray> lines = header.split('\n');
    for (QByteArray line : lines) {
        line = line.trimmed();
        const QByteArray lowerLine = line.toLower();
        if (lowerLine.startsWith("content-length:")) {
            const int colon = line.indexOf(':');
            if (colon >= 0) {
                contentLength = line.mid(colon + 1).trimmed().toInt();
            }
            break;
        }
    }

    if (body.size() < contentLength) {
        return;
    }

    const int requestSize = headerEnd + 4 + contentLength;
    const QByteArray request = buffer.left(requestSize);
    m_buffers_[socket].remove(0, requestSize);
    processRequest(socket, request);
}

void NetworkServer::processRequest(QTcpSocket* socket,
                                   const QByteArray& requestData) {
    const int headerEnd = requestData.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return;
    }

    const QString headerText =
        QString::fromUtf8(requestData.left(headerEnd));
    const QByteArray body = requestData.mid(headerEnd + 4);

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

    if (method == "POST" &&
        (path == "/api/manual-mode" ||
         path == "/api/manual_mode" ||
         path == "/api/manual-control")) {
        handleManualMode(socket, body);
        return;
    }

    QJsonObject res;
    res["result"] = "fail";
    res["error"] = "Not Found";
    sendHttpResponse(socket, 404, res);
}

void NetworkServer::handleGetStatus(QTcpSocket* socket) {
    QJsonObject res;
    res["status"] = m_topicPublisher_ ?
        QString::fromStdString(m_topicPublisher_->status()) : QStringLiteral("idle");
    res["manual_mode"] = m_manualMode_;
    res["wheel_rpm"] = 0;
    res["camera_url"] = m_cameraUrl_;
    res["server"] = "agv_hmi_topic_only";

    sendHttpResponse(socket, 200, res);
}

void NetworkServer::handleManualMode(QTcpSocket* socket,
                                     const QByteArray& body) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid JSON format";
        sendHttpResponse(socket, 400, res);
        return;
    }

    const QJsonObject obj = doc.object();
    if (!obj.contains("manual_mode") || !obj.value("manual_mode").isBool()) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "manual_mode must be a JSON boolean";
        sendHttpResponse(socket, 400, res);
        return;
    }

    m_manualMode_ = obj.value("manual_mode").toBool();

    QJsonObject res;
    res["result"] = "success";
    res["manual_mode"] = m_manualMode_;
    sendHttpResponse(socket, 200, res);

    qInfo() << "[NetworkServer] manual_mode ->" << m_manualMode_;
}

void NetworkServer::handlePostCommand(QTcpSocket* socket,
                                      const QByteArray& body) {
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid JSON format";
        sendHttpResponse(socket, 400, res);
        return;
    }

    const QJsonObject req = doc.object();

    // Destination commands are the only robot-motion commands owned by this server.
    // The server publishes /goal_pose; it does not call Nav2 actions or /cmd_vel.
    if (req.contains("destination")) {
        const QString destination = req.value("destination").toString().trimmed();
        if (destination.isEmpty() || !m_topicPublisher_) {
            QJsonObject res;
            res["result"] = "fail";
            res["error"] = "Invalid destination or topic publisher unavailable";
            sendHttpResponse(socket, 400, res);
            return;
        }

        if (!m_topicPublisher_->hasDestination(destination.toStdString())) {
            QJsonObject res;
            res["result"] = "fail";
            res["error"] = "등록되지 않은 목적지입니다: " + destination;
            sendHttpResponse(socket, 400, res);
            return;
        }

        m_topicPublisher_->publishGoal(destination.toStdString());

        QJsonObject res;
        res["result"] = "success";
        res["message"] = "Goal topic published";
        res["destination"] = destination;
        res["status"] = "moving";
        sendHttpResponse(socket, 200, res);
        return;
    }

    // Keep the legacy API shape, but do not perform Nav2/cmd_vel control here.
    if (req.contains("command")) {
        const QString command = req.value("command").toString().trimmed();
        if (command == "cancel") {
            if (m_topicPublisher_) {
                m_topicPublisher_->setStatus("idle");
            }
            QJsonObject res;
            res["result"] = "success";
            res["message"] = "Cancel state accepted; robot-side cancellation remains external";
            res["status"] = "idle";
            sendHttpResponse(socket, 200, res);
            return;
        }

        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Manual motion is not handled by this topic-only server";
        sendHttpResponse(socket, 409, res);
        return;
    }

    QJsonObject res;
    res["result"] = "fail";
    res["error"] = "Missing destination or command";
    sendHttpResponse(socket, 400, res);
}

void NetworkServer::sendHttpResponse(QTcpSocket* socket,
                                     int statusCode,
                                     const QJsonObject& jsonObject) {
    const QByteArray body =
        QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);

    QString statusText = "Error";
    switch (statusCode) {
    case 200: statusText = "OK"; break;
    case 400: statusText = "Bad Request"; break;
    case 404: statusText = "Not Found"; break;
    case 409: statusText = "Conflict"; break;
    default: break;
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
