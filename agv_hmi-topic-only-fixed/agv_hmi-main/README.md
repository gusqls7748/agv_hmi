# AGV HMI Server - Topic Only

## 1. 목적

이 프로젝트는 **Nav2를 실행하거나 직접 제어하지 않습니다.**

- Nav2 / AMCL / Planner / Controller / 실제 자율주행은 다른 팀이 담당합니다.
- 이 서버의 역할은 HMI에서 받은 HTTP 명령을 ROS 2 Topic으로 전달하는 것입니다.
- 기존 HMI와의 호환성을 위해 HTTP 서버는 `8080` 포트를 유지합니다.

```text
HMI
  │ HTTP
  ▼
NetworkServer :8080
  │
  ▼
RosTopicPublisher
  ├── /goal_pose  (geometry_msgs/msg/PoseStamped)
  └── /cmd_vel    (geometry_msgs/msg/Twist)
           │
           ▼
     다른 팀의 ROS 2 / Nav2
           │
           ▼
          AGV
```

> `/goal_pose`는 현재 인터페이스 예시입니다. Nav2 담당 팀에서 실제 Topic 이름/Message Type을 정하면 `RosTopicPublisher.hpp/.cpp`를 그 규격에 맞춰 변경해야 합니다.

---

## 2. 이번 수정 사항

### Nav2 직접 제어 제거

다음 Nav2 의존성을 제거했습니다.

- `Nav2Manager.hpp/.cpp`
- `nav2_msgs/`
- `rclcpp_action`
- `NavigateToPose` Action Client
- `sendGoal()` / `async_send_goal()`
- Nav2 Action 취소 로직
- 자동 `/initialpose` 발행

대신 `RosTopicPublisher`가 Topic만 발행합니다.

### 수동 모드 409 오류 수정

기존에는 서버가 먼저 `manual_mode:true`를 받은 상태에서만 `forward`, `backward`, `left`, `right`, `stop`을 허용했습니다. 그래서 단순히

```json
{"command":"forward"}
```

를 보내면 `409 manual_mode가 꺼져 있습니다`가 발생했습니다.

이번 버전에서는 두 방식 모두 지원합니다.

#### 권장 방식

```json
{"manual_mode":true,"command":"forward"}
```

한 번의 요청에서 모드와 명령을 함께 처리합니다.

#### 기존 클라이언트 호환 방식

```json
{"command":"forward"}
```

`manual_mode`가 명시되지 않은 수동 명령이면 서버가 자동으로 수동 모드로 전환한 뒤 명령을 처리합니다.

단, 다음처럼 명시적으로 `false`를 보내면 안전을 위해 거부합니다.

```json
{"manual_mode":false,"command":"forward"}
```

### 수동 `/cmd_vel` 재발행 문제 수정

`/cmd_vel`은 100ms마다 현재 Twist를 재발행합니다. `stop` 이후 timer를 취소하지 않고 0 속도를 계속 발행하므로, 다시 `forward` 등을 눌렀을 때 재발행이 끊기지 않습니다.

---

## 3. Topic 인터페이스

| 기능 | Topic | Type |
|---|---|---|
| 목적지 | `/goal_pose` | `geometry_msgs/msg/PoseStamped` |
| 수동주행 | `/cmd_vel` | `geometry_msgs/msg/Twist` |

### 목적지

현재 목적지 이름과 좌표:

```text
HOME     : (-0.13,  0.00)
room_301 : (-4.00, -3.89)
room_302 : ( 3.99,  4.13)
room_303 : ( 1.50, -2.10)
room_304 : ( 0.00,  0.00)
room_305 : ( 0.00,  0.00)
```

실제 Nav2 `map` 좌표와 일치하는지는 Nav2/맵 담당 팀과 확인해야 합니다.

---

## 4. HTTP API

### 서버

```text
127.0.0.1:8080
```

### 상태 확인

```bash
curl http://127.0.0.1:8080/api/status
```

예상 응답:

```json
{"status":"idle","wheel_rpm":0,"camera_url":"","manual_mode":false}
```

### 목적지 전송

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"destination":"room_301"}'
```

서버는 `/goal_pose`에 `PoseStamped`를 publish합니다.

### 수동주행

권장:

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"manual_mode":true,"command":"forward"}'
```

