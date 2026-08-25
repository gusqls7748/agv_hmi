# Server Local Test Guide

## 전제

현재 구성은 **Qt와 C++ 서버가 같은 PC에서 실행되는 로컬 통신**을 기준으로 합니다.
Cloudflare/ngrok 등의 외부 터널은 사용하지 않습니다.

## 실행 순서

### 터미널 1: ROS2 / Nav2

```bash
source /opt/ros/jazzy/setup.bash
# 사용하는 Nav2/시뮬레이터 launch 명령 실행
```

### 터미널 2: C++ 백엔드

```bash
source /opt/ros/jazzy/setup.bash
cd ~/agv_hmi
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server
```

정상 실행 확인:

```text
NetworkServer listening on port: 8080
```

### Qt

Qt는 같은 PC에서 실행하고 서버 주소를 다음으로 설정합니다.

```text
http://127.0.0.1:8080
```

## API

### GET /api/status

```bash
curl http://127.0.0.1:8080/api/status
```

응답 예시:

```json
{"status":"idle","wheel_rpm":0,"camera_url":"","manual_mode":false}
```

### POST /api/command - 목적지 이동

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"destination":"room_301"}'
```

### POST /api/command - 수동 모드

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"manual_mode":true}'
```

### POST /api/command - 수동 전진

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"command":"forward"}'
```

## 주소 구조

```text
Qt
 │
 │ HTTP
 ▼
127.0.0.1:8080
 │
 ▼
agv_hmi_server
 │
 ▼
Nav2 / ROS2
 │
 ▼
AGV
```


## 최신 수동주행 테스트

수동 명령은 이제 `manual_mode`를 별도 요청하지 않아도 됩니다.

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"command":"forward"}'
```

또는 한 요청에 명시할 수 있습니다.

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"manual_mode":true,"command":"forward"}'
```

수동 모드를 끄면 즉시 정지합니다.

```bash
curl -X POST http://127.0.0.1:8080/api/command \
  -H 'Content-Type: application/json' \
  -d '{"manual_mode":false}'
```
