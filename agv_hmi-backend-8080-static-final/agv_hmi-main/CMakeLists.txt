cmake_minimum_required(VERSION 3.16)
project(agv_hmi_server VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt5 REQUIRED COMPONENTS Core Network)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(agv_hmi_server
    main.cpp
    networkserver.h
    networkserver.cpp
    RosTopicPublisher.hpp
    RosTopicPublisher.cpp
)

target_include_directories(agv_hmi_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(agv_hmi_server
    Qt5::Core
    Qt5::Network
)

ament_target_dependencies(agv_hmi_server
    rclcpp
    geometry_msgs
    std_msgs
)

install(TARGETS agv_hmi_server
    DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY web/
    DESTINATION share/${PROJECT_NAME}/web
)

ament_package()
