#include "pico/stdlib.h"
#include "wifi/network.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "audio/stream.h"
#include <stdint.h>
#include <string.h>

typedef struct UDP_SERVER_T_
{
    struct udp_pcb *pcb;
} UDP_SERVER_T;

static UDP_SERVER_T audio_udp_server;
static uint32_t packet_count = 0;
static uint32_t dropped_packet_count = 0;

static void log_audio_packet_stats(const char *data,
                                   u16_t length,
                                   const ip_addr_t *addr,
                                   u16_t port)
{
    int16_t min_sample = INT16_MAX;
    int16_t max_sample = INT16_MIN;
    uint32_t sum_abs = 0;
    uint32_t active_samples = 0;
    size_t sample_count = length / PCM_AUDIO_SAMPLE_SIZE;

    for (size_t i = 0; i < sample_count; i++)
    {
        int16_t sample;
        memcpy(&sample, &data[i * PCM_AUDIO_SAMPLE_SIZE], PCM_AUDIO_SAMPLE_SIZE);

        if (sample < min_sample)
        {
            min_sample = sample;
        }
        if (sample > max_sample)
        {
            max_sample = sample;
        }

        int32_t magnitude = sample < 0 ? -(int32_t)sample : (int32_t)sample;
        sum_abs += (uint32_t)magnitude;

        if (magnitude > 512)
        {
            active_samples++;
        }
    }

    uint32_t avg_abs = sample_count > 0 ? (sum_abs / (uint32_t)sample_count) : 0;

    printf("Audio UDP packets=%lu last_len=%u samples=%u min=%d max=%d avg_abs=%lu active=%lu dropped=%lu from %s:%u\n",
           (unsigned long)packet_count,
           length,
           (unsigned)sample_count,
           min_sample,
           max_sample,
           (unsigned long)avg_abs,
           (unsigned long)active_samples,
           (unsigned long)dropped_packet_count,
           ipaddr_ntoa(addr),
           port);
}

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
    copy_length -= (copy_length % PCM_AUDIO_SAMPLE_SIZE);

    if (copy_length == 0)
    {
        pbuf_free(p);
        return;
    }

    // copy over data and length
    pbuf_copy_partial(p, entry.data, copy_length, 0);
    entry.length = copy_length;

    // queue is full
    if (!queue_try_add(&pcm_audio_queue, &entry))
    {
        // try removing oldest
        pcm_entry_t dummy;
        queue_try_remove(&pcm_audio_queue, &dummy);
        dropped_packet_count++;

        // try adding again
        queue_try_add(&pcm_audio_queue, &entry);
    }

    packet_count++;
    if (packet_count <= 5 || (packet_count % 50) == 0)
    {
        log_audio_packet_stats(entry.data, copy_length, addr, port);
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
