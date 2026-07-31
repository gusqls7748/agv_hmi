#ifndef NAV2_MANAGER_HPP
#define NAV2_MANAGER_HPP

#include <memory>
#include <string>
#include <map>
#include <chrono>
#include <functional>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "std_msgs/msg/string.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"

using namespace std::chrono_literals;

struct Location {
    double x;
    double y;
};

class Nav2Manager : public rclcpp::Node {
public:
    using NavigateToPose = nav2_msgs::action::NavigateToPose;
    using GoalHandleNavigateToPose = rclcpp_action::ClientGoalHandle<NavigateToPose>;

    Nav2Manager() : Node("agv_nav2_manager") {
        // 1. Action Client 생성
        this->action_client_ = rclcpp_action::create_client<NavigateToPose>(
            this, "navigate_to_pose");

        // 2. Publisher 생성
        this->status_publisher_ = this->create_publisher<std_msgs::msg::String>("/agv_status", 10);
        this->initial_pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>("/initialpose", 10);

        // 3. Location Map 초기화
        location_map_["HOME"]     = { -0.13,  0.00 };
        location_map_["room_301"] = { -4.00, -3.89 };
        location_map_["room_302"] = {  3.99,  4.13 };
        location_map_["room_303"] = {  1.50, -2.10 };
        location_map_["room_304"] = {  0.00,  0.00 };
        location_map_["room_305"] = {  0.00,  0.00 };

        // 📌 4. 노드 시작 1초 후 초기 위치 1회 설정
        this->timer_ = this->create_wall_timer(
            1s, std::bind(&Nav2Manager::setInitialPose, this));
    }

    bool moveToLocation(const std::string& destination) {
        auto it = location_map_.find(destination);
        if (it == location_map_.end()) {
            RCLCPP_WARN(this->get_logger(), "등록되지 않은 목적지입니다: %s", destination.c_str());
            return false;
        }

        Location target = it->second;
        RCLCPP_INFO(this->get_logger(), "[이동 명령 수신] %s -> 좌표 (x: %.2f, y: %.2f)", 
                    destination.c_str(), target.x, target.y);

        publishStatus("NAVIGATING_TO_" + destination);
        sendGoal(target.x, target.y);
        return true;
    }

    // 📌 cancelAndReturnHome 개선: async_cancel 콜백으로 귀환 타이밍 보장
    void cancelAndReturnHome() {
        RCLCPP_INFO(this->get_logger(), "[취소 명령 수신] 이동 취소 후 HOME으로 복귀 요청...");
        publishStatus("CANCEL_AND_RETURNING_HOME");

        if (this->action_client_) {
            // 현재 실행 중인 Goal들을 취소 요청하고, 취소가 끝난 후 HOME으로 이동
            this->action_client_->async_cancel_all_goals(
                [this](auto result) {
                    (void)result;
                    RCLCPP_INFO(this->get_logger(), "이전 목표 취소 완료. HOME으로 복귀합니다.");
                    
                    Location home = location_map_.count("HOME") ? location_map_["HOME"] : Location{-0.13, 0.00};
                    this->sendGoal(home.x, home.y);
                }
            );
        }
    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_publisher_;
    rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initial_pose_pub_;
    rclcpp_action::Client<NavigateToPose>::SharedPtr action_client_;
    rclcpp::TimerBase::SharedPtr timer_;
    std::map<std::string, Location> location_map_;

    void setInitialPose() {
        // 🚨 중요: 1회 발행 후 타이머 취소
        this->timer_->cancel();

        auto message = geometry_msgs::msg::PoseWithCovarianceStamped();
        message.header.frame_id = "map";
        message.header.stamp = this->now();

        message.pose.pose.position.x = -2.0;
        message.pose.pose.position.y = -0.5;
        message.pose.pose.orientation.w = 1.0;

        // AMCL 가우시안 공분산 기본값 설정
        message.pose.covariance[0] = 0.25;  // X
        message.pose.covariance[7] = 0.25;  // Y
        message.pose.covariance[35] = 0.068; // Yaw

        this->initial_pose_pub_->publish(message);
        RCLCPP_INFO(this->get_logger(), "📍 초기 위치 설정 완료 (-2.0, -0.5)");
    }

    void publishStatus(const std::string& status_text) {
        if (status_publisher_) {
            auto msg = std_msgs::msg::String();
            msg.data = status_text;
            status_publisher_->publish(msg);
        }
    }

    void sendGoal(double x, double y) {
        if (!this->action_client_->wait_for_action_server(3s)) {
            RCLCPP_ERROR(this->get_logger(), "Nav2 Action Server가 연결되지 않았습니다!");
            publishStatus("FAILED");
            return;
        }

        auto goal_msg = NavigateToPose::Goal();
        goal_msg.pose.header.frame_id = "map";
        goal_msg.pose.header.stamp = this->now();
        goal_msg.pose.pose.position.x = x;
        goal_msg.pose.pose.position.y = y;
        goal_msg.pose.pose.orientation.w = 1.0;

        auto send_goal_options = rclcpp_action::Client<NavigateToPose>::SendGoalOptions();
        
        send_goal_options.result_callback = 
            [this](const GoalHandleNavigateToPose::WrappedResult & result) {
                switch (result.code) {
                    case rclcpp_action::ResultCode::SUCCEEDED:
                        RCLCPP_INFO(this->get_logger(), "🎯 목적지 도착 완료!");
                        publishStatus("ARRIVED");
                        break;
                    case rclcpp_action::ResultCode::CANCELED:
                        RCLCPP_INFO(this->get_logger(), "🛑 이동 명령이 취소되었습니다.");
                        publishStatus("CANCELED");
                        break;
                    default:
                        RCLCPP_ERROR(this->get_logger(), "❌ 목적지 이동 실패!");
                        publishStatus("FAILED");
                        break;
                }
            };

        this->action_client_->async_send_goal(goal_msg, send_goal_options);
    }
};

#endif // NAV2_MANAGER_HPP