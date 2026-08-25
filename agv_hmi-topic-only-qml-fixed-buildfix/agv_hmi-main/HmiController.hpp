
#ifndef HMICONTROLLER_HPP
#define HMICONTROLLER_HPP

#include <QObject>
#include <QString>

class NetworkServer;

class HmiController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isManualMode READ isManualMode NOTIFY manualModeChanged)

public:
    explicit HmiController(NetworkServer *server, QObject *parent = nullptr);

    bool isManualMode() const { return m_isManualMode; }

    Q_INVOKABLE bool setManualMode(bool enabled);
    Q_INVOKABLE bool sendCommand(const QString &command);
    Q_INVOKABLE bool sendDestination(const QString &destination);

signals:
    void manualModeChanged(bool isManualMode);
    void commandSucceeded(const QString &command);
    void commandFailed(const QString &command);

private:
    NetworkServer *m_server = nullptr;
    bool m_isManualMode = false;
};

#endif
