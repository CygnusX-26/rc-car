
#include "pico/stdlib.h"
#include "motor/tb6612fng.h"
#include "pico/cyw43_arch.h"

#define DELAY_MS (2000)

static const tb6612fng_t left_driver = {
    .stby_pin = 16,
    .pwma_pin = 17,
    .ain1_pin = 18,
    .ain2_pin = 19,
    .pwmb_pin = 20,
    .bin1_pin = 21,
    .bin2_pin = 22,
};

int main() {
    // Init cyw43 board
    hard_assert(cyw43_arch_init() == PICO_OK);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    tb6612fng_init(&left_driver);
    tb6612fng_toggle_enable(&left_driver, true);
    tb6612fng_set_pwm(&left_driver, MOTOR_A, 0); // currently just enables the pin fix for speed later.

    while (true) {
        tb6612fng_set_action(&left_driver, MOTOR_A, MOTOR_ACTION_FORWARD);
        sleep_ms(DELAY_MS);

        tb6612fng_set_action(&left_driver, MOTOR_A, MOTOR_ACTION_BACKWARD);
        sleep_ms(DELAY_MS);

        tb6612fng_set_action(&left_driver, MOTOR_A, MOTOR_ACTION_BRAKE);
        sleep_ms(DELAY_MS);

        tb6612fng_set_action(&left_driver, MOTOR_A, MOTOR_ACTION_COAST);
        sleep_ms(DELAY_MS);
    }
}
