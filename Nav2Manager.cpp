#pragma once

#include <memory>
#include <string>
#include <map>
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "std_msgs/msg/string.hpp" // 1. Publisher용 메시지 헤더 추가

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

        // 2. Publisher 생성 (/agv_status 토픽, 큐 크기 10)
        this->status_publisher_ = this->create_publisher<std_msgs::msg::String>("/agv_status", 10);

        // 📌 기존 좌표 데이터 그대로 유지!
        location_map_["HOME"]     = { -0.13,  0.00 }; // 취소 시 복귀할 원점
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

        // 3. [토픽 발행] 목적지 이동 토픽 쏘기
        publishStatus("NAVIGATING_TO_" + destination);

        sendGoal(target.x, target.y);
        return true;
    }

    // 취소 명령 수신 시: 진행 중인 Goal 취소 후 HOME(-0.13, 0.00)으로 이동
    void cancelAndReturnHome() {
        RCLCPP_INFO(this->get_logger(), "[취소 명령 수신] 이동 취소 후 HOME(-0.13, 0.00)으로 복귀합니다.");

        // 4. [토픽 발행] 취소 및 귀환 시작 토픽 쏘기
        publishStatus("CANCEL_AND_RETURNING_HOME");

        // 1. 현재 수행 중인 모든 Goal 액션 취소
        if (this->action_client_) {
            this->action_client_->async_cancel_all_goals();
        }

        // 2. HOME 좌표로 복귀 명령 전송
        if (location_map_.find("HOME") != location_map_.end()) {
            Location home = location_map_["HOME"];
            sendGoal(home.x, home.y);
        } else {
            // HOME 키가 없더라도 안전하게 Default (-0.13, 0.00) 전송
            sendGoal(-0.13, 0.00);
        }
    }

private:
    rclcpp_action::Client<NavigateToPose>::SharedPtr action_client_;
    std::map<std::string, Location> location_map_;

    // 5. Publisher 멤버 변수 선언
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_publisher_;

    // 6. 토픽 발행 헬퍼 함수 (상태 문자열 발행)
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

        goal_msg.pose.pose.position.x = x;
        goal_msg.pose.pose.position.y = y;
        goal_msg.pose.pose.position.z = 0.0;
        goal_msg.pose.pose.orientation.w = 1.0;

        auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
        
        send_goal_options.result_callback = 
            [this](const GoalHandleNavigateToPose::WrappedResult & result) {
                if (result.code == rclcpp_action::ResultCode::SUCCEEDED) {
                    RCLCPP_INFO(this->get_logger(), "🎯 목적지 도착 완료!");
                    // 7. [토픽 발행] 도착 완료 토픽
                    publishStatus("ARRIVED");
                } else if (result.code == rclcpp_action::ResultCode::CANCELED) {
                    RCLCPP_INFO(this->get_logger(), "🛑 이동 명령이 취소되었습니다.");
                    // 8. [토픽 발행] 취소 상태 토픽
                    publishStatus("CANCELED");
                } else {
                    RCLCPP_ERROR(this->get_logger(), "❌ 목적지 이동 실패!");
                    // 9. [토픽 발행] 이동 실패 토픽
                    publishStatus("FAILED");
                }
            };

        this->action_client_->async_send_goal(goal_msg, send_goal_options);
    }
};