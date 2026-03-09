#include "pico/stdlib.h"

#include "pico/cyw43_arch.h"

#define LED_DELAY_MS 250

int pico_led_init(void)
{
    return cyw43_arch_init();
}

// Turn the led on or off
void pico_set_led(bool led_on)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
}

int main()
{
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    while (true)
    {
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
    }
}
