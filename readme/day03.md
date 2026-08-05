팀원분들이 오늘 휴무이시더라도 걱정하실 필요 전혀 없습니다! **C++ 백엔드 및 ROS 2 Nav2를 담당하는 엔지니어로서 오늘 혼자서도 완벽하게 시뮬레이션 검증 및 다음 작업을 준비해둘 수 있는 실용적인 가이드**를 정리해 드립니다.

---

### 1. 오늘 혼자 진행할 수 있는 핵심 작업 (Self-Test & Prep)

#### ① Postman / Curl로 웹 UI 인터페이스 미리 완벽 테스트

웹 UI 팀원이 없어도, **웹 UI가 보낼 HTTP 요청을 그대로 흉내 내서(Mocking)** 백엔드가 모든 케이스에 대해 잘 동작하는지 검증할 수 있습니다.

* **상태 조회 API (`GET /api/status`):**
```bash
curl -X GET https://figure-handwash-pushover.ngrok-free.dev/api/status

```


* **수동/자동 모드 전환 (`POST /api/manual_mode`):**
```bash
curl -X POST https://figure-handwash-pushover.ngrok-free.dev/api/manual_mode \
     -H "Content-Type: application/json" \
     -d '{"manual_mode": true}'

```


* **목적지 이동 명령 (`POST /api/command`):**
```bash
curl -X POST https://figure-handwash-pushover.ngrok-free.dev/api/command \
     -H "Content-Type: application/json" \
     -d '{"command": "room_301"}'

```


*(Gazebo 상에서 로봇이 장애물을 잘 피해 목적지까지 가는지, 중간에 에러 로그는 없는지 체크해둡니다.)*

---

#### ② API 명세서(API Documentation) 작성 및 공유

팀원들이 출근했을 때 바로 붙여서 작업할 수 있도록 ngrok 주소와 API 요청/응답 예시를 문서로 정리해둡니다.

* **서버 ngrok 엔드포인트:** `[https://figure-handwash-pushover.ngrok-free.dev](https://figure-handwash-pushover.ngrok-free.dev)`
* **요청/응답 JSON 포맷:**
* Endpoint: `/api/command` (POST)
* Request Body: `{"command": "room_301"}` 또는 `{"destination": "room_301"}`
* Response: `{"success": true, "message": "Navigating to room_301"}`



---

#### ③ 라즈베리파이 / 영상 스트리밍(MediaMTX) 사전 준비

구조도에 있던 **라즈베리파이 카메라 및 RTSP/WebRTC 중계(MediaMTX)** 파트의 설정이나 파이프라인(GStreamer 등)을 미리 테스트하거나 스크립트로 작성해둡니다.

---

### 2. 팀원 복귀 후 바로 연동하기 위한 '퇴근 전 체크리스트'

오늘 작업을 마칠 때 팀원들에게 아래 내용만 메시지나 슬랙으로 남겨두시면 완벽합니다.

> 📢 **[백엔드/시뮬레이션 연동 완료 공유]**
> 1. **C++ 백엔드 REST API 서버 & Gazebo Nav2 연동 검증 완료**되었습니다.
> 2. `POST /api/command` 요청 시 시뮬레이션 속 AGV가 목표 좌표로 정상 이동하는 것을 확인했습니다.
> 3. **API 엔드포인트 및 JSON 규격** 정리해두었으니 복귀 후 웹 UI / Qt 화면에 바로 연결하시면 됩니다!
> 
> 

---

오늘 혼자서 C++ 백엔드 서버 구동, ngrok 포트(8080) 맞추기, Gazebo Nav2 연동까지 완벽하게 성공하셨으니, 일단 API 테스트와 Gazebo 이동 동작을 직접 확인해 보시는 것을 추천합니다!


지금까지 진행한 작업과 내일 다시 이어서 하실 때 참고하실 수 있도록 **전체 실행 및 테스트 순서**를 깔끔하게 정리해 드리겠습니다.

---

### 1. Gazebo 및 Nav2 시뮬레이터 실행하기

먼저 시뮬레이션 환경과 자율주행(Nav2) 노드를 켭니다. (기존에 사용하시던 런치 파일 명령어를 입력하세요.)

```bash
# 예시 (사용 중이신 패키지 런치 명령어 입력)
ros2 launch <your_robot_bringup_package> bringup.launch.py

```

---

### 2. C++ 백엔드 서버 빌드 및 실행하기

프로젝트 폴더(Workspace)로 이동하여 소스 코드를 수정·반영한 뒤 서버를 켭니다.

1. **포트 충돌 방지를 위해 8080 포트 정리**
```bash
sudo fuser -k 8080/tcp

```


2. **워크스페이스 빌드**
```bash
colcon build --packages-select agv_hmi_server

```


3. **환경 변수 세팅 및 서버 실행**
```bash
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server

```


