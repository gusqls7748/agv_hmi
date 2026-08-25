#include "RosTopicPublisher.hpp"

RosTopicPublisher::RosTopicPublisher()
    : Node("agv_topic_publisher") {
    goal_pose_pub_ =
        this->create_publisher<geometry_msgs::msg::PoseStamped>(
            GOAL_TOPIC, 10);

    // 기존 HMI에서 사용하던 목적지 좌표
    location_map_["HOME"]     = {0.00, 0.00};
    location_map_["room_301"] = {-2.00, -0.50};
    location_map_["room_302"] = {2.00, 2.00};
    location_map_["room_303"] = {1.50, -1.50};
    location_map_["room_304"] = {0.00, 0.00};
    location_map_["room_305"] = {0.00, 0.00};

    RCLCPP_INFO(this->get_logger(), "ROS Topic Publisher started");
    RCLCPP_INFO(this->get_logger(), "Goal topic: %s", GOAL_TOPIC);
}

bool RosTopicPublisher::publishGoal(const std::string& destination) {
    auto it = location_map_.find(destination);
    if (it == location_map_.end()) {
        RCLCPP_WARN(
            this->get_logger(),
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

    RCLCPP_INFO(
        this->get_logger(),
        "[Goal Topic] %s -> x=%.2f, y=%.2f",
        destination.c_str(), target.x, target.y);

    return true;
}
