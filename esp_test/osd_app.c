/*
 * OpenSeizureDetector - ESP8266 Version.
 *  ADXL code based on https://gist.github.com/shunkino/6b1734ee892fe2efbd12
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "espressif/esp_common.h"
#include "uart.h"
//#include "i2c/i2c.h"
//#include "queue.h"
#include "osd_app.h"

//#include "adxl345/adxl345.h"

#define ACC_BUF_LEN 50

/* GLOBAL VARIABLES */
Sd_Settings sdS;        // SD setings structure.
Wifi_Settings wifiS;    // Wifi Settings structure.

// Settings (obtained from default constants or persistent storage)

static xQueueHandle tsqueue;
static xQueueHandle commsQueue;

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




void LEDBlinkTask(void *pvParam) {
  printf("LEDBlinkTask()");
  //gpio_enable(2,GPIO_OUTPUT);
  while(1) {
    vTaskDelay(500 / portTICK_RATE_MS);
    printf("Blink...\n");
    if (true) //(gpio_read(2))
      {
	// set gpio low
	//gpio_write(2,false);
      }
    else
      {
	// set gpio high
	//gpio_write(2,true);
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
  //uart_set_baud(0, 74880);
  UART_SetBaudrate(0,74880);
  printf("SDK version:%s\n", system_get_sdk_version());
  
  xTaskCreate(LEDBlinkTask,"Blink",256,NULL,2,NULL);
  //xTaskCreate(i2cScanTask,"i2cScan",256,NULL,2,NULL);
}
