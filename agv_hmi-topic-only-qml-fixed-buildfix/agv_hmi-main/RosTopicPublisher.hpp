#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"

struct Location {
    double x;
    double y;
};

// Nav2를 직접 호출하지 않고, 다른 ROS2 노드가 받을 Topic만 발행합니다.
class RosTopicPublisher : public rclcpp::Node {
public:
    RosTopicPublisher();

    // 목적지 이름을 좌표로 변환하여 /goal_pose에 발행합니다.
    bool publishGoal(const std::string& destination);
  

private:
    static constexpr const char* GOAL_TOPIC = "/goal_pose";
    static constexpr const char* CMD_VEL_TOPIC = "/cmd_vel";

    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr goal_pose_pub_;

    geometry_msgs::msg::Twist current_twist_;
    std::map<std::string, Location> location_map_;
};
