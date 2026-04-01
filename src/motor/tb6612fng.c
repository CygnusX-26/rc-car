#include "motor/tb6612fng.h"
#include "hardware/pwm.h"
#include <stdint.h>

static void handle_motor_action(uint8_t in1_pin, uint8_t in2_pin, motor_action_t action) {
    switch (action) {
        case MOTOR_ACTION_FORWARD:
            gpio_put(in1_pin, 1);
            gpio_put(in2_pin, 0);
            break;
        case MOTOR_ACTION_BACKWARD:
            gpio_put(in1_pin, 0);
            gpio_put(in2_pin, 1);
            break;
        case MOTOR_ACTION_BRAKE:
            gpio_put(in1_pin, 1);
            gpio_put(in2_pin, 1);
            break;
        case MOTOR_ACTION_COAST:
            gpio_put(in1_pin, 0);
            gpio_put(in2_pin, 0);
            break;
    }
}

void tb6612fng_init(const tb6612fng_t *drv) {
    INIT_PIN(drv->ain1_pin);
    INIT_PIN(drv->ain2_pin);
    INIT_PIN(drv->bin1_pin);
    INIT_PIN(drv->bin2_pin);
    INIT_PIN(drv->stby_pin);

    gpio_set_function(drv->pwma_pin, GPIO_FUNC_PWM);
    gpio_set_function(drv->pwmb_pin, GPIO_FUNC_PWM);

    uint slice_a = pwm_gpio_to_slice_num(drv->pwma_pin);
    uint slice_b = pwm_gpio_to_slice_num(drv->pwmb_pin);

    pwm_set_wrap(slice_a, 255);
    pwm_set_wrap(slice_b, 255);

    pwm_set_gpio_level(drv->pwma_pin, 0);
    pwm_set_gpio_level(drv->pwmb_pin, 0);

    pwm_set_enabled(slice_a, true);
    pwm_set_enabled(slice_b, true);
}

void tb6612fng_toggle_enable(const tb6612fng_t *drv, bool enabled) {
    gpio_put(drv->stby_pin, enabled ? 1 : 0);
}

void tb6612fng_set_pwm(const tb6612fng_t *drv, motor_t motor, uint8_t pwm) {
    switch (motor) {
        case MOTOR_LEFT:
            pwm_set_gpio_level(drv->pwma_pin, pwm);
            break;
        case MOTOR_RIGHT:
            pwm_set_gpio_level(drv->pwmb_pin, pwm);
            break;
    }
}

void tb6612fng_set_action(const tb6612fng_t *drv, motor_t motor, motor_action_t action) {
    switch (motor) {
        case MOTOR_LEFT:
            handle_motor_action(drv->ain1_pin, drv->ain2_pin, action);
            break;
        case MOTOR_RIGHT:
            handle_motor_action(drv->bin1_pin, drv->bin2_pin, action);
            break;
    }
}
