# API 接口文档

## 1. 基础信息

默认服务地址：

```text
http://<RK3568_IP>:8081
```

开发板热点网络示例：

```text
http://192.168.233.107:8081
```

本机调试示例：

```text
http://127.0.0.1:8081
```

响应格式以 JSON 为主，视频帧接口返回 JPEG。

## 2. 系统接口

### 2.1 健康检查

```http
GET /api/health
```

响应示例：

```json
{
  "status": "正常"
}
```

### 2.2 版本查询

```http
GET /api/version
```

响应示例：

```json
{
  "version": "0.1.0"
}
```

## 3. 设备接口

### 3.1 查询设备列表

```http
GET /api/devices
```

用途：返回设备注册表中的所有设备，包括传感器和执行器。

响应字段说明：

| 字段 | 说明 |
| --- | --- |
| `id` | 设备 ID |
| `kind` | `sensor` 或 `actuator` |
| `transport` | 当前通信方式，通常为 `mqtt` |
| `telemetry_topic` | 遥测主题 |
| `command_topic` | 控制主题 |
| `status.online` | 是否在线 |
| `status.last_seen_ms` | 最近更新时间 |
| `status.last_topic` | 最近一次 MQTT topic |
| `status.last_payload` | 最近一次 payload |

### 3.2 查询单个设备

```http
GET /api/devices/{id}
```

示例：

```bash
wget -qO- http://127.0.0.1:8081/api/devices/temp
```

响应示例：

```json
{
  "id": "temp",
  "kind": "sensor",
  "transport": "mqtt",
  "telemetry_topic": "iotgw/dev/telemetry/temp",
  "command_topic": "",
  "status": {
    "online": true,
    "last_seen_ms": 1780000000000,
    "last_topic": "iotgw/dev/telemetry/temp",
    "last_payload": "{\"device_id\":\"temp\",\"data\":{\"value\":27.7}}"
  }
}
```

### 3.3 查询设备状态聚合

```http
GET /api/status
```

用途：给 Web/Qt 首页快速刷新当前传感器和执行器状态。

响应示例：

```json
{
  "buzzer": "",
  "humi": 60.1,
  "ir": 0,
  "led": "",
  "light": 64,
  "motor": "",
  "temp": 27.7
}
```

说明：

- 传感器有数据时返回数值。
- 执行器没有遥测数据时通常为空字符串。
- 如果某个传感器一直为空，优先检查 MQTT telemetry 是否进入网关。

## 4. 控制接口

### 4.1 统一控制接口

```http
POST /api/control
Content-Type: application/json
```

请求示例：

```json
{
  "type": "control",
  "payload": {
    "led_on": 1,
    "led_br": 80,
    "motor_on": 1,
    "motor_sp": 30,
    "motor_dir": 0,
    "buzzer": 0
  }
}
```

字段说明：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `led_on` | number | LED 开关，`1` 开，`0` 关 |
| `led_br` | number | LED 亮度，建议 `0-100` |
| `motor_on` | number | 电机开关，`1` 开，`0` 关 |
| `motor_sp` | number | 电机速度，建议 `0-100` |
| `motor_dir` | number | 电机方向，`0` 正转，`1` 反转 |
| `buzzer` | number | 蜂鸣器，`1` 开，`0` 关 |

响应示例：

```json
{
  "status": "正常"
}
```

开发板无 curl 时可用 `wget` 测试：

```bash
wget -qO- \
  --header="Content-Type: application/json" \
  --post-data='{"type":"control","payload":{"led_on":1,"led_br":80,"motor_on":0,"motor_sp":30,"motor_dir":0,"buzzer":0}}' \
  http://127.0.0.1:8081/api/control
```

网关会发布到 MQTT：

```text
iotgw/dev/cmd/control
```

发布 payload：

```json
{
  "type": "control",
  "payload": {
    "led_on": 1,
    "led_br": 80,
    "motor_on": 0,
    "motor_sp": 30,
    "motor_dir": 0,
    "buzzer": 0
  }
}
```

### 4.2 单执行器接口

```http
POST /api/actuators/{id}/set
```

示例：

```bash
wget -qO- \
  --header="Content-Type: application/json" \
  --post-data='{"value":1}' \
  http://127.0.0.1:8081/api/actuators/led/set
```

说明：该接口会按设备注册表中的 `command_topic` 发布 MQTT，适合单设备调试；Web 页面主要使用 `/api/control`。

## 5. 历史数据接口

### 5.1 查询历史遥测

```http
GET /api/history?device_id={id}&limit={n}&from={from_ms}&to={to_ms}
```

参数说明：

| 参数 | 必填 | 说明 |
| --- | --- | --- |
| `device_id` | 是 | 设备 ID，如 `temp`、`humi`、`light`、`ir` |
| `limit` | 否 | 返回条数，默认 `100` |
| `from` | 否 | 起始时间，毫秒时间戳 |
| `to` | 否 | 结束时间，毫秒时间戳 |

示例：