또는 기존 클라이언트와 호환:

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"forward"}'
```

지원 명령:

```text
forward
backward
left
right
stop
```

기본 속도:

```text
forward   linear.x  =  0.2
backward  linear.x  = -0.2
left      angular.z =  0.5
right     angular.z = -0.5
stop      linear.x  =  0
          angular.z =  0
```

### 수동 모드 끄기

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"manual_mode":false}'
```

수동 모드를 끄는 순간 `/cmd_vel`에 0 속도를 publish합니다.

### 정지 / cancel

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"stop"}'
```

`cancel`도 현재는 Nav2 Action 취소가 아니라 `/cmd_vel` 정지만 수행합니다. 실제 Nav2 목표 취소가 필요하면 Nav2 팀이 별도의 cancel Topic/인터페이스를 정해야 합니다.

---

## 5. 실행

ROS 2 환경이 이미 자동 설정되어 있다는 전제입니다.

### 1) 빌드

```bash
cd ~/agv_hmi-main
colcon build --packages-select agv_hmi_server
```

### 2) 설치 환경 적용

```bash
source install/setup.bash
```

### 3) 서버 실행

```bash
ros2 run agv_hmi_server agv_hmi_server
```

서버가 실행되면:

```text
AGV Headless Server Started
Listening on Port 8080
```

등의 로그가 표시됩니다.

### 4) Topic 확인

다른 터미널에서:

```bash
ros2 topic list
```

목적지 확인:

```bash
ros2 topic echo /goal_pose
```

수동주행 확인:

```bash
ros2 topic echo /cmd_vel
```

---

## 6. 실제 동작 흐름

### 자동주행

```text
HMI에서 room_301 선택
        ↓
POST /api/command
{"destination":"room_301"}
        ↓
NetworkServer
        ↓
RosTopicPublisher::publishGoal()
        ↓
/goal_pose
        ↓
Nav2 담당 팀의 노드
        ↓
AGV 이동
```

### 수동주행

```text
HMI에서 전진
        ↓
POST /api/command
{"command":"forward"}
        ↓
NetworkServer
        ↓
manual_mode 자동 활성화
        ↓
RosTopicPublisher::manualDrive()
        ↓
/cmd_vel
        ↓
로봇 Controller / Nav2 측
        ↓
AGV 이동
```

---

## 7. Nav2 담당 팀과 맞춰야 하는 것

현재 네가 담당할 부분과 Nav2 팀이 담당할 부분을 분리합니다.

### HMI Server 담당

- HTTP API 수신
- 목적지 명령을 `/goal_pose`에 publish
- 수동 명령을 `/cmd_vel`에 publish
- Topic 이름/Message Type에 맞춘 인터페이스 제공

### Nav2 담당

- Nav2 실행
- `/goal_pose` 수신 후 실제 자율주행 처리
- Planner / Controller / Costmap / AMCL 등
- 실제 AGV 제어

Nav2 팀에서 아래 정보를 최종 확정해주면 됩니다.

```text
1. 목적지 Topic 이름
2. 목적지 Message Type
3. frame_id
4. orientation을 어떤 방식으로 사용할지
5. 수동주행에 사용할 Topic과 Message Type
6. Nav2 목표 취소가 필요하다면 cancel Topic/서비스/Action 인터페이스
```

---

## 8. 문제 확인

### `/goal_pose`가 안 보이는 경우

```bash
ros2 topic list | grep goal
ros2 topic info /goal_pose
```

### `/cmd_vel`이 안 보이는 경우

```bash
ros2 topic list | grep cmd_vel
ros2 topic info /cmd_vel
```

### 서버는 실행되는데 HTTP가 안 되는 경우

```bash
curl http://127.0.0.1:8080/api/status
```

### 수동 명령이 409로 거부되는 경우

최신 버전에서는 `{"command":"forward"}`만 보내도 자동으로 수동 모드가 켜집니다.

명시적으로 수동 모드를 지정하려면:

```json
{"manual_mode":true,"command":"forward"}
```

를 사용합니다.

---

## 9. 핵심 요약

이 프로그램은 **Nav2 프로그램이 아닙니다.**

```text
HMI
 ↓ HTTP
HMI Server
 ↓ ROS2 Topic
다른 팀의 Nav2
 ↓
AGV
```

즉 네 프로그램은 **명령을 Topic으로 전달하는 역할만 담당**합니다.
