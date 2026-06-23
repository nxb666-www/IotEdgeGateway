# IotEdgeGateway

基于 RK3568 的物联网边缘网关系统，实现传感器数据采集、设备控制、视频监控、规则引擎等功能。

## 系统架构

```
STM32 ──(Zigbee/串口)──► RK3568 边缘网关 ──(MQTT)──► Web/Qt 客户端
  │                        │
  ├─ AHT30 温湿度          ├─ iotgw_mqtt_broker    (MQTT 消息转发)
  ├─ 光照传感器             ├─ iotgw_gateway         (HTTP + WebSocket + 规则引擎)
  ├─ 红外对射               ├─ mjpg-streamer         (视频流)
  ├─ LED / 电机 / 蜂鸣器    └─ SQLite3               (数据存储)
  └─ 舵机
```

## 技术栈

| 层级 | 技术 |
|------|------|
| MCU 端 | STM32F411 + AHT30 + 光照传感器 + 红外对射 + LED + 电机 + 蜂鸣器 + ESP8266(WiFi/MQTT) |
| 网关端 | C/C++14 + CMake + Mongoose(HTTP/MQTT) + SQLite3 + rapidyaml + mjpg-streamer |
| 客户端 | Web (HTML/JS) + Qt5 (C++ Widgets) |
| 通信 | MQTT (iotgw/dev/telemetry/* ↑, iotgw/dev/cmd/* ↓) + HTTP REST API + WebSocket |

## 目录结构

```
IotEdgeGateway/
├── F411/                    # STM32 固件
│   ├── Hardware/            # 外设驱动 (AHT30, LED, Motor, Buzzer, Infrared, LightSensor, ESP8266)
│   ├── Protocol/            # 通信协议 (JSON 封装 + MQTT 发布/解析)
│   ├── Library/             # STM32 标准库
│   └── User/                # 主程序入口
├── src/                     # RK3568 网关源码
│   ├── gateway/             # 网关核心 (启动流程、MQTT 消费、规则触发)
│   ├── core/
│   │   ├── common/          # 日志、配置管理、工具函数
│   │   ├── device/          # 设备注册表、MQTT 适配器
│   │   └── storage/         # SQLite3 存储层
│   ├── services/
│   │   └── web_services/    # REST API、WebSocket、摄像头 API
│   └── tools/
│       └── mqtt_broker/     # 轻量 MQTT broker (基于 Mongoose, 162 行)
├── www/                     # Web 前端
│   ├── index.html           # 单页应用 (实时监控 + 历史数据)
│   └── js/
│       └── hls.min.js       # HLS 播放库 (占位, 阶段五可选)
├── qt_client/               # Qt5 桌面客户端
│   ├── mainwidget.h/cpp     # 主界面 (2 Tab: 实时监控 + 历史数据)
│   └── iotgw_qt_client.pro  # Qt 工程文件
├── config/                  # 网关配置
│   ├── environments/        # 开发/生产环境配置
│   ├── devices/             # 设备定义 (传感器 + 执行器)
│   └── rules/               # 规则引擎 (告警 + 自动化)
├── scripts/                 # 部署脚本
├── iotgw_package/           # 打包部署目录 (bin + config + www)
├── docs/                    # 开发文档
├── CMakeLists.txt           # 构建系统
└── build.sh                 # 一键编译脚本
```

## 快速开始

### 编译 (交叉编译 aarch64)

```bash
./build.sh -a        # Debug 版本
./build.sh -a -r     # Release 版本
```

### 编译 (本地开发)

```bash
./build.sh -x        # 本机 Debug 版本
```

### 部署到 RK3568

将 `iotgw_package/` 目录复制到板子上，运行：

```bash
chmod +x start.sh
./start.sh
```

或通过 NFS 共享方式（板子挂载虚拟机目录）：

```bash
# 板子端
mount -t nfs 192.168.137.118:/home/nfs /mnt
cd /mnt/IotEdgeGateway
./scripts/board_start.sh
```

### 访问

- Web 界面: `http://<板子IP>:8081`
- Qt 客户端: 运行 `qt_client/release/iotgw_qt_client.exe`，配置 `iotgw_qt_client.ini` 中的网关地址

## API 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/status` | 所有传感器最新值 |
| GET | `/api/devices` | 设备列表 |
| GET | `/api/history?device_id=xxx&limit=50` | 历史数据查询 |
| POST | `/api/control` | 下发控制指令 |
| GET | `/stream/live.jpg` | 视频帧抓取 |
| WebSocket | `/ws` | 实时数据推送 |

### 控制指令格式

```json
{
    "type": "control",
    "payload": {
        "led_on": 1,
        "led_br": 50,
        "motor_on": 1,
        "motor_sp": 30,
        "motor_dir": 0,
        "buzzer": 0
    }
}
```

### 传感器数据格式 (STM32 → MQTT)

```json
{
    "device_id": "temp",
    "type": "sensor",
    "source": "stm32",
    "driver": "aht30",
    "data": {"value": 28.4},
    "ts": 0
}
```

## 外设清单

| 外设 | 功能 | 上传 | 下发 |
|------|------|------|------|
| AHT30 | 温湿度传感器 | ✅ 温度/湿度 | - |
| 光照传感器 | 环境光照 | ✅ 光照值 | - |
| 红外对射 | 物体检测 | ✅ 0/1 状态 | - |
| LED | 照明控制 | - | ✅ 开关 + 亮度 |
| 直流电机 | 风扇控制 | - | ✅ 开关 + 转速 + 正反转 |
| 蜂鸣器 | 报警 | - | ✅ 开关 |

## 网关依赖 (全部静态链接，无需安装)

| 库 | 用途 | 集成方式 |
|----|------|---------|
| Mongoose 7.15 | HTTP/MQTT/WebSocket 服务器 | FetchContent 源码编译 |
| SQLite3 | 数据存储 | FetchContent 单文件编译 |
| rapidyaml 0.10 | YAML 配置解析 | FetchContent 源码编译 |
