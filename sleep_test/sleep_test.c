/*
 * OpenSeizureDetector - ESP8266 Version.
 * Simple sleep test.
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
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
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG,"wifiOff(): sleep mode=%d",sdk_wifi_get_sleep_type());
  wifi_fpm_open();
  sdk_wifi_set_sleep_type(WIFI_SLEEP_MODEM);
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG,"wifiOff(): sleep mode=%d",sdk_wifi_get_sleep_type());
}

/**
 * Enable wifi 
 */
void wifiOn() {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"wifiOn(): sleep mode=%d",sdk_wifi_get_sleep_type());
  sdk_wifi_set_sleep_type(WIFI_SLEEP_NONE);
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG,"wifiOn(): sleep mode=%d",sdk_wifi_get_sleep_type());
}


void ToggleSleepTask(void *pvParam) {
  static int state = 0;
  printf("ToggleSleepTask() - running every second\n");
  while(1) {
    vTaskDelay(3000 / portTICK_PERIOD_MS);
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


/**
 * This is the freeRTOS equivalent of main()!!!
 */
void user_init(void)
{
  // we just do this so we can use the same serial monitor to look at the
  // boot messages as the debug ones - otherwise the boot messages are mangled..
  uart_set_baud(0, 74880);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());

  gpio_enable(SETUP_PIN,GPIO_INPUT);
  gpio_set_pullup(SETUP_PIN,true,false);
  if (gpio_read(SETUP_PIN)) {
    printf("Starting in Run Mode\n");
    xTaskCreate(ToggleSleepTask, "ToggleSleepTask",
		512, NULL, 2, NULL);
  } else {
    printf("Starting in Setup Mode\n");
    //xTaskCreate(&httpd_task, "HTTP Daemon", 256, NULL, 2, NULL);
  }
}
