/*
 * OpenSeizureDetector - ESP8266 Version.
 *  ADXL code based on https://gist.github.com/shunkino/6b1734ee892fe2efbd12
 *  GPIO code based on https://github.com/espressif/esp8266-rtos-sample-code/tree/master/02Peripheral/GPIOtest
 */

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "esp_common.h"
#include "gpio.h"
#include "i2c.h"
#include "adxl345.h"
#include "osd_app.h"

// Define which GPIO pins are used to connect to the ADXL345 accelerometer.
//#define SCL_PIN (5)  // Wemos D1 Mini D1 = GPIO5
//#define SDA_PIN (4)  // Wemos D1 Mini D2 = GPIO4
//#define INTR_PIN (13) // Wemos D1 Mini D7 = GPIO13
//#define SETUP_PIN (12)  // Wemos D1 Mini D6 = GPIO12

//GPIO OUTPUT SETTINGS
#define  LED_IO_NUM  2
#define  LED_IO_FUNC FUNC_GPIO2
#define  LED_IO_PIN  GPIO_Pin_2

//GPIO INPUT SETTINGS
#define  BUTTON_IO_NUM   12
#define  BUTTON_IO_FUNC  FUNC_GPIO12
#define  BUTTON_IO_PIN   GPIO_Pin_12

#define ACC_BUF_LEN 50


/* GLOBAL VARIABLES */
Sd_Settings sdS;        // SD setings structure.
Wifi_Settings wifiS;    // Wifi Settings structure.

static xQueueHandle tsqueue;
static xQueueHandle commsQueue;


void settings_init() {
  sdS.samplePeriod = SAMPLE_PERIOD_DEFAULT;
  sdS.sampleFreq = SAMPLE_FREQ_DEFAULT;
  sdS.freqCutoff = FREQ_CUTOFF_DEFAULT;
  sdS.dataUpdatePeriod = DATA_UPDATE_PERIOD_DEFAULT;
  sdS.sdMode = SD_MODE_DEFAULT;
  sdS.alarmFreqMin = ALARM_FREQ_MIN_DEFAULT;
  sdS.alarmFreqMax = ALARM_FREQ_MAX_DEFAULT;
  sdS.warnTime = WARN_TIME_DEFAULT;
  sdS.alarmTime = ALARM_TIME_DEFAULT;
  sdS.alarmThresh = ALARM_THRESH_DEFAULT;
  sdS.alarmRatioThresh = ALARM_RATIO_THRESH_DEFAULT;
  sdS.fallActive = FALL_ACTIVE_DEFAULT;
  sdS.fallThreshMin = FALL_THRESH_MIN_DEFAULT;
  sdS.fallThreshMax = FALL_THRESH_MAX_DEFAULT;
  sdS.fallWindow = FALL_WINDOW_DEFAULT;
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

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/**********************************SAMPLE CODE*****************************/
#define ETS_GPIO_INTR_ENABLE()  _xt_isr_unmask(1 << ETS_GPIO_INUM)  //ENABLE INTERRUPTS
#define ETS_GPIO_INTR_DISABLE() _xt_isr_mask(1 << ETS_GPIO_INUM)    //DISABLE INTERRUPTS


// Interrupt handler - looks at the accerometer data ready
// signal on pin INTR_PIN, and the configuration switch on BUTTON_IO_PIN
void gpio_intr_handler(uint8_t gpio_num)
{
  uint32 status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);          //READ STATUS OF INTERRUPT
  static uint8 val = 0;

  if (status & INTR_PIN) {
    //printf("ISR - GPIO_%d\n",gpio_num);
    uint32_t now = xTaskGetTickCountFromISR();
    xQueueSendToBackFromISR(tsqueue, &now, NULL);
  }

  if (status & BUTTON_IO_PIN) {
    if (val == 0) {
      GPIO_OUTPUT_SET(LED_IO_NUM, 1);
      val = 1;
    } else {
      GPIO_OUTPUT_SET(LED_IO_NUM, 0);
      val = 0;
    }
  }


  //CLEAR THE STATUS IN THE W1 INTERRUPT REGISTER
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);       
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

/**
 * Initialise the ADXL345 accelerometer trip to use a FIFO buffer
 * and send an interrupt when the FIFO is full.
 */
