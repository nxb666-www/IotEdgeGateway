# IoT Edge Gateway 项目

## 项目概述
嵌入式系统课程项目，构建物联网边缘网关系统。STM32端负责传感器数据采集和执行器控制，通过Zigbee与RK3568边缘网关通信。

## 项目分组
- 组1: 刘天赐(组长)、王昶斌
- 组2: 于霄(组长)、白子枫
- 组3: 金培辰(组长)、毛志龙、聂潇滨
- 组4: 朱博林(组长)、焦培杰

## 技术栈
C/C++14/17 + CMake + Mongoose/nginx + mjpg-streamer/gstreamer/ffmpeg + SQLite3 + Qt5 + Web + Zigbee + **MQTT（必做，没有mqtt直接不过关）**

## 四大模块
1. **Web端**: 视频流拉取(录视频+拍照) + 传感器数据显示 + 下发控制命令
2. **Qt端**: 功能与Web端一致，数据同步
3. **RK3568边缘节点**: 系统移植(buildroot) + 主程序转发
4. **单片机模块**: 至少6个外设，上传+下发功能

## STM32外设（至少6个）
LED灯、电机(风扇)、舵机、蜂鸣器、DHT11温湿度、光照传感器、红外检测

## 通信链路
STM32 → Zigbee → RK3568 → MQTT → Web/Qt

## JSON数据格式（STM32端）
```json
{
    "source": "stm32",
    "driver": "dht11",
    "id": 1,
    "data": {
        "data_time": 1756899527,
        "humi_int": 32,
        "humi_deci": 0,
        "temp_int": 28,
        "temp_deci": 4,
        "check_sum": 64
    }
}
```

## JSON数据格式（网关统一设备模型）
```json
{
    "device_id": "temp",
    "type": "sensor",
    "data": {"value": 25.5},
    "ts": 1700000000
}
```

## RK3568网关五阶段开发计划
1. 基础框架 — 日志、配置、HTTP服务器、health API（30%）
2. 设备管理+MQTT — 设备注册表、MQTT客户端（30%）
3. Web前端+WebSocket — 传感器仪表盘、执行器控制（20%）
4. 规则引擎 — 自动化+告警规则（20%）
5. 可选进阶 — 摄像头、Modbus、Zigbee、SQLite（加分）

## RK3568网关API接口
- GET /api/health — 健康检查
- GET /api/version — 版本查询
- GET /api/devices — 设备列表
- GET /api/devices/:id — 设备详情
- POST /api/actuators/:id/set — 下发命令
- GET /api/status — 设备状态聚合
- POST /api/control — 下发控制指令
- GET /api/rules — 规则列表
- POST /api/rules/reload — 重载规则
- WebSocket: ws://host:port/ws

## 验收要求
- 每天整理遇到的问题及解决方案
- 组长负责需求分配、代码整合(git)、文档、PPT
- 功能完整性40%、代码质量25%、工程化15%、创新扩展10%、演示效果10%
