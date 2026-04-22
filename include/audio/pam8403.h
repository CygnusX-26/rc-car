#ifndef AMPLIFIER_H
#define AMPLIFIER_H

#include "pico/stdlib.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define PWM_WRAP 1023u
#define PWM_SILENCE ((PWM_WRAP + 1u) / 2u)

typedef struct pam8403
{
    uint8_t pwm_pin; // GPIO0
    uint16_t pwm_slice;
    uint8_t pwm_channel;
    uint32_t sample_rate;
} pam8403_t;

#define INIT_PIN(pin)                \
    {                                \
        gpio_init(pin);              \
        gpio_set_dir(pin, GPIO_OUT); \
        gpio_put(pin, 0);            \
    }

void pam8403_init(pam8403_t *amp);
void second_core_audio_init(void);
#endif
