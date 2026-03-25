#include "car/car_control.h"

static int8_t clamp_100(int16_t x) {
    if (x > 100) return 100;
    if (x < -100) return -100;
    return x;
}

void car_control_init(car_control_t *car, const tb6612fng_t *driver) {
    car->driver = driver;
    tb6612fng_init(driver);
    tb6612fng_toggle_enable(driver, true);
    car_control_stop(car);
}

void car_control_set_tank(car_control_t *car, int8_t left_speed, int8_t right_speed) {
    tb6612fng_drive(car->driver, MOTOR_LEFT, left_speed);
    tb6612fng_drive(car->driver, MOTOR_RIGHT, right_speed);
}

void car_control_set_command(car_control_t *car, int8_t throttle, int8_t turn) {
    int16_t left  = throttle + turn;
    int16_t right = throttle - turn;
    car_control_set_tank(car, clamp_100(left), clamp_100(right));
}

void car_control_stop(car_control_t *car) {
    car_control_set_tank(car, 0, 0);
}