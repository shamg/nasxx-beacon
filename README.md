# nasxx-beacon

ESP32-S3 固件，承担 nasxx 在外网场景下的 **常在线唤醒 + 信令中继** 角色。

当 app 与 hub 不在同一局域网时，beacon 作为常驻设备：
- 持有到 nxx-server 内置 MQTT broker 的长连接
- 接收 app 经由 server 下发的唤醒指令，在内网对 hub 电脑发送 WoL Magic Packet
- 可选：作为 hub 状态/在线心跳的中继节点

## 硬件 / 框架

- SoC：ESP32-S3
- 框架：ESP-IDF v5.3 LTS
- 目标 board：`esp32s3`

## 目录结构

```
nasxx-beacon/
├── CMakeLists.txt          # 顶层 CMake（idf 构建入口）
├── main/
│   ├── CMakeLists.txt
│   ├── main.c              # app_main 入口
│   ├── beacon.h            # 内部 API
│   ├── beacon_wifi.c       # Wi-Fi 连接管理
│   ├── beacon_mqtt.c       # MQTT 客户端（连 nxx-server broker）
│   ├── beacon_wol.c        # WoL Magic Packet 发送
│   └── beacon_pairing.c    # 与 hub/server 的配对/凭据存储（NVS）
├── sdkconfig.defaults      # 默认配置（Wi-Fi/MQTT/NVS/分区）
└── partitions.csv          # 分区表（nvs / phy_init / factory / ota_data / ota）
```

## 协议

- 传输：MQTT v5（broker 复用 nxx-server）
- 主题命名（初稿，待协议落地）：
  - `nasxx/{hub_id}/beacon/cmd/wake` — app → beacon 唤醒指令
  - `nasxx/{hub_id}/beacon/status` — beacon → server 心跳/在线
  - `nasxx/{hub_id}/beacon/evt` — beacon 上报事件
- 鉴权：每台 beacon 持有 server 签发的 device credential，存 NVS；详见 `docs/protocol/PAIRING.md`（后续扩展 beacon 配对流）。

## 构建 / 烧录

前置：已安装 ESP-IDF v5.3 并 source 了 `export.sh`。

```bash
idf.py set-target esp32s3
idf.py menuconfig    # 配 Wi-Fi SSID/密码、MQTT broker 地址
idf.py build
idf.py -p /dev/cu.usbmodem* flash monitor
```

## 与元仓库的关系

本目录是 nasxx 元仓库的 git 子模块，对应远程 `git@github.com:shamg/nasxx-beacon.git`。元仓库仅跟踪子模块指针，固件开发在本仓库内进行。
