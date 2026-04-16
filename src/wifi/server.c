#include "pico/stdlib.h"
#include "wifi/network.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"

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

    printf("Got UDP packet from %s:%u, length=%d\n",
           ipaddr_ntoa(addr),
           port,
           p->tot_len);

    // print raw bytes
    uint8_t buf[64];
    int len = pbuf_copy_partial(p, buf, sizeof(buf), 0);

    printf("Data (%d bytes): ", len);
    for (int i = 0; i < len; i++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\n");

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
