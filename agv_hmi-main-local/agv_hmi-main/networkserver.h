#pragma once
#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include <QString>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include "Nav2Manager.hpp" // Nav2Manager 헤더 직접 포함

class NetworkServer : public QObject {
    Q_OBJECT
    // QML에서 server.isManualMode로 직접 바인딩할 수 있도록 프로퍼티 등록
    Q_PROPERTY(bool isManualMode READ isManualMode NOTIFY manualModeChanged)

public:
    explicit NetworkServer(QObject *parent = nullptr);
    ~NetworkServer();

    void startServer(quint16 port);

    void setNav2Manager(Nav2Manager* nav2Manager) {
        m_nav2Manager = nav2Manager;
    }

    // manual_mode 상태 읽기용 Getter
    bool isManualMode() const { return m_isManualMode; }

signals:
    // manual_mode 값이 파싱/변경되었을 때 UI(QML)나 타 클래스로 전달할 Signal
    void manualModeChanged(bool isManualMode);

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
    
    // manual_mode 상태 저장 변수 추가
    bool m_isManualMode = false; 
};

#endif // NETWORKSERVER_H