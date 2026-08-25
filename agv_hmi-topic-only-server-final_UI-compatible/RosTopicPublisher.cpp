#include "RosTopicPublisher.hpp"

#include <cstdlib>

namespace {
bool readEnvDouble(const char* name, double& value) {
    const char* raw = std::getenv(name);
    if (!raw || *raw == '\0') {
        return false;
    }

    char* end = nullptr;
    const double parsed = std::strtod(raw, &end);
    if (end == raw || *end != '\0') {
        return false;
    }

    value = parsed;
    return true;
}
}

RosTopicPublisher::RosTopicPublisher()
    : Node("agv_topic_publisher") {
    goal_pose_pub_ =
        this->create_publisher<geometry_msgs::msg::PoseStamped>(
            GOAL_TOPIC, 10);

    // UI가 보내는 목적지 ID와 ROS map 좌표를 여기에서 연결합니다.
    // room_301 / room_302는 기존 서버의 좌표를 유지합니다.
    location_map_["room_301"] = {-2.00, -0.50};
    location_map_["room_302"] = { 2.00,  2.00};

    // 실제 맵 좌표가 정해지면 환경변수로 설정할 수 있습니다.
    // 예:
    // export AGV_RESTROOM_X=-1.0
    // export AGV_RESTROOM_Y=1.5
    // export AGV_ELEVATOR_X=3.0
    // export AGV_ELEVATOR_Y=-2.0
    double x = 0.0;
    double y = 0.0;

    if (readEnvDouble("AGV_RESTROOM_X", x) &&
        readEnvDouble("AGV_RESTROOM_Y", y)) {
        location_map_["restroom"] = {x, y};
    }

    if (readEnvDouble("AGV_ELEVATOR_X", x) &&
        readEnvDouble("AGV_ELEVATOR_Y", y)) {
        location_map_["elevator"] = {x, y};
    }

    RCLCPP_INFO(this->get_logger(), "ROS Topic Publisher started");
    RCLCPP_INFO(this->get_logger(), "Goal topic: %s", GOAL_TOPIC);
    RCLCPP_INFO(this->get_logger(),
                "Supported destinations: restroom, room_301, room_302, elevator");

    if (location_map_.find("restroom") == location_map_.end()) {
        RCLCPP_WARN(this->get_logger(),
                    "restroom 좌표가 설정되지 않았습니다. "
                    "AGV_RESTROOM_X/Y 환경변수를 설정하세요.");
    }

    if (location_map_.find("elevator") == location_map_.end()) {
        RCLCPP_WARN(this->get_logger(),
                    "elevator 좌표가 설정되지 않았습니다. "
                    "AGV_ELEVATOR_X/Y 환경변수를 설정하세요.");
    }
}

bool RosTopicPublisher::publishGoal(const std::string& destination) {
    auto it = location_map_.find(destination);
    if (it == location_map_.end()) {
        RCLCPP_WARN(
            this->get_logger(),
            "등록되지 않았거나 좌표가 설정되지 않은 목적지입니다: %s",
            destination.c_str());
        return false;
    }

    const Location& target = it->second;

    geometry_msgs::msg::PoseStamped msg;
    msg.header.stamp = this->now();
    msg.header.frame_id = "map";

    msg.pose.position.x = target.x;
    msg.pose.position.y = target.y;
    msg.pose.position.z = 0.0;

    msg.pose.orientation.x = 0.0;
    msg.pose.orientation.y = 0.0;
    msg.pose.orientation.z = 0.0;
    msg.pose.orientation.w = 1.0;

    goal_pose_pub_->publish(msg);

    RCLCPP_INFO(
        this->get_logger(),
        "[Goal Topic] %s -> x=%.2f, y=%.2f",
        destination.c_str(), target.x, target.y);

    return true;
}
