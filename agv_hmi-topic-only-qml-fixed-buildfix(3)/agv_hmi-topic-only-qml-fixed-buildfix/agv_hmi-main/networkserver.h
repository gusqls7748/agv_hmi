
#pragma once
#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <QString>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include "RosTopicPublisher.hpp"

class NetworkServer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isManualMode READ isManualMode NOTIFY manualModeChanged)

public:
    explicit NetworkServer(QObject *parent = nullptr);
    ~NetworkServer();

    void startServer(quint16 port);

    void setTopicPublisher(RosTopicPublisher* topicPublisher) {
        m_topicPublisher = topicPublisher;
    }

    bool isManualMode() const { return m_isManualMode; }

    // QML/기타 로컬 클라이언트에서 직접 사용할 수 있는 명령 API
    Q_INVOKABLE bool setManualMode(bool enabled);
    Q_INVOKABLE bool sendCommand(const QString& command);
    Q_INVOKABLE bool sendDestination(const QString& destination);

signals:
    void manualModeChanged(bool isManualMode);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    void handleGetStatus(QTcpSocket *socket);
    void handlePostCommand(QTcpSocket *socket, const QByteArray &body);
    void sendHttpResponse(QTcpSocket *socket, int statusCode, const QJsonObject &jsonObj);

    bool processCommandObject(const QJsonObject& jsonReq, QJsonObject& response);

    QTcpServer *m_tcpServer = nullptr;
    RosTopicPublisher *m_topicPublisher = nullptr;

    int m_wheelRpm = 0;
    QString m_cameraUrl;
    QString m_robotStatus = "idle";
    bool m_isManualMode = false;
};

#endif
