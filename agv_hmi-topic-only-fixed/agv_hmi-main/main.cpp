#include <QCoreApplication>
#include <QTimer>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "networkserver.h"
#include "RosTopicPublisher.hpp"

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    
    // GUI 없는 콘솔 전용 애플리케이션 생성
    QCoreApplication app(argc, argv);

    auto topic_publisher = std::make_shared<RosTopicPublisher>();

    // NetworkServer 객체 생성 및 포트 8080 시작
    NetworkServer server;
    server.setTopicPublisher(topic_publisher.get());
    server.startServer(8080);

    // ROS 2 Spin Timer (10ms 주기)
    QTimer rosTimer;
    QObject::connect(&rosTimer, &QTimer::timeout, [topic_publisher]() {
        if (rclcpp::ok()) {
            rclcpp::spin_some(topic_publisher);
        }
    });
    rosTimer.start(10);

    qDebug() << "==========================================";
    qDebug() << "  AGV Headless Server Started (No GUI)    ";
    qDebug() << "  Listening on Port 8080...               ";
    qDebug() << "==========================================";

    int result = app.exec();

    rosTimer.stop();
    rclcpp::shutdown();

    return result;
}