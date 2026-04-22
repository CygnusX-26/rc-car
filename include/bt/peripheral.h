#ifndef BLUETOOTH_PERIPHERAL_H
#define BLUETOOTH_PERIPHERAL_H

#include <stdint.h>

// Advertising configuration
#define APP_AD_FLAGS 0x06
// in units of 0.625ms based on bluetooth specifications, so 500ms or twice a second
#define ADVERTISMENT_INTERVAL_0625MS 800
// 0 is ADV_IND which is connectable, scannnable, and unidirected
#define ADVERTISEMENT_TYPE 0
// 0 means public address
#define OWN_ADDRESS_TYPE 0
// 0x07 in binary is 111 which means all channels to advertise as only 3 channels
#define CHANNEL_MAP 0x07
#define FILTER_POLICY 0x00

#define LIGHT_ON_COMMAND 0X00
#define LIGHT_OFF_COMMAND 0x01

typedef void (*bluetooth_command_handler_t)(uint8_t speed, uint16_t direction);
void bluetooth_set_command_handler(bluetooth_command_handler_t handler);
void bluetooth_init(void);

#endif // BLUETOOTH_PERIPHERAL_H