```bash
wget -qO- "http://127.0.0.1:8081/api/history?device_id=temp&limit=20"
```

响应示例：

```json
[
  {
    "value": 27.7,
    "ts": 123456,
    "created_at": 1780000000000
  }
]
```

说明：

- SQLite 启用时才可用。
- 数据库路径由配置项 `storage.db_path` 决定，默认 `data/iotgw.db`。
- 当前只保存遥测数据，不保存控制命令。

## 6. 规则接口

### 6.1 查询规则列表

```http
GET /api/rules
```

响应示例：

```json
[
  {
    "id": "cooling_on",
    "category": "automation",
    "enabled": true,
    "sensor_id": "temp",
    "op": ">=",
    "value": 30
  }
]
```

### 6.2 启用规则

```http
POST /api/rules/{id}/enable
```

示例：

```bash
wget -qO- --post-data='' http://127.0.0.1:8081/api/rules/cooling_on/enable
```

### 6.3 禁用规则

```http
POST /api/rules/{id}/disable
```

示例：

```bash
wget -qO- --post-data='' http://127.0.0.1:8081/api/rules/cooling_on/disable
```

### 6.4 重新加载规则

```http
POST /api/rules/reload
```

示例：

```bash
wget -qO- --post-data='' http://127.0.0.1:8081/api/rules/reload
```

说明：

- 规则配置来自 `config/rules/automation-rules.yaml` 和 `config/rules/alarm-rules.yaml`。
- 条件运算符支持 `>`、`>=`、`<`、`<=`、`==`、`!=`。
- 人工控制优先级高于自动规则，默认 30 秒内规则不覆盖手动控制。

## 7. 视频接口

### 7.1 探测摄像头

```http
GET /api/camera/probe
```

返回纯文本，包含当前配置设备、检测到的视频节点、视频流状态等信息。

### 7.2 启动视频预览

```http
GET /api/camera/start_stream
```

响应示例：

```json
{
  "result": "ok",
  "mode": "preview",
  "backend": "ffmpeg",
  "device": "/dev/video0"
}
```

说明：

- 优先使用 FFmpeg。
- 如果 FFmpeg 获取不到预览帧，会尝试 GStreamer。
- 实时帧写入 `/tmp/rk_stream/live.jpg`。

### 7.3 获取实时 JPEG

```http
GET /stream/live.jpg
```

响应类型：

```text
image/jpeg
```

Web 页面通过不断刷新该接口显示视频。

### 7.4 停止视频预览

```http
GET /api/camera/stop_stream
```

响应示例：

```json
{
  "result": "ok"
}
```

### 7.5 拍照

```http
GET /api/camera/snapshot
```

响应示例：

```json
{
  "result": "ok",
  "file": "www/snap_20260623_120000.jpg"
}
```

### 7.6 开始录制

```http
GET /api/camera/start_record
```

说明：需要先启动视频预览。

响应示例：

```json
{
  "result": "ok",
  "file": "www/rec_20260623_120000.mp4"
}
```

### 7.7 停止录制

```http
GET /api/camera/stop_record
```

响应示例：

```json
{
  "result": "ok",
  "file": "www/rec_20260623_120000.mp4"
}
```

## 8. WebSocket 接口

```text
ws://<RK3568_IP>:8081/ws
```

用途：

1. 网关收到 MQTT telemetry 后广播给浏览器。
2. 浏览器也可通过 WebSocket 发送 MQTT 发布请求。

广播示例：

```json
{
  "type": "mqtt_msg",
  "topic": "iotgw/dev/telemetry/temp",
  "payload": "{\"device_id\":\"temp\",\"data\":{\"value\":27.7}}"
}
```

## 9. MQTT 主题约定

### 9.1 遥测上行

```text
iotgw/dev/telemetry/{device_id}
```

设备：

```text
temp
humi
light
ir
```

### 9.2 命令下行

```text
iotgw/dev/cmd/control
iotgw/dev/cmd/led
iotgw/dev/cmd/motor
iotgw/dev/cmd/buzzer
```

当前 Web 页面主要使用：

```text
iotgw/dev/cmd/control
```

## 10. 常用测试命令

查看状态：

```bash
wget -qO- http://127.0.0.1:8081/api/status
```

查看单设备：

```bash
wget -qO- http://127.0.0.1:8081/api/devices/temp
```

控制 LED：

```bash
wget -qO- \
  --header="Content-Type: application/json" \
  --post-data='{"type":"control","payload":{"led_on":1,"led_br":80,"motor_on":0,"motor_sp":30,"motor_dir":0,"buzzer":0}}' \
  http://127.0.0.1:8081/api/control
```

查询历史：

```bash
wget -qO- "http://127.0.0.1:8081/api/history?device_id=temp&limit=20"
```

启动视频：

```bash
wget -qO- http://127.0.0.1:8081/api/camera/start_stream
```

抓取实时帧：

```bash
wget -O /tmp/live.jpg http://127.0.0.1:8081/stream/live.jpg
```
