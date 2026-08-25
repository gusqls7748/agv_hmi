#include "RosTopicPublisher.hpp"

using namespace std::chrono_literals;

RosTopicPublisher::RosTopicPublisher()
    : Node("agv_topic_publisher") {
    goal_pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>(
        GOAL_TOPIC, 10);

    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(
        CMD_VEL_TOPIC, 10);

    // 현재 Twist 값을 100ms마다 재발행합니다.
    // stop 이후에도 timer를 유지하므로 다음 수동 명령에서 재시작 문제를 방지합니다.
    manual_drive_timer_ = this->create_wall_timer(100ms, [this]() {
        if (rclcpp::ok()) {
            cmd_vel_pub_->publish(current_twist_);
        }
    });

    // 기존 HMI에서 사용하던 목적지 좌표를 유지합니다.
    // 실제 Nav2/로봇 팀의 좌표계와 맞는지 확인해서 수정하세요.
    location_map_["HOME"]     = { 0.00,  0.00 };
    location_map_["room_301"] = { -2.00, -0.50 };
    location_map_["room_302"] = {  2.00,  2.00 };
    location_map_["room_303"] = {  1.50, -1.50 };
    location_map_["room_304"] = {  0.00,  0.00 };
    location_map_["room_305"] = {  0.00,  0.00 };

    RCLCPP_INFO(this->get_logger(), "ROS Topic Publisher started");
    RCLCPP_INFO(this->get_logger(), "Goal topic: %s", GOAL_TOPIC);
    RCLCPP_INFO(this->get_logger(), "CmdVel topic: %s", CMD_VEL_TOPIC);
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

bool RosTopicPublisher::manualDrive(const std::string& direction) {
    constexpr double LINEAR_SPEED = 0.2;
    constexpr double ANGULAR_SPEED = 0.5;

    geometry_msgs::msg::Twist twist;

    if (direction == "forward") {
        twist.linear.x = LINEAR_SPEED;
    } else if (direction == "backward") {
        twist.linear.x = -LINEAR_SPEED;
    } else if (direction == "left") {
        twist.angular.z = ANGULAR_SPEED;
    } else if (direction == "right") {
        twist.angular.z = -ANGULAR_SPEED;
    } else if (direction == "stop") {
        // 0으로 유지
    } else {
        RCLCPP_WARN(
            this->get_logger(),
            "알 수 없는 수동 제어 명령입니다: %s",
            direction.c_str());
        return false;
    }

    current_twist_ = twist;
    cmd_vel_pub_->publish(current_twist_);

    RCLCPP_INFO(
        this->get_logger(),
        "[cmd_vel] %s (linear.x=%.2f, angular.z=%.2f)",
        direction.c_str(), twist.linear.x, twist.angular.z);

    // timer는 생성자에서 항상 실행됩니다. stop 이후에도 0 속도를 계속 발행하고,
    // 다음 forward/backward 명령이 들어오면 current_twist_만 바뀌어 즉시 재개됩니다.
    return true;
}

void RosTopicPublisher::stopManualDrive() {
    current_twist_ = geometry_msgs::msg::Twist();
    cmd_vel_pub_->publish(current_twist_);

    RCLCPP_INFO(this->get_logger(), "[cmd_vel] stop");
}
