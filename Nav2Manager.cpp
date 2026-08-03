#pragma once

#include <memory>
#include <string>
#include <map>
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "std_msgs/msg/string.hpp"

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

        this->status_publisher_ = this->create_publisher<std_msgs::msg::String>("/agv_status", 10);

        // 📌 안전한 좌표로 업데이트 (테스트 검증 완료된 좌표)
        location_map_["HOME"]     = {  0.00,  0.00 }; // 원점
        location_map_["room_301"] = { -2.00, -0.50 }; // 테스트 시 작동한 안전 좌표
        location_map_["room_302"] = {  2.00,  2.00 };
        location_map_["room_303"] = {  1.50, -1.50 };
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

        publishStatus("NAVIGATING_TO_" + destination);

        sendGoal(target.x, target.y);
        return true;
    }

    void cancelAndReturnHome() {
        RCLCPP_INFO(this->get_logger(), "[취소 명령 수신] 이동 취소 후 HOME으로 복귀합니다.");

        publishStatus("CANCEL_AND_RETURNING_HOME");

        if (this->action_client_) {
            this->action_client_->async_cancel_all_goals();
        }

        if (location_map_.find("HOME") != location_map_.end()) {
            Location home = location_map_["HOME"];
            sendGoal(home.x, home.y);
        } else {
            sendGoal(0.00, 0.00);
        }
    }

private:
    rclcpp_action::Client<NavigateToPose>::SharedPtr action_client_;
    std::map<std::string, Location> location_map_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_publisher_;

    void publishStatus(const std::string& status_text) {
        if (status_publisher_) {
            auto msg = std_msgs::msg::String();
            msg.data = status_text;
            status_publisher_->publish(msg);
        }
    }

    void sendGoal(double x, double y) {
        if (!this->action_client_->wait_for_action_server(std::chrono::seconds(3))) {
            RCLCPP_ERROR(this->get_logger(), "Nav2 Action Server가 연결되지 않았습니다!");
            return;
        }

        auto goal_msg = NavigateToPose::Goal();
        goal_msg.pose.header.frame_id = "map";
        goal_msg.pose.header.stamp = this->now();

        // 💡 수정된 부분: 전달받은 x, y 좌표 할당
        goal_msg.pose.pose.position.x = x;
        goal_msg.pose.pose.position.y = y;
        goal_msg.pose.pose.position.z = 0.0;
        goal_msg.pose.pose.orientation.w = 1.0;

        auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
        
        send_goal_options.result_callback = 
            [this](const GoalHandleNavigateToPose::WrappedResult & result) {
                if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
                    RCLCPP_INFO(this->get_logger(), "🎯 목적지 도착 완료!");
                    publishStatus("ARRIVED");
                } else if (result.code == rclcpp_action::ResultCode::CANCELED) {
                    RCLCPP_INFO(this->get_logger(), "🛑 이동 명령이 취소되었습니다.");
                    publishStatus("CANCELED");
                } else {
                    RCLCPP_ERROR(this->get_logger(), "❌ 목적지 이동 실패!");
                    publishStatus("FAILED");
                }
            };

        this->action_client_->async_send_goal(goal_msg, send_goal_options);
    }
};