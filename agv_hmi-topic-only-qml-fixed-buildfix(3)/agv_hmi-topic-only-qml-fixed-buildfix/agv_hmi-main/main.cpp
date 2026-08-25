#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QDebug>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "networkserver.h"
#include "RosTopicPublisher.hpp"
#include "HmiController.hpp"

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);

    QGuiApplication app(argc, argv);

    auto topicPublisher = std::make_shared<RosTopicPublisher>();

    // HTTP API 서버
    NetworkServer server;
    server.setTopicPublisher(topicPublisher.get());
    server.startServer(8080);

    // QML에서 직접 서버 명령을 호출할 수 있도록 Controller 연결
    HmiController hmiController(&server);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("hmiController", &hmiController);

    const QUrl url(QStringLiteral("qrc:/hmi_design.qml"));
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        rclcpp::shutdown();
        return -1;
    }

    // Qt 이벤트 루프를 사용하면서 ROS 2 callback도 주기적으로 처리
    QTimer rosTimer;
    QObject::connect(&rosTimer, &QTimer::timeout, [&topicPublisher]() {
        if (rclcpp::ok())
            rclcpp::spin_some(topicPublisher);
    });
    rosTimer.start(10);

    qInfo() << "==========================================";
    qInfo() << "  AGV HMI Server + QML Started";
    qInfo() << "  HTTP: http://127.0.0.1:8080";
    qInfo() << "  Goal Topic: /goal_pose";
    qInfo() << "  CmdVel Topic: /cmd_vel";
    qInfo() << "==========================================";

    const int result = app.exec();

    rosTimer.stop();
    rclcpp::shutdown();
    return result;
}
