#ifndef STREAM_H
#define STREAM_H

#include "pico/util/queue.h"
#include "lwip/arch.h"
#include <stdint.h>

#define PCM_AUDIO_MAX_PACKET_SIZE 1024
#define PCM_AUDIO_QUEUE_SIZE 256
#define PCM_AUDIO_DELAY_MS 20

typedef struct
{
    char data[PCM_AUDIO_MAX_PACKET_SIZE];
    u16_t length;
} pcm_entry_t;

extern queue_t pcm_audio_queue;

void second_core_audio_init(void);

#endif // STREAM_H
