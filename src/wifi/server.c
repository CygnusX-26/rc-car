#include "pico/stdlib.h"
#include "wifi/network.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "pico/cyw43_arch.h"

typedef struct UDP_SERVER_T_
{
    struct udp_pcb *pcb;
} UDP_SERVER_T;

static UDP_SERVER_T audio_udp_server;

static void udp_receive(void *arg,
                        struct udp_pcb *pcb,
                        struct pbuf *p,
                        const ip_addr_t *addr,
                        u16_t port)
{
    (void)arg;
    (void)pcb;

    // if nothing in buffer, ignore
    if (!p)
    {
        return;
    }

    printf("Got UDP packet from %s:%u, length=%d\n", ipaddr_ntoa(addr), port, p->tot_len);

    // clamp length to buffer size
    u16_t copy_length = MIN(p->tot_len, PCM_AUDIO_MAX_PACKET_SIZE);

    // clean up pbuffer
    pbuf_free(p);
}

void udp_server_open(void)
{
    printf("Starting UDP server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), UDP_PORT);

    audio_udp_server.pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!audio_udp_server.pcb)
    {
        printf("server: failed to create pcb\n");
        return;
    }

    err_t err = udp_bind(audio_udp_server.pcb, IP_ADDR_ANY, UDP_PORT);
    if (err != ERR_OK)
    {
        printf("server: udp bind failed: %d\n", err);
        return;
    }

    udp_recv(audio_udp_server.pcb, udp_receive, &audio_udp_server);

    printf("\nUDP server started\n\n");
}

void second_core_init()
{
    // wifi
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        printf("\nFailed to connect to wifi. Streaming audio disabled.\n\n");
        return;
    }
    else
    {
        printf("\nConnected. Streaming audio enabled.\n\n");
        // current core, start udp producer only if connected
        udp_server_open();
    }

    // infinite loop to allow wifi callbacks
    while (1)
    {
        async_context_poll(cyw43_arch_async_context());
        async_context_wait_for_work_until(cyw43_arch_async_context(), at_the_end_of_time);
    }

    cyw43_arch_deinit();
}
