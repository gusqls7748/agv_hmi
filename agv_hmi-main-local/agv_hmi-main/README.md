# AGV HMI Server

ROS2/Nav2와 로컬 Qt HMI 사이를 연결하는 C++ HTTP API 서버입니다.

## 현재 실행 방식

**Qt와 C++ 서버가 같은 PC에서 실행되는 로컬 통신을 기준으로 합니다.**
Cloudflare, ngrok 등의 외부 터널은 사용하지 않습니다.

```text
Qt
 │
 │ HTTP / JSON
 ▼
127.0.0.1:8080
 │
 ▼
agv_hmi_server
 │
 ├─ GET  /api/status
 └─ POST /api/command
 │
 ▼
Nav2Manager
 │
 ▼
ROS2 / Nav2
 │
 ▼
AGV
```

## 실행 순서

### 터미널 1: ROS2 / Nav2

사용 중인 시뮬레이터 또는 로봇의 Nav2 launch를 먼저 실행합니다.

```bash
source /opt/ros/jazzy/setup.bash
# 사용하는 Nav2 / 로봇 launch 명령 실행
```

### 터미널 2: C++ 서버

```bash
source /opt/ros/jazzy/setup.bash
cd ~/agv_hmi
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server
```

정상 실행 로그:

```text
NetworkServer listening on port: 8080
```

서버는 **로컬호스트(`127.0.0.1`)에서만** 8080 포트를 수신합니다.

## Qt 연결 주소

Qt가 같은 PC에서 실행될 경우 서버 기본 주소는:

```text
http://127.0.0.1:8080
```

### 상태 조회

```text
GET http://127.0.0.1:8080/api/status
```

```bash
curl http://127.0.0.1:8080/api/status
```

### 명령 전송

```text
POST http://127.0.0.1:8080/api/command
```

목적지 이동 예:

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"destination":"room_301"}'
```

수동 모드 활성화:

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"manual_mode":true}'
```

수동 전진:

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"command":"forward"}'
```

## 소스 역할

| 파일 | 역할 |
|---|---|
| `main.cpp` | ROS2 초기화, `Nav2Manager` 생성, 8080 서버 시작 |
| `networkserver.cpp/.h` | HTTP 요청 수신/파싱 및 JSON 응답 |
| `Nav2Manager.cpp/.hpp` | Nav2 목표 이동, `/cmd_vel` 수동 제어 |
| `hmi_design.qml` | 기존 QML HMI 리소스 |

## 주의

이 버전의 `NetworkServer`는 `QHostAddress::LocalHost`에 바인딩합니다. 따라서 다른 PC에서 8080에 직접 접속하는 용도가 아니라 **같은 PC의 Qt ↔ C++ 서버 통신**에 맞춰져 있습니다.
