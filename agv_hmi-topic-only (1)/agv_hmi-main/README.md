# AGV HMI Server - Topic Only Version

## 1. 프로젝트 목적

이 버전의 AGV HMI Server는 **Nav2를 직접 실행하거나 제어하지 않습니다.**

- Nav2 / AMCL / Controller / Planner 등 자율주행 기능은 **다른 팀원이 Raspberry Pi에서 담당**합니다.
- 이 프로그램의 역할은 HMI에서 들어온 명령을 받아 **ROS2 Topic으로 발행하는 것**입니다.
- 기존 HMI와의 연결 방식을 최대한 유지하기 위해 HTTP 서버(`:8080`)는 그대로 사용합니다.

즉 전체 구조는 다음과 같습니다.

```text
Qt HMI
   |
   | HTTP :8080
   v
AGV HMI Server
(NetworkServer)
   |
   v
RosTopicPublisher
   |-----------------------> /goal_pose
   |                         geometry_msgs/msg/PoseStamped
   |
   +-----------------------> /cmd_vel
                             geometry_msgs/msg/Twist
                                      |
                                      v
                         Raspberry Pi의 ROS2 / Nav2
                         (다른 팀원 담당)
```

> **중요:** `/goal_pose`는 현재 임시 인터페이스입니다. Nav2 담당 팀원이 실제로 받기로 한 Topic 이름과 Message Type이 따로 있다면 `RosTopicPublisher.hpp`의 Topic 설정과 메시지 타입을 그 규격에 맞춰 변경해야 합니다.

---

## 2. 이번 버전에서 수정된 내용

### 2.1 Nav2 Action Client 제거

기존에는 HMI 서버가 Nav2의 `NavigateToPose` Action을 직접 호출하는 구조였습니다.

기존 구조:

```text
HMI
 ↓
NetworkServer
 ↓
Nav2Manager
 ↓
NavigateToPose Action
 ↓
Nav2
```

이번 버전에서는 Nav2 Action을 사용하지 않습니다.

변경 후:

```text
HMI
 ↓
NetworkServer
 ↓
RosTopicPublisher
 ↓
ROS2 Topic
 ↓
다른 팀원의 Nav2
```

따라서 다음 Nav2 전용 코드가 제거되었습니다.

- `Nav2Manager.hpp`
- `Nav2Manager.cpp`
- `nav2_msgs/`
- `rclcpp_action`
- `nav2_msgs`
- `NavigateToPose` Action Client
- `sendGoal()`
- `async_send_goal()`
- Nav2 Goal 취소 처리

---

### 2.2 `Nav2Manager`를 `RosTopicPublisher`로 변경

기존의 `Nav2Manager`를 제거하고 다음 두 파일을 추가했습니다.

```text
RosTopicPublisher.hpp
RosTopicPublisher.cpp
```

이 클래스의 역할은 **ROS2 Topic을 발행하는 것뿐**입니다.

현재 담당 기능:

```text
publishGoal()
    → /goal_pose

manualDrive()
    → /cmd_vel

stopManualDrive()
    → /cmd_vel에 0값 발행
```

---

## 3. 현재 사용하는 ROS2 Topic

| 기능 | Topic | Message Type |
|---|---|---|
| 자동주행 목적지 | `/goal_pose` | `geometry_msgs/msg/PoseStamped` |
| 수동주행 | `/cmd_vel` | `geometry_msgs/msg/Twist` |

### 자동주행

HMI에서:

```json
{"destination":"room_301"}
```

을 보내면 서버가 목적지를 찾아 `PoseStamped`를 `/goal_pose`에 발행합니다.

예시:

```text
header.frame_id = "map"
position.x = 목적지 X
position.y = 목적지 Y
orientation.w = 1.0
```

**주의:** 현재 프로젝트에 들어 있는 좌표와 `map` 좌표계가 실제 로봇의 좌표계와 맞는지는 Nav2/로봇 팀과 반드시 확인해야 합니다.

---

## 4. 수동주행

수동주행은 `/cmd_vel`로 `geometry_msgs/msg/Twist`를 발행합니다.

지원 명령:

```text
forward
backward
left
right
stop
```

현재 코드의 기본 속도는 다음과 같습니다.

```text
forward   : linear.x  =  0.2
backward  : linear.x  = -0.2
left      : angular.z =  0.5
right     : angular.z = -0.5
stop      : linear.x  = 0
            angular.z = 0
```

수동주행 중에는 `cmd_vel`을 100ms 주기로 다시 발행하도록 되어 있습니다. 로봇 Controller의 watchdog 요구사항이 있는 경우를 고려한 동작입니다.

---

## 5. HTTP API는 기존 구조를 유지

ROS2 Topic만 보내도록 변경했지만 기존 HMI를 최대한 수정하지 않도록 HTTP 서버는 유지했습니다.

서버 주소:

```text
http://127.0.0.1:8080
```

