#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__


#define INTERFACE_DOMAIN "smart.relay.com"


#define FLASH_512K
// #define FLASH_1M
// #define FLASH_2M
// #define FLASH_4M
//#define FLASH_AUTOSIZE
#define DEVELOP_VERSION
//#define FULL_VERSION_FOR_USER

#define USE_OPTIMIZE_PRINTF

#ifdef DEVELOP_VERSION
#define NODE_DEBUG
#endif	/* DEVELOP_VERSION */

//#define NODE_ERROR

#ifdef NODE_DEBUG
#define NODE_DBG(...) os_printf("\r\n");os_printf( __VA_ARGS__ )
#else
#define NODE_DBG
#endif	/* NODE_DEBUG */

#ifdef NODE_ERROR
#define NODE_ERR c_printf
#else
#define NODE_ERR
#endif	/* NODE_ERROR */

#define ICACHE_STORE_TYPEDEF_ATTR __attribute__((aligned(4),packed))
#define ICACHE_STORE_ATTR __attribute__((aligned(4)))
#define ICACHE_RAM_ATTR __attribute__((section(".iram0.text")))
// #define ICACHE_RODATA_ATTR __attribute__((section(".rodata2.text")))


#define GPIO_INTERRUPT_ENABLE

//#define BUILD_WOFS		1
#define BUILD_SPIFFS	1

#define PRINTF_LONG_SUPPORT



#endif	/* __USER_CONFIG_H__ */
