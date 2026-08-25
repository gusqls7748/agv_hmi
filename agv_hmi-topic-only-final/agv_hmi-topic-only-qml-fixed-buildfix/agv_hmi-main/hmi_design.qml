
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1024
    height: 600
    title: "AGV HMI Controller"

    property string lastMessage: "대기 중"

    Connections {
        target: hmiController

        function onManual (사용 안 함)ModeChanged(enabled) {
            lastMessage = enabled ? "수동 모드 ON" : "자동 모드 ON"
            if (enabled)
                manualModeOverlay.open()
            else
                manualModeOverlay.close()
        }

        function onCommandSucceeded(command) {
            lastMessage = "명령 전송 완료: " + command
        }

        function onCommandFailed(command) {
            lastMessage = "명령 전송 실패: " + command
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 18

        Rectangle {
            Layout.preferredWidth: 520
            Layout.preferredHeight: 70
            radius: 10
            color: hmiController.isManual (사용 안 함)Mode ? "#E74C3C" : "#2ECC71"

            Text {
                anchors.centerIn: parent
                text: hmiController.isManual (사용 안 함)Mode
                      ? "수동 제어 모드 (MANUAL)"
                      : "자동 주행 모드 (AUTO)"
                color: "white"
                font.pixelSize: 22
                font.bold: true
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            Label {
                text: "수동 모드"
                font.pixelSize: 18
            }

            Switch {
                checked: hmiController.isManual (사용 안 함)Mode
                onToggled: hmiController.setManual (사용 안 함)Mode(checked)
            }

            Label {
                text: mainWindow.lastMessage
                font.pixelSize: 14
            }
        }

        // 자동주행 목적지 테스트/전송 영역
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            Label { text: "목적지:" }

            Button {
                text: "HOME"
                onClicked: hmiController.sendDestination("HOME")
            }
            Button {
                text: "301"
                onClicked: hmiController.sendDestination("room_301")
            }
            Button {
                text: "302"
                onClicked: hmiController.sendDestination("room_302")
            }
            Button {
                text: "303"
                onClicked: hmiController.sendDestination("room_303")
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: "수동 버튼은 누르고 있는 동안 이동하며, 손을 떼면 자동으로 STOP 됩니다."
            font.pixelSize: 14
        }

        GridLayout {
            columns: 3
            Layout.alignment: Qt.AlignHCenter
            columnSpacing: 10
            rowSpacing: 10

            Item { Layout.preferredWidth: 120; Layout.preferredHeight: 80 }

            Button {
                text: "▲ Forward"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 80
                enabled: hmiController.isManual (사용 안 함)Mode
            }

            Item { Layout.preferredWidth: 120; Layout.preferredHeight: 80 }

            Button {
                text: "◀ Left"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 80
                enabled: hmiController.isManual (사용 안 함)Mode
            }

            Button {
                text: "■ STOP"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 80
                enabled: hmiController.isManual (사용 안 함)Mode
            }

            Button {
                text: "Right ▶"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 80
                enabled: hmiController.isManual (사용 안 함)Mode
            }

            Item { Layout.preferredWidth: 120; Layout.preferredHeight: 80 }

            Button {
                text: "▼ Backward"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 80
                enabled: hmiController.isManual (사용 안 함)Mode
            }

            Item { Layout.preferredWidth: 120; Layout.preferredHeight: 80 }
        }
    }

    Popup {
        id: manualModeOverlay
        x: (parent.width - width) / 2
        y: 20
        width: 320
        height: 55
        modal: false
        focus: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#333333"
            radius: 10
            border.color: "#E74C3C"
            border.width: 2
        }

        contentItem: Text {
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: "수동 모드가 활성화되었습니다."
            color: "white"
            font.pixelSize: 14
        }
    }
}
