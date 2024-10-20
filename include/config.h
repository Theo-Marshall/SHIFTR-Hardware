#ifndef CONFIG_H
#define CONFIG_H

#define DEVICE_NAME_PREFIX "SHIFTR"

#define WEB_SERVER_PORT 80

// -- Configuration specific key. The value should be modified if config structure was changed.
#define WIFI_CONFIG_VERSION "init"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define WIFI_CONFIG_PIN IO12

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define WIFI_STATUS_PIN IO14

#define DIRCON_TCP_PORT 8080
#define DIRCON_MAX_CLIENTS 3

#endif

