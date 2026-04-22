#ifndef WIFI_H
#define WIFI_H

#define WIFI_SSID "white"
#define WIFI_PASSWORD "whiteblueblack"

#define UDP_PORT 1111

extern volatile uint32_t audio_packet_count;

void udp_server_open(void);
void audio_packet_indicator_poll(void);

#endif // WIFI_H
