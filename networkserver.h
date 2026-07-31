#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include "Nav2Manager.hpp" // Nav2Manager 헤더 직접 포함

class NetworkServer : public QObject {
    Q_OBJECT
public:
    explicit NetworkServer(QObject *parent = nullptr);
    ~NetworkServer();

    void startServer(quint16 port);

    void setNav2Manager(Nav2Manager* nav2Manager) {
        m_nav2Manager = nav2Manager;
    }

private slots:
    void onNewConnection();
    void onReadyRead();
    
private:
    void handleGetStatus(QTcpSocket *socket);
    void handlePostCommand(QTcpSocket *socket, const QByteArray &body);
    void sendHttpResponse(QTcpSocket *socket, int statusCode, const QJsonObject &jsonObj);

    QTcpServer *m_tcpServer = nullptr;
    Nav2Manager *m_nav2Manager = nullptr;

    int m_wheelRpm = 0;
    QString m_cameraUrl = "";
    QString m_robotStatus = "idle";
};