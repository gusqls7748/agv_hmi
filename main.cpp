#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QTimer>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "networkserver.h"
#include "Nav2Manager.hpp" // 👈 .hpp 포함!

int main(int argc, char *argv[]) {
    // 1. ROS 2 초기화
    rclcpp::init(argc, argv);

    // 2. Qt GUI 애플리케이션 생성
    QGuiApplication app(argc, argv);

    // 3. Nav2Manager (ROS 2 노드) 객체 생성
    auto nav2_manager = std::make_shared<Nav2Manager>();

    // 4. NetworkServer 객체 생성 및 Nav2Manager 연결
    NetworkServer server;
    server.setNav2Manager(nav2_manager.get()); // 📌 서버와 Nav2Manager 연동
    server.startServer(8080);                   // HTTP REST API 8080 포트 실행

    // 5. Qt 이벤트 루프에서 ROS 2 콜백(Spin)을 정기적으로 처리하기 위한 QTimer 설정
    QTimer rosTimer;
    QObject::connect(&rosTimer, &QTimer::timeout, [nav2_manager]() {
        rclcpp::spin_some(nav2_manager);
    });
    rosTimer.start(10); // 10ms 마다 ROS 2 메시지/액션 응답 수신 처리

    // 6. QML 엔진 로드
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("server", &server);

    const QUrl url(QStringLiteral("qrc:/hmi_design.qml"));
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    qDebug() << "AGV HMI Server Application Started Successfully!";

    // 7. Qt 애플리케이션 실행
    int result = app.exec();

    // 8. 종료 시 ROS 2 종료 처리
    rclcpp::shutdown();

    return result;
}