좋아. 지금까지 확인한 **실제 실행 결과**를 기준으로, 다른 팀원이 받아서 바로 실행할 수 있는 README 형태로 정리해줄게.

핵심은 **Cloudflare 없이 같은 PC에서 Qt ↔ C++ 서버가 로컬 통신**하는 구조로 적는 거야.

# AGV HMI Server

C++ / ROS2 기반 AGV 제어 서버입니다.
Qt HMI에서 HTTP API를 통해 AGV 상태를 조회하거나 이동·수동 제어 명령을 전달할 수 있습니다.

## 1. 시스템 구조

```text
┌─────────────────┐
│      Qt HMI     │
│   같은 PC에서 실행  │
└────────┬────────┘
         │
         │ HTTP
         │ 127.0.0.1:8080
         ▼
┌─────────────────┐
│  AGV HMI Server │
│      C++        │
│   Port : 8080   │
└────────┬────────┘
         │
         │ ROS2
         ▼
┌─────────────────┐
│      Nav2       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│       AGV       │
└─────────────────┘
```

### 다른 팀원이 그대로 따라 하면 되는 터미널 실행 순서만 적어줄게.

### 터미널 1 — Nav2
source /opt/ros/jazzy/setup.bash

→ 프로젝트에서 사용하는 Nav2 실행 명령

### 터미널 2 — C++ 서버
source /opt/ros/jazzy/setup.bash
cd ~/agv_hmi-main-local/agv_hmi-main
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server

처음 받았거나 코드가 변경됐으면 먼저:

colcon build --packages-select agv_hmi_server
### 터미널 3 — 서버 확인
curl http://127.0.0.1:8080/api/status

정상 응답이 나오면 서버 OK.

### 터미널 4 — Qt

Qt HMI 실행.

서버 주소:
http://127.0.0.1:8080

순서만 요약하면:

1. Nav2 실행
   ↓
2. C++ 서버 실행 (:8080)
   ↓
3. curl로 API 확인
   ↓
4. Qt 실행

Cloudflare는 사용하지 않음.

### 구성 요소

| 구성 요소          | 역할                        |
| -------------- | ------------------------- |
| Qt HMI         | 사용자 인터페이스 및 서버 API 호출     |
| AGV HMI Server | HTTP 요청 처리 및 AGV 제어       |
| NetworkServer  | `8080` 포트에서 HTTP API 제공   |
| Nav2Manager    | ROS2 Nav2와 연동하여 AGV 이동 제어 |
| Nav2           | AGV 자율주행                  |
| AGV            | 실제 로봇                     |

---

# 2. 실행 환경

* Ubuntu 24.04
* ROS2 Jazzy
* C++
* CMake
* colcon

---

# 3. 프로젝트 위치

프로젝트는 다음 위치에 있다고 가정합니다.

```bash
~/agv_hmi-main-local/agv_hmi-main
```

---

# 4. 서버 실행

## ① ROS2 환경 설정

터미널을 열고:

```bash
source /opt/ros/jazzy/setup.bash
```

---

## ② 프로젝트 폴더 이동

```bash
cd ~/agv_hmi-main-local/agv_hmi-main
```

---

## ③ 빌드

```bash
colcon build --packages-select agv_hmi_server
```

정상적으로 빌드되면:

```text
Finished <<< agv_hmi_server
```

가 출력됩니다.

---

## ④ 빌드 환경 적용

```bash
source install/setup.bash
```

---

## ⑤ 서버 실행

```bash
ros2 run agv_hmi_server agv_hmi_server
```

정상적으로 실행되면 다음과 같은 로그가 출력됩니다.

```text
NetworkServer listening on port: 8080
==========================================
  AGV Headless Server Started (No GUI)
  Listening on Port 8080...
==========================================
```

---

# 5. Nav2 실행

AGV 자율주행을 사용할 경우 **Nav2가 먼저 실행되어 있어야 합니다.**

별도의 터미널에서 ROS2 환경을 설정한 후 프로젝트 환경에 맞는 Nav2 launch 파일을 실행합니다.

```bash
source /opt/ros/jazzy/setup.bash
```

> Nav2의 구체적인 launch 명령은 로봇/시뮬레이터 환경에 따라 달라집니다.

---

# 6. 서버 동작 확인

C++ 서버를 실행한 상태에서 **새 터미널**을 엽니다.

```bash
curl http://127.0.0.1:8080/api/status
```

정상 응답:

```json
{
  "camera_url": "",
  "manual_mode": false,
  "status": "idle",
  "wheel_rpm": 0
}
```

현재 테스트에서 위 API가 정상적으로 응답하는 것을 확인했습니다.

---

# 7. Qt 연결

Qt HMI는 C++ 서버와 **같은 PC에서 실행**합니다.

Qt에서 사용하는 서버 주소:

```text
http://127.0.0.1:8080
```

따라서 상태 조회 API는:

```text
GET http://127.0.0.1:8080/api/status
```

입니다.

명령 API는:

```text
POST http://127.0.0.1:8080/api/command
```

입니다.

---

# 8. API

## 상태 조회

### Request

```http
GET /api/status
```

전체 주소:

```text
http://127.0.0.1:8080/api/status
```

### Response

