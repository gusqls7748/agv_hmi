#include "RosTopicPublisher.hpp"

#include <cstdlib>
#include <limits>
#include <stdexcept>

RosTopicPublisher::RosTopicPublisher()
    : Node("agv_topic_publisher") {
    goal_pose_pub_ =
        this->create_publisher<geometry_msgs::msg::PoseStamped>(
            GOAL_TOPIC, 10);

    // Robot/Nav2 side can publish arrival/state information here.
    agv_status_sub_ =
        this->create_subscription<std_msgs::msg::String>(
            AGV_STATUS_TOPIC,
            10,
            std::bind(&RosTopicPublisher::onAgvStatus, this, std::placeholders::_1));

    loadLocations();

    RCLCPP_INFO(this->get_logger(), "ROS Topic Publisher started");
    RCLCPP_INFO(this->get_logger(), "Goal topic: %s", GOAL_TOPIC);
    RCLCPP_INFO(this->get_logger(), "Status topic: %s", AGV_STATUS_TOPIC);
    RCLCPP_INFO(this->get_logger(),
                "Supported destinations: restroom, room_301, room_302, elevator");

    if (!hasDestination("restroom")) {
        RCLCPP_WARN(this->get_logger(),
                    "restroom 좌표가 설정되지 않았습니다. "
                    "AGV_RESTROOM_X/Y 환경변수를 설정하세요.");
    }
    if (!hasDestination("elevator")) {
        RCLCPP_WARN(this->get_logger(),
                    "elevator 좌표가 설정되지 않았습니다. "
                    "AGV_ELEVATOR_X/Y 환경변수를 설정하세요.");
    }
}

void RosTopicPublisher::loadLocations() {
    // Existing map coordinates from the previous AGV project are preserved.
    location_map_["room_301"] = {-4.00, -3.89};
    location_map_["room_302"] = { 3.99,  4.13};

    // Web HMI destinations are configurable because their final map coordinates
    // were not fixed in the handoff document.
    loadOptionalLocationFromEnv("AGV_RESTROOM_X", "AGV_RESTROOM_Y", "restroom");
    loadOptionalLocationFromEnv("AGV_ELEVATOR_X", "AGV_ELEVATOR_Y", "elevator");
}

bool RosTopicPublisher::loadOptionalLocationFromEnv(
    const char* x_name,
    const char* y_name,
    const std::string& key) {
    const char* x_text = std::getenv(x_name);
    const char* y_text = std::getenv(y_name);

    if (!x_text || !y_text) {
        return false;
    }

    try {
        location_map_[key] = {std::stod(x_text), std::stod(y_text)};
        RCLCPP_INFO(this->get_logger(),
                    "%s -> x=%.2f, y=%.2f",
                    key.c_str(),
                    location_map_[key].x,
                    location_map_[key].y);
        return true;
    } catch (const std::exception&) {
        RCLCPP_ERROR(this->get_logger(),
                     "좌표 환경변수 형식 오류: %s/%s",
                     x_name, y_name);
        return false;
    }
}

bool RosTopicPublisher::hasDestination(const std::string& destination) const {
    return location_map_.find(destination) != location_map_.end();
}

bool RosTopicPublisher::publishGoal(const std::string& destination) {
    auto it = location_map_.find(destination);
    if (it == location_map_.end()) {
        RCLCPP_WARN(this->get_logger(),
                    "등록되지 않은 목적지입니다: %s",
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
    status_ = "moving";

    RCLCPP_INFO(this->get_logger(),
                "[Goal Topic] %s -> x=%.2f, y=%.2f",
                destination.c_str(), target.x, target.y);

    return true;
}

void RosTopicPublisher::setStatus(const std::string& status) {
    if (status == "idle" || status == "moving" ||
        status == "arrived" || status == "failed") {
        status_ = status;
    }
}

void RosTopicPublisher::onAgvStatus(
    const std_msgs::msg::String::SharedPtr msg) {
    if (!msg) {
        return;
    }

    std::string value = msg->data;
    for (char& c : value) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }

    if (value == "arrived" || value == "moving" ||
        value == "idle" || value == "failed") {
        status_ = value;
        RCLCPP_INFO(this->get_logger(),
                    "[AGV Status] %s",
                    status_.c_str());
    }
}
