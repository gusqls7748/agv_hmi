#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "networkserver.h"
#include "RosTopicPublisher.hpp"

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);

    QCoreApplication app(argc, argv);

    auto topicPublisher = std::make_shared<RosTopicPublisher>();

    NetworkServer server;
    server.setTopicPublisher(topicPublisher.get());
    server.startServer(8080);

    QTimer rosTimer;
    QObject::connect(&rosTimer, &QTimer::timeout, [&topicPublisher]() {
        if (rclcpp::ok()) {
            rclcpp::spin_some(topicPublisher);
        }
    });
    rosTimer.start(10);

    qInfo() << "==========================================";
    qInfo() << "  AGV HMI Topic Server Started";
    qInfo() << "  HTTP: port 8080";
    qInfo() << "  ROS Topic: /goal_pose";
    qInfo() << "  Bind: 0.0.0.0 (LAN access enabled)";
    qInfo() << "==========================================";

    const int result = app.exec();

    rosTimer.stop();
    rclcpp::shutdown();
    return result;
}