```json
{
  "camera_url": "",
  "manual_mode": false,
  "status": "idle",
  "wheel_rpm": 0
}
```

### Response 항목

| 항목            | 설명        |
| ------------- | --------- |
| `status`      | 현재 AGV 상태 |
| `wheel_rpm`   | 휠 RPM     |
| `camera_url`  | 카메라 URL   |
| `manual_mode` | 수동 모드 여부  |

---

# 9. AGV 목적지 이동

`/api/command`로 목적지를 전달합니다.

```http
POST /api/command
Content-Type: application/json
```

예:

```json
{
  "destination": "room_301"
}
```

전체 주소:

```text
http://127.0.0.1:8080/api/command
```

서버는 목적지를 `Nav2Manager`로 전달하고, `Nav2Manager`가 ROS2 Nav2를 통해 AGV를 이동시킵니다.

```text
Qt
 ↓
POST /api/command
 ↓
NetworkServer
 ↓
Nav2Manager
 ↓
Nav2
 ↓
AGV
```

---

# 10. 수동 제어

수동 모드를 활성화합니다.

```json
{
  "manual_mode": true
}
```

이후 수동 명령을 전달합니다.

```json
{
  "command": "forward"
}
```

지원 명령:

```text
forward
backward
left
right
stop
cancel
```

구조:

```text
Qt
 ↓
POST /api/command
 ↓
NetworkServer
 ↓
Nav2Manager
 ↓
/cmd_vel
 ↓
AGV
```

---

# 11. `cancel`

`cancel`은 현재 이동 명령을 취소하는 용도로 사용합니다.

```json
{
  "command": "cancel"
}
```

현재 프로젝트의 구현에 따라 Nav2 목표를 취소하고 지정된 동작을 수행합니다.

---

# 12. 터미널 구성

실제 실행할 때는 최소 다음과 같이 사용합니다.

### Terminal 1 — Nav2

```bash
source /opt/ros/jazzy/setup.bash

# 프로젝트 환경에 맞는 Nav2 launch 실행
```

### Terminal 2 — C++ 서버

```bash
source /opt/ros/jazzy/setup.bash

cd ~/agv_hmi-main-local/agv_hmi-main

colcon build --packages-select agv_hmi_server

source install/setup.bash

ros2 run agv_hmi_server agv_hmi_server
```

### Terminal 3 — API 테스트

```bash
curl http://127.0.0.1:8080/api/status
```

### Terminal 4 — Qt HMI

Qt 프로그램 실행

```text
Server URL:
http://127.0.0.1:8080
```

---

# 13. 네트워크 구성

현재 프로젝트는 **로컬 실행을 기준으로 합니다.**

```text
Qt
 │
 │ localhost
 ▼
127.0.0.1:8080
 │
 ▼
C++ AGV Server
```

따라서 **Cloudflare Tunnel은 사용하지 않습니다.**

기존에 사용하던:

```bash
cloudflared tunnel ...
```

명령은 현재 로컬 구성에서는 필요하지 않습니다.

---

# 14. 문제 발생 시 확인

### 서버가 실행되지 않는 경우

```bash
ros2 pkg list | grep agv_hmi
```

정상적으로:

```text
agv_hmi_server
```

가 나오는지 확인합니다.

---

### 8080 포트 확인

```bash
curl http://127.0.0.1:8080/api/status
```

응답이 나오면 서버가 정상적으로 동작 중입니다.

---

### 빌드가 이상한 경우

반드시 프로젝트 폴더에서 실행합니다.

```bash
cd ~/agv_hmi-main-local/agv_hmi-main
```

그리고:

```bash
colcon build --packages-select agv_hmi_server
```

**홈 디렉터리 `/home/ubuntu`에서 `colcon build`를 실행하지 마세요.**

---

# 15. 전체 동작 흐름

### 자동 주행

```text
Qt HMI
  │
  │ POST /api/command
  │ {"destination":"room_301"}
  ▼
NetworkServer : 8080
  │
  ▼
Nav2Manager
  │
  │ NavigateToPose
  ▼
ROS2 Nav2
  │
  ▼
AGV
```

### 상태 조회

```text
Qt HMI
  │
  │ GET /api/status
  ▼
NetworkServer
  │
  ▼
JSON Response
  │
  ▼
Qt HMI
```

### 수동 주행

```text
Qt HMI
  │
  │ {"command":"forward"}
  ▼
NetworkServer
  │
  ▼
Nav2Manager
  │
  │ /cmd_vel
  ▼
AGV
```

---

## ⚠️ 현재 개발 버전에서 알아둘 점

현재 서버가 정상적으로 실행되고 `/api/status`도 정상 응답하지만, `status`, `wheel_rpm`, `camera_url` 등의 값이 **실제 AGV 센서 데이터와 모두 연결되어 있는지는 별도로 확인이 필요합니다.**

현재 확인된 `/api/status` 응답은:

```json
{
  "camera_url": "",
  "manual_mode": false,
  "status": "idle",
  "wheel_rpm": 0
}
```

입니다.

따라서 **Qt 팀은 우선 이 API 형식에 맞춰 연동하고**, 실제 휠 RPM/카메라/로봇 상태 데이터를 연결하는 작업은 별도 작업으로 보면 됩니다.