void setup_adxl345() {
  uint8_t devAddr;
  
  printf("setup_adxl345()\n");
  
  // Initialise the ADXL345 i2c interface and search for the ADXL345
  devAddr = ADXL345_init(SCL_PIN,SDA_PIN);
  printf("ADXL345 found at address 0x%x\n",devAddr);
  
  //printf("Clearing Settings...\n");
  //ADXL345_clearSettings();
  
  // Bypass the FIFO buffer
  ADXL345_writeRegisterBit(ADXL345_REG_FIFO_CTL,6,0);
  ADXL345_writeRegisterBit(ADXL345_REG_FIFO_CTL,7,0);
  
  printf("Setting 4G range\n");
  ADXL345_setRange(ADXL345_RANGE_4G);
  
  printf("Setting to 100Hz data rate\n");
  ADXL345_setDataRate(ADXL345_DATARATE_100HZ);
  //ADXL345_setDataRate(ADXL345_DATARATE_12_5HZ);
  
  /*printf("Setting Data Format - interupts active low.\n");
    if (ADXL345_writeRegister8(ADXL345_REG_DATA_FORMAT,0x2B)==-1) {
    printf("*** Error setting Data Format ***\n");
    } else {
    printf("Data Format Set to 0x%02x\n",
    ADXL345_readRegister8(ADXL345_REG_DATA_FORMAT));
    }
  */
  // Set to 4g range, 4mg/LSB resolution, interrupts active high.
  ADXL345_writeRegister8(ADXL345_REG_DATA_FORMAT,0b00001001);
  
  printf("Starting Measurement\n");
  if (ADXL345_writeRegister8(ADXL345_REG_POWER_CTL,0x08)==-1) {
    printf("*** Error setting measurement Mode ***\n");
  } else {
    printf("Measurement Mode set to 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_POWER_CTL));
  }
  
  
  printf("Enabling Data Ready Interrupt\n");
  if (ADXL345_writeRegister8(ADXL345_REG_INT_ENABLE,0x80)==-1) {
    printf("*** Error enabling interrupt ***\n");
  } else {
    printf("Interrupt Enabled - set to 0x%02x\n",
	   ADXL345_readRegister8(ADXL345_REG_INT_ENABLE));
  }
  
  
  printf("************************************\n");
  printf("*           setup_adxl345()        *\n");
  printf("DEV_ID=     0x%02x\n",ADXL345_readRegister8(ADXL345_REG_DEVID));
  printf("INT_SOURCE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_SOURCE));
  printf("BW_RATE=    0x%02x\n",ADXL345_readRegister8(ADXL345_REG_BW_RATE));
  printf("DATA_FORMAT=0x%02x\n",ADXL345_readRegister8(ADXL345_REG_DATA_FORMAT));
  printf("INT_ENABLE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_ENABLE));
  printf("INT_MAP=    0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_MAP));
  printf("POWER_CTL=  0x%02x\n",ADXL345_readRegister8(ADXL345_REG_POWER_CTL));
  printf("************************************\n");
  
}


/**
 * monitor the ADXL345 regularly to check register states.
 * Assumes it has been initialised by calling setup_adxl345() before
 * this task is started.
 */
void monitorAdxl345Task(void *pvParameters) {
  ADXL345_IVector r;
  
  while(1) {
    printf("*****************************************\n");
    printf("*        Periodic Monitoring            *\n");
    printf("INT_ENABLE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_ENABLE));
    printf("INT_MAP=    0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_MAP));
    printf("INT_SOURCE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_SOURCE));
    printf("FIFO_STATUS= %d\n",ADXL345_readRegister8(ADXL345_REG_FIFO_STATUS));
    //    printf("Interrupt Pin GPIO%d value = %d\n",INTR_PIN,gpio_read(INTR_PIN));
    printf("Interrupt Pin GPIO%d value = %d\n",INTR_PIN,GPIO_INPUT_GET(INTR_PIN));
    // Read the data from FIFO - we probably do not really want this!
    /*int i;
    for (i=0;i<33;i++) {
      r = ADXL345_readRaw();
      printf("r.x=%7d, r.y=%7d, r.z=%7d\n",r.XAxis,r.YAxis,r.ZAxis);
    }
    printf("INT_SOURCE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_SOURCE));
    printf("*****************************************\n");
    */
    vTaskDelay(3000 / portTICK_RATE_MS);
  }
}


/* 
 *  Initialises the ADX345 accelerometer, 
 *  If USE_INTR is true,  waits for queue message from
 *  interrupt handler to say data is ready.
 *  Otherwise polls periodically to read data from the adxl345 FIFO
 */
void receiveAccelDataTask(void *pvParameters)
{
  ADXL345_IVector r;
  ADXL345_IVector buf[ACC_BUF_LEN];
  
  printf("receiveAccelDataTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);

  if (USE_INTR) {
    printf("SETUP INTERRUPT..\r\n");
    GPIO_ConfigTypeDef io_in_conf;
    io_in_conf.GPIO_IntrType = GPIO_PIN_INTR_POSEDGE;
    io_in_conf.GPIO_Mode = GPIO_Mode_Input;
    io_in_conf.GPIO_Pin = INTR_PIN;
    io_in_conf.GPIO_Pullup = GPIO_PullUp_EN;
    gpio_config(&io_in_conf);
    gpio_intr_handler_register(gpio_intr_handler, NULL);
    printf("Waiting for accelerometer data ready interrupt on gpio %d...\r\n", INTR_PIN);
  } else {
    printf("NOT using interrupts - will poll adxl345 periodically\n");
  }

  // Now initialise the adxl345 (which also initialises the i2c bus
  setup_adxl345();
  ADXL345_enableFifo();
  xQueueHandle *tsqueue = (xQueueHandle *)pvParameters;
  
  // Start the routine monitoring task
  //xTaskCreate(monitorAdxl345Task,"monitorAdxl345",256,NULL,2,NULL);
  
  while(1) {
    uint32_t data_ts;
    if (USE_INTR) {
      xQueueReceive(*tsqueue, &data_ts, portMAX_DELAY);
      printf("Accelerometer data ready....\n");
      data_ts *= portTICK_RATE_MS;
    } else {
      // Wait for 310 ms - 100Hz sample rate, fifo is 32 readings
      // so we wait for 31 readings = 310 ms.
      vTaskDelay(310 / portTICK_RATE_MS);
      int nFifo = ADXL345_readRegister8(ADXL345_REG_FIFO_STATUS);
      if (nFifo ==32) {
	printf("receiveAccelDataTask() - Warning - FIFO Overflow\n");
      } else {
	printf("receiveAccelDataTask() - Reading data based on timer - %d readings\n",nFifo);
      }
      data_ts = 0;
    }
    
    /// Now read all the data from the FIFO buffer on the ADXL345.
    int i=0;
    bool finished = false;
    while (!finished) {
      r = ADXL345_readRaw();
      buf[i] = r;
      i++;
      //printf("%d:%d,  receiveAccelDataTask: %dms r.x=%7d, r.y=%7d, r.z=%7d\n",
      //     i,
      //     ADXL345_readRegister8(ADXL345_REG_FIFO_STATUS),
      //     data_ts,r.XAxis,r.YAxis,r.ZAxis);
      
      // have we emptied the fifo or filled our buffer yet?
      if ((ADXL345_readRegister8(ADXL345_REG_FIFO_STATUS)==0)
	  || (i==ACC_BUF_LEN)) finished = 1;
    }
    printf("receiveAccelDataTask: read %d points from fifo, %dms r.x=%7d, r.y=%7d, r.z=%7d\n",
    	   i,
    	   data_ts,r.XAxis,r.YAxis,r.ZAxis);
    /*for (int n=0;n<i;n++) {
      printf("n=%d r.x=%7d, r.y=%7d, r.z=%7d\n",
      n,
      buf[n].XAxis,buf[n].YAxis,buf[n].ZAxis);
      }
    */

    // Call the acceleration handler in analysis.c
    accel_handler(buf,i);
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
    
    // Initial flashing of LED to prove it is alive!
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
    gpio_intr_handler_register(gpio_intr_handler, NULL);
    //ETS_GPIO_INTR_ENABLE();


    // reate the freeRTOS queues we use for comms between tasks
    tsqueue = xQueueCreate(2, sizeof(uint32_t));
    commsQueue = xQueueCreate(2, sizeof(uint32_t));


    //xTaskCreate(i2cScanTask,"i2cScan",256,NULL,2,NULL);

    settings_init();
    analysis_init();
    // Start the routine monitoring task
    //xTaskCreate(monitorAdxl345Task,"monitorAdxl345",256,NULL,2,NULL);
    xTaskCreate(receiveAccelDataTask, "receiveAccelDataTask",
		256, &tsqueue, 2, NULL);

    
    
}

