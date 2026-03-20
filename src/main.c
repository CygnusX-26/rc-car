
#include "pico/stdlib.h"
#include "motor/tb6612fng.h"
#include "pico/cyw43_arch.h"
#include "car/car_control.h"

#define DELAY_MS (2000)

static const tb6612fng_t driver = {
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

    car_control_t car;
    car_control_init(&car, &driver);

    while (true) {
        // forward slow
        car_control_set_command(&car, 30, 0);
        sleep_ms(DELAY_MS);

        // forward medium
        car_control_set_command(&car, 60, 0);
        sleep_ms(DELAY_MS);

        // forward fast
        car_control_set_command(&car, 100, 0);
        sleep_ms(DELAY_MS);

        // left and foward
        car_control_set_command(&car, 60, -25);
        sleep_ms(DELAY_MS);

        // right and forward
        car_control_set_command(&car, 60, 25);
        sleep_ms(DELAY_MS);

        // backward
        car_control_set_command(&car, -50, 0);
        sleep_ms(DELAY_MS);

        // spin left
        car_control_set_command(&car, 0, -60);
        sleep_ms(DELAY_MS);

        // spin right
        car_control_set_command(&car, 0, 60);
        sleep_ms(DELAY_MS);

        // stop
        car_control_stop(&car);
        sleep_ms(DELAY_MS);
    }
}
