# 🤖 AGV HMI 서버 (agv_hmi_server) 테스트 가이드

> 이 문서는 `agv_hmi_server`(C++/ROS2 백엔드) 담당자가 다른 팀원(웹 UI 팀 / Qt 팀)과 함께
> 테스트할 때 참고하는 문서입니다. 서버 자체의 역할, 실행 순서, API 명세, 테스트 방법을 정리했습니다.

---

## 1. 이 서버는 무엇을 하나요?

`agv_hmi_server`는 화면(GUI) 없이 콘솔에서만 돌아가는 **REST API 서버 + ROS2 노드**입니다.

**8080(C++ 서버)이 유일한 "진짜" 백엔드이고, 거기까지 가는 문이 두 개입니다.** 포트를 하나로 통합한 게 아니라, 두 팀이 서로 다른 경로로 결국 같은 8080에 도달하는 구조예요.

```
Qt 팀 (원격 앱)                                    ┐
   └─► 8080 터널 (직접 호출) ─────────────────────┤
                                                   ├─► agv_hmi_server (8080) ─► Nav2 ─► 실제 로봇
웹 UI 팀 (브라우저) ─► 8000 터널 ─► Blazor 서버      │
                        (127.0.0.1:8080으로 내부 호출)┘
```

| | Qt 팀 | 웹 UI 팀 |
|---|---|---|
| 직접 접속하는 곳 | **8080 터널** (C++ 서버) | **8000 터널** (Blazor 서버) |
| 8080은 언제 호출되나 | Qt 앱이 직접, 외부에서 | Blazor 서버가 로봇 PC **내부에서** `127.0.0.1:8080`으로 대신 호출 |
| 8080 터널 주소를 알아야 하나 | 예, 코드에 직접 넣어야 함 | 아니요, 몰라도 됨 |

정식 배포(nginx) 환경에서는 아예 외부에서 `/api/`로 직접 못 들어오게 막아둡니다(`guide-robot-webhmi.conf`의 `location ^~ /api/ { return 404; }`). 브라우저가 8080을 직접 두드리는 경로를 원천 차단해서, 웹 UI 쪽은 반드시 Blazor를 거쳐서만 로봇에 명령이 나가도록 강제한 것입니다.

- 서버는 받은 명령을 파싱해서 ROS2 `navigate_to_pose` 액션이나 `/cmd_vel` 토픽으로 로봇에 전달합니다.
- **주의**: 이 서버가 Nav2를 "실행"시켜주지는 않습니다. Nav2/로봇 컨트롤러가 이미 켜져 있어야 명령이 실제로 로봇을 움직입니다. (자세한 내용은 6번 문제 해결 참고)

---

## 2. 빌드 & 실행 순서

> ⚠️ **워크스페이스 경로 주의**: 이 프로젝트는 `~/ros2_ws/src/...` 같은 중첩 구조가 아니라, **`~/agv_hmi` 폴더 자체가 패키지 루트이자 colcon 워크스페이스 루트**입니다. 헷갈리면 아래 명령어로 실제 경로를 먼저 확인하세요.
> ```bash
> find ~ -maxdepth 5 -iname "networkserver.cpp" 2>/dev/null
> ```
> 결과가 `/home/ubuntu/agv_hmi/networkserver.cpp`처럼 `src/` 없이 바로 나오면, 워크스페이스 루트는 `~/agv_hmi`입니다.

### 2-1. 코드 반영 & 재빌드 (수정했을 때만)

```bash
cd ~/agv_hmi
rm -rf build install log
colcon build --packages-select agv_hmi_server
```

### 2-2. 로봇 본체 PC에서 켜는 전체 순서 (터미널 3~4개)

| 순서 | 터미널 | 명령어 | 확인 방법 |
|---|---|---|---|
| 1 | Nav2 (미리 켜져 있어야 함) | `ros2 launch nav2_bringup bringup_launch.py use_sim_time:=False` | RViz에서 맵/로봇 위치 정상 표시 |
| 2 | C++ 백엔드 서버 | `cd ~/agv_hmi && source install/setup.bash && ros2 run agv_hmi_server agv_hmi_server` | `NetworkServer listening on port: 8080` 로그 |
| 3 | Qt 팀용 터널 (8080) | `cloudflared tunnel --protocol http2 --url http://localhost:8080` | `https://xxxx.trycloudflare.com` 주소 발급 |
| 4 | 웹 HMI(.NET) 서버 | `RobotServer__BaseUrl=http://127.0.0.1:8080/ dotnet GuideRobot.WebHmi.dll --urls http://127.0.0.1:8000` | 정상 구동 로그 |
| 5 | 웹 UI 팀용 터널 (8000) | `cloudflared tunnel --protocol http2 --url http://localhost:8000` | `https://yyyy.trycloudflare.com` 주소 발급 |

> ⚠️ 터널 터미널 창들은 테스트가 끝날 때까지 끄지 마세요.

### 2-3. 팀원에게 전달할 주소

- **웹 UI 팀** → 8000 터널 주소만 전달하면 됩니다. 브라우저에 그대로 접속하면 대시보드가 뜹니다. (8080 터널 주소는 몰라도 됩니다 — Blazor 서버가 내부적으로 8080을 대신 호출합니다.)
- **Qt 팀** → 8080 터널 주소 + 아래 API 경로를 코드에서 **직접** 호출하면 됩니다. 브라우저로 직접 접속하면 `{"error": "Not Found"}`가 뜨는 게 정상입니다(API 전용 서버라서 그렇습니다).

---

## 3. REST API 명세

### 3-1. `GET /api/status` — 로봇 상태 조회

