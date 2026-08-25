
#include "HmiController.hpp"
#include "networkserver.h"

HmiController::HmiController(NetworkServer *server, QObject *parent)
    : QObject(parent), m_server(server) {
    if (m_server) {
        m_isManualMode = m_server->isManualMode();

        connect(m_server, &NetworkServer::manualModeChanged,
                this, [this](bool enabled) {
                    if (m_isManualMode != enabled) {
                        m_isManualMode = enabled;
                        emit manualModeChanged(m_isManualMode);
                    }
                });
    }
}

bool HmiController::setManualMode(bool enabled) {
    if (!m_server)
        return false;

    const bool ok = m_server->setManualMode(enabled);
    if (ok && m_isManualMode != enabled) {
        m_isManualMode = enabled;
        emit manualModeChanged(m_isManualMode);
    }
    return ok;
}

bool HmiController::sendCommand(const QString &command) {
    if (!m_server)
        return false;

    const bool ok = m_server->sendCommand(command);
    if (ok)
        emit commandSucceeded(command);
    else
        emit commandFailed(command);
    return ok;
}

bool HmiController::sendDestination(const QString &destination) {
    if (!m_server)
        return false;

    const bool ok = m_server->sendDestination(destination);
    if (ok)
        emit commandSucceeded(QString("destination:%1").arg(destination));
    else
        emit commandFailed(QString("destination:%1").arg(destination));
    return ok;
}
