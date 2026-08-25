#pragma once

#include <map>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

struct Location {
    double x;
    double y;
};

// Nav2를 직접 호출하지 않고, 다른 ROS 2 노드가 받을 /goal_pose만 발행합니다.
class RosTopicPublisher : public rclcpp::Node {
public:
    RosTopicPublisher();

    // 목적지 ID를 좌표로 변환하여 /goal_pose에 발행합니다.
    bool publishGoal(const std::string& destination);

private:
    static constexpr const char* GOAL_TOPIC = "/goal_pose";

    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr goal_pose_pub_;
    std::map<std::string, Location> location_map_;
};
