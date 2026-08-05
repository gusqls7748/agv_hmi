아, 맞네요! Nginx뿐만 아니라 **ROS2 패키지(`agv_hmi_server`) 기반의 실제 백엔드/HMI 서버**까지 실행해야 완전한 서비스가 돌아가는 거였죠.

전체 과정(ROS2 환경 설정 ➡️ ROS2 HMI 서버 실행 ➡️ Cloudflare 터널 열기)을 **처음부터 끝까지 순서대로** 깔끔하게 정리해 드릴게요.

---

## 🚀 Web HMI 서버 전체 실행 과정

터미널을 하나 또는 두 개 열어서 아래 순서대로 진행해 주시면 됩니다.

---

### 1단계: ROS2 워크스페이스 이동 및 환경 빌드/로드

ROS2 패키지를 실행하기 위해 워크스페이스로 이동하고 빌드 환경을 적용합니다.

```bash
# 1. ROS2 워크스페이스 디렉토리로 이동 (본인의 워크스페이스 경로)
cd ~/ros2_ws  # (만약 경로가 다르면 본인 워크스페이스 경로로 이동)

# 2. ROS2 환경 변수 로드
source install/setup.bash

```

---

### 2단계: ROS2 HMI 서버 실행

이제 AGV HMI 노드를 실행합니다.

```bash
ros2 run agv_hmi_server agv_hmi_server

```

* Executable이 정상 동작하면서 웹 백엔드 서비스(또는 80번 포트로 연결되는 로컬 서버)가 활성화됩니다.
* 💡 **Tip:** 이 터미널은 ROS2 서버 로그가 찍혀야 하므로 **닫지 말고 그대로 둡니다.**

---

### 3단계: (선택) Nginx 재시작

만약 ROS2 서버 앞단에서 Nginx가 프록시(80번 포트) 역할을 해주고 있다면, Nginx도 한 번 깔끔하게 재시작해 줍니다.

*(새 터미널을 열거나 background에서 실행)*

```bash
sudo systemctl restart nginx

```

---

### 4단계: Cloudflare 외부 접속 터널 열기

이제 외부(모바일/다른 네트워크)에서 경고 팝업 없이 바로 들어올 수 있도록 터널을 엽니다. **(새 터미널 창을 열어서 실행해 주세요)**

```bash
cloudflared tunnel --protocol http2 --url http://localhost:80

```

---

### 5단계: 접속 주소 확인

`cloudflared`를 실행한 터미널에 출력되는 아래 형태의 주소를 복사합니다.

> **`[https://xxxx-xxxx-xxxx.trycloudflare.com](https://xxxx-xxxx-xxxx.trycloudflare.com)`**

이 주소로 스마트폰이나 웹 브라우저에서 접속하시면 **경고창, IP 입력, 팝업 버튼 클릭 없이 즉시 AGV Web HMI 화면으로 진입**하게 됩니다!

---

### 📌 요약하면?

1. **터미널 1:** `source install/setup.bash` ➡️ `ros2 run agv_hmi_server agv_hmi_server`
2. **터미널 2:** `cloudflared tunnel --protocol http2 --url http://localhost:80`
3. **접속:** 화면에 나온 `trycloudflare.com` 링크로 바로 접속!


## 팀원들(UI팀, 백엔드/로봇 제어팀 등)에게 바로 공유하거나 보고서/스프린트 기록에 붙여넣기 좋게 **의도, 원인, 해결 내용, 변경 사항** 중심으로 깔끔하게 정리했습니다. 그대로 전달해서 사용하시면 됩니다!

---

# 📝 Web HMI 외부 접속 환경 개선 및 이슈 해결 보고

### 1. 개요 및 변경 목적

* **목적:** 외부 네트워크(모바일, 타 기기 등)에서 AGV Web HMI 서버에 접속할 때 발생하는 **보안 경고 팝업, Verification(인증) 페이지, IP 입력 단계를 완전히 제거**하여 접근성과 UI/UX 편의성을 개선함.

---

### 2. 주요 수정 및 조치 내용 (What was fixed?)

#### 1) 외부 터널링 솔루션 교체 (`ngrok` / `localtunnel` ➡️ `Cloudflare Tunnel`)

* **기존 문제점:**
* `ngrok` 및 `localtunnel`: 무료 플랜 정책 강화로 인해 접속 시마다 **"안전하지 않은 사이트" 안내 팝업**이나 **서버 공인 IP(210.119.x.x)를 직접 입력해야 하는 절차**가 발생하여 UI/UX 저해 및 접속 오류 발생.


* **조치 사항:**
* Cloudflare의 공식 무료 개발자 터널링 도구인 **`cloudflared` 도입**.
* **개선 결과:** 외부 접속 시 팝업창, verification 버튼, IP 입력 절차가 **100% 제거**되어 주소 클릭 즉시 Web HMI 화면으로 진입 가능.



#### 2) 네트워크 프로토콜 최적화 (HTTP/2 설정)

* **기존 문제점:**
* `cloudflared` 기본 실행 시 UDP(QUIC) 통신을 시도하는데, 방화벽/네트워크 환경에 의해 UDP 통신이 차단되어 수 초간 재연결 지연 로그(`QUIC connection failed`) 발생.


* **조치 사항:**
* 터널 실행 옵션에 `--protocol http2` 명시적 부여.


* **개선 결과:** 딜레이 없이 **1초 만에 깔끔하게 터널 개방 및 TCP/HTTP2 기반의 안정적인 웹 통신 확보**.

---

### 3. 팀원별 참고 및 전달 사항

* **🎨 UI/UX 팀원 참고:**
* 앞으로 외부 접속 테스트 시 **불필요한 경고창이나 IP 입력 창이 전혀 뜨지 않습니다.**
* 발급된 `[https://...trycloudflare.com](https://...trycloudflare.com)` 링크로 접속하면 실제 로컬 환경과 동일하게 **HMI UI 화면으로 직행**하므로 반응형 Web UI 및 모바일 화면 테스트가 훨씬 원활해집니다.


* **⚙️ 백엔드 / 로봇제어 팀원 참고:**
* 기존 Nginx 및 ROS2 패키지(`agv_hmi_server`) 구조나 포트(80번) 설정은 전혀 건드리지 않고, 외부 연결 통로만 Cloudflare 인프라로 안심하고 교체했습니다.
* 테스트 종료 후 터널 프로세스(`cloudflared`)를 종료하면 외부 포트가 즉시 닫히므로 보안상 안전합니다.



---

### 4. 최신 HMI 서버 실행 절차 (전체 가이드)

다음 작업 시 아래 순서대로 서버 및 터널을 실행하면 됩니다.

```bash
# [터미널 1] ROS2 HMI 서버 실행
cd ~/ros2_ws  # (본인 ROS2 워크스페이스 경로)
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server

# [터미널 2] (선택) Nginx 재시작
sudo systemctl restart nginx

# [터미널 3] Cloudflare 외부 접속 터널 실행 (팝업 0% 방식)
cloudflared tunnel --protocol http2 --url http://localhost:80

```

> **접속:** 터미널 3 출력 화면의 **`[https://xxxx.trycloudflare.com](https://xxxx.trycloudflare.com)`** 링크로 바로 접속