#pragma once

#include <QHash>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QString>

#include "RosTopicPublisher.hpp"

class NetworkServer : public QObject {
    Q_OBJECT

public:
    explicit NetworkServer(QObject* parent = nullptr);
    ~NetworkServer();

    bool startServer(quint16 port);

    void setTopicPublisher(RosTopicPublisher* topicPublisher) {
        m_topicPublisher_ = topicPublisher;
    }

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void processRequest(QTcpSocket* socket, const QByteArray& requestData);
    void handleGetStatus(QTcpSocket* socket);
    void handlePostCommand(QTcpSocket* socket, const QByteArray& body);
    void handleManualMode(QTcpSocket* socket, const QByteArray& body);
    void serveStatic(QTcpSocket* socket, const QString& path);
    void sendHttpResponse(QTcpSocket* socket,
                          int statusCode,
                          const QJsonObject& jsonObject);
    void sendFileResponse(QTcpSocket* socket,
                          const QByteArray& contentType,
                          const QByteArray& body);
    QByteArray contentTypeForPath(const QString& path) const;
    QString webRootPath() const;

    QTcpServer* m_tcpServer_{nullptr};
    QHash<QTcpSocket*, QByteArray> m_buffers_;
    RosTopicPublisher* m_topicPublisher_{nullptr};

    bool m_manualMode_{false};
    QString m_cameraUrl_;
    QString m_webRoot_;
};
