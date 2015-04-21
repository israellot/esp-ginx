#ifndef __RELAY_H
#define __RELAY_H

#define RELAY_COUNT 2

typedef void (*relay_change_callback)(int relay,int state);  //callback function

void ICACHE_FLASH_ATTR relay_register_listener(relay_change_callback f);
void ICACHE_FLASH_ATTR relay_unregister_listener(relay_change_callback f);

int ICACHE_FLASH_ATTR relay_get_state(int relayNumber);
int ICACHE_FLASH_ATTR relay_set_state(int relayNumber,unsigned state);
int ICACHE_FLASH_ATTR relay_toggle_state(int relayNumber);
void ICACHE_FLASH_ATTR relay_init();
int ICACHE_FLASH_ATTR relay_count();
#endif