# AGV HMI Topic-Only Server — UI 호환 최종본

## 1. 역할

이 프로그램은 **GUI/QML 없이 HTTP API와 ROS 2 `/goal_pose` Topic만 담당하는 서버**입니다.

```text
[Web HMI : 8000] ──HTTP──> [이 서버 : 8080] ──ROS 2──> [/goal_pose] ──> [Nav2/로봇 팀]
[Qt 관리자 화면] ──HTTP──> [이 서버 : 8080]
```

- Web HMI는 별도 팀이 실행합니다.
- Qt 관리자 화면도 별도 팀이 실행합니다.
- 이 프로그램은 QML/GUI를 실행하지 않습니다.
- Nav2를 직접 실행하거나 Action을 호출하지 않습니다.
- **`/cmd_vel`을 발행하지 않습니다.**
- 자동 목적지 명령은 `/goal_pose` (`geometry_msgs/msg/PoseStamped`)만 발행합니다.

## 2. UI 팀과의 API 규격

UI가 사용하는 주소:

```text
http://<이 서버 PC IP>:8080
```

예:

```text
http://192.168.0.9:8080
```

### 상태 조회

```http
GET /api/status
```

응답:

```json
{
  "status": "moving",
  "manual_mode": false,
  "server": "agv_hmi_topic_only"
}
```

`status`:
- `idle`: 대기/취소
- `moving`: 목적지 명령을 정상 접수함
- `arrived`: 실제 도착 상태를 외부 로봇 제어부가 반영함

`manual_mode`:
- `true`: Qt 관리자가 수동 조종 중인 상태
- `false`: 수동 조종 상태가 아님

## 3. 목적지 명령

UI가 보내는 형식은 기존과 동일합니다.

```http
POST /api/command
Content-Type: application/json
```

```json
{
  "destination": "room_301"
}
```

지원 ID:

```text
restroom
room_301
room_302
elevator
```

성공하면 서버는 `status`를 `moving`으로 바꾸고 `/goal_pose`를 발행합니다.

### 좌표 설정

현재 소스에 확정되어 있던 좌표는 그대로 유지합니다.

```text
room_301 -> (-2.00, -0.50)
room_302 -> ( 2.00,  2.00)
```

`restroom`과 `elevator`는 **UI ZIP에 실제 좌표가 들어 있지 않으므로 임의의 좌표를 넣지 않았습니다.**
실제 map 좌표를 정한 뒤 아래 환경변수로 설정할 수 있습니다.

```bash
export AGV_RESTROOM_X=<화장실_X>
export AGV_RESTROOM_Y=<화장실_Y>

export AGV_ELEVATOR_X=<엘리베이터_X>
export AGV_ELEVATOR_Y=<엘리베이터_Y>
```

그 다음 서버를 실행하면 됩니다.

환경변수가 없으면 해당 목적지는 오류로 처리되어 잘못된 위치로 로봇을 보내지 않습니다.

## 4. 안내 취소

UI가 보내는 형식:

```http
POST /api/command
Content-Type: application/json
```

```json
{
  "command": "cancel"
}
```

서버 상태가 `idle`로 바뀝니다.

## 5. Qt 관리자 수동 모드 상태

Web HMI가 `manual_mode`를 알아야 하므로 서버에 아래 API를 제공합니다.

### 수동 모드 ON

```http
POST /api/manual-mode
Content-Type: application/json
```

```json
{
  "manual_mode": true
}
```

### 수동 모드 OFF

```json
{
  "manual_mode": false
}
```

이 서버는 이 API에서 **수동 주행 명령을 보내지 않습니다.**
즉 `/cmd_vel`은 발행하지 않습니다.

Qt/로봇 제어부가 실제 수동 주행을 담당하고, 이 서버는 Web HMI에 보여줄 `manual_mode` 상태만 저장합니다.

`manual_mode=true`일 때 Web HMI의 목적지 명령은 `409 Conflict`로 거부합니다.

