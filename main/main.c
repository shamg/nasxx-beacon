#include "beacon.h"

#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static const char *TAG = "beacon.main";

void app_main(void)
{
    ESP_LOGI(TAG, "nasxx-beacon booting");

    /* NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    beacon_ctx_t *ctx = beacon_ctx_get();
    if (beacon_pairing_load(ctx) != 0) {
        ESP_LOGW(TAG, "no pairing data yet; running unpaired (TODO: provisioning flow)");
        /* 临时占位：默认 broker 地址，便于开发联调 */
        snprintf(ctx->mqtt_host, sizeof(ctx->mqtt_host), CONFIG_BEACON_DEFAULT_BROKER_HOST);
        ctx->mqtt_port = CONFIG_BEACON_DEFAULT_BROKER_PORT;
        snprintf(ctx->mqtt_client_id, sizeof(ctx->mqtt_client_id), "beacon-%llX",
                 (unsigned long long)esp_efuse_mac_get_default());
    }

    beacon_wifi_start();
    /* Wi-Fi 连上后由 wifi event handler 触发 beacon_mqtt_start() */
    (void)ctx;
}
