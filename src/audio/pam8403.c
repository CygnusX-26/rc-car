#include "audio/pam8403.h"
#include "audio/stream.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Carrier = 125MHz / 3125 = 40kHz  (well above 4kHz audio bandwidth)
// Sample rate = 40kHz / 5 = 8000Hz exactly
#define PWM_WRAP          3124
#define WRAPS_PER_SAMPLE  5
#define PWM_SILENCE       (PWM_WRAP / 2)  // mid-rail = silence

queue_t pcm_audio_queue;
char    pcm_audio_buffer[PCM_AUDIO_MAX_PACKET_SIZE];

static volatile int      total_samples = 0;
static volatile int      sample_index  = 0;
static volatile uint32_t wrap_counter  = 0;

static pam8403_t amplifier = {
    .pwm_pin     = 0,
    .sample_rate = 8000,
};

static pam8403_t *global_amp;

static uint16_t get_next_sample(void);

static void on_pwm_wrap(void)
{
    pwm_clear_irq(global_amp->pwm_slice);

    if (++wrap_counter >= WRAPS_PER_SAMPLE)
    {
        wrap_counter = 0;
        pwm_set_chan_level(
            global_amp->pwm_slice,
            global_amp->pwm_channel,
            get_next_sample()
        );
    }
}

void pam8403_init(pam8403_t *amp)
{
    global_amp = amp;

    gpio_set_function(amp->pwm_pin, GPIO_FUNC_PWM);
    amp->pwm_slice   = pwm_gpio_to_slice_num(amp->pwm_pin);
    amp->pwm_channel = pwm_gpio_to_channel(amp->pwm_pin);

    // clkdiv=1 (integer, exact), wrap=3124
    // → carrier = 125MHz / 3125 = 40kHz
    // → 3125 PWM levels → slightly better than 8-bit resolution
    pwm_set_wrap(amp->pwm_slice, PWM_WRAP);
    pwm_set_clkdiv(amp->pwm_slice, 1.0f);

    pwm_set_chan_level(amp->pwm_slice, amp->pwm_channel, PWM_SILENCE);

    pwm_clear_irq(amp->pwm_slice);
    pwm_set_irq_enabled(amp->pwm_slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_set_enabled(amp->pwm_slice, true);
}


static uint16_t pcm16_to_pwm(int16_t pcm_sample)
{
    return (uint16_t)(((uint32_t)((int32_t)pcm_sample + 32768) * 3125) >> 16);
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

    return pcm16_to_pwm(pcm_sample);
}

void second_core_audio_init(void)
{
    printf("Starting audio consumer on second core\n");
    pam8403_init(&amplifier);
}
