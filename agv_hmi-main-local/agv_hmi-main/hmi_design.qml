import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1024
    height: 600
    title: "AGV HMI Controller"

    // -------------------------------------------------------------
    // 1. C++ Signal 수신 연결
    // -------------------------------------------------------------
    Connections {
        target: hmiController

        function onManualModeChanged(isManualMode) {
            console.log("[QML] 수동 모드 변경 수신:", isManualMode)
            if (isManualMode) {
                manualModeOverlay.open()
            } else {
                manualModeOverlay.close()
            }
        }
    }

    // -------------------------------------------------------------
    // 2. UI 레이아웃 및 상태 바인딩
    // -------------------------------------------------------------
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        // 상단 상태 표시 바
        Rectangle {
            Layout.preferredWidth: 400
            Layout.preferredHeight: 60
            radius: 8
            color: hmiController.isManualMode ? "#E74C3C" : "#2ECC71" 

            Text {
                anchors.centerIn: parent
                text: hmiController.isManualMode ? "수동 제어 모드 (MANUAL)" : "자동 주행 모드 (AUTO)"
                color: "white"
                font.pixelSize: 20
                font.bold: true
            }
        }

        // 수동 모드 시 활성화되는 방향 버튼 영역
        GridLayout {
            columns: 3
            
            enabled: hmiController.isManualMode 
            opacity: hmiController.isManualMode ? 1.0 : 0.3

            Item { Layout.preferredWidth: 80; Layout.preferredHeight: 80 }
            Button {
                text: "▲ Forward"
                Layout.preferredWidth: 80; Layout.preferredHeight: 80
                onPressed: console.log("Manual: Move Forward")
            }
            Item { Layout.preferredWidth: 80; Layout.preferredHeight: 80 }

            Button {
                text: "◄ Left"
                Layout.preferredWidth: 80; Layout.preferredHeight: 80
                onPressed: console.log("Manual: Turn Left")
            }
            Button {
                text: "■ STOP"
                Layout.preferredWidth: 80; Layout.preferredHeight: 80
                onPressed: console.log("Manual: Stop")
            }
            Button {
                text: "► Right"
                Layout.preferredWidth: 80; Layout.preferredHeight: 80
                onPressed: console.log("Manual: Turn Right")
            }
        }
    }

    // -------------------------------------------------------------
    // 3. 수동 모드 전환 알림 팝업 (완전 수정)
    // -------------------------------------------------------------
    Popup {
        id: manualModeOverlay
        x: (parent.width - width) / 2
        y: 50
        width: 320
        height: 60
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
            text: "⚠️ 관리자 권한으로 수동 모드로 변경됨"
            color: "white"
            font.pixelSize: 14
        }
    }
}