```bash
curl -X GET https://<터널주소>/api/status
```

응답 예시:
```json
{
  "status": "idle",
  "wheel_rpm": 0,
  "camera_url": "",
  "manual_mode": false
}
```

### 3-2. `POST /api/command` — 이동 / 취소 / 수동 조종

| 목적 | Body 예시 | 비고 |
|---|---|---|
| 목적지 이동 | `{"destination": "room_301"}` | 등록된 목적지: `room_301` ~ `room_305`, `HOME` |
| 이동 취소 후 복귀 | `{"command": "cancel"}` | `stop`이 아니라 `cancel`입니다 |
| 수동 조종 | `{"command": "forward"}` (또는 `backward`/`left`/`right`/`stop`) | **`manual_mode`가 켜져 있어야만** 동작 |

### 3-3. `POST /api/manual-mode` (`/api/manual_mode`, `/api/manual-control`도 동일) — 수동/자동 모드 전환

```bash
curl -X POST https://<터널주소>/api/manual-mode \
     -H "Content-Type: application/json" \
     -d '{"manual_mode": true}'
```

### 3-4. 응답 코드 정리

| 코드 | 의미 | 언제 나오나요 |
|---|---|---|
| `200 OK` | 성공 | 명령이 정상적으로 Nav2/로봇에 전달됨 |
| `400 Bad Request` | 실패 | JSON 형식 오류, 또는 등록 안 된 목적지 |
| `409 Conflict` | 거부 | `manual_mode`가 꺼진 상태에서 forward/left 등 수동 명령을 보냄 |
| `404 Not Found` | 경로 오류 | 정의되지 않은 경로로 요청함 (정상적인 케이스도 있음, 위 참고) |

---

## 4. 테스트 시나리오 (curl로 먼저 혼자 검증)

```bash
# 1) 상태 조회
curl -X GET https://<터널주소>/api/status

# 2) 목적지 이동 - Nav2가 켜져 있으면 로봇이 실제로 움직여야 함
curl -X POST https://<터널주소>/api/command \
     -H "Content-Type: application/json" -d '{"destination": "room_301"}'

# 3) 이동 취소 후 HOME 복귀
curl -X POST https://<터널주소>/api/command \
     -H "Content-Type: application/json" -d '{"command": "cancel"}'

# 4) 수동 모드 켜기
curl -X POST https://<터널주소>/api/manual-mode \
     -H "Content-Type: application/json" -d '{"manual_mode": true}'

# 5) 전진 (100ms 주기로 재발행되므로 몇 초간 계속 움직여야 정상)
curl -X POST https://<터널주소>/api/command \
     -H "Content-Type: application/json" -d '{"command": "forward"}'

# 6) 정지
curl -X POST https://<터널주소>/api/command \
     -H "Content-Type: application/json" -d '{"command": "stop"}'

# 7) manual_mode 끈 상태에서 forward 시도 → 409 확인용
curl -X POST https://<터널주소>/api/manual-mode \
     -H "Content-Type: application/json" -d '{"manual_mode": false}'
curl -X POST https://<터널주소>/api/command \
     -H "Content-Type: application/json" -d '{"command": "forward"}'
# → {"result":"fail","error":"manual_mode가 꺼져 있어..."} 와 409가 나오면 정상
```

정상 동작 판단 기준: 서버 터미널에 `[NetworkServer] Received destination: room_301` 같은 로그가 찍히고, **동시에** Nav2/RViz 쪽에서도 로봇이 실제로 반응해야 합니다. 로그만 찍히고 로봇이 안 움직이면 아래 5번을 확인하세요.

---

## 5. 자주 발생하는 문제

| 증상 | 원인 | 해결 |
|---|---|---|
| 응답은 `200 success`인데 로봇이 안 움직임 | Nav2 액션 서버가 안 떠 있음 | 서버 로그에 `Nav2 Action Server가 연결되지 않았습니다!` 확인 → Nav2 bringup 먼저 실행 |
| 수동 조종(`forward` 등)이 `409`로 거부됨 | `manual_mode`가 꺼져 있음 | `{"manual_mode": true}` 먼저 전송 |
| 브라우저로 8080 주소 접속 시 `{"error":"Not Found"}` | 정상입니다 | API 전용 서버라 `/`에는 아무것도 없음. `/api/status`로 접속해서 확인 |
| 목적지 이동인데 `400 fail` | `destination` 값이 `location_map_`에 없는 이름 | `room_301`~`room_305`, `HOME` 중 하나로 확인 |
| `cd: /home/ubuntu/ros2_ws: No such file or directory` | 문서마다 다른 옛날 경로 참고함 | `find ~ -maxdepth 5 -iname "networkserver.cpp"`로 실제 경로 확인 후 그 경로(보통 `~/agv_hmi`) 사용 |
| 서버 실행 시 포트 충돌 | 이전 프로세스가 8080을 물고 있음 | `sudo fuser -k 8080/tcp` 후 재실행 |
| 팀원 접속이 갑자기 끊김 | 터널 터미널을 닫음 | cloudflared/ngrok 터미널 창 유지 필요 |

---

## 6. 알려진 제한사항 (다음 작업 예정)

- 수동 조종 속도값(`0.2 m/s`, `0.5 rad/s`)은 임시값입니다. 실제 로봇/Gazebo 환경에서 크기에 맞게 조정이 필요합니다.
- `200 success` 응답은 "명령이 접수되어 Nav2로 전달됨"을 의미할 뿐, "목적지 도착 완료"를 보장하지 않습니다. 도착 여부는 `/agv_status` 토픽 또는 `GET /api/status`로 별도 확인이 필요합니다.