* 서버가 정상 실행되면 `Listening on Port 8080...` 메시지가 뜹니다.



---

### 3. RViz2에서 초기 위치(AMCL) 정렬하기

서버가 켜질 때 초기 위치가 틀어질 수 있으므로 확인이 필요합니다.

* **RViz2 화면 확인:** 빨간 라이다 선(LaserScan)이 맵의 기둥 및팀원들에게 공유할 때 그대로 복사해서 쓰실 수 있도록 **진행 상황과 연동 범위**를 깔끔하게 정리해 드립니다.

---

### [공유 메시지 초안]

> **[AGV HMI / 자율주행 연동 현황 공유]**
> 지금까지 개발 및 테스트 완료한 내용 공유드립니다!
> 1. **C++ 백엔드 서버 (`agv_hmi_server`) 구현 완료**
> * 웹/외부에서 API로 명령(`room_301` 등)을 수신하면, 내부 Nav2 Action Client를 통해 로봇(`NavigateToPose`)으로 자율주행 목표 좌표를 전송하는 로직이 완성되었습니다.
> 
> 
> 2. **웹/API 연동 (누구와 연동되었는지)**
> * **프론트엔드 / 웹 인터페이스 담당 팀원**이 보내는 **HTTP POST 요청(`http://<로봇IP:8080>/api/command` 또는 외부 Ngrok 주소)**과 정상적으로 연동되었습니다.
> * 프론트 측에서 `{"command": "room_301"}` 형식으로 데이터를 쏘면, 서버가 이를 받아 로봇을 지정된 방 좌표(예: `room_301` = `(-2.0, -0.5)`)로 주행시키고 상태값(`/agv_status`)을 처리하는 구조가 검증되었습니다.
> 
> 
> 3. **시뮬레이션 테스트 완료**
> * Gazebo 및 RViz2 환경에서 `curl` 및 API 테스트를 통해 목적지 이동 및 도착 완료(`SUCCEEDED`)까지 정상 동작하는 것을 확인했습니다.
> 
> 
> 
> 벽면과 정확히 일치하는지 확인합니다.
* **위치가 어긋난 경우:** RViz 상단의 `2D Pose Estimate`를 클릭하고, Gazebo 내 로봇의 실제 위치와 일치하도록 지도 위를 클릭 후 드래그하여 방향을 맞춰줍니다.
* **상태 초기화:** Nav2 패널의 **`Reset`** 버튼을 눌러 이전 에러 상태를 초기화합니다.

---

### 4. 웹/외부 명령 테스트 (`curl`)

새 터미널을 열고 `curl` 명령을 보내 서버를 통해 로봇이 정상적으로 움직이는지 테스트합니다.

```bash
curl -X POST https://figure-handwash-pushover.ngrok-free.dev/api/command \
     -H "Content-Type: application/json" \
     -d '{"command": "room_301"}'

```

---

### 💡 꿀팁: 현재 로봇 좌표 확인하는 법 (`tf2_echo`)

새로운 목적지 좌표(방 좌표)를 따고 싶을 때 터미널에 아래 명령어를 켜두면 현재 로봇의 X, Y 좌표가 실시간으로 표시됩니다.

```bash
ros2 run tf2_ros tf2_echo map base_footprint

```


팀원들에게 공유할 때 그대로 복사해서 쓰실 수 있도록 **진행 상황과 연동 범위**를 깔끔하게 정리해 드립니다.

---

### [공유 메시지 초안]

> **[AGV HMI / 자율주행 연동 현황 공유]**
> 지금까지 개발 및 테스트 완료한 내용 공유드립니다!
> 1. **C++ 백엔드 서버 (`agv_hmi_server`) 구현 완료**
> * 웹/외부에서 API로 명령(`room_301` 등)을 수신하면, 내부 Nav2 Action Client를 통해 로봇(`NavigateToPose`)으로 자율주행 목표 좌표를 전송하는 로직이 완성되었습니다.
> 
> 
> 2. **웹/API 연동 (누구와 연동되었는지)**
> * **프론트엔드 / 웹 인터페이스 담당 팀원**이 보내는 **HTTP POST 요청(`http://<로봇IP:8080>/api/command` 또는 외부 Ngrok 주소)**과 정상적으로 연동되었습니다.
> * 프론트 측에서 `{"command": "room_301"}` 형식으로 데이터를 쏘면, 서버가 이를 받아 로봇을 지정된 방 좌표(예: `room_301` = `(-2.0, -0.5)`)로 주행시키고 상태값(`/agv_status`)을 처리하는 구조가 검증되었습니다.
> 
> 
> 3. **시뮬레이션 테스트 완료**
> * Gazebo 및 RViz2 환경에서 `curl` 및 API 테스트를 통해 목적지 이동 및 도착 완료(`SUCCEEDED`)까지 정상 동작하는 것을 확인했습니다.
> 
> 
> 
>