#include "audio/pam8403.h"
#include "audio/stream.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>
#include <string.h>

queue_t pcm_audio_queue;
char pcm_audio_buffer[PCM_AUDIO_MAX_PACKET_SIZE];

static volatile int total_samples = 0;
static volatile int sample_index  = 0;

static pam8403_t amplifier = {
    .pwm_pin     = 0,
    .sample_rate = 8000,
};

static pam8403_t *global_amp;
static struct repeating_timer sample_timer;

// Forward declaration
static uint16_t get_next_sample(void);

static bool on_sample_timer(__unused struct repeating_timer *t)
{
    pwm_set_chan_level(
        global_amp->pwm_slice,
        global_amp->pwm_channel,
        get_next_sample()
    );
    return true;
}

void pam8403_init(pam8403_t *amp)
{
    global_amp = amp;

    gpio_set_function(amp->pwm_pin, GPIO_FUNC_PWM);
    amp->pwm_slice   = pwm_gpio_to_slice_num(amp->pwm_pin);
    amp->pwm_channel = pwm_gpio_to_channel(amp->pwm_pin);

    // Keep the PWM carrier ultrasonic and update the duty cycle separately
    // at the audio sample rate. With wrap=1023 and clkdiv=1, carrier is
    // ~122 kHz on RP2040 and ~146 kHz on RP2350.
    pwm_set_wrap(amp->pwm_slice, PWM_WRAP);
    pwm_set_clkdiv(amp->pwm_slice, 1.0f);

    pwm_set_chan_level(amp->pwm_slice, amp->pwm_channel, PWM_SILENCE);
    pwm_set_enabled(amp->pwm_slice, true);

    int64_t sample_period_us = -(1000000ll / (int64_t)amp->sample_rate);
    add_repeating_timer_us(sample_period_us, on_sample_timer, NULL, &sample_timer);
}

static uint16_t pcm16_to_pwm_level(int16_t pcm_sample)
{
    // Shift signed 16-bit PCM into the configured unsigned PWM range.
    return (uint16_t)(((uint16_t)(pcm_sample + 32768)) >> 6);
}

static uint16_t get_next_sample(void)
{
    if (total_samples == 0)
    {
        pcm_entry_t entry;
        if (!queue_try_remove(&pcm_audio_queue, &entry))
        {
            return PWM_SILENCE;
        }
        memcpy(pcm_audio_buffer, entry.data, entry.length);
        total_samples = entry.length / PCM_AUDIO_SAMPLE_SIZE;
        sample_index  = 0;
    }

    int16_t pcm_sample;
    memcpy(&pcm_sample,
           &pcm_audio_buffer[sample_index * PCM_AUDIO_SAMPLE_SIZE],
           PCM_AUDIO_SAMPLE_SIZE);

    sample_index++;
    if (sample_index >= total_samples)
    {
        sample_index  = 0;
        total_samples = 0;
    }

    return pcm16_to_pwm_level(pcm_sample);
}

void second_core_audio_init(void)
{
    printf("Starting audio consumer on second core\n");
    pam8403_init(&amplifier);
}
