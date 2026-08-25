#pragma once

#include <map>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "std_msgs/msg/string.hpp"

struct Location {
    double x{0.0};
    double y{0.0};
};

class RosTopicPublisher : public rclcpp::Node {
public:
    RosTopicPublisher();

    bool publishGoal(const std::string& destination);
    bool hasDestination(const std::string& destination) const;

    std::string status() const { return status_; }
    void setStatus(const std::string& status);

private:
    void loadLocations();
    bool loadOptionalLocationFromEnv(const char* x_name,
                                     const char* y_name,
                                     const std::string& key);
    void onAgvStatus(const std_msgs::msg::String::SharedPtr msg);

    static constexpr const char* GOAL_TOPIC = "/goal_pose";
    static constexpr const char* AGV_STATUS_TOPIC = "/agv_status";

    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr goal_pose_pub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr agv_status_sub_;
    std::map<std::string, Location> location_map_;
    std::string status_ = "idle";
};
