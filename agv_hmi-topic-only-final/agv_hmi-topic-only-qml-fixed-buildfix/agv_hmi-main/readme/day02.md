오늘 작업하시느라 정말 고생 많으셨습니다! C++ 백엔드 서버 구축부터 웹 연동 시 발생했던 트러블슈팅, 그리고 헤드리스 실행 환경까지 완벽하게 처리하셨네요.

보고서 작성, 포트폴리오, 혹은 팀원들과 공유할 때 바로 활용하실 수 있도록 **오늘 진행한 작업 내용과 성과**를 깔끔하게 요약해 드립니다.

---

# 📝 [오늘의 작업 정리] AGV HMI REST API 서버 구축 및 헤드리스 연동

## 1. 🎯 주요 작업 성과 (Done)

### 1) QML GUI 제거 및 헤드리스(Headless) 백엔드 전환

* `QGuiApplication` 및 `QQmlApplicationEngine` 구동을 제거하고 `QCoreApplication` 기반의 **콘솔 백엔드 전용 모드**로 전환하였습니다.
* 화면 출력 없이 터미널 상에서만 독립적으로 동작하는 백그라운드 REST API 서버 환경을 구축했습니다.

### 2) ROS 2 Nav2 및 Qt C++ 기반 REST API 서버 완성

* Qt C++(`QTcpServer`)를 사용해 로컬 `8080` 포트로 통신하는 HTTP API 서버를 완성했습니다.
* **ngrok 터널링**을 적용하여 외부 웹 UI/클라이언트에서 들어오는 `GET`/`POST` 요청을 수신할 수 있도록 연동했습니다.
* `POST /api/manual_mode` 및 `POST /api/command` 수신 로직을 통해 **수동/자동 모드 전환** 및 **AGV 이동 명령** 제어 기반을 마련했습니다.

### 3) ngrok 프록시 패킷 트러블슈팅 완벽 해결 🛠️

* **411 Length Required 에러 수정**: ngrok을 거치면서 소문자로 들어오는 `content-length` 헤더 파싱 실패 문제를 보완했습니다.
* **JSON 파싱 실패(Illegal Value) 수정**: HTTP Chunked Transfer Encoding으로 들어오는 패킷 껍데기(`1a\r\n...`)를 제거하고, Pure JSON(`{ ... }`) 데이터만 추출하도록 파서 로직을 정교화했습니다.

---

## 2. 📡 구축 완료된 REST API 명세 (ngrok 공유용)

| 기능 | HTTP Method | Endpoint URL | 요청 Body (JSON) |
| --- | --- | --- | --- |
| **AGV 상태 조회** | `GET` | `/api/status` | *-* |
| **수동/자동 모드 전환** | `POST` | `/api/manual_mode` | `{"manual_mode": true}` |
| **AGV 이동 명령** | `POST` | `/api/command` | `{"command": "forward"}` 또는 `{"destination": "301"}` |

---

## 3. 📋 프로젝트 체크리스트 현황

* [x] **C++ REST API 서버 구축 및 Web UI 외부 연동** *(완료)*
* [x] **헤드리스 백그라운드 프로세스 실행 환경 세팅** *(완료)*
* [ ] **Gazebo 가상 환경 + SLAM / Nav2 실제 목표 좌표 이동 검증** *(다음 단계)*

---

## 🚀 다음 진행 예정 작업 (Next Step)

1. **Gazebo 시뮬레이터 구동**: `ros2 launch nav2_bringup ...` 명령을 통한 가상 지도 환경 및 Nav2 노드 셋업
2. **API 연동 좌표 이동 테스트**: 웹 UI에서 보낸 목적지/이동 명령 수신 시, Nav2 Manager가 가상 환경 속 AGV를 목표 좌표로 정상 이동시키는지 종합 테스트 진행