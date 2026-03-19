#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdint.h>

#define STBY_PIN_1 (16)

#define PWMA_PIN_1 (17)
#define AIN1_PIN_1 (18)
#define AIN2_PIN_1 (19)

#define PWMB_PIN_1 (20)
#define BIN1_PIN_1 (21)
#define BIN2_PIN_1 (22)

#define INIT_PIN(pin) { \
    gpio_init(pin); \
    gpio_set_dir(pin, GPIO_OUT); \
    gpio_put(pin, 0); \
}

typedef struct tb6612fng {
    uint8_t stby_pin;
    uint8_t pwma_pin;
    uint8_t pwmb_pin;
    uint8_t ain1_pin;
    uint8_t ain2_pin;
    uint8_t bin1_pin;
    uint8_t bin2_pin;
} tb6612fng_t;

typedef enum {
    MOTOR_A,
    MOTOR_B,
} motor_t;

typedef enum {
    MOTOR_ACTION_FORWARD,
    MOTOR_ACTION_BACKWARD,
    MOTOR_ACTION_BRAKE,
    MOTOR_ACTION_COAST,
} motor_action_t;

void tb6612fng_init(const tb6612fng_t*);
void tb6612fng_toggle_enable(const tb6612fng_t*, bool);
void tb6612fng_set_pwm(const tb6612fng_t*, motor_t, uint8_t);
void tb6612fng_set_action(const tb6612fng_t*, motor_t, motor_action_t);

#endif // MOTOR_CONTROLLER_H
