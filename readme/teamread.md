서버를 켤 때의 순서와 팀원들에게 각각 어떤 주소를 줘야 하는지 깔끔하게 정리해 드리겠습니다. 로봇 현장에서 서버를 켤 때는 **순서대로 터미널을 열고 실행**하셔야 충돌이 안 납니다.

---

### 🚀 [1단계] 로봇 본체 PC에서 서버 켜는 순서

터미널 창을 2개 열어서 순서대로 실행해 주세요.

1. **C++ 백엔드 서버 실행 (8080번 포트)**
* 로봇의 핵심 제어 및 REST API를 담당하는 서버입니다.
* 명령어:
```bash
cd ~/ros2_ws
ros2 run agv_hmi_server agv_hmi_server

```


*(또는 평소에 실행하시는 C++ 서버 실행 명령어로 켜시면 됩니다.)*


2. **웹 HMI (UI) 서버 실행 (8000번 포트)**
* 새 터미널 창을 열고, 웹 화면을 보여주는 서버를 켭니다.
* 명령어:
```bash
cd ~/GuideRobot_WebHmi/GuideRobot.WebHmi-ubuntu-net10-final
RobotServer__BaseUrl=http://127.0.0.1:8080/ dotnet GuideRobot.WebHmi.dll --urls http://127.0.0.1:8000

```





---

### 🌐 [2단계] 외부 접속용 클라우드플레어(Cloudflare) 터널 켜기

팀원들이 외부에서 접속할 수 있도록 터널을 각각 켜줍니다. (이 터미널 창들은 테스트가 끝날 때까지 **끄지 말고 켜두셔야 합니다**.)

1. **C++ API 서버용 터널 (Qt 팀 전달용 - 8080번)**
* 새 터미널을 열고 실행:
```bash
cloudflared tunnel --protocol http2 --url http://localhost:8080

```


* 여기서 나온 주소(예: `[https://xxxx.trycloudflare.com](https://xxxx.trycloudflare.com)`)를 **Qt 팀**에게 줍니다.


2. **웹 HMI UI 서버용 터널 (웹 UI 팀 전달용 - 8000번)**
* 또 다른 새 터미널을 열고 실행:
```bash
cloudflared tunnel --protocol http2 --url http://localhost:8000

```


* 여기서 나온 주소(예: `[https://yyyy.trycloudflare.com](https://yyyy.trycloudflare.com)`)를 **웹 UI 팀**에게 줍니다.



---

### 👥 [3단계] 팀원들에게 전달할 내용 (복사해서 쓰세요)

#### 1. 🖥️ 웹 UI 팀에게 줄 내용

* **전달할 주소:** 8000번 포트로 만든 터널 주소 (`[https://yyyy.trycloudflare.com](https://yyyy.trycloudflare.com)`)
* **설명:** "이 주소를 웹 브라우저에 그대로 치면 로봇 제어 대시보드 화면이 뜹니다."

#### 2. 🤖 Qt 팀에게 줄 내용

* **전달할 주소:** 8080번 포트로 만든 터널 주소 (`[https://xxxx.trycloudflare.com](https://xxxx.trycloudflare.com)`)
* **설명 / 주의사항:**
> 브라우저로 접속했을 때 화면이 안 뜨고 `{"error": "Not Found"}`가 뜨는 게 **정상**입니다. (API 전용 서버이기 때문입니다.)
> Qt 프로그램 코드 내에서 아래 경로를 붙여서 통신을 구현해 주세요.
> * **상태 조회 (GET):** `[https://xxxx.trycloudflare.com/api/status](https://xxxx.trycloudflare.com/api/status)`
> * **명령 전송 (POST):** `[https://xxxx.trycloudflare.com/api/command](https://xxxx.trycloudflare.com/api/command)`
> * 목적지 이동 Body: `{"destination": "room_301"}`
> * 주행 취소 Body: `{"command": "cancel"}` *(※ stop 아님)*
> 
> 
> 
>