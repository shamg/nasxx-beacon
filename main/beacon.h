#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 设备运行状态 */
typedef enum {
    BEACON_STATE_INIT = 0,
    BEACON_STATE_WIFI_CONNECTING,
    BEACON_STATE_WIFI_CONNECTED,
    BEACON_STATE_MQTT_CONNECTING,
    BEACON_STATE_MQTT_CONNECTED,
    BEACON_STATE_ONLINE,
    BEACON_STATE_ERROR,
} beacon_state_t;

/* 全局上下文 */
typedef struct {
    beacon_state_t state;
    char hub_id[32];           /* 配对时写入的 hub 标识 */
    char mqtt_host[64];        /* nxx-server broker host */
    uint16_t mqtt_port;        /* 默认 8883 (TLS) */
    char mqtt_client_id[48];
    char mqtt_username[48];
    char mqtt_password[64];
} beacon_ctx_t;

beacon_ctx_t *beacon_ctx_get(void);
void beacon_set_state(beacon_state_t s);

/* wifi */
void beacon_wifi_start(void);

/* mqtt */
void beacon_mqtt_start(void);
void beacon_mqtt_publish_status(void);

/* wol — 发送 Magic Packet 到 mac(6字节) */
int beacon_wol_send(const uint8_t mac[6], const char *bcast_ip);

/* pairing — 从 NVS 读取/写入配对凭据 */
int beacon_pairing_load(beacon_ctx_t *ctx);
int beacon_pairing_save(const beacon_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
