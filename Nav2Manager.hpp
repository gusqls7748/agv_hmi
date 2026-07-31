#ifndef NAV2_MANAGER_HPP
#define NAV2_MANAGER_HPP

#include <memory>
#include <string>
#include <map>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

struct Location {
    double x;
    double y;
};

class Nav2Manager : public rclcpp::Node {
public:
    using NavigateToPose = nav2_msgs::action::NavigateToPose;
    using GoalHandleNavigateToPose = rclcpp_action::ClientGoalHandle<NavigateToPose>;

    Nav2Manager() : Node("agv_nav2_manager") {
        this->action_client_ = rclcpp_action::create_client<NavigateToPose>(
            this, "navigate_to_pose");

        location_map_["HOME"]     = { -0.13,  0.00 };
        location_map_["room_301"] = { -4.00, -3.89 };
        location_map_["room_302"] = {  3.99,  4.13 };
        location_map_["room_303"] = {  1.50, -2.10 };
        location_map_["room_304"] = {  0.00,  0.00 };
        location_map_["room_305"] = {  0.00,  0.00 };
    }

    bool moveToLocation(const std::string& destination) {
        if (location_map_.find(destination) == location_map_.end()) {
            RCLCPP_WARN(this->get_logger(), "등록되지 않은 목적지입니다: %s", destination.c_str());
            return false;
        }

        Location target = location_map_[destination];
        RCLCPP_INFO(this->get_logger(), "[이동 명령 수신] %s -> 좌표 (x: %.2f, y: %.2f)", 
                    destination.c_str(), target.x, target.y);

        sendGoal(target.x, target.y);
        return true;
    }

    // 📌 cancelAndReturnHome 구현부
    void cancelAndReturnHome() {
        RCLCPP_INFO(this->get_logger(), "[취소 명령 수신] 이동 취소 후 HOME(-0.13, 0.00)으로 복귀합니다.");

        if (this->action_client_) {
            this->action_client_->async_cancel_all_goals();
        }

        if (location_map_.find("HOME") != location_map_.end()) {
            Location home = location_map_["HOME"];
            sendGoal(home.x, home.y);
        } else {
            sendGoal(-0.13, 0.00);
        }
    }

private:
    rclcpp_action::Client<NavigateToPose>::SharedPtr action_client_;
    std::map<std::string, Location> location_map_;

    void sendGoal(double x, double y) {
        if (!this->action_client_->wait_for_action_server(std::chrono::seconds(3))) {
            RCLCPP_ERROR(this->get_logger(), "Nav2 Action Server가 연결되지 않았습니다!");
            return;
        }

        auto goal_msg = NavigateToPose::Goal();
        goal_msg.pose.header.frame_id = "map";
        goal_msg.pose.header.stamp = this->now();

        goal_msg.pose.pose.position.x = x;
        goal_msg.pose.pose.position.y = y;
        goal_msg.pose.pose.position.z = 0.0;
        goal_msg.pose.pose.orientation.w = 1.0;

        auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
        
        send_goal_options.result_callback = 
            [this](const GoalHandleNavigateToPose::WrappedResult & result) {
                if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
                    RCLCPP_INFO(this->get_logger(), "🎯 목적지 도착 완료!");
                } else if (result.code == rclcpp_action::ResultCode::CANCELED) {
                    RCLCPP_INFO(this->get_logger(), "🛑 이동 명령이 취소되었습니다.");
                } else {
                    RCLCPP_ERROR(this->get_logger(), "❌ 목적지 이동 실패!");
                }
            };

        this->action_client_->async_send_goal(goal_msg, send_goal_options);
    }
};

#endif // NAV2_MANAGER_HPP