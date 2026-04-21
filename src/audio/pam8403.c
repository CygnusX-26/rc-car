#include "audio/pam8403.h"
#include "audio/stream.h"
#include "hardware/pwm.h"

#include <stdio.h>

queue_t pcm_audio_queue;
char pcm_audio_buffer[PCM_AUDIO_MAX_PACKET_SIZE];
int total_samples = 0, sample_index = 0;

static pam8403_t amplifier = {
    .pwm_pin = 0,
    .sample_rate = 8000,
};

static pam8403_t *const amplifiers[] = {
    &amplifier,
};

// enables the PWM output for PAM8403 amplifier

void pam8403_init(pam8403_t *amp)
{
    amp->pwm_slice = pwm_gpio_to_slice_num(amp->pwm_pin);
    amp->pwm_channel = pwm_gpio_to_channel(amp->pwm_pin);

    gpio_set_function(amp->pwm_pin, GPIO_FUNC_PWM);

    pwm_set_wrap(amp->pwm_slice, 255);
    pwm_set_clkdiv(amp->pwm_slice, 1.1f);
    pwm_set_chan_level(amp->pwm_slice, amp->pwm_channel, 0);
    pwm_set_enabled(amp->pwm_slice, true);
}

void pam8403_write(pam8403_t *amp, uint8_t data)
{
    pwm_set_chan_level(amp->pwm_slice, amp->pwm_channel, data);
}

bool pam8403_audio_callback(struct repeating_timer *t)
{
    pam8403_t *amp = (pam8403_t *)t->user_data;
    pam8403_write(amp, get_next_sample());
    return true;
}

// repeating_timer struct has user data build in
void pam8403_start(pam8403_t *amp, struct repeating_timer *t)
{
    add_repeating_timer_us(-62, pam8403_audio_callback, amp, t);
}

uint8_t pcm16_to_pwm8(int16_t pcm_sample)
{
    return (uint8_t)((pcm_sample + 32768) >> 8);
}

uint8_t get_next_sample(void)
{
    pcm_entry_t entry;
    int16_t pcm_sample;

    // if theres no samples in the buffer, try to get a sample from the queue
    if (total_samples == 0)
    {
        // if no samples in the queue, give silence
        if (!queue_try_remove(&pcm_audio_queue, &entry))
        {
            return PWM_SILENCE;
        }

        // we got a sample, copy it into the buffer
        memcpy(pcm_audio_buffer, entry.data, entry.length);

        // mark how many samples and reset the index
        total_samples = entry.length / PCM_AUDIO_SAMPLE_SIZE;
        sample_index = 0;
    }

    memcpy(&pcm_sample, &pcm_audio_buffer[sample_index * PCM_AUDIO_SAMPLE_SIZE], PCM_AUDIO_SAMPLE_SIZE);
    sample_index += 1;

    // we reached the end of the buffer, so reset the sample counter
    if (sample_index >= total_samples)
    {
        sample_index = 0;
        total_samples = 0;
    }

    // convert pcm sample into pwm
    return pcm16_to_pwm8(pcm_sample);
}

void second_core_audio_init(void)
{
    struct repeating_timer t;

    printf("Starting audio consumer on second core\n");

    pam8403_init(&amplifier);
    pam8403_start(&amplifier, &t);
}
