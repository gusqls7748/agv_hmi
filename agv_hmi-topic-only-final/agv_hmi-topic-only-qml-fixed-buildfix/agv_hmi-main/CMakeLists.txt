
cmake_minimum_required(VERSION 3.16)
project(agv_hmi_server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick Network)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(agv_hmi_server
    main.cpp
    RosTopicPublisher.hpp
    RosTopicPublisher.cpp
    networkserver.h
    networkserver.cpp
    HmiController.hpp
    HmiController.cpp
    qml.qrc
)

target_include_directories(agv_hmi_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(agv_hmi_server
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::Network
)

ament_target_dependencies(agv_hmi_server
    rclcpp
    geometry_msgs
)

install(TARGETS agv_hmi_server
    DESTINATION lib/${PROJECT_NAME}
)

ament_package()
