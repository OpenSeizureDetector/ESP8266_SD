/*
 * OpenSeizureDetector - ESP8266 Version.
 * Simple sleep test.
 */
#include <stddef.h>
#include "espressif/c_types.h"
#include "lwipopts.h"
#include "lwip/ip_addr.h"
#include "espressif/esp_libc.h"
#include "espressif/esp_misc.h"
#include "espressif/esp_common.h"
#include "espressif/esp_wifi.h"
#include "espressif/esp_sta.h"
#include "espressif/esp_softap.h"

//#include "esp/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "queue.h"

#define ACC_BUF_LEN 50

#define DEBUG true

/* GLOBAL VARIABLES */
#define APP_LOG(lvl,fmt, ...) printf("%s: " fmt "\n", "osd_app", ## __VA_ARGS__)
#define SETUP_PIN (12)  // Wemos D1 Mini D6 = GPIO12


// defined in libmain.a
void wifi_fpm_open(void);


/**
 * disable wifi
 */
void wifiOff() {
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG,"wifiOff(): sleep mode=%d",wifi_fpm_get_sleep_type());
      os_printf("wifi_set_mode - requested mode=0 so entering MODEM_SLEEP\n");
      bool s = wifi_set_opmode(0);
      wifi_fpm_open();
      wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
      wifi_fpm_do_sleep(0xFFFFFFFF);
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG,"wifiOff(): sleep mode=%d",wifi_fpm_get_sleep_type());
}

/**
 * Enable wifi 
 */
void wifiOn() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"wifiOn(): sleep mode=%d",wifi_fpm_get_sleep_type());
  wifi_fpm_close();
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG,"wifiOn(): sleep mode=%d",wifi_fpm_get_sleep_type());
}


void ToggleSleepTask(void *pvParam) {
  static int state = 0;
  printf("ToggleSleepTask() - running every second\n");
  while(1) {
    vTaskDelay(3000 / portTICK_RATE_MS);
    printf("ToggleSleepTask() - heap=%d.\n", xPortGetFreeHeapSize());

    if (state) {
      wifiOn();
      state = 0;
    } else {
      wifiOff();
      state = 1;
    }
  }  
}




/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}




/**
 * This is the freeRTOS equivalent of main()!!!
 */
void user_init(void)
{
  // we just do this so we can use the same serial monitor to look at the
  // boot messages as the debug ones - otherwise the boot messages are mangled..
  //uart_set_baud(0, 74880);
  printf("SDK version:%s\n", system_get_sdk_version());

  //gpio_enable(SETUP_PIN,GPIO_INPUT);
  //gpio_set_pullup(SETUP_PIN,true,false);
  //if (gpio_read(SETUP_PIN)) {
    printf("Starting in Run Mode\n");
    xTaskCreate(ToggleSleepTask, "ToggleSleepTask",
		512, NULL, 2, NULL);
    //} else {
    //printf("Starting in Setup Mode\n");
    //xTaskCreate(&httpd_task, "HTTP Daemon", 256, NULL, 2, NULL);
    //}
}
