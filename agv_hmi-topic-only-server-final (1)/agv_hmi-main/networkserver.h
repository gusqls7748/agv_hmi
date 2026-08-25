#pragma once

#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QHash>
#include <QByteArray>

#include "RosTopicPublisher.hpp"

class NetworkServer : public QObject {
    Q_OBJECT

public:
    explicit NetworkServer(QObject *parent = nullptr);
    ~NetworkServer();

    void startServer(quint16 port);

    void setTopicPublisher(RosTopicPublisher *topicPublisher) {
        m_topicPublisher = topicPublisher;
    }

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    void processRequest(QTcpSocket *socket, const QByteArray &requestData);
    void handleGetStatus(QTcpSocket *socket);
    void handlePostCommand(QTcpSocket *socket, const QByteArray &body);
    void sendHttpResponse(QTcpSocket *socket,
                          int statusCode,
                          const QJsonObject &jsonObj);

    QTcpServer *m_tcpServer = nullptr;
    RosTopicPublisher *m_topicPublisher = nullptr;
    QHash<QTcpSocket *, QByteArray> m_buffers;

    int m_wheelRpm = 0;
    QString m_cameraUrl;
    QString m_robotStatus = "idle";
};
