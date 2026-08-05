팀원들에게 지금까지 구현하신 작업 내용과 테스트 결과를 논리적이고 명확하게 설명하실 수 있도록 정리해 드릴게요.

아래 내용을 팀원과의 회의나 메신저 공유용으로 활용해 보세요!

---

### 📝 [AGV HMI REST API 서버 구축 및 외부 연동] 발표/공유 정리

#### 1. 주요 구현 내용 (백엔드 서버 구축)

* **REST API 서버 구현**: Qt C++(`QTcpServer`)를 기반으로 외부에서 AGV 상태를 조회하고 명령을 내릴 수 있는 **HTTP REST API 서버**를 만들었습니다.
* **CORS 설정 완료**: 웹 브라우저/프론트엔드에서 통신할 때 발생하는 보안 차단 문제(CORS)를 해결하기 위해 `OPTIONS` 사전 요청(Preflight) 및 Response Header 처리를 구현했습니다.
* **외부 접속 통로(ngrok) 개방**: 로컬 환경(`localhost:8080`)에서 동작하는 서버를 외부(팀원 PC, 모바일, 외부 웹 서버)에서 접근할 수 있도록 포트 파워딩/터널링을 완료했습니다.

---

#### 2. 제공하는 API 명세 (팀원들이 사용할 주소)

현재 사용 가능한 도메인 주소:

`[https://figure-handwash-pushover.ngrok-free.dev](https://figure-handwash-pushover.ngrok-free.dev)`

##### ① AGV 상태 조회 (`GET`)

* **URL**: `[https://figure-handwash-pushover.ngrok-free.dev/api/status](https://figure-handwash-pushover.ngrok-free.dev/api/status)`
* **설명**: 프론트엔드나 클라이언트가 AGV의 현재 바퀴 RPM, 카메라 스트리밍 URL, 동작 상태를 조회할 때 사용합니다.
* **응답 예시 (JSON)**:
```json
{
  "camera_url": "http://192.168.0.50:8081/stream",
  "status": "ok",
  "wheel_rpm": 0
}

```



##### ② AGV 이동 제어 명령 전송 (`POST`)

* **URL**: `[https://figure-handwash-pushover.ngrok-free.dev/api/command](https://figure-handwash-pushover.ngrok-free.dev/api/command)`
* **Header**: `Content-Type: application/json`
* **설명**: 프론트엔드/앱에서 AGV에게 목적지 이동 명령을 내릴 때 사용합니다.
* **요청 본문 예시 (JSON)**:
```json
{
  "destination": "Station_A"
}

```


* **응답 예시 (JSON)**:
```json
{
  "result": "success",
  "target": "Station_A"
}

```



---

#### 3. 팀원들에게 전달할 멘트 (말 설명용)

> "저희 AGV HMI C++ 백엔드 쪽에 HTTP REST API 서버 구동을 완료했습니다.
> 외부 웹 페이지나 앱, 또는 Postman에서 AGV 상태를 조회하거나 명령을 전송할 수 있도록 ngrok으로 통신 통로도 열어 두었습니다.
> 1. **AGV 상태 데이터 받아오기**: 브라우저나 `fetch`로 `GET /api/status`로 접속하시면 현재 바퀴 RPM, 카메라 URL, 상태 JSON 값을 바로 받아서 화면에 띄우실 수 있습니다.
> 2. **AGV에 목적지 명령 보내기**: `POST /api/command` 엔드포인트로 `{"destination": "목적지이름"}` 형태의 JSON을 쏴주시면 제가 만든 C++ 서버 터미널에서 실시간으로 웹 명령을 수신하여 AGV 제어로 연결하도록 세팅해 두었습니다.
> 
> 
> 프론트엔드나 다른 클라이언트 작업하실 때 이 주소로 연동해서 테스트해 주시면 됩니다!"

## 1. 1차 원인: Content-Length 헤더 파싱 실패 (411 에러)
상황: ngrok을 거쳐 웹 UI에서 요청이 들어올 때 헤더 이름이 소문자(content-length)로 들어오거나 정규식 패턴과 맞지 않아 C++ 서버가 Content-Length를 찾아내지 못했습니다.

결과: 서버가 "헤더에 길이 정보가 없다"고 판단하여 411 Length Required 에러를 응답하고 요청을 거부했습니다.

2. 2차 원인: Chunked Transfer Encoding (청크 분할 전송) (JSON 파싱 에러)
상황: 1차 에러를 피하기 위해 바이트 크기 검사를 우회했더니, 이번에는 ngrok 프록시 특성상 본문(Body) 데이터가 청크 분할 패킷 껍데기(1a\r\n ... \r\n0\r\n\r\n)에 싸여서 들어왔습니다.

결과: Pure JSON만 읽어야 하는 QJsonDocument::fromJson에 1a 같은 16진수 크기 값이 포함된 문자열이 그대로 전달되어 illegal value (JSON 파싱 실패) 에러가 났던 것입니다.

💡 한 줄 요약
"ngrok/웹에서 보낸 HTTP 패킷 형태(헤더 대소문자 & 청크 포맷)를 C++ 서버가 그대로 못 알아들어서 일어난 헤더/바이트 파싱 해프닝"이었습니다!

이제 { 와 } 사이의 순수 JSON만 쏙 뽑아내도록 로직을 수정했으니, 프록시를 거쳐 들어오는 어떤 포맷의 요청도 문제없이 다 처리됩니다.

[v] C++ API 서버와 Web UI 연동
[]  Gazebo 가상 환경 + SLAM / Nav2를 연결