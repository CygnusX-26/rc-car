#include "pico/stdlib.h"
#include "wifi/network.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "audio/stream.h"

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

    pcm_entry_t entry;
    // clamp length to buffer size
    u16_t copy_length = MIN(p->tot_len, PCM_AUDIO_MAX_PACKET_SIZE);

    // copy over data and length
    pbuf_copy_partial(p, entry.data, copy_length, 0);
    entry.length = copy_length;

    // queue is full
    if (!queue_try_add(&pcm_audio_queue, &entry))
    {
        // try removing oldest
        pcm_entry_t dummy;
        queue_try_remove(&pcm_audio_queue, &dummy);

        // try adding again
        queue_try_add(&pcm_audio_queue, &entry);
    }

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
