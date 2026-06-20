#include "beacon.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_timer.h"

static const char *TAG = "beacon.mqtt";

static esp_mqtt_client_handle_t s_mqtt;
static beacon_ctx_t *s_ctx;

/* 构造主题：nasxx/{hub_id}/{tail} */
static int build_topic(char *out, size_t outlen, const beacon_ctx_t *c, const char *tail)
{
    return snprintf(out, outlen, "nasxx/%s/%s",
                    c->hub_id[0] ? c->hub_id : "_unknown", tail);
}

static void publish_status_locked(void)
{
    char topic[96];
    char payload[64];
    build_topic(topic, sizeof(topic), s_ctx, "beacon/status");
    snprintf(payload, sizeof(payload), "{\"online\":true,\"state\":%d}", s_ctx->state);
    esp_mqtt_client_publish(s_mqtt, topic, payload, 0, 1, 1);

    /* TODO(MQTT5): 用 esp_mqtt5_client_set_publish_property 配置 topic alias，
     * 之后用 esp_mqtt_client_publish_with_topic_alias 节省上行带宽。 */
}

static void on_mqtt_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    esp_mqtt_event_handle_t e = data;
    switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "mqtt v5 connected");
        beacon_set_state(BEACON_STATE_MQTT_CONNECTED);
        /* 订阅唤醒命令主题 */
        {
            char cmd_topic[96];
            build_topic(cmd_topic, sizeof(cmd_topic), s_ctx, "beacon/cmd/wake");
            esp_mqtt_client_subscribe(s_mqtt, cmd_topic, 1);
            ESP_LOGI(TAG, "subscribed %s", cmd_topic);
        }
        beacon_set_state(BEACON_STATE_ONLINE);
        publish_status_locked();
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "mqtt disconnected");
        beacon_set_state(BEACON_STATE_MQTT_CONNECTING);
        break;
    case MQTT_EVENT_DATA: {
        /* payload 期望是 JSON {"mac":"AA:BB:CC:DD:EE:FF"} 或 6 字节 hex */
        ESP_LOGI(TAG, "topic=%.*s len=%d", e->topic_len, e->topic, e->data_len);
        /* 简化：把 payload 当成 mac 字符串解析；实际协议待落地 */
        uint8_t mac[6] = {0};
        if (e->data_len >= 17) {
            if (sscanf(e->data, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                       &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6) {
                /* TODO(MQTT5): WoL 执行结果用 esp_mqtt5_client_set_user_property
                 * 写到回执 PUBLISH 的 User Property，app 订阅 evt 主题接收。 */
                (void)beacon_wol_send(mac, "255.255.255.255");
            }
        }
        break;
    }
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "mqtt error type=%d",
                 e->error_handle ? e->error_handle->error_type : 0);
        beacon_set_state(BEACON_STATE_ERROR);
        break;
    default: break;
    }
}

void beacon_mqtt_start(void)
{
    if (s_mqtt) {
        ESP_LOGW(TAG, "mqtt already started");
        return;
    }
    s_ctx = beacon_ctx_get();

    char uri[128];
    snprintf(uri, sizeof(uri), "mqtts://%s:%u", s_ctx->mqtt_host, s_ctx->mqtt_port);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = uri,
        .credentials.client_id = s_ctx->mqtt_client_id,
        .credentials.username = s_ctx->mqtt_username,
        .credentials.authentication.password = s_ctx->mqtt_password,
        .network.reconnect_timeout_ms = 5000,
        .session.keepalive = 30,
        /* MQTT 5.0：用 reason code / user property / topic alias / shared sub 等特性 */
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
    };
    s_mqtt = esp_mqtt_client_init(&cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt, ESP_EVENT_ANY_ID, on_mqtt_event, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt));
    beacon_set_state(BEACON_STATE_MQTT_CONNECTING);

    /* TODO(MQTT5): 进入 ONLINE 后用 esp_mqtt5_client_set_connect_property 设置
     * session_expiry_interval / topic_alias_maximum / message_expiry_interval 等，
     * 待联调时按 ESP-IDF v5.3 实际 API 名落地。 */
}

void beacon_mqtt_publish_status(void)
{
    if (s_mqtt && s_ctx) publish_status_locked();
}


