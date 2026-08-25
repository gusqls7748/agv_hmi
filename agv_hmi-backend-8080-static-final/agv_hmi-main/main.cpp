#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <memory>

#include "RosTopicPublisher.hpp"
#include "networkserver.h"
#include "rclcpp/rclcpp.hpp"

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    QCoreApplication app(argc, argv);

    auto topicPublisher = std::make_shared<RosTopicPublisher>();

    NetworkServer server;
    server.setTopicPublisher(topicPublisher.get());

    if (!server.startServer(8080)) {
        rclcpp::shutdown();
        return 1;
    }

    QTimer rosTimer;
    QObject::connect(&rosTimer, &QTimer::timeout, [&topicPublisher]() {
        if (rclcpp::ok()) {
            rclcpp::spin_some(topicPublisher);
        }
    });
    rosTimer.start(10);

    qInfo() << "==========================================";
    qInfo() << "  AGV HMI Server Started";
    qInfo() << "  Web HMI: http://<server-ip>:8080/";
    qInfo() << "  HTTP API: http://<server-ip>:8080/api/...";
    qInfo() << "  Qt: direct/tunneled access -> 8080";
    qInfo() << "  ROS Goal Topic: /goal_pose";
    qInfo() << "==========================================";

    const int result = app.exec();

    rosTimer.stop();
    rclcpp::shutdown();
    return result;
}
