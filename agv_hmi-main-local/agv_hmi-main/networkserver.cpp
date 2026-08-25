#include "networkserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
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
    if (!m_tcpServer->listen(QHostAddress::LocalHost, port)) {
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

        bool accepted = m_nav2Manager && m_nav2Manager->moveToLocation(dest.toStdString());

        if (!accepted) {
            qWarning() << "[NetworkServer] Rejected destination:" << dest;
            QJsonObject res;
            res["result"] = "fail";
            res["error"] = "등록되지 않은 목적지이거나 Nav2Manager가 연결되지 않았습니다: " + dest;
            sendHttpResponse(socket, 400, res);
            return;
        }
        updated = true;
    }

    // 2. "command" 키 처리 (cancel, forward, backward, left, right, stop 등)
    if (jsonReq.contains("command")) {
        QString cmd = jsonReq["command"].toString();
        qDebug() << "[NetworkServer] Received command:" << cmd;

        static const QSet<QString> manualDirections = {"forward", "backward", "left", "right", "stop"};

        if (cmd == "cancel") {
            if (m_nav2Manager) {
                m_nav2Manager->cancelAndReturnHome();
            }
            updated = true;
        } else if (manualDirections.contains(cmd)) {
            // manual_mode가 켜져 있을 때만 수동 조종을 허용합니다.
            // (자동 주행 중에 실수로 forward/left 등이 들어와 Nav2 목표와 충돌하는 것을 방지)
            if (!m_isManualMode) {
                qWarning() << "[NetworkServer] Manual command rejected (not in manual mode):" << cmd;
                QJsonObject res;
                res["result"] = "fail";
                res["error"] = "manual_mode가 꺼져 있어 수동 조종을 거부했습니다. 먼저 manual_mode:true를 보내주세요.";
                sendHttpResponse(socket, 409, res);
                return;
            }
            if (m_nav2Manager) {
                m_nav2Manager->manualDrive(cmd.toStdString());
            }
            updated = true;
        } else {
            qWarning() << "[NetworkServer] Unknown command value:" << cmd;
        }
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

            // 수동 모드가 꺼지는 순간, 조종 중이던 로봇을 안전하게 정지시킵니다.
            if (!m_isManualMode && m_nav2Manager) {
                m_nav2Manager->stopManualDrive();
            }
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
    
    QString statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 409: statusText = "Conflict"; break;
        default:  statusText = "Error"; break;
    }
    
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