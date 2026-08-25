
#include "networkserver.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

NetworkServer::NetworkServer(QObject *parent) : QObject(parent) {
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection,
            this, &NetworkServer::onNewConnection);
}

NetworkServer::~NetworkServer() {
    if (m_tcpServer->isListening())
        m_tcpServer->close();
}

void NetworkServer::startServer(quint16 port) {
    // QML과 서버가 같은 PC에서 동작하므로 localhost에서만 수신합니다.
    if (!m_tcpServer->listen(QHostAddress::LocalHost, port)) {
        qCritical() << "Server failed to start on port:" << port
                    << m_tcpServer->errorString();
    } else {
        qInfo() << "NetworkServer listening on http://127.0.0.1:" << port;
    }
}

void NetworkServer::onNewConnection() {
    QTcpSocket *socket = m_tcpServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead,
            this, &NetworkServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected,
            socket, &QTcpSocket::deleteLater);
}

void NetworkServer::onReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray requestData = socket->readAll();
    QString requestStr = QString::fromUtf8(requestData);

    const int headerEndIndex = requestStr.indexOf("\r\n\r\n");
    if (headerEndIndex == -1) return;

    QStringList headerLines =
        requestStr.left(headerEndIndex).split("\r\n");

    int contentLength = 0;
    for (const QString &line : headerLines) {
        if (line.startsWith("Content-Length:", Qt::CaseInsensitive)) {
            contentLength = line.section(':', 1).trimmed().toInt();
            break;
        }
    }

    QByteArray bodyPart = requestData.mid(headerEndIndex + 4);
    if (contentLength > 0 && bodyPart.size() < contentLength)
        return;

    if (headerLines.isEmpty()) return;

    QStringList tokens = headerLines.first().split(' ');
    if (tokens.size() < 2) return;

    const QString method = tokens[0];
    const QString path = tokens[1];

    qDebug() << "[NetworkServer] Request:" << method << path;

    if (method == "GET" && path == "/api/status") {
        handleGetStatus(socket);
    } else if (method == "POST" &&
               (path == "/api/command" ||
                path == "/api/manual-mode" ||
                path == "/api/manual_mode" ||
                path == "/api/manual-control")) {
        handlePostCommand(socket, bodyPart);
    } else {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Not Found";
        sendHttpResponse(socket, 404, res);
    }
}

bool NetworkServer::setManualMode(bool enabled) {
    if (m_isManualMode == enabled)
        return true;

    m_isManualMode = enabled;
    emit manualModeChanged(m_isManualMode);

    if (!m_isManualMode && m_topicPublisher)
qInfo() << "[NetworkServer] manual_mode =" << m_isManualMode;
    return true;
}

bool NetworkServer::sendCommand(const QString& command) {
    const QString cmd = command.trimmed().toLower();

    static const QSet<QString> manualDirections = {
        "forward", "backward", "left", "right", "stop"
    };

    if (cmd == "cancel") {
        if (m_topicPublisher)
return true;
    }

    if (!manualDirections.contains(cmd))
        return false;

    // 기존 HMI와의 호환: command만 오면 자동으로 수동 모드 진입.
    if (!m_isManualMode)
        setManualMode(true);

    if (!m_topicPublisher)
        return false;

    return
}

bool NetworkServer::sendDestination(const QString& destination) {
    if (!m_topicPublisher)
        return false;

    return m_topicPublisher->publishGoal(destination.toStdString());
}

bool NetworkServer::processCommandObject(const QJsonObject& jsonReq,
                                         QJsonObject& response) {
    bool updated = false;

    // manual_mode를 먼저 적용하여 {"manual_mode":true,"command":"forward"}
    // 한 번의 요청으로도 정상 동작하도록 합니다.
    const bool manualModeSpecified = jsonReq.contains("manual_mode");

    if (manualModeSpecified) {
        bool newMode = false;
        const QJsonValue value = jsonReq.value("manual_mode");

        if (value.isBool()) {
            newMode = value.toBool();
        } else if (value.isDouble()) {
            newMode = (value.toInt() == 1);
        } else if (value.isString()) {
            const QString s = value.toString().trimmed().toLower();
            newMode = (s == "true" || s == "on" || s == "1");
        } else {
            response["result"] = "fail";
            response["error"] = "manual_mode must be boolean, 0/1, or true/false string";
            return false;
        }

        setManualMode(newMode);
        updated = true;
    }

    if (jsonReq.contains("destination")) {
        const QString destination = jsonReq.value("destination").toString();

        if (!sendDestination(destination)) {
            response["result"] = "fail";
            response["error"] =
                "등록되지 않은 목적지이거나 Topic Publisher가 연결되지 않았습니다: "
                + destination;
            return false;
        }

        updated = true;
    }

    if (jsonReq.contains("command")) {
        const QString command = jsonReq.value("command").toString().trimmed().toLower();

        // 명시적으로 manual_mode:false를 함께 보낸 경우만 거부.
        // command만 보내는 기존 HMI는 자동으로 manual mode를 켭니다.
        if (manualModeSpecified && !m_isManualMode &&
            command != "cancel" &&
            command != "stop") {
            response["result"] = "fail";
            response["error"] =
                "manual_mode가 false입니다. 수동 조종 전에 manual_mode:true를 보내주세요.";
            response["manual_mode"] = m_isManualMode;
            return false;
        }

        if (!sendCommand(command)) {
            response["result"] = "fail";
            response["error"] = "수동 주행 명령을 처리하지 못했습니다: " + command;
            return false;
        }

        updated = true;
    }

    if (!updated) {
        response["result"] = "fail";
        response["error"] = "Missing parameters";
        return false;
    }

    response["result"] = "success";
    response["message"] = "Command processed successfully";
    response["manual_mode"] = m_isManualMode;
    return true;
}

void NetworkServer::handlePostCommand(QTcpSocket *socket,
                                      const QByteArray &body) {
    QByteArray cleanBody = body;

    const int firstJsonBrace = cleanBody.indexOf('{');
    const int lastJsonBrace = cleanBody.lastIndexOf('}');

    if (firstJsonBrace != -1 && lastJsonBrace > firstJsonBrace) {
        cleanBody = cleanBody.mid(
            firstJsonBrace,
            lastJsonBrace - firstJsonBrace + 1);
    }

    QJsonParseError parseError;
    const QJsonDocument jsonDoc =
        QJsonDocument::fromJson(cleanBody, &parseError);

    if (parseError.error != QJsonParseError::NoError ||
        !jsonDoc.isObject()) {
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid JSON format";
        sendHttpResponse(socket, 400, res);
        return;
    }

    QJsonObject response;
    const bool ok = processCommandObject(jsonDoc.object(), response);
    sendHttpResponse(socket, ok ? 200 : 400, response);
}

void NetworkServer::handleGetStatus(QTcpSocket *socket) {
    QJsonObject res;
    res["status"] = m_robotStatus;
    res["wheel_rpm"] = m_wheelRpm;
    res["camera_url"] = m_cameraUrl;
    res["manual_mode"] = m_isManualMode;
    sendHttpResponse(socket, 200, res);
}

void NetworkServer::sendHttpResponse(QTcpSocket *socket,
                                     int statusCode,
                                     const QJsonObject &jsonObj) {
    const QByteArray body =
        QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);

    QString statusText = "Error";
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 409: statusText = "Conflict"; break;
    }

    const QString header = QString(
        "HTTP/1.1 %1 %2\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %3\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n")
        .arg(statusCode)
        .arg(statusText)
        .arg(body.size());

    socket->write(header.toUtf8());
    socket->write(body);
    socket->flush();
    socket->disconnectFromHost();
}
