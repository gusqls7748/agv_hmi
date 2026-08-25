# GuideRobot AGV HMI Server — Single-Port 8080

## 역할

이 버전은 **C++/ROS2 서버 하나가 Web HMI 정적 파일과 REST API를 모두 제공**합니다.

별도 Web HMI 서버(8000)는 필요 없습니다.

```text
Browser / Web HMI
        |
        | http://192.168.0.9:8080/
        | http://192.168.0.9:8080/api/...
        v
+----------------------------+
| C++ Robot API + Static HMI |
|          :8080             |
+-------------+--------------+
              |
              | /goal_pose
              v
          Nav2 / AGV

Qt -----------------------> :8080
```

## 실행

```bash
cd /home/ubuntu/agv_hmi/agv_hmi-backend-8000-8080-final/agv_hmi-main
rm -rf build install log
colcon build --packages-select agv_hmi_server
source install/setup.bash
ros2 run agv_hmi_server agv_hmi_server
```

서버가 실행되면 웹 브라우저에서:

```text
http://192.168.0.9:8080/
```

상태 API:

```text
GET http://192.168.0.9:8080/api/status
```

목적지 명령:

```http
POST http://192.168.0.9:8080/api/command
Content-Type: application/json

{"destination":"room_301"}
```

수동 모드 상태 변경:

```http
POST http://192.168.0.9:8080/api/manual-mode
Content-Type: application/json

{"manual_mode":true}
```

## 정적 파일

서버가 직접 다음 파일을 제공합니다.

- `/` -> `index.html`
- `/app.css`
- `/app.js`
- `/robot-mark.svg`

정적 파일에는 `Cache-Control: no-cache, no-store`를 적용합니다.

## 목적지

- `restroom`
- `room_301`
- `room_302`
- `elevator`

`restroom`, `elevator` 좌표는 환경변수로 설정합니다:

```bash
export AGV_RESTROOM_X=...
export AGV_RESTROOM_Y=...
export AGV_ELEVATOR_X=...
export AGV_ELEVATOR_Y=...
```

## 중요한 역할 분리

- Web HMI 화면: 이 C++ 서버가 `8080`에서 직접 제공
- Qt: 같은 `8080` API를 사용
- ROS: `/goal_pose`만 발행
- Nav2 실행: 다른 담당자가 수행
- `/cmd_vel`: 이 서버에서 발행하지 않음
