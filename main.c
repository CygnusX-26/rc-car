#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"

#define DELAY_MS (2000)
#define STBY_PIN (16)
#define PWMA_PIN (17)
#define AIN1_PIN (18)
#define AIN2_PIN (19)

#define INIT_GPIO_OUT(pin) \
    do { \
        gpio_init(pin); \
        gpio_set_dir(pin, GPIO_OUT); \
    } while (0)

int pico_led_init(void)
{
    return cyw43_arch_init();
}

void pico_gpio_init(void) {
    INIT_GPIO_OUT(STBY_PIN);
    INIT_GPIO_OUT(PWMA_PIN);
    INIT_GPIO_OUT(AIN1_PIN);
    INIT_GPIO_OUT(AIN2_PIN);
}

// Just using this to sanity check the pico is working
void pico_set_led(bool led_on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
}

void motor_1_forward(void) {
    gpio_put(AIN1_PIN, 1);
    gpio_put(AIN2_PIN, 0);
}

void motor_1_reverse(void) {
    gpio_put(AIN1_PIN, 0);
    gpio_put(AIN2_PIN, 1);
}

void motor_1_brake(void) {
    gpio_put(AIN1_PIN, 1);
    gpio_put(AIN2_PIN, 1);
}

void motor_1_coast(void) {
    gpio_put(AIN1_PIN, 0);
    gpio_put(AIN2_PIN, 0);
}

int main() {
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    pico_set_led(true);
    pico_gpio_init();
    gpio_put(STBY_PIN, 1);
    gpio_put(PWMA_PIN, 1);
    while (true) {
        motor_1_forward();
        sleep_ms(DELAY_MS);

        motor_1_reverse();
        sleep_ms(DELAY_MS);

        motor_1_brake();
        sleep_ms(DELAY_MS);

        motor_1_coast();
        sleep_ms(DELAY_MS);
    }
}
