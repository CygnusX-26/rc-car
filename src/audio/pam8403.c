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
    .sample_rate = 16000,
};

static pam8403_t *global_amp;
static struct repeating_timer sample_timer;
static bool pwm_output_enabled = false;
static uint32_t test_tone_phase = 0;
static uint32_t test_tone_samples_remaining = 0;

// Forward declaration
static bool get_next_sample(uint16_t *sample_level);

static void enable_pwm_output(void)
{
    if (pwm_output_enabled)
    {
        return;
    }

    gpio_set_function(global_amp->pwm_pin, GPIO_FUNC_PWM);
    pwm_set_chan_level(global_amp->pwm_slice, global_amp->pwm_channel, PWM_SILENCE);
    pwm_set_enabled(global_amp->pwm_slice, true);
    pwm_output_enabled = true;
}

static void disable_pwm_output(void)
{
    if (pwm_output_enabled)
    {
        pwm_set_enabled(global_amp->pwm_slice, false);
    }
    gpio_set_function(global_amp->pwm_pin, GPIO_FUNC_SIO);
    gpio_set_dir(global_amp->pwm_pin, GPIO_OUT);
    gpio_put(global_amp->pwm_pin, 0);
    pwm_output_enabled = false;
}

static bool on_sample_timer(__unused struct repeating_timer *t)
{
    uint16_t sample_level;
    if (!get_next_sample(&sample_level))
    {
        disable_pwm_output();
        return true;
    }

    enable_pwm_output();
    pwm_set_chan_level(global_amp->pwm_slice, global_amp->pwm_channel, sample_level);
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
    disable_pwm_output();

    int64_t sample_period_us = -(1000000ll / (int64_t)amp->sample_rate);
    add_repeating_timer_us(sample_period_us, on_sample_timer, NULL, &sample_timer);
}

static uint16_t pcm16_to_pwm_level(int16_t pcm_sample)
{
    // Shift signed 16-bit PCM into the configured unsigned PWM range.
    return (uint16_t)(((uint16_t)(pcm_sample + 32768)) >> 6);
}

static bool get_next_sample(uint16_t *sample_level)
{
    if (test_tone_samples_remaining > 0)
    {
        // Short square-wave chirp at boot to verify the amplifier path without Wi-Fi.
        test_tone_phase += 880u;
        *sample_level = (test_tone_phase >= amplifier.sample_rate / 2u)
            ? pcm16_to_pwm_level(18000)
            : pcm16_to_pwm_level(-18000);
        test_tone_phase %= amplifier.sample_rate;
        test_tone_samples_remaining--;
        return true;
    }

    if (total_samples == 0)
    {
        pcm_entry_t entry;
        if (!queue_try_remove(&pcm_audio_queue, &entry))
        {
            return false;
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

    *sample_level = pcm16_to_pwm_level(pcm_sample);
    return true;
}

void second_core_audio_init(void)
{
    printf("Starting audio consumer on second core\n");
    test_tone_samples_remaining = amplifier.sample_rate / 8u;
    pam8403_init(&amplifier);
}
