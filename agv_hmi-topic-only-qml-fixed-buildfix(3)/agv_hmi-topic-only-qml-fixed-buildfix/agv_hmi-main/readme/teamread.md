# 팀 실행 가이드 - 로컬 Qt 연동

## 1. ROS2 / Nav2 실행

터미널 1:

```bash
source /opt/ros/jazzy/setup.bash
# 사용하는 Nav2 / 시뮬레이터 launch 명령 실행
```

## 2. C++ 백엔드 실행

터미널 2:

```bash
source /opt/ros/jazzy/setup.bash
cd ~/agv_hmi
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server
```

정상 실행:

```text
NetworkServer listening on port: 8080
```

## 3. Qt 연결

Qt와 C++ 서버는 같은 PC에서 실행합니다.

```text
http://127.0.0.1:8080
```

API:

```text
GET  /api/status
POST /api/command
```

### 상태 조회

```text
GET http://127.0.0.1:8080/api/status
```

### 목적지 이동

```text
POST http://127.0.0.1:8080/api/command
Content-Type: application/json

{"destination":"room_301"}
```

### 수동 모드

```text
POST http://127.0.0.1:8080/api/command
Content-Type: application/json

{"manual_mode":true}
```

### 수동 조종

```text
POST http://127.0.0.1:8080/api/command
Content-Type: application/json

{"command":"forward"}
```

## 전체 구조

```text
┌───────────────┐
│      Qt       │
│ 같은 PC에서 실행 │
└───────┬───────┘
        │ HTTP / JSON
        ▼
┌────────────────────┐
│ 127.0.0.1:8080     │
│ agv_hmi_server      │
└─────────┬──────────┘
          │
          ▼
┌────────────────────┐
│    Nav2Manager      │
└─────────┬──────────┘
          │ ROS2
          ▼
┌────────────────────┐
│      Nav2 / AGV     │
└────────────────────┘
```

## 더 이상 사용하지 않는 것

```text
cloudflared
ngrok
trycloudflare.com
외부 8080 터널 주소
외부 8000 터널 주소
```

모두 필요 없습니다.
