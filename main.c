#include "btstack.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"

#include "bluetooth_peripheral.h"

#define LED_DELAY_MS 250
#define APP_AD_FLAGS 0x06

// Advertising configuration
// in units of 0.625ms based on bluetooth specifications, so 500ms or twice a second
#define ADVERTISMENT_INTERVAL_0625MS 800
// 0 is ADV_IND which is connectable, scannnable, and unidirected
#define ADVERTISEMENT_TYPE 0
// 0 means public address
#define OWN_ADDRESS_TYPE 0
// 0x07 in binary is 111 which means all channels to advertise as only 3 channels
#define CHANNEL_MAP 0x07
#define FILTER_POLICY 0x00

/*

Standalone example
https://github.com/raspberrypi/pico-examples/tree/master/pico_w/bt/standalone

*/

static uint8_t advertisement_data[] = {
    // Flags general discoverable
    0x02,
    BLUETOOTH_DATA_TYPE_FLAGS,
    APP_AD_FLAGS,

    // Name
    0x07,
    BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'R',
    'C',
    ' ',
    'C',
    'A',
    'R',

    // service uuid
    0x03,
    BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
    0xAA,
    0xAA,
};
static const uint8_t advertisement_data_length = sizeof(advertisement_data);

static btstack_packet_callback_registration_t hci_event_callback_registration;
// consumes the header file generated from GATT
extern uint8_t const profile_data[];

// app READ FROM -> car READ FROM HERE
static uint16_t att_read_callback(hci_con_handle_t connection_handle,
                                  uint16_t att_handle,
                                  uint16_t offset, uint8_t *buffer,
                                  uint16_t buffer_size)
{
    UNUSED(connection_handle);
    UNUSED(att_handle);
    UNUSED(offset);
    UNUSED(buffer);
    UNUSED(buffer_size);

    return 0;
}

// app WRITE TO -> car, WRITE TO HERE
static int att_write_callback(hci_con_handle_t connection_handle,
                              uint16_t att_handle,
                              uint16_t transaction_mode,
                              uint16_t offset,
                              uint8_t *buffer,
                              uint16_t buffer_size)
{
    UNUSED(connection_handle);
    UNUSED(att_handle);
    UNUSED(transaction_mode);
    UNUSED(offset);

    printf("Received %d bytes:\n", buffer_size);

    for (int i = 0; i < buffer_size; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");

    if (buffer_size == 2)
    {
        printf("Throttle: %d, Steering: %d\n", buffer[0], buffer[1]);
    }

    return 0;
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(size);
    UNUSED(channel);

    bd_addr_t local_mac_address;
    uint8_t event_type;

    if (packet_type != HCI_EVENT_PACKET)
    {
        return;
    }

    event_type = hci_event_packet_get_type(packet);
    switch (event_type)
    {

    // bluetooth changed state
    case BTSTACK_EVENT_STATE:
        // is the stack ready
        if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING)
        {
            // not ready
            return;
        }

        // MAC address
        gap_local_bd_addr(local_mac_address);
        printf("BTstack up and running on %s.\n", bd_addr_to_str(local_mac_address));

        // peer address but not sure if being used
        bd_addr_t null_addr;
        memset(null_addr, 0, 6);

        // advertise parameters
        gap_advertisements_set_params(ADVERTISMENT_INTERVAL_0625MS,
                                      ADVERTISMENT_INTERVAL_0625MS,
                                      ADVERTISEMENT_TYPE,
                                      OWN_ADDRESS_TYPE,
                                      null_addr,
                                      CHANNEL_MAP,
                                      FILTER_POLICY);

        // advertise data with limit of 31 bytes
        assert(advertisement_data_length <= 31);
        gap_advertisements_set_data(advertisement_data_length, (uint8_t *)advertisement_data);

        // start advertising
        gap_advertisements_enable(1);

        printf("Advertising started\n");
        break;

    // disconnected from host
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        printf("Disconnected\n");
        break;

    // we are ready to send
    case ATT_EVENT_CAN_SEND_NOW:
        // att_server_notify(con_handle, ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_TEMPERATURE_01_VALUE_HANDLE, (uint8_t *)&current_temp, sizeof(current_temp));
        break;

    default:
        break;
    }
}

void bluetooth_init(void)
{
    // bluetooth transport layer
    l2cap_init();
    // security manager if needed such as pairing or encryption
    sm_init();

    // GATT server
    att_server_init(profile_data, att_read_callback, att_write_callback);

    // inform about BTstack state
    // only start advertising if the stack is ready, in packet handler
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(packet_handler);

    printf("Starting BLE server...\n");
    // turn on bluetooth
    hci_power_control(HCI_POWER_ON);
}

int main()
{
    // enable IO for printing
    stdio_init_all();
    // turn on board
    hard_assert(cyw43_arch_init() == PICO_OK);

    // start bluetooth
    bluetooth_init();
    btstack_run_loop_execute();
}
