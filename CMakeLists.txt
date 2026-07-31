cmake_minimum_required(VERSION 3.16)
project(AgvHmiServer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 1. 패키지 검색
find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick Network)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)

# 2. 실행 파일 생성
add_executable(AgvHmiServer
    main.cpp
    Nav2Manager.cpp
    networkserver.h
    networkserver.cpp
    qml.qrc
)

# 3. ROS 2 및 Qt 헤더 경로 포함
target_include_directories(AgvHmiServer PRIVATE
    ${rclcpp_INCLUDE_DIRS}
    ${rclcpp_action_INCLUDE_DIRS}
    ${nav2_msgs_INCLUDE_DIRS}
    ${geometry_msgs_INCLUDE_DIRS}
)

# 4. 라이브러리 링크 (모두 PRIVATE 키워드로 통일)
target_link_libraries(AgvHmiServer PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::Network
    rclcpp::rclcpp
    rclcpp_action::rclcpp_action
    nav2_msgs::nav2_msgs
    geometry_msgs::geometry_msgs
)

# 5. 설치 규칙
install(TARGETS AgvHmiServer
    DESTINATION lib/${PROJECT_NAME}
)

ament_package()