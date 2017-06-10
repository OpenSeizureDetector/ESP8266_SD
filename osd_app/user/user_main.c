/*
 * OpenSeizureDetector - ESP8266 Version.
 * Based on ESPRESSIF sample project (MIT Licensed)
 */

#include <esp_common.h>
#include <i2c.h>
#include "esp_common.h"
#include "osd_app.h"

#define DEV_ADDR 0x1D
#define REG_ADDR 0x32

#define SCL_PIN (4)
#define SDA_PIN (5)

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


void adxl_init() {
  uint8_t slave_addr = 0x20;
  uint8_t reg_addr = 0x1f;
  uint8_t reg_data;
  uint8_t data[] = {reg_addr, 0xc8};
  bool success;
  
  printf("adxl_init()\n");
  int response;
  i2c_init(SCL_PIN, SDA_PIN);
  // read device id.
  reg_addr = 0x00;
  slave_addr = DEV_ADDR;
  success = i2c_slave_read(slave_addr, reg_addr,&reg_data,1);
  printf("device: %x\n", reg_data);
}


LOCAL void ICACHE_FLASH_ATTR ReadAccelTask(void *pvParam) {
  int32 response;
  while(1) {
    vTaskDelay(1000/portTICK_RATE_MS);
    printf("Read Accel...\n");
    // FIXME - where does the response go????
    read_acc_reg(response, REG_ADDR);
    printf("Read Accel...response=%d\n",response);
  }
}




LOCAL void ICACHE_FLASH_ATTR LEDBlinkTask(void *pvParam) {
  while(1) {
    vTaskDelay(1000/portTICK_RATE_MS);
    printf("Blink...\n");
    if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT4)
      {
	// set gpio low
	gpio_output_set(0,BIT4,BIT4,0);
	gpio_output_set(BIT5,0,BIT5,0);
      }
    else
      {
	// set gpio high
	gpio_output_set(BIT4,0,BIT4,0);
	gpio_output_set(0,BIT5,BIT5,0);
      }
  }
}




/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
  printf("*****************************************\n");
  printf("* OpenSeizureDetector - ESP8266 Version *\n");
  printf("SDK version:%s\n", system_get_sdk_version());
  //printf("Graham's i2c.h - %d\n",GRAHAMS_VERSION);
  printf("*****************************************\n\n");


  adxl_init();
  
   // Config pin as GPIO12
  //PIN_FUNC_SELECT (PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);

  //xTaskCreate(LEDBlinkTask,(signed char *)"Blink",256,NULL,2,NULL);
  xTaskCreate(ReadAccelTask,(signed char *)"ReadAccel",256,NULL,2,NULL);


}

