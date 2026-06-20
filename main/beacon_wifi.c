#include "beacon.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "beacon.wifi";

#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t s_wifi_eg;
static beacon_ctx_t *s_ctx;

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base != WIFI_EVENT) return;
    switch (id) {
    case WIFI_EVENT_STA_START:
        beacon_set_state(BEACON_STATE_WIFI_CONNECTING);
        esp_wifi_connect();
        break;
    case WIFI_EVENT_STA_DISCONNECTED: {
        beacon_set_state(BEACON_STATE_WIFI_CONNECTING);
        ESP_LOGW(TAG, "wifi disconnected, reconnecting");
        esp_wifi_connect();
        break;
    }
    default: break;
    }
}

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base != IP_EVENT || id != IP_EVENT_STA_GOT_IP) return;
    ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
    ESP_LOGI(TAG, "got ip " IPSTR, IP2STR(&e->ip_info.ip));
    beacon_set_state(BEACON_STATE_WIFI_CONNECTED);
    xEventGroupSetBits(s_wifi_eg, WIFI_CONNECTED_BIT);
    /* 拉起 MQTT */
    beacon_mqtt_start();
}

void beacon_wifi_start(void)
{
    s_ctx = beacon_ctx_get();
    s_wifi_eg = xEventGroupCreate();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_ip_event, NULL));

    wifi_config_t wc = {
        .sta = {
            .ssid = CONFIG_BEACON_WIFI_SSID,
            .password = CONFIG_BEACON_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = { .capable = true, .required = false },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi sta started, ssid=%s", CONFIG_BEACON_WIFI_SSID);
}
