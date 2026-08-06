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

    // 소켓 버퍼 데이터 전부 수신
    QByteArray requestData = socket->readAll();
    QString requestStr = QString::fromUtf8(requestData);

    int headerEndIndex = requestStr.indexOf("\r\n\r\n");
    if (headerEndIndex == -1) return; // 헤더가 다 안 들어왔으면 일단 대기

    QString headerPart = requestStr.left(headerEndIndex);
    QByteArray bodyPart = requestData.mid(headerEndIndex + 4);

    // 헤더에서 Content-Length 추출하여 body가 다 모였는지 확인
    int contentLength = 0;
    QStringList headerLines = headerPart.split("\r\n");
    for (const QString &line : headerLines) {
        if (line.startsWith("Content-Length:", Qt::CaseInsensitive)) {
            contentLength = line.section(':', 1).trimmed().toInt();
            break;
        }
    }

    // 본문이 아직 다 안 들어왔으면 대기 (패킷 쪼개짐 방지)
    if (contentLength > 0 && bodyPart.size() < contentLength) {
        return; 
    }

    if (headerLines.isEmpty()) return;

    QString firstLine = headerLines.first();
    QStringList tokens = firstLine.split(" ");

    if (tokens.size() < 2) return;

    QString method = tokens[0];
    QString path = tokens[1];

    qDebug() << "[NetworkServer] Incoming Request:" << method << path;

    if (method == "GET" && path == "/api/status") {
        handleGetStatus(socket);
    } 
    // [수정] /api/manual-mode, /api/manual_mode, /api/manual-control 경로 모두 허용
    else if (method == "POST" && (path == "/api/command" || 
                                 path == "/api/manual-mode" || 
                                 path == "/api/manual_mode" || 
                                 path == "/api/manual-control")) {
        handlePostCommand(socket, bodyPart);
    } else {
        qWarning() << "[NetworkServer] Path not matched! 404 returned for:" << path;
        QJsonObject res;
        res["error"] = "Not Found";
        sendHttpResponse(socket, 404, res);
    }
}

/// POST 요청 처리 (destination / command / manual_mode / manual-control 파싱)
void NetworkServer::handlePostCommand(QTcpSocket *socket, const QByteArray &body) {
    qDebug() << "[NetworkServer] Received Raw Body:" << body;

    QByteArray cleanBody = body;

    // Chunked 인코딩 껍데기 제거 로직
    int firstJsonBrace = cleanBody.indexOf('{');
    int lastJsonBrace = cleanBody.lastIndexOf('}');

    if (firstJsonBrace != -1 && lastJsonBrace != -1 && lastJsonBrace > firstJsonBrace) {
        cleanBody = cleanBody.mid(firstJsonBrace, (lastJsonBrace - firstJsonBrace + 1));
        qDebug() << "[NetworkServer] Cleaned Body:" << cleanBody;
    }

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(cleanBody, &parseError);

    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        qWarning() << "[API 400] Invalid JSON received:" << parseError.errorString();
        QJsonObject res;
        res["result"] = "fail";
        res["error"] = "Invalid JSON format";
        sendHttpResponse(socket, 400, res);
        return;
    }

    QJsonObject jsonReq = jsonDoc.object();
    bool updated = false;

    // 1. "destination" 키 처리 (방 이동: room_301 등)
    if (jsonReq.contains("destination")) {
        QString dest = jsonReq["destination"].toString();
        qDebug() << "[NetworkServer] Received destination:" << dest;
        
        // TODO: Nav2 이동 명령 연동 시그널/함수 호출
        updated = true;
    }

    // 2. "command" 키 처리 (cancel, forward, backward, left, right, stop 등)
    if (jsonReq.contains("command")) {
        QString cmd = jsonReq["command"].toString();
        qDebug() << "[NetworkServer] Received command:" << cmd;
        
        // TODO: 수동 제어(forward/backward/left/right/stop) 및 명령(cancel 등) 연동
        updated = true;
    }

    // 3. "manual_mode" 키 처리
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

        if (m_isManualMode != newMode) {
            m_isManualMode = newMode;
            emit manualModeChanged(m_isManualMode);
            qDebug() << "[NetworkServer] Manual Mode updated ->" << m_isManualMode;
        }
        updated = true;
    }

    // 결과 응답
    QJsonObject res;
    if (updated) {
        res["result"] = "success";
        res["message"] = "Command processed successfully";
        res["manual_mode"] = m_isManualMode; // 현재 수동 모드 상태도 함께 반환
        sendHttpResponse(socket, 200, res);
    } else {
        qWarning() << "[API 400] Valid parameter missing";
        res["result"] = "fail";
        res["error"] = "Missing parameters";
        sendHttpResponse(socket, 400, res);
    }
}

void NetworkServer::handleGetStatus(QTcpSocket *socket) {
    QJsonObject res;
    res["status"] = m_robotStatus;
    res["wheel_rpm"] = m_wheelRpm;
    res["camera_url"] = m_cameraUrl;
    res["manual_mode"] = m_isManualMode;

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