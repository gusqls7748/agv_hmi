
---

## 🚀 1. C++ 서버 재빌드 & 실행 (코드 수정 시)

코드(`networkserver.cpp` 등) 변경 후 적용할 때 프로젝트 루트(`~/agv_hmi`)에서 아래 명령어를 순서대로 실행합니다.

```bash
# 1. 빌드 캐시 삭제 및 단독 재빌드
cd ~/agv_hmi
rm -rf build/AgvHmiServer install/AgvHmiServer log/
colcon build --packages-select AgvHmiServer

# 2. 환경 변수 적용 및 서버 실행
source install/setup.bash
ros2 run AgvHmiServer AgvHmiServer

```

---

## 🌐 2. ngrok 외부 포트 개방 (외부/UI 팀원 연결용)

로컬의 8080 포트를 외부에 공개할 때 새 터미널에서 실행합니다.

```bash
# 최초 1회 인증 토큰 등록 (가입 시 발급된 토큰 입력)
ngrok config add-authtoken <YOUR_AUTHTOKEN>

# 8080 포트 외부 개방
ngrok http 8080

```

> **출력 결과 확인:** `Forwarding` 항목의 `[https://xxxx.ngrok-free.app](https://xxxx.ngrok-free.app)` 주소를 팀원에게 전달하세요.

---

## 📡 3. REST API 명세 (UI 팀원 공유용)

해당 서버는 HTML 화면이 아닌 **JSON API 서버**이므로, 기본 주소(`/`) 접속 시 `404 Not Found`가 반환되는 것이 정상입니다. 아래 엔드포인트로 테스트해 달라고 안내해 주세요.

### ① AGV 상태 조회 (GET)

* **URL:** `https://<ngrok주소>/api/status`
* **Response 예시:** `{"wheel_rpm": ..., "camera_url": ..., "status": "ok"}`

### ② 목적지 제어 명령 전송 (POST)

* **URL:** `https://<ngrok주소>/api/command`
* **Header:** `Content-Type: application/json`
* **Body (JSON):**
```json
{
  "destination": "301"
}

```



> **성공 여부:** UI 팀원이 요청을 보냈을 때 터미널에 `[웹 명령 수신] 목적지: 301` 로그가 찍히면 정상 연동된 것입니다.

---

## 🛠️ 4. 주요 유틸리티 명령어 모음

| 목적 | 실행 명령어 |
| --- | --- |
| **Nav2 내비게이션 구동** | `ros2 launch nav2_bringup bringup_launch.py use_sim_time:=False` |
| **TF 트리 시각화 (pdf 생성)** | `ros2 run tf2_tools view_frames` |

[]가제보 nav2켜서 좌표대로 가는지 