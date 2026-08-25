cmake_minimum_required(VERSION 3.16)
project(agv_hmi_server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt MOC, RCC, UIC 자동화 설정
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 1. 패키지 검색 (Qt5 및 ROS 2 패키지)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick Network)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

# 2. 실행 파일 생성
add_executable(agv_hmi_server
    main.cpp
    RosTopicPublisher.hpp
    RosTopicPublisher.cpp
    networkserver.h
    networkserver.cpp
    HmiController.hpp  # <--- 이 부분 추가!
    qml.qrc
)

# 3. 헤더 경로 포함
target_include_directories(agv_hmi_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 4. Qt5 라이브러리 및 ROS 2 의존성 관리
# 🚨 PRIVATE 키워드를 제거하여 ament_target_dependencies와의 충돌 방지
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
    std_msgs
)

# 5. ROS 2 패키지 설치 규칙
install(TARGETS agv_hmi_server
    DESTINATION lib/${PROJECT_NAME}
)

ament_package()