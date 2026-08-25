# AGV HMI Backend — 8000 / 8080 구조

## 핵심 구조

```text
Qt 팀 (원격 앱)
    │
    └── 직접/터널로 C++ API :8080
                         │
                         ▼
                  agv_hmi_server
                         │
                         └── /goal_pose
                                  │
                                  ▼
                             Nav2 / 로봇팀

Web HMI 팀
    │
    └── Web HMI :8000
              │
              └── Blazor 서버가 같은 PC의
                  127.0.0.1:8080으로 API 호출
```

### 포트 역할

- **8080**: 이 프로젝트의 C++/ROS 2 백엔드 API. Qt는 이 포트에 직접 연결합니다.
- **8000**: Blazor Web HMI가 실행되는 포트. 이 프로젝트가 8000을 열지 않습니다.
- **7090**: 이 구조에서 사용하지 않습니다.

브라우저에서 `http://<server-ip>:8080/`을 열어 웹 화면이 안 나오는 것은 정상입니다. 8080은 HTML 서버가 아니라 JSON API 서버입니다.

## 이 프로젝트가 하는 일

1. `POST /api/command`의 `{"destination":"..."}`를 받습니다.
2. 목적지 ID를 서버 내부 좌표로 변환합니다.
3. `geometry_msgs/msg/PoseStamped`를 `/goal_pose`로 publish합니다.
4. Nav2 Action을 직접 호출하지 않습니다.
5. `/cmd_vel`을 publish하지 않습니다.
6. GUI/QML을 실행하지 않습니다.
7. `GET /api/status`에서 `status`와 `manual_mode`를 제공합니다.
8. Qt가 `POST /api/manual-mode`로 수동모드 상태를 on/off 할 수 있습니다. 이 프로젝트는 실제 수동 속도 제어는 담당하지 않습니다.

## Web HMI 연결

웹 HMI가 같은 로봇/서버 PC에서 실행되는 배포 구조라면:

```text
Blazor :8000
   ↓
http://127.0.0.1:8080/api/...
```

웹 HMI가 별도 PC라면 127.0.0.1은 그 PC 자신을 의미하므로, 현재 배포 구조를 바꾸지 않는 한 같은 서버 PC에서 Blazor를 실행해야 합니다.

## Qt 연결

Qt가 다른 PC에 있으면 C++ 서버 PC의 IP로 직접 연결합니다.

예:

```text
http://192.168.0.9:8080
```

## API

### 상태

```http
GET /api/status
```

예:

```json
{
  "status": "idle",
  "manual_mode": false,
  "wheel_rpm": 0,
  "camera_url": "",
  "server": "agv_hmi_topic_only"
}
```

### 목적지

```http
POST /api/command
Content-Type: application/json
```

```json
{"destination":"room_301"}
```

현재 지원 ID:

- `restroom`
- `room_301`
- `room_302`
- `elevator`

`room_301`, `room_302`는 기존 프로젝트 좌표를 유지합니다. `restroom`, `elevator`는 최종 map 좌표가 정해질 때 환경변수로 설정합니다.

```bash
export AGV_RESTROOM_X=1.0
export AGV_RESTROOM_Y=2.0
export AGV_ELEVATOR_X=-1.0
export AGV_ELEVATOR_Y=3.0
```

좌표는 실제 Nav2 map 좌표로 교체해야 합니다.

### 수동모드 상태

```http
POST /api/manual-mode
Content-Type: application/json
```

```json
{"manual_mode":true}
```

또는

```json
{"manual_mode":false}
```

이 서버는 수동모드 상태를 저장해 `/api/status`로 반환합니다. `forward/backward/left/right` 같은 실제 주행 명령과 `/cmd_vel`은 처리하지 않습니다.

### 도착 상태

이 서버는 로봇 팀이 다음 ROS 2 topic으로 상태를 알려주는 것을 전제로 합니다.

```text
/agv_status
std_msgs/msg/String
```

허용 상태:

```text
moving
arrived
idle
failed
```

예:

```bash
ros2 topic pub --once /agv_status std_msgs/msg/String "{data: arrived}"
```

실제 로봇의 상태 topic 이름/메시지 타입이 다르면 `RosTopicPublisher.cpp`의 구독부만 Nav2/로봇 팀 인터페이스에 맞춰 변경합니다.

## 빌드

**중첩된 다른 ROS 프로젝트가 있는 홈 디렉터리에서 `colcon build`하지 마세요.** 이 프로젝트 폴더에서만 빌드합니다.

```bash
cd ~/agv_hmi
rm -rf build install log
colcon build --packages-select agv_hmi_server
source install/setup.bash
```

실제 프로젝트 경로가 다르면 `package.xml`이 있는 폴더로 이동합니다.

## 실행

```bash
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server
```

정상 로그:

```text
[agv_topic_publisher]: ROS Topic Publisher started
[NetworkServer] Listening on 0.0.0.0: 8080
AGV HMI Topic Server Started
```

## 테스트

상태:

```bash
curl http://127.0.0.1:8080/api/status
```

목적지:

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"destination":"room_301"}'
```

ROS topic 확인:

```bash
ros2 topic echo /goal_pose
```

수동모드:

```bash
curl -X POST http://127.0.0.1:8080/api/manual-mode \
  -H 'Content-Type: application/json' \
  -d '{"manual_mode":true}'
```

## 이전 프로젝트에서 수정한 핵심 문제

- Nav2 Action Client 제거
- `navigate_to_pose` 직접 호출 제거
- `/cmd_vel` publisher 제거
- `/initialpose` publisher 제거
- QML/QGuiApplication/HmiController 제거
- Qt GUI 대신 QCoreApplication만 사용
- `package.xml` 정리
- Qt5 `QByteArray` API 오류 수정
- HTTP 서버를 `0.0.0.0:8080`에 bind
- 웹 HMI의 8000 포트와 C++ 8080 포트를 분리
- `manual_mode` API 추가
- `/goal_pose` 목적지 API 유지
- `/agv_status` 구독으로 `arrived` 상태 연동 가능
- 프로젝트 내부에 다른 ZIP/중첩 ROS workspace를 넣지 않음
