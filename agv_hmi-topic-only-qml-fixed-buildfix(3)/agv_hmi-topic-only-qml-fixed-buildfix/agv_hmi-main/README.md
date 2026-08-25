
# AGV HMI Server - Topic Only + QML Manual Control

## 1. 목적

이 프로젝트는 **Nav2를 직접 실행하거나 `navigate_to_pose` Action을 호출하지 않습니다.**

역할은 다음과 같습니다.

```text
Qt/QML HMI
   │
   ├─ 자동 목적지 명령
   │
   └─ 수동 주행 명령
          │
          ▼
    HMI Server (HTTP :8080)
          │
          ▼
       ROS 2 Topic
          │
          ├─ /goal_pose
          └─ /cmd_vel
          │
          ▼
   다른 팀원의 Nav2 / Robot Controller
```

Nav2의 planner, controller, costmap, localization 등은 이 프로젝트의 담당 범위가 아닙니다.

---

## 2. 이번 수정 사항

### Nav2 직접 제어 제거

다음 항목을 제거했습니다.

- `Nav2Manager.hpp`
- `Nav2Manager.cpp`
- `nav2_msgs`
- `rclcpp_action`
- `NavigateToPose`
- `navigate_to_pose` Action Client
- `/initialpose` 자동 발행
- Nav2 Action cancel/return-home 로직

따라서 이 프로그램은 Nav2를 실행하지 않습니다.

### Topic Publisher로 변경

`RosTopicPublisher`가 다음 Topic만 발행합니다.

| 용도 | Topic | Message Type |
|---|---|---|
| 자동 목적지 | `/goal_pose` | `geometry_msgs/msg/PoseStamped` |
| 수동 주행 | `/cmd_vel` | `geometry_msgs/msg/Twist` |

> `/goal_pose`는 현재 임시 인터페이스입니다. Nav2 담당 팀이 실제 Topic 이름/Message Type을 정하면 그 규격으로 변경해야 합니다.

---

## 3. 수동 주행 처리

기존에는 QML 버튼이 `console.log()`만 실행하여 실제 서버 명령을 보내지 않았습니다.

이번 버전에서는 QML 버튼이 실제 `HmiController`를 통해 서버에 연결됩니다.

### Forward

```text
Forward 버튼 누름
      ↓
hmiController.sendCommand("forward")
      ↓
NetworkServer
      ↓
manual_mode 자동 ON
      ↓
RosTopicPublisher
      ↓
/cmd_vel
linear.x = 0.2
```

### Backward

```text
/cmd_vel
linear.x = -0.2
```

### Left

```text
/cmd_vel
angular.z = 0.5
```

### Right

```text
/cmd_vel
angular.z = -0.5
```

### STOP

```text
/cmd_vel
linear.x = 0
angular.z = 0
```

수동 이동 버튼은 **누르고 있는 동안 이동하고 손을 떼면 STOP**을 보냅니다.

또한 `RosTopicPublisher`는 현재 Twist를 100ms마다 계속 발행합니다. 따라서 로봇 쪽에 cmd_vel watchdog이 있어도 한 번의 명령만 보내고 멈추는 문제가 생기지 않도록 했습니다.

---

## 4. manual_mode 처리

기존 오류:

```text
409 Conflict
manual_mode가 꺼져있어 수동 조종을 거부했습니다.
manual_mode:true를 보내주세요
```

를 방지하도록 수정했습니다.

### QML에서

수동 모드 Switch를 켜면:

```text
hmiController.setManualMode(true)
```

가 호출됩니다.

그리고 방향 버튼을 누르면:

```text
hmiController.sendCommand("forward")
```

가 호출됩니다.

### HTTP에서 기존 클라이언트가 다음처럼 보내도 동작합니다.

```json
{
  "command": "forward"
}
```

서버가 자동으로 manual mode를 켭니다.

또는 명시적으로:

```json
{
  "manual_mode": true,
  "command": "forward"
}
```

도 가능합니다.

반대로:

```json
{
  "manual_mode": false
}
```

를 보내면 수동 모드가 꺼지고 현재 `/cmd_vel`을 0으로 만들어 정지합니다.

---

## 5. QML 연결

기존 QML은 방향 버튼을 눌러도:

```qml
onPressed: console.log("Manual: Move Forward")
```

만 실행했습니다.

이번 버전은 실제 명령을 호출합니다.

```qml
onPressed: hmiController.sendCommand("forward")
onReleased: hmiController.sendCommand("stop")
```

