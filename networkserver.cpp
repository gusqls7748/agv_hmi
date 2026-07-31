#include "networkserver.h"
#include "Nav2Manager.hpp" // 프로젝트 내 실제 헤더 파일명 대소문자 확인 필수!

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>

namespace
{
constexpr auto HttpBufferProperty = "guideRobotHttpBuffer";

QString reasonPhraseFor(int statusCode)
{
    switch (statusCode) {
    case 200: return "OK";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 404: return "Not Found";
    case 411: return "Length Required";
    default:  return "Internal Server Error";
    }
}

bool tryGetContentLength(const QByteArray &headers, int *contentLength)
{
    static const QRegularExpression contentLengthPattern(
        "^Content-Length\\s*:\\s*(\\d+)\\s*$",
        QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);

    const auto match = contentLengthPattern.match(QString::fromLatin1(headers));
    if (!match.hasMatch()) {
        return false;
    }

    bool isNumber = false;
    const int value = match.captured(1).toInt(&isNumber);
    if (!isNumber || value < 0) {
        return false;
    }

    *contentLength = value;
    return true;
}
}

NetworkServer::NetworkServer(QObject *parent) : QObject(parent)
{
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &NetworkServer::onNewConnection);
}

NetworkServer::~NetworkServer()
{
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
}

void NetworkServer::startServer(quint16 port)
{
    if (m_tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "HTTP REST API Server started on port" << port;
    } else {
        qWarning() << "HTTP REST API Server failed to start:" << m_tcpServer->errorString();
    }
}

void NetworkServer::onNewConnection()
{
    QTcpSocket *socket = m_tcpServer->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
}

void NetworkServer::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }

    QByteArray requestData = socket->property(HttpBufferProperty).toByteArray();
    requestData.append(socket->readAll());

    const int headerEnd = requestData.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        socket->setProperty(HttpBufferProperty, requestData);
        return;
    }

    const QByteArray headers = requestData.left(headerEnd);
    const QByteArray requestLine = headers.left(headers.indexOf("\r\n"));
    const int bodyStart = headerEnd + 4;

    if (requestLine.startsWith("OPTIONS ")) {
        const QByteArray response =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
            "Connection: close\r\n\r\n";
        socket->write(response);
        socket->disconnectFromHost();
        return;
    }

    if (requestLine.startsWith("GET /api/status ")) {
        socket->setProperty(HttpBufferProperty, QByteArray());
        handleGetStatus(socket);
        return;
    }

    if (!requestLine.startsWith("POST /api/command ")) {
        QJsonObject error;
        error["error"] = "Not Found";
        sendHttpResponse(socket, 404, error);
        return;
    }

    // Content-Length 파싱 실패 시 남은 바이트 처리
    int contentLength = 0;
    if (!tryGetContentLength(headers, &contentLength)) {
        contentLength = requestData.size() - bodyStart;
    }

    const int fullRequestSize = bodyStart + contentLength;
    if (requestData.size() < fullRequestSize) {
        socket->setProperty(HttpBufferProperty, requestData);
        return;
    }

    const QByteArray body = requestData.mid(bodyStart, contentLength);
    socket->setProperty(HttpBufferProperty, QByteArray());
    handlePostCommand(socket, body);
}

void NetworkServer::handleGetStatus(QTcpSocket *socket)
{
    QJsonObject response;
    response["wheel_rpm"] = m_wheelRpm;
    response["camera_url"] = m_cameraUrl;
    response["status"] = m_robotStatus;

    // (필요 시 더미 좌표 전달)
    response["x"] = 0.0;
    response["y"] = 0.0;

    sendHttpResponse(socket, 200, response);
}

void NetworkServer::handlePostCommand(QTcpSocket *socket, const QByteArray &body)
{
    // Chunked Encoding 패딩 제거 ({ ... } 추출)
    QByteArray cleanBody = body;
    int firstBrace = cleanBody.indexOf('{');
    int lastBrace = cleanBody.lastIndexOf('}');

    if (firstBrace >= 0 && lastBrace > firstBrace) {
        cleanBody = cleanBody.mid(firstBrace, lastBrace - firstBrace + 1);
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(cleanBody, &parseError);

    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "[JSON 파싱 실패]:" << parseError.errorString() << "body:" << body;

        QJsonObject error;
        error["result"] = "fail";
        error["error"] = "Invalid JSON body";
        sendHttpResponse(socket, 400, error);
        return;
    }

    const QJsonObject json = document.object();
    const QString command = json.value("command").toString().trimmed();
    const QString destination = json.value("destination").toString().trimmed();

    qDebug() << "[API 요청 수신] command:" << command << "destination:" << destination;

    QJsonObject response;

    // 1. 캔슬 명령 (Nav2 이동 취소 후 HOME 복귀)
    if (command == "cancel") {
        qDebug() << "[제어 명령 수신]: cancel - 안내 취소 및 원점(HOME) 복귀";
        
        m_robotStatus = "returning";

        if (m_nav2Manager) {
            m_nav2Manager->cancelAndReturnHome();
        }

        response["result"] = "success";
        response["message"] = "Guide cancelled, returning home";
        response["status"] = m_robotStatus;
        sendHttpResponse(socket, 200, response);
        return;
    }

    // 2. 목적지 안내 명령
    if (!destination.isEmpty()) {
        qDebug() << "[목적지 명령 수신]:" << destination;

        m_robotStatus = "moving";

        if (m_nav2Manager) {
            m_nav2Manager->moveToLocation(destination.toStdString());
        }

        response["result"] = "success";
        response["target"] = destination;
        response["status"] = m_robotStatus;
        sendHttpResponse(socket, 200, response);
        return;
    }

    qWarning() << "[경고]: destination 또는 command 파라미터 누락. body:" << body;
    response["result"] = "fail";
    response["error"] = "destination or command parameter missing";
    sendHttpResponse(socket, 400, response);
}

void NetworkServer::sendHttpResponse(QTcpSocket *socket, int statusCode, const QJsonObject &jsonObj)
{
    const QJsonDocument document(jsonObj);
    const QByteArray body = document.toJson(QJsonDocument::Compact);

    const QString header = QString("HTTP/1.1 %1 %2\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Access-Control-Allow-Origin: *\r\n"
                                   "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                                   "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                                   "Content-Length: %3\r\n"
                                   "Connection: close\r\n\r\n")
                               .arg(statusCode)
                               .arg(reasonPhraseFor(statusCode))
                               .arg(body.size());

    socket->write(header.toUtf8());
    socket->write(body);
    socket->disconnectFromHost();
}

#include "networkserver.moc"