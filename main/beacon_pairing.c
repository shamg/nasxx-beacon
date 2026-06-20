#include "beacon.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include <string.h>

static const char *TAG = "beacon.pairing";
static const char *NVS_NAMESPACE = "beacon";
static const char *KEY_HUB_ID    = "hub_id";
static const char *KEY_MQTT_HOST = "mqtt_host";
static const char *KEY_MQTT_PORT = "mqtt_port";
static const char *KEY_MQTT_USER = "mqtt_user";
static const char *KEY_MQTT_PASS = "mqtt_pass";

/* 单例上下文 */
static beacon_ctx_t s_ctx;

beacon_ctx_t *beacon_ctx_get(void) { return &s_ctx; }

void beacon_set_state(beacon_state_t s)
{
    if (s_ctx.state != s) {
        ESP_LOGI(TAG, "state %d -> %d", s_ctx.state, s);
        s_ctx.state = s;
    }
}

int beacon_pairing_load(beacon_ctx_t *ctx)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        return -1;
    }

    size_t need = 0;
    bool ok = true;

#define R_STR(k, field) do { \
    need = sizeof(ctx->field); \
    if (nvs_get_str(h, k, ctx->field, &need) != ESP_OK) ok = false; \
} while (0)

    R_STR(KEY_HUB_ID,    hub_id);
    R_STR(KEY_MQTT_HOST, mqtt_host);
    R_STR(KEY_MQTT_USER, mqtt_username);
    R_STR(KEY_MQTT_PASS, mqtt_password);

    uint16_t port = 0;
    if (nvs_get_u16(h, KEY_MQTT_PORT, &port) == ESP_OK) {
        ctx->mqtt_port = port;
    } else {
        ctx->mqtt_port = CONFIG_BEACON_DEFAULT_BROKER_PORT;
    }

    /* client_id 默认用 MAC */
    if (ctx->mqtt_client_id[0] == 0) {
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        snprintf(ctx->mqtt_client_id, sizeof(ctx->mqtt_client_id),
                 "beacon-%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

#undef R_STR
    nvs_close(h);
    return ok ? 0 : -1;
}

int beacon_pairing_save(const beacon_ctx_t *ctx)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return -1;

    nvs_set_str(h, KEY_HUB_ID,    ctx->hub_id);
    nvs_set_str(h, KEY_MQTT_HOST, ctx->mqtt_host);
    nvs_set_str(h, KEY_MQTT_USER, ctx->mqtt_username);
    nvs_set_str(h, KEY_MQTT_PASS, ctx->mqtt_password);
    nvs_set_u16(h, KEY_MQTT_PORT, ctx->mqtt_port);

    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "pairing saved (hub_id=%s)", ctx->hub_id);
    return 0;
}
