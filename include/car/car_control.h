#ifndef CAR_CONTROL_H
#define CAR_CONTROL_H

#include "motor/tb6612fng.h"

#include <stdint.h>

typedef struct {
    const tb6612fng_t *driver;
} car_control_t;

void car_control_init(car_control_t *car, const tb6612fng_t *driver);
void car_control_set_tank(car_control_t *car, int8_t left_speed, int8_t right_speed);
void car_control_set_command(car_control_t *car, int8_t throttle, int8_t turn);
void car_control_stop(car_control_t *car);

#endif // CAR_CONTROL_H