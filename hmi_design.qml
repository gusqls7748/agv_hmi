import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    width: 800
    height: 480

    // C++ 서버에서 넘겨주는 cameraUrl을 바로 바인딩
    Text {
        text: "Camera Stream: " + server.cameraUrl
        anchors.centerIn: parent
        font.pixelSize: 20
    }
}