### 5.1 자동주행 목적지

```http
POST /api/command
Content-Type: application/json
```

Request:

```json
{
  "destination": "room_301"
}
```

처리 과정:

```text
POST /api/command
       ↓
NetworkServer
       ↓
RosTopicPublisher::publishGoal()
       ↓
/goal_pose
       ↓
다른 팀원의 Nav2
```

---

### 5.2 수동 모드 활성화

```http
POST /api/command
Content-Type: application/json
```

```json
{
  "manual_mode": true
}
```

수동 모드를 켠 후 수동 명령을 사용할 수 있습니다.

---

### 5.3 수동 이동

전진:

```json
{
  "command": "forward"
}
```

후진:

```json
{
  "command": "backward"
}
```

좌회전:

```json
{
  "command": "left"
}
```

우회전:

```json
{
  "command": "right"
}
```

정지:

```json
{
  "command": "stop"
}
```

---

## 6. `cancel` 동작 변경

기존에는 `cancel` 명령이 Nav2 Goal을 취소하는 동작과 연결되어 있었습니다.

현재 버전에서는 Nav2 Action을 사용하지 않기 때문에 **Nav2 Goal 취소는 하지 않습니다.**

현재 `cancel`은 수동주행 중인 경우 `/cmd_vel`에 정지값을 발행하는 용도로만 처리됩니다.

```text
cancel
 ↓
stopManualDrive()
 ↓
/cmd_vel = 0
```

Nav2의 자동주행 Goal까지 취소해야 하는 경우에는 Nav2 담당 팀원이 별도의 cancel Topic을 정의해야 합니다.

---

## 7. 제거된 `/initialpose` 자동 발행

이번 버전에서는 HMI 서버가 `/initialpose`를 자동으로 발행하지 않습니다.

이유는 초기 위치 설정 역시 Nav2/AMCL을 담당하는 팀의 영역으로 분리하기 위해서입니다.

따라서 초기 위치 설정이 필요하다면 Nav2 담당 팀에서 사용하는 방식에 맞춰 처리해야 합니다.

---

## 8. 파일별 역할

### `RosTopicPublisher.hpp`

ROS2 Publisher와 목적지 좌표 등을 정의합니다.

주요 Publisher:

```text
/goal_pose
/cmd_vel
```

### `RosTopicPublisher.cpp`

실제로 ROS2 Topic을 발행합니다.

주요 함수:

```text
publishGoal()
manualDrive()
stopManualDrive()
```

### `networkserver.h / networkserver.cpp`

기존 HMI의 HTTP 요청을 처리합니다.

예:

```text
POST /api/command
```

로 들어온 JSON을 분석하고 `RosTopicPublisher`에 전달합니다.

### `main.cpp`

프로그램을 시작하고 다음을 실행합니다.

```text
QCoreApplication
NetworkServer :8080
RosTopicPublisher
ROS2 spin_some()
```

### `CMakeLists.txt`

Nav2 관련 의존성을 제거하고 다음 ROS2 패키지만 사용하도록 변경했습니다.

```text
rclcpp
geometry_msgs
std_msgs
```

### `package.xml`

다음 Nav2 관련 의존성을 제거했습니다.

```text
rclcpp_action
nav2_msgs
```

---

## 9. 실행 전 준비

### ROS2

현재 프로젝트는 ROS2 Jazzy 기준입니다.

```bash
source /opt/ros/jazzy/setup.bash
```

### Qt5

`CMakeLists.txt`에서 Qt5를 사용합니다.

필요한 Qt 모듈:

```text
Core
Gui
Qml
Quick
Network
```

Qt5 개발 환경이 설치되어 있어야 합니다.

---

# 10. 실행 방법

## Terminal 1 - Nav2 담당 팀

이 프로젝트에서는 Nav2를 실행하지 않습니다.

Nav2 담당 팀원이 Raspberry Pi에서 평소 사용하는 Nav2 실행 명령을 먼저 실행합니다.

예:

```bash
source /opt/ros/jazzy/setup.bash

# Nav2 담당 팀에서 사용하는 launch 명령 실행
```

> 구체적인 Nav2 launch 명령은 이 프로젝트에서 정의하지 않습니다.

---

## Terminal 2 - AGV HMI Server

프로젝트 폴더로 이동합니다.

```bash
source /opt/ros/jazzy/setup.bash
cd ~/agv_hmi-main-local/agv_hmi-main
```

처음 실행하거나 소스를 수정했다면 빌드합니다.

```bash
colcon build --packages-select agv_hmi_server
```

빌드가 끝나면 환경을 적용합니다.

```bash
source install/setup.bash
```

서버를 실행합니다.

```bash
ros2 run agv_hmi_server agv_hmi_server
```

정상적으로 실행되면 대략 다음과 같은 로그가 표시됩니다.

```text
AGV Headless Server Started (No GUI)
Listening on Port 8080...
```

---

