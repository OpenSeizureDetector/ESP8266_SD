/*
 * OpenSeizureDetector - ESP8266 Version.
 *  ADXL code based on https://gist.github.com/shunkino/6b1734ee892fe2efbd12
 *  GPIO code based on https://github.com/espressif/esp8266-rtos-sample-code/tree/master/02Peripheral/GPIOtest
 */

#include "esp_common.h"
#include "gpio.h"
#include "i2c.h"
#include "adxl345.h"

// Define which GPIO pins are used to connect to the ADXL345 accelerometer.
#define SCL_PIN (5)  // Wemos D1 Mini D1 = GPIO5
#define SDA_PIN (4)  // Wemos D1 Mini D2 = GPIO4
#define INTR_PIN (13) // Wemos D1 Mini D7 = GPIO13
#define SETUP_PIN (12)  // Wemos D1 Mini D6 = GPIO12

//GPIO OUTPUT SETTINGS
#define  LED_IO_NUM  2
#define  LED_IO_FUNC FUNC_GPIO2
#define  LED_IO_PIN  GPIO_Pin_2

//GPIO INPUT SETTINGS
#define  BUTTON_IO_NUM   12
#define  BUTTON_IO_FUNC  FUNC_GPIO12
#define  BUTTON_IO_PIN   GPIO_Pin_12




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

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/**********************************SAMPLE CODE*****************************/
#define ETS_GPIO_INTR_ENABLE()  _xt_isr_unmask(1 << ETS_GPIO_INUM)  //ENABLE INTERRUPTS
#define ETS_GPIO_INTR_DISABLE() _xt_isr_mask(1 << ETS_GPIO_INUM)    //DISABLE INTERRUPTS


void io_intr_handler(void)
{
    uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);          //READ STATUS OF INTERRUPT
    static uint8 val = 0;
    if (status & BUTTON_IO_PIN) {
        if (val == 0) {
            GPIO_OUTPUT_SET(LED_IO_NUM, 1);
            val = 1;
        } else {
            GPIO_OUTPUT_SET(LED_IO_NUM, 0);
            val = 0;
        }
    }
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);       //CLEAR THE STATUS IN THE W1 INTERRUPT REGISTER
}

void i2cScanTask(void *pvParameters) {
  uint8_t i;
  printf("i2cScanTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);
  i2c_init(SCL_PIN,SDA_PIN);
  printf("i2c_init() complete\n");

  while(1) {
    i = ADXL345_findDevice();
    printf("ADXL found at address %x\n",i);
    vTaskDelay(5000 / portTICK_RATE_MS);
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

    wifi_set_opmode(NULL_MODE);
    wifi_set_sleep_type(MODEM_SLEEP_T);
    
    printf("LED Toggle using switch attached to GPIO12\n");
    GPIO_ConfigTypeDef io_out_conf;
    io_out_conf.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
    io_out_conf.GPIO_Mode = GPIO_Mode_Output;
    io_out_conf.GPIO_Pin = LED_IO_PIN;
    io_out_conf.GPIO_Pullup = GPIO_PullUp_DIS;
    gpio_config(&io_out_conf);

    GPIO_OUTPUT_SET(LED_IO_NUM, 0);  // LED On
    vTaskDelay(100);
    GPIO_OUTPUT_SET(LED_IO_NUM, 1);  // LED Off
    vTaskDelay(100);
    GPIO_OUTPUT_SET(LED_IO_NUM, 0);  // LED On
    vTaskDelay(100);

    GPIO_OUTPUT_SET(LED_IO_NUM, 1);  // LED Off
    vTaskDelay(100);
    GPIO_OUTPUT_SET(LED_IO_NUM, 0);   // LED On
    vTaskDelay(100);

    printf("SETUP switch INTERRUPT..\r\n");
    GPIO_ConfigTypeDef io_in_conf;
    io_in_conf.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
    io_in_conf.GPIO_Mode = GPIO_Mode_Input;
    io_in_conf.GPIO_Pin = BUTTON_IO_PIN;
    io_in_conf.GPIO_Pullup = GPIO_PullUp_EN;
    gpio_config(&io_in_conf);

    gpio_intr_handler_register(io_intr_handler, NULL);
    ETS_GPIO_INTR_ENABLE();


    xTaskCreate(i2cScanTask,"i2cScan",256,NULL,2,NULL);
}

