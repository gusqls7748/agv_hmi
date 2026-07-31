### 서버 실행 명령어

# 1. 빌드 폴더로 이동
cd ~/agv_hmi/build

# 2. 충돌을 일으키는 환경 변수 초기화 후 서버 실행
unset LD_LIBRARY_PATH
./AgvHmiServer

#### 실행 후 UI 팀원과 통신 테스트
터미널에 HTTP REST API Server started on port 8080 메시지가 뜨면 서버가 정상 작동 중인 것입니다. 바로 UI 팀원분께 아래 내용을 공유해 주세요!

현빈 님 PC IP: 192.168.0.9

서버 주소: [http://192.168.0.9:8080](http://192.168.0.9:8080)

1. 상태 데이터 확인 (GET)

URL: [http://192.168.0.9:8080/api/status](http://192.168.0.9:8080/api/status)

2. 목적지 명령 전송 (POST)

URL: [http://192.168.0.9:8080/api/command](http://192.168.0.9:8080/api/command)

Body (JSON): {"destination": "301"}

UI 팀원이 명령을 보냈을 때 현빈 님 터미널에 [웹 명령 수신] 목적지: 301 로그가 찍히면 오늘 할 일 성공입니다!

## 2단계: IP가 그대로 192.168.0.9인데도 안 되는 경우

- ngrok을 이용해 터널링 주소를 뚫어주는 것이 가장 빠르고 확실합니다.

- 1. ngrok 설치
    sudo snap install ngrok

- 2. 8080 포트 외부에 뚫기
    ngrok http 8080


- **ngrok** 계정 가입부터 8080 포트(AGV HMI) 터널링까지 순서대로 실행하는 방법입니다.

---

### ngrok 설정 및 실행 (3단계)

1. **ngrok 가입 및 토큰 발급:** 웹 브라우저 작업.
1. [ngrok 회원가입 페이지](https://dashboard.ngrok.com/signup)로 이동해 계정을 만듭니다.
2. 로그인 후 [Authtoken 확인 페이지](https://dashboard.ngrok.com/get-started/your-authtoken)에서 화면에 나오는 **Your Authtoken** (긴 알파벳/숫자 조합)을 복사합니다.


2. **Authtoken 등록:** 우분투 터미널 작업 (최초 1회만 실행).
복사한 토큰을 터미널에 등록합니다.

```bash
ngrok config add-authtoken <복사한_AUTHTOKEN_문자열>

```

*(예시: `ngrok config add-authtoken 2X9abc1234567890...`)*


3. **8080 포트 터널링 개방:** 우분투 터미널 작업.
AGV HMI 웹 서버가 켜져 있는 상태에서 아래 명령어를 실행합니다.

```bash
ngrok http 8080

```


---

### 실행 후 화면 확인

명령어가 정상적으로 실행되면 터미널 화면에 아래와 같은 정보가 출력됩니다.

```text
Session Status                online                                            
Account                       your-email@example.com
Version                       3.39.10                                           
Region                        United States (us)                                
Forwarding                    https://xxxx-xxx-xxx.ngrok-free.app -> http://localhost:8080

```

* `Forwarding`에 있는 `[https://xxxx-xxx-xxx.ngrok-free.app](https://xxxx-xxx-xxx.ngrok-free.app)` 주소를 복사해 휴대폰이나 외부 PC 브라우저에 입력하면 내 컴퓨터의 8080 포트로 접속됩니다.
* **종료 방법:** 터미널에서 `Ctrl + C`를 누르면 터널이 즉시 닫히고 외부 접속이 차단됩니다.

화면(HTML)을 보여주는 웹 서버가 아니라, JSON 데이터를 주고받는 API 서버입니다.

코드에서 정의된 URL 엔드포인트는 딱 두 가지입니다:

GET /api/status (AGV 상태 조회)

POST /api/command (AGV 제어 명령 전송)

기본 경로(/)로 접속했을 때는 else 구문으로 빠져서 {"error": "Not Found"} (404)를 반환하도록 짜여 있었기 때문에 아까 그 화면이 뜬 것입니다.

상대방 컴퓨터에서 접속 및 테스트하는 방법
1. 웹 브라우저에서 상태(JSON) 조회할 때 (GET)
상대방 컴퓨터 브라우저 주소창에 주소 뒤에 /api/status를 붙여서 들어오라고 하시면 됩니다.

접속 주소:

[https://figure-handwash-pushover.ngrok-free.dev/api/status](https://figure-handwash-pushover.ngrok-free.dev/api/status)

결과: 브라우저 화면에 {"wheel_rpm": ..., "camera_url": ..., "status": "ok"} 형태의 JSON 데이터가 정상 출력됩니다.

2. 외부 웹 프론트엔드/스마트폰/다른 프로그램에서 명령 보낼 때 (POST)
상대방이 웹 화면이나 Postman 등에서 명령(JSON)을 보낼 때는 아래 엔드포인트로 요청을 전송하면 됩니다.

요청 URL: [https://figure-handwash-pushover.ngrok-free.dev/api/command](https://figure-handwash-pushover.ngrok-free.dev/api/command)

Method: POST

Header: Content-Type: application/json

Body (JSON 예시):

JSON
{
  "destination": "Station_A"
}
요약
상대방에게 "그냥 주소만 치지 말고 뒤에 /api/status 붙여서 브라우저로 들어가봐!"라고 알려주시면 Not Found 대신 AGV 상태 JSON 데이터가 뜨는 것을 바로 확인하실 수 있습니다!

실행할 때 꿀팁
다음번에 터미널을 새로 열어서 실행하실 때는 아래 두 줄만 순서대로 입력하시면 됩니다!

```Bash
source install/setup.bash
ros2 run AgvHmiServer AgvHmiServer
```

rm -rf build/AgvHmiServer install/AgvHmiServer log/
colcon build --packages-select AgvHmiServer

source install/setup.bash
ros2 run AgvHmiServer AgvHmiServer

```css
어떤 걸 여는 명령어를 찾으시나요? 상황에 맞는 실행 명령어 정리해 드릴게요!

---

### 1. ngrok 외부 포트 개방 (터널링)

8080 포트를 외부에 뚫어줄 때 쓰는 명령어입니다.

```bash
ngrok http 8080

```

---

### 2. AGV HMI 서버 실행

C++ REST API 서버를 켤 때 쓰는 명령어입니다.

```bash
source install/setup.bash
ros2 run AgvHmiServer AgvHmiServer

```

---

### 3. Nav2 내비게이션 구동

ROS 2 Nav2 액션 서버(bringup)를 켤 때 쓰는 기본 명령어입니다.

```bash
ros2 launch nav2_bringup bringup_launch.py use_sim_time:=False

```

---

### 4. TF 트리 시각화 확인

`odom` 프레임 연결태를 PDF로 뽑아서 확인할 때 쓰는 명령어입니다.

```bash
ros2 run tf2_tools view_frames

```

혹시 이 중에 찾으시던 게 있거나 다른 프로그램/파일을 말씀하신 거라면 바로 알려주세요!
```

```basj
수정한 코드(`networkserver.cpp`)를 반영하기 위해 서버를 재빌드하고 실행하는 전체 과정입니다.

터미널을 열고 아래 순서대로 실행해 주세요!

---

### 🔨 C++ 서버 재빌드 및 실행 3단계

#### 1. 기존 서버 종료 및 빌드 캐시 삭제

서버가 켜져 있다면 터미널에서 `Ctrl + C`를 눌러 종료한 뒤, 이전에 빌드된 임시 파일들을 깔끔하게 지워줍니다.

```bash
cd ~/agv_hmi
rm -rf build/AgvHmiServer install/AgvHmiServer log/

```

#### 2. AgvHmiServer 패키지 단독 재빌드

`colcon` 명령어로 수정된 C++ 소스 코드를 다시 컴파일합니다.

```bash
colcon build --packages-select AgvHmiServer

```

*(터미널 끝에 `Summary: 1 package finished [x.xs]` 라고 뜨면 정상적으로 빌드 성공입니다.)*

#### 3. 환경 변수 적용 및 서버 재실행

새로 빌드된 파일의 환경 변수를 불러온 뒤 서버를 실행합니다.

```bash
1. 기존 서버 종료 및 빌드 캐시 삭제
cd ~/agv_hmi
rm -rf build/AgvHmiServer install/AgvHmiServer log/

2. AgvHmiServer 패키지 단독 재빌드
colcon build --packages-select AgvHmiServer

3. 환경 변수 적용 및 서버 재실행

source install/setup.bash
ros2 run AgvHmiServer AgvHmiServer


```

---

### 💡 잘 들어갔는지 확인하는 팁

서버가 켜진 상태에서 UI 팀원이 요청을 다시 보냈을 때:

* `Content-Length header missing or invalid` 에러 메시지가 더 이상 뜨지 않고,
* 터미널 로그에 `[API 요청 수신]` 또는 `[목적지 명령 수신]` 메시지가 정상적으로 찍히는지 확인하시면 됩니다!

```