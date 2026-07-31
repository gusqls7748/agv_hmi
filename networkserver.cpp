#include "networkserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

NetworkServer::NetworkServer(QObject *parent) : QObject(parent) {
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &NetworkServer::onNewConnection);
}

NetworkServer::~NetworkServer() {
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
}

void NetworkServer::startServer(quint16 port) {
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        qCritical() << "Server failed to start on port:" << port;
    } else {
        qDebug() << "NetworkServer listening on port:" << port;
    }
}

void NetworkServer::onNewConnection() {
    QTcpSocket *socket = m_tcpServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void NetworkServer::onReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray requestData = socket->readAll();
    QString requestStr = QString::fromUtf8(requestData);

    int headerEndIndex = requestStr.indexOf("\r\n\r\n");
    if (headerEndIndex == -1) return;

    QString headerPart = requestStr.left(headerEndIndex);
    QByteArray bodyPart = requestData.mid(headerEndIndex + 4);

    QStringList requestLines = headerPart.split("\r\n");
    if (requestLines.isEmpty()) return;

    QString firstLine = requestLines.first();
    QStringList tokens = firstLine.split(" ");

    if (tokens.size() < 2) return;

    QString method = tokens[0];
    QString path = tokens[1];

    // [디버그 로그 추가] 터미널에 수신된 method와 path 출력
    qDebug() << "[NetworkServer] Incoming Request:" << method << path;

    if (method == "GET" && path == "/api/status") {
        handleGetStatus(socket);
    } else if (method == "POST" && (path == "/api/command" || path == "/api/manual_mode")) {
        handlePostCommand(socket, bodyPart);
    } else {
        qWarning() << "[NetworkServer] Path not matched! 404 returned for:" << path;
        QJsonObject res;
        res["error"] = "Not Found";
        sendHttpResponse(socket, 404, res);
    }
}

// POST 요청 처리 (JSON 파싱 및 Signal emit)
void NetworkServer::handlePostCommand(QTcpSocket *socket, const QByteArray &body) {
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(body, &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "Invalid JSON received:" << parseError.errorString();
        QJsonObject res;
        res["success"] = false;
        res["message"] = "Invalid JSON format";
        sendHttpResponse(socket, 400, res);
        return;
    }

    QJsonObject jsonReq = jsonDoc.object();
    bool updated = false;

    // 1. "manual_mode" 키 처리 (bool 타입 또는 int/string 처리)
    if (jsonReq.contains("manual_mode")) {
        bool newMode = false;
        
        if (jsonReq["manual_mode"].isBool()) {
            newMode = jsonReq["manual_mode"].toBool();
        } else if (jsonReq["manual_mode"].isDouble()) {
            newMode = (jsonReq["manual_mode"].toInt() == 1);
        } else if (jsonReq["manual_mode"].isString()) {
            QString modeStr = jsonReq["manual_mode"].toString().toLower();
            newMode = (modeStr == "true" || modeStr == "on" || modeStr == "1");
        }

        // 상태값이 변경된 경우에만 emit
        if (m_isManualMode != newMode) {
            m_isManualMode = newMode;
            emit manualModeChanged(m_isManualMode);
            qDebug() << "[NetworkServer] Manual Mode updated ->" << m_isManualMode;
        }
        updated = true;
    }

    // 2. Nav2 명령 등 기존 command 파싱 (필요 시 연동)
    if (jsonReq.contains("command")) {
        QString cmd = jsonReq["command"].toString();
        qDebug() << "[NetworkServer] Received command:" << cmd;
        // Nav2Manager 연동 로직
        updated = true;
    }

    // HTTP 응답 반환
    QJsonObject res;
    res["success"] = true;
    res["manual_mode"] = m_isManualMode;
    res["message"] = updated ? "Command processed successfully" : "No recognized parameters";
    sendHttpResponse(socket, 200, res);
}

void NetworkServer::handleGetStatus(QTcpSocket *socket) {
    QJsonObject res;
    res["status"] = m_robotStatus;
    res["wheel_rpm"] = m_wheelRpm;
    res["camera_url"] = m_cameraUrl;
    res["manual_mode"] = m_isManualMode; // 현재 매뉴얼 모드 상태 반환

    sendHttpResponse(socket, 200, res);
}

void NetworkServer::sendHttpResponse(QTcpSocket *socket, int statusCode, const QJsonObject &jsonObj) {
    QByteArray body = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    
    QString statusText = (statusCode == 200) ? "OK" : (statusCode == 400 ? "Bad Request" : "Not Found");
    
    QString responseHeader = QString(
        "HTTP/1.1 %1 %2\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %3\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n"
    ).arg(statusCode).arg(statusText).arg(body.size());

    socket->write(responseHeader.toUtf8());
    socket->write(body);
    socket->flush();
    socket->disconnectFromHost();
}