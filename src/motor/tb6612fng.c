#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "motor/tb6612fng.h"

#include <stdint.h>

#define TB6612_PWM_WRAP 255

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

static void init_pwm_pin(uint8_t pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);

    int slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();

    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_config_set_wrap(&config, TB6612_PWM_WRAP);

    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);
}

void tb6612fng_init(const tb6612fng_t *drv) {
    INIT_PIN(drv->ain1_pin);
    INIT_PIN(drv->ain2_pin);
    INIT_PIN(drv->bin1_pin);
    INIT_PIN(drv->bin2_pin);
    INIT_PIN(drv->stby_pin);

    init_pwm_pin(drv->pwma_pin);
    init_pwm_pin(drv->pwmb_pin);
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

void tb6612fng_drive(const tb6612fng_t *drv, motor_t motor, int8_t speed) {
    if (speed < -100) speed = 100;
    else if (speed > 100) speed = 100;

    if (speed > 0) {
        tb6612fng_set_action(drv, motor, MOTOR_ACTION_FORWARD);
        tb6612fng_set_pwm(drv, motor, (uint8_t)((speed * 255) / 100));
    } else if (speed < 0) {
        int8_t magnitude = -speed;
        tb6612fng_set_action(drv, motor, MOTOR_ACTION_BACKWARD);
        tb6612fng_set_pwm(drv, motor, (uint8_t)((magnitude * 255) / 100));
    } else {
        tb6612fng_set_action(drv, motor, MOTOR_ACTION_COAST);
        tb6612fng_set_pwm(drv, motor, 0);
    }
}