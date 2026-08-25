# AGV HMI Topic-Only Server — Final

## 역할

이 프로그램은 **UI/Qt 프로그램과 ROS 2 사이의 서버 역할만** 합니다.

- UI/Qt 프로그램: 다른 PC에서 HTTP 요청을 보냄
- 이 서버: HTTP 요청을 받아 목적지를 ROS 2 Topic으로 publish
- Nav2: 다른 팀이 실행
- 이 서버는 Nav2를 직접 실행하지 않음
- 이 서버는 GUI/QML을 실행하지 않음

구조:

```text
[UI / Qt 팀]
       |
       | HTTP
       | http://192.168.0.9:8080
       v
[이 서버]
       |
       | /goal_pose
       v
[Nav2 / Robot 팀]
```

## ROS 2 Topic

발행:

```text
/goal_pose
```

메시지:

```text
geometry_msgs/msg/PoseStamped
```

frame:

```text
map
```

이 서버는 로봇 속도 제어 Topic을 발행하지 않습니다.

## HTTP API

### 서버 상태

```http
GET /api/status
```

예:

```json
{
  "status": "idle",
  "server": "agv_hmi_topic_only"
}
```

### 목적지 전송

```http
POST /api/command
Content-Type: application/json
```

Body:

```json
{
  "destination": "room_301"
}
```

성공 응답:

```json
{
  "result": "success",
  "message": "Goal topic published",
  "destination": "room_301"
}
```

지원 목적지:

```text
HOME
room_301
room_302
room_303
room_304
room_305
```

현재 좌표:

```text
HOME     -> ( 0.00,  0.00)
room_301 -> (-2.00, -0.50)
room_302 -> ( 2.00,  2.00)
room_303 -> ( 1.50, -1.50)
room_304 -> ( 0.00,  0.00)
room_305 -> ( 0.00,  0.00)
```

실제 로봇 map 좌표와 맞는지는 Nav2/로봇 팀과 확인해야 합니다.

## 실행 순서

### 1. 서버 PC

```bash
cd /home/ubuntu/agv_hmi/agv_hmi-topic-only-qml-fixed/agv_hmi-main
colcon build --packages-select agv_hmi_server
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server
```

서버는 `0.0.0.0:8080`으로 listen 하므로 같은 네트워크의 다른 PC에서 접근할 수 있습니다.

서버 PC IP 확인:

```bash
hostname -I
```

예:

```text
192.168.0.9
```

그러면 UI/Qt 팀은:

```text
http://192.168.0.9:8080
```

을 서버 주소로 사용합니다.

### 2. ROS 2 Topic 확인

새 터미널:

```bash
ros2 topic list
```

그리고:

```bash
ros2 topic echo /goal_pose
```

UI/Qt에서 목적지를 전송했을 때 메시지가 나오면 연결이 정상입니다.

## 팀원에게 설명

> 나는 UI를 실행하는 역할이 아니라 HTTP 서버를 실행하는 역할이다.
> UI/Qt는 `http://서버PC_IP:8080/api/command`로 목적지를 보낸다.
> 서버는 그 목적지를 `/goal_pose` (`geometry_msgs/msg/PoseStamped`) Topic으로 publish한다.
> Nav2는 다른 팀이 담당하고 별도로 실행한다.

## 이번 수정 사항

1. `package.xml`
   - XML 선언 위치 문제를 수정했습니다.
   - ROS 2 `ament_package`가 정상적으로 읽을 수 있게 했습니다.

2. Qt/QML 실행 제거
   - 서버 PC에서 QML 화면을 띄우지 않습니다.
   - 따라서 서버 실행 시 GUI/EGL/MESA 경고가 발생하지 않는 구조입니다.
   - UI는 외부 UI/Qt 팀 프로그램이 담당합니다.

3. 네트워크 bind 수정
   - `localhost` 전용 listen이 아니라 `0.0.0.0`으로 listen합니다.
   - 따라서 UI/Qt가 다른 PC에서 `192.168.0.9:8080`으로 접속할 수 있습니다.

4. ROS Topic 전용 구조
   - 목적지 요청만 `/goal_pose`로 publish합니다.
   - Nav2 Action을 직접 호출하지 않습니다.

5. 수동 주행 코드 제거
   - 수동 주행 관련 API/상태/코드를 서버에서 제거했습니다.
   - UI/Qt가 수동 주행 명령을 보내더라도 이 서버에서는 처리하지 않습니다.

6. HTTP 요청 수신 보완
   - TCP 패킷이 여러 번 나뉘어 들어오는 경우를 고려해 요청 버퍼를 추가했습니다.
