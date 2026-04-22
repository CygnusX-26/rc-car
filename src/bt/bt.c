#include "btstack.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "bt/peripheral.h"

#include "bp.h"

/*

Standalone example
https://github.com/raspberrypi/pico-examples/tree/master/pico_w/bt/standalone

*/

static uint8_t advertisement_data[] = {
    // flags general discoverable
    0x02,
    BLUETOOTH_DATA_TYPE_FLAGS,
    APP_AD_FLAGS,

    // name
    0x04,
    BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
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

static bluetooth_command_handler_t g_command_handler = NULL;
void bluetooth_set_command_handler(bluetooth_command_handler_t handler)
{
    g_command_handler = handler;
}

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
    UNUSED(transaction_mode);
    UNUSED(offset);

    // printf("Received %d bytes:\n", buffer_size);

    // for (int i = 0; i < buffer_size; i++)
    // {
    //     printf("%02x ", buffer[i]);
    // }
    // printf("\n");

    // driving
    if (att_handle == ATT_CHARACTERISTIC_BBBB_01_VALUE_HANDLE && buffer_size == 3)
    {
        uint8_t magnitude = buffer[0];
        // assume little endian sent over
        uint16_t angle = (uint16_t)buffer[1] | ((uint16_t)buffer[2] << 8);

        if (g_command_handler != NULL)
        {
            g_command_handler(magnitude, angle);
        }
    }

    // voice command
    if (att_handle == ATT_CHARACTERISTIC_BBBC_01_VALUE_HANDLE && buffer_size == 1)
    {
        uint8_t voice_command = buffer[0];

        if (voice_command == LIGHT_ON_COMMAND)
        {
        }
        else if (voice_command == LIGHT_OFF_COMMAND)
        {
        }
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

    btstack_run_loop_execute();
}
