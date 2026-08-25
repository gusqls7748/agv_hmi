#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "networkserver.h"
#include "RosTopicPublisher.hpp"

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
    qInfo() << "  AGV HMI Topic Server Started";
    qInfo() << "  HTTP API: 0.0.0.0:8080";
    qInfo() << "  ROS Goal Topic: /goal_pose";
    qInfo() << "  Web HMI: port 8000 -> 127.0.0.1:8080";
    qInfo() << "  Qt: direct/tunneled access -> 8080";
    qInfo() << "==========================================";

    const int result = app.exec();
    rosTimer.stop();
    rclcpp::shutdown();
    return result;
}
