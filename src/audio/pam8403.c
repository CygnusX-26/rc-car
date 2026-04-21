#include "audio/pam8403.h"
#include "hardware/pwm.h"

// enables the PWM output for PAM8403 amplifier

void pam8403_init(pam8403_t *amp) {
    amp->pwm_slice = pwm_gpio_to_slice_num(amp->pwm_pin);
    amp->pwm_channel = pwm_gpio_to_channel(amp->pwm_pin);

    gpio_set_function(amp->pwm_pin, GPIO_FUNC_PWM);

    pwm_set_wrap(amp->pwm_slice, 255);
    pwm_set_clkdiv(amp->pwm_slice, 1.1f);
    pwm_set_chan_level(amp->pwm_slice, amp->pwm_channel, 0);
    pwm_set_enabled(amp->pwm_slice, true);
}

void pam8403_write(pam8403_t *amp, uint8_t data) {
    pwm_set_chan_level(amp->pwm_slice, amp->pwm_channel, data);
}

bool pam8403_audio_callback(struct repeating_timer *t) {
    pam8403_t *amp = (pam8403_t *)t->user_data;
    pam8403_write(amp, get_next_sample());
    return true;
}

// repeating_timer struct has user data build in
void pam8403_start(pam8403_t *amp, struct repeating_timer *t) {
    add_repeating_timer_us(-50, pam8403_audio_callback, amp, t);
}

uint8_t get_next_sample(void) {
    return 128;
}
