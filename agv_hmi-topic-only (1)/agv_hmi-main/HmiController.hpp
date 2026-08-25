#ifndef HMICONTROLLER_HPP
#define HMICONTROLLER_HPP

#include <QObject>

class HmiController : public QObject {
    Q_OBJECT
    // QML에서 읽을 수 있고, 값이 변경되면 manualModeChanged 신호를 발생시키는 프로퍼티
    Q_PROPERTY(bool isManualMode READ isManualMode WRITE setManualMode NOTIFY manualModeChanged)

public:
    explicit HmiController(QObject *parent = nullptr) : QObject(parent), m_isManualMode(false) {}

    bool isManualMode() const { return m_isManualMode; }

public slots:
    void setManualMode(bool manual) {
        if (m_isManualMode != manual) {
            m_isManualMode = manual;
            emit manualModeChanged(m_isManualMode);
        }
    }

signals:
    void manualModeChanged(bool isManualMode);

private:
    bool m_isManualMode;
};

#endif // HMICONTROLLER_HPP