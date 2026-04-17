#include "audio/stream.h"

#include <stdio.h>

queue_t pcm_audio_queue;

void second_core_audio_init(void)
{
    pcm_entry_t entry;

    printf("Starting audio consumer on second core\n");

    while (1)
    {
        sleep_ms(PCM_AUDIO_DELAY_MS);

        // nothing in queue
        if (!queue_try_remove(&pcm_audio_queue, &entry))
        {
            continue;
        }

        // something in queue
        printf("Consuming audio packet of size %d\n", entry.length);
    }
}