## Terminal 3 - ROS2 Topic 확인

서버가 실행된 상태에서 새 터미널을 엽니다.

```bash
source /opt/ros/jazzy/setup.bash
```

Topic 목록 확인:

```bash
ros2 topic list
```

목적지 Topic 확인:

```bash
ros2 topic echo /goal_pose
```

수동주행 Topic 확인:

```bash
ros2 topic echo /cmd_vel
```

---

# 11. Topic 테스트

## 목적지 Topic 직접 테스트

HMI를 거치지 않고 Topic 자체가 정상적으로 발행되는지 확인할 수 있습니다.

```bash
ros2 topic pub --once /goal_pose geometry_msgs/msg/PoseStamped \
"{header: {frame_id: map}, pose: {position: {x: 0.0, y: 0.0}, orientation: {w: 1.0}}}"
```

다른 터미널에서:

```bash
ros2 topic echo /goal_pose
```

메시지가 보이면 Topic 통신 자체는 정상입니다.

> 이 테스트는 Nav2가 해당 Topic을 실제로 구독하고 있다는 것을 보장하지는 않습니다. Nav2 담당 팀의 실제 Subscriber Topic 이름과 타입을 확인해야 합니다.

---

## `/cmd_vel` 직접 테스트

```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
"{linear: {x: 0.2}, angular: {z: 0.0}}"
```

정지:

```bash
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
"{linear: {x: 0.0}, angular: {z: 0.0}}"
```

**실제 AGV가 연결되어 있는 상태에서는 안전을 확인한 후 테스트해야 합니다.**

---

# 12. HMI를 통한 전체 테스트 순서

```text
1. Raspberry Pi에서 Nav2 실행
        ↓
2. AGV HMI Server 실행
        ↓
3. /goal_pose 또는 /cmd_vel echo 실행
        ↓
4. Qt HMI 실행
        ↓
5. HMI에서 목적지 선택 또는 수동주행
        ↓
6. NetworkServer가 HTTP 요청 수신
        ↓
7. RosTopicPublisher가 ROS2 Topic 발행
        ↓
8. Topic을 다른 팀원의 ROS2/Nav2가 수신
```

---

# 13. 다른 팀원에게 확인해야 하는 것

현재 `/goal_pose`는 임시 Topic입니다.

Nav2 담당 팀에게 다음 정보를 받아야 합니다.

```text
[필수]
1. 자동주행 명령 Topic 이름
2. 자동주행 Message Type
3. 좌표계(frame_id)
4. 목적지 좌표를 어떤 형식으로 전달할지
5. 수동주행 Topic 이름
6. 수동주행 Message Type
```

예를 들어 Nav2 담당 팀이 다음과 같이 정하면:

```text
Topic: /goal_pose
Type : geometry_msgs/msg/PoseStamped
Frame: map
```

현재 코드와 그대로 연결할 수 있습니다.

만약 다른 이름을 사용한다면 `RosTopicPublisher.hpp/.cpp`의 Topic 이름과 메시지 타입을 그 규격에 맞춰 수정해야 합니다.

---

# 14. 역할 분담

## HMI / 이 프로젝트 담당

```text
Qt HMI
HTTP API
ROS2 Topic Publish
/goal_pose
/cmd_vel
```

## Nav2 담당 팀

```text
Nav2
AMCL / Localization
Planner
Controller
Costmap
Robot Base Controller
목적지 Topic Subscriber
필요한 Goal 처리
```

즉 이 프로젝트는 **Nav2가 어떻게 움직이는지 알 필요가 없고, 약속된 ROS2 Topic을 정확하게 발행하는 것**이 핵심입니다.

---

# 15. 문제 해결

### 서버가 실행되지 않는 경우

```bash
source /opt/ros/jazzy/setup.bash
cd ~/agv_hmi-main-local/agv_hmi-main
source install/setup.bash
ros2 pkg list | grep agv_hmi
```

다음이 출력되는지 확인합니다.

```text
agv_hmi_server
```

### Topic이 보이지 않는 경우

서버가 실행 중인지 확인합니다.

```bash
ros2 node list
```

그리고:

```bash
ros2 topic list
```

### `/goal_pose`는 나오는데 Nav2가 반응하지 않는 경우

이 경우 HMI 서버 문제가 아니라 **Topic 인터페이스가 Nav2 담당 팀의 규격과 맞는지 먼저 확인**해야 합니다.

확인할 것:

```bash
ros2 topic info /goal_pose
ros2 interface show geometry_msgs/msg/PoseStamped
```

Nav2 담당 팀이 실제로 해당 Topic을 subscribe하는지도 확인합니다.

---

# 16. 한 줄 요약

이 프로젝트의 역할은 다음 한 문장으로 정리할 수 있습니다.

> **HMI 명령을 받아 ROS2 Topic으로 보내고, 그 이후의 자율주행은 다른 팀원의 Nav2가 담당한다.**