## 6. 실제 도착 상태

Web HMI는 `/api/status`의 `status == "arrived"`를 보고 도착 화면으로 전환합니다.

로봇 제어부/도착 감지부에서 실제 도착을 확인한 경우 다음 API를 사용할 수 있습니다.

```http
POST /api/status
Content-Type: application/json
```

```json
{
  "status": "arrived"
}
```

대기 상태:

```json
{
  "status": "idle"
}
```

안내 시작 상태는 목적지 명령 성공 시 서버가 자동으로 `moving`으로 변경합니다.

## 7. ROS 2 Topic

발행 Topic:

```text
/goal_pose
```

타입:

```text
geometry_msgs/msg/PoseStamped
```

frame:

```text
map
```

예:

```bash
ros2 topic echo /goal_pose
```

Web HMI에서 `room_301`을 선택하면 다음과 같이 확인할 수 있습니다.

```text
[Goal Topic] room_301 -> x=-2.00, y=-0.50
```

## 8. `/cmd_vel` 관련

이 서버에는 수동 주행 코드가 없습니다.

발행하지 않는 것:

```text
/cmd_vel
```

따라서 전진/후진/좌/우/STOP 같은 실제 수동 주행은 Qt/로봇 제어 담당자가 별도로 처리합니다.

## 9. 빌드

프로젝트 루트에서:

```bash
cd "/home/ubuntu/agv_hmi/agv_hmi-topic-only-server-final (1)/agv_hmi-main"
```

기존 잘못된 build/install/log가 있다면 한 번 정리:

```bash
rm -rf build install log
```

빌드:

```bash
colcon build --packages-select agv_hmi_server
```

성공 후:

```bash
source install/setup.bash
```

실행:

```bash
ros2 run agv_hmi_server agv_hmi_server
```

## 10. 서버 PC IP 확인

```bash
hostname -I
```

예:

```text
192.168.0.9
```

서버는:

```text
0.0.0.0:8080
```

으로 listen하므로 같은 LAN의 UI/Qt PC가:

```text
http://192.168.0.9:8080/api/status
```

로 접근할 수 있습니다.

브라우저에서:

```text
http://192.168.0.9:8080/
```

를 직접 열었을 때 `Not Found`가 나오는 것은 정상입니다.
루트 `/` 페이지를 제공하는 서버가 아니며 API 서버입니다.

## 11. 테스트

### 상태

```bash
curl http://192.168.0.9:8080/api/status
```

### 목적지

```bash
curl -X POST http://192.168.0.9:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"destination":"room_301"}'
```

### 취소

```bash
curl -X POST http://192.168.0.9:8080/api/command \
  -H "Content-Type: application/json" \
  -d '{"command":"cancel"}'
```

### 수동 모드 ON

```bash
curl -X POST http://192.168.0.9:8080/api/manual-mode \
  -H "Content-Type: application/json" \
  -d '{"manual_mode":true}'
```

### 수동 모드 OFF

```bash
curl -X POST http://192.168.0.9:8080/api/manual-mode \
  -H "Content-Type: application/json" \
  -d '{"manual_mode":false}'
```

### 도착 상태

```bash
curl -X POST http://192.168.0.9:8080/api/status \
  -H "Content-Type: application/json" \
  -d '{"status":"arrived"}'
```

## 12. 팀원에게 설명할 내용

> 내 프로그램은 GUI가 아니라 서버 역할만 한다.
> Web HMI와 Qt가 8080 API로 연결된다.
> Web HMI가 `POST /api/command`로 `{"destination":"room_301"}`을 보내면 서버가 좌표를 찾아 `/goal_pose`를 ROS 2 Topic으로 발행한다.
> Nav2는 다른 팀이 담당한다.
> 이 서버는 `/cmd_vel`을 발행하지 않고 수동 주행도 담당하지 않는다.
> Web HMI의 `manual_mode`와 `status` 표시를 위해 `/api/status`를 제공한다.
