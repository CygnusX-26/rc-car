#include "bt/peripheral.h"
#include "pico/stdlib.h"
#include "motor/tb6612fng.h"
#include "audio/pam8403.h"
#include "pico/cyw43_arch.h"
#include "wifi/network.h"
#include "audio/stream.h"
#include "pico/multicore.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define PAM_TEST

#define DELAY_MS (2000)

static const tb6612fng_t front_driver = {
    .stby_pin = 16,
    .pwma_pin = 17,
    .ain1_pin = 18,
    .ain2_pin = 19,
    .pwmb_pin = 20,
    .bin1_pin = 21,
    .bin2_pin = 22,
};

static const tb6612fng_t back_driver = {
    .stby_pin = 15,
    .pwma_pin = 14,
    .ain1_pin = 13,
    .ain2_pin = 12,
    .pwmb_pin = 11,
    .bin1_pin = 10,
    .bin2_pin = 9,
};

static pam8403_t amplifier = {
    .pwm_pin = 0,
    .sample_rate = 8000,
};

static pam8403_t *const amplifiers[] = {
    &amplifier,
};

static const tb6612fng_t *const drivers[] = {
    &front_driver,
    &back_driver,
};

static bool motor_is_inverted(motor_t motor)
{
    return motor == MOTOR_RIGHT;
}

static uint8_t speed_to_pwm(uint8_t speed)
{
    // keep 0 speed as stopped
    if (speed == 0)
    {
        return 0;
    }

    // map 1-255 to 196-255 because it takes a certain amount for the wheels to move
    return (uint8_t)(196 + (((uint16_t)(speed - 1) * 59) / 254));
}

static uint16_t quantize_direction(uint16_t direction)
{
    uint16_t normalized = direction % 360;
    uint16_t sector = ((normalized + 22) / 45) % 8;
    return sector * 45;
}

static void stop_motor(motor_t motor)
{
    for (size_t i = 0; i < sizeof(drivers) / sizeof(drivers[0]); i++)
    {
        tb6612fng_set_pwm(drivers[i], motor, 0);
        tb6612fng_set_action(drivers[i], motor, MOTOR_ACTION_COAST);
    }
}

static void set_motor_from_command(motor_t motor, float command, uint8_t max_pwm)
{
    if (command > 1.0)
    {
        command = 1.0;
    }
    else if (command < -1.0)
    {
        command = -1.0;
    }

    float magnitude = fabsf(command);

    if (magnitude < 0.01 || max_pwm == 0)
    {
        stop_motor(motor);
        return;
    }

    if (motor_is_inverted(motor))
    {
        command = -command;
    }

    motor_action_t action = command >= 0.0 ? MOTOR_ACTION_FORWARD : MOTOR_ACTION_BACKWARD;

    uint8_t pwm = 128;
    if (max_pwm > 128)
    {
        pwm = (uint8_t)lroundf(128.0f + magnitude * (float)(max_pwm - 128));
    }
    if (pwm > max_pwm)
    {
        pwm = max_pwm;
    }

    for (size_t i = 0; i < sizeof(drivers) / sizeof(drivers[0]); i++)
    {
        tb6612fng_set_action(drivers[i], motor, action);
        tb6612fng_set_pwm(drivers[i], motor, pwm);
    }
}

static void handle_bluetooth_command(uint8_t speed, uint16_t direction)
{
    if (speed == 0)
    {
        stop_motor(MOTOR_LEFT);
        stop_motor(MOTOR_RIGHT);
        return;
    }

    uint8_t pwm = speed_to_pwm(speed);
    uint16_t snapped_direction = quantize_direction(direction);

    // Interpret the incoming 0-359 heading as one of 8 directions.
    const float PI = 3.141592;
    float radians = ((float)snapped_direction) * PI / 180.0;
    float throttle = cosf(radians);
    float steering = sinf(radians);

    float left = throttle + steering;
    float right = throttle - steering;

    float max = fmaxf(fabsf(left), fabsf(right));
    if (max > 1.0)
    {
        left /= max;
        right /= max;
    }

    // Keep steering in the same travel direction as throttle instead of point turning.
    if (throttle > 0.0f)
    {
        if (left < 0.0f)
        {
            left = 0.0f;
        }
        if (right < 0.0f)
        {
            right = 0.0f;
        }
    }
    else if (throttle < 0.0f)
    {
        if (left > 0.0f)
        {
            left = 0.0f;
        }
        if (right > 0.0f)
        {
            right = 0.0f;
        }
    }

    set_motor_from_command(MOTOR_LEFT, left, pwm);
    set_motor_from_command(MOTOR_RIGHT, right, pwm);
}

int main()
{
    // enable IO for printing
    stdio_init_all();
    // delay before any printing to get setup
    sleep_ms(5000);

    // Init cyw43 board
    hard_assert(cyw43_arch_init() == PICO_OK);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);

    #ifdef PAM_TEST
    pam8403_init(&amplifier);

    while (1) {
        // for (int i = 0; i < 256; i++) {
        //     pam8403_write(&amplifier, i);
        //     sleep_us(50);
        // }
        pam8403_write(&amplifier, 50);
        sleep_us(DELAY_MS);
    }
    #endif

    // prepare queue for multicore consumer and udp producer
    // queue_init(&pcm_audio_queue, sizeof(pcm_entry_t), PCM_AUDIO_QUEUE_SIZE);

    // // other core, audio consumer
    // multicore_launch_core1(second_core_audio_init);

    // // wifi
    // cyw43_arch_enable_sta_mode();
    // if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000))
    // {
    //     printf("\nFailed to connect to wifi. Speech disabled.\n\n");
    // }
    // else
    // {
    //     printf("\nConnected. Speech enabled.\n\n");
    //     // current core, start udp producer only if connected
    //     udp_server_open();
    // }

    // // motors
    // for (size_t i = 0; i < sizeof(drivers) / sizeof(drivers[0]); i++)
    // {
    //     tb6612fng_init(drivers[i]);
    //     tb6612fng_toggle_enable(drivers[i], true);
    // }

    // stop_motor(MOTOR_LEFT);
    // stop_motor(MOTOR_RIGHT);

    // bluetooth_set_command_handler(handle_bluetooth_command);
    // // start bluetooth
    // bluetooth_init();

    // // infinite loop to allow wifi + bluetoooth callbacks
    // while (1)
    // {
    //     async_context_poll(cyw43_arch_async_context());
    //     async_context_wait_for_work_until(cyw43_arch_async_context(), at_the_end_of_time);
    // }

    cyw43_arch_deinit();
}
