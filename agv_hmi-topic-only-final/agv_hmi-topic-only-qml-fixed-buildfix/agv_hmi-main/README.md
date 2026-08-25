# AGV HMI - Topic Only (Goal Pose)

## 목적
이 버전은 Nav2를 직접 실행하거나 Action을 호출하지 않고, HMI에서 선택한 목적지를 ROS 2 Topic으로 전달합니다.

## ROS 2 인터페이스
- 발행 Topic: `/goal_pose`
- Message Type: `geometry_msgs/msg/PoseStamped`
- Frame: `map`

### 중요
이 버전의 HMI Server는 `/cmd_vel`을 **생성하거나 발행하지 않습니다.**
수동주행(`forward`, `backward`, `left`, `right`, `stop`) 기능과 관련된 ROS 2 코드도 제거했습니다.

수동주행은 다른 팀의 로봇 제어 노드가 담당하는 경우 그 팀의 인터페이스를 사용합니다.

## 실행

```bash
cd /home/ubuntu/agv_hmi/agv_hmi-topic-only-qml-fixed/agv_hmi-main
colcon build --packages-select agv_hmi_server
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server
```

다른 터미널에서:

```bash
ros2 topic list
ros2 topic echo /goal_pose
```

HMI에서 목적지를 선택하면 `/goal_pose`가 publish됩니다.

## 목적지 좌표
현재 코드에는 기존 HMI 좌표가 유지되어 있습니다.

- HOME: (0.00, 0.00)
- room_301: (-2.00, -0.50)
- room_302: (2.00, 2.00)
- room_303: (1.50, -1.50)
- room_304: (0.00, 0.00)
- room_305: (0.00, 0.00)

실제 로봇의 `map` 좌표계와 일치하는지는 Nav2/로봇 담당 팀과 확인해야 합니다.

## 역할 분담

```text
HMI
  ↓ HTTP
HMI Server
  ↓ /goal_pose
Nav2 담당 노드
  ↓
AGV
```

HMI Server는 Nav2를 실행하지 않습니다.
