#include "beacon.h"

#include "esp_log.h"
#include "lwip/sockets.h"
#include <string.h>

static const char *TAG = "beacon.wol";

int beacon_wol_send(const uint8_t mac[6], const char *bcast_ip)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        return -1;
    }

    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    /* Magic Packet: 6 * 0xFF + 16 * mac */
    uint8_t pkt[6 + 16 * 6];
    memset(pkt, 0xFF, 6);
    for (int i = 0; i < 16; i++) memcpy(pkt + 6 + i * 6, mac, 6);

    struct sockaddr_in dst = {
        .sin_family = AF_INET,
        .sin_port = htons(9),
    };
    inet_aton(bcast_ip ? bcast_ip : "255.255.255.255", &dst.sin_addr);

    int n = sendto(sock, pkt, sizeof(pkt), 0,
                   (struct sockaddr *)&dst, sizeof(dst));
    close(sock);

    if (n < 0) {
        ESP_LOGE(TAG, "sendto failed");
        return -1;
    }
    ESP_LOGI(TAG, "WoL sent to %02X:%02X:%02X:%02X:%02X:%02X via %s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             bcast_ip ? bcast_ip : "255.255.255.255");
    return 0;
}