QML은 `main.cpp`에서 다음 이름으로 연결됩니다.

```text
hmiController
```

즉 QML에서:

```qml
hmiController.isManualMode
hmiController.setManualMode(...)
hmiController.sendCommand(...)
hmiController.sendDestination(...)
```

를 사용할 수 있습니다.

---

## 6. 자동주행

자동 목적지 버튼은 예를 들어:

```text
HOME
301
302
303
```

을 누르면 다음과 같이 동작합니다.

```text
QML
 ↓
hmiController.sendDestination("room_301")
 ↓
NetworkServer
 ↓
RosTopicPublisher
 ↓
/goal_pose
```

현재 좌표:

```text
HOME      ( 0.00,  0.00)
room_301  (-2.00, -0.50)
room_302  ( 2.00,  2.00)
room_303  ( 1.50, -1.50)
room_304  ( 0.00,  0.00)
room_305  ( 0.00,  0.00)
```

좌표는 기존 프로젝트에서 사용하던 테스트 좌표를 기준으로 유지했습니다.

**실제 Nav2 map 좌표와 반드시 확인해야 합니다.**

---

## 7. 실행 방법

ROS 2 환경이 이미 자동으로 설정되어 있다는 전제입니다.

### 1) 프로젝트 이동

```bash
cd ~/agv_hmi-main
```

### 2) 빌드

```bash
colcon build --packages-select agv_hmi_server
```

### 3) 설치 환경 적용

```bash
source install/setup.bash
```

### 4) 서버 + QML 실행

```bash
ros2 run agv_hmi_server agv_hmi_server
```

이번 버전은 서버와 QML이 같은 프로그램에서 실행됩니다.

실행되면:

```text
HTTP: http://127.0.0.1:8080
Goal Topic: /goal_pose
CmdVel Topic: /cmd_vel
```

메시지가 표시됩니다.

---

## 8. Topic 확인

별도 터미널에서:

### 수동주행 확인

```bash
ros2 topic echo /cmd_vel
```

HMI에서 수동 모드를 켜고 Forward를 누르면:

```text
linear:
  x: 0.2
angular:
  z: 0.0
```

가 반복해서 출력되어야 합니다.

버튼에서 손을 떼면:

```text
linear:
  x: 0.0
angular:
  z: 0.0
```

이 출력되어야 합니다.

### 자동 목적지 확인

```bash
ros2 topic echo /goal_pose
```

HMI에서 목적지를 선택하면 `PoseStamped`가 출력됩니다.

---

## 9. HTTP 서버 테스트

QML을 사용하지 않고 서버만 테스트하려면 같은 PC에서:

### 수동 전진

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"command":"forward"}'
```

### 수동 정지

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"command":"stop"}'
```

### 수동 모드 OFF

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"manual_mode":false}'
```

### 자동 목적지

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"destination":"room_301"}'
```

### 상태 확인

```bash
curl http://127.0.0.1:8080/api/status
```

---

## 10. 팀원 간 역할

### HMI / Server 담당

- QML UI
- HTTP Server
- `/goal_pose` publish
- `/cmd_vel` publish
- 사용자 입력 처리

### Nav2 담당

- Nav2 실행
- `/goal_pose`를 실제 Nav2 주행으로 연결
- 경로 계획
- 장애물 회피
- localization
- controller
- 실제 AGV 구동

---

## 11. Nav2 팀에게 확인해야 할 것

최종 연결 전에 아래 정보를 받아야 합니다.

```text
① 자동주행 명령 Topic 이름
② 자동주행 Message Type
③ 좌표 frame_id
④ /cmd_vel을 Nav2/Controller가 직접 받을지
⑤ /cmd_vel의 속도 제한
```

현재 기본값은:

```text
/goal_pose
geometry_msgs/msg/PoseStamped

frame_id = map

/cmd_vel
geometry_msgs/msg/Twist
```

입니다.

---

## 12. 중요

이 프로그램은 **Nav2를 실행하지 않습니다.**

따라서 Nav2가 실행되지 않은 상태에서도 서버/QML 자체는 실행될 수 있지만, `/goal_pose`를 받은 뒤 실제 로봇이 움직이는 것은 Nav2/로봇 Controller가 해당 Topic을 구독하고 있을 때만 가능합니다.

최종적으로 Nav2 팀과 Topic 인터페이스를 맞춘 후 사용하세요.
