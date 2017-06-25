/*
 * OpenSeizureDetector - ESP8266 Version.
 *  ADXL code based on https://gist.github.com/shunkino/6b1734ee892fe2efbd12
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "i2c/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "osd_app.h"

#include "adxl345/adxl345.h"

void read_reg(int *result, uint8_t reg_addr);

static QueueHandle_t tsqueue;

//static QueueHandle_t mainqueue;


void LEDBlinkTask(void *pvParam) {
  printf("LEDBlinkTask()");
  gpio_enable(2,GPIO_OUTPUT);
  while(1) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    printf("Blink...\n");
    if (gpio_read(2))
      {
	// set gpio low
	gpio_write(2,false);
      }
    else
      {
	// set gpio high
	gpio_write(2,true);
      }
  }
}



void task1(void *pvParameters)
{
    QueueHandle_t *queue = (QueueHandle_t *)pvParameters;
    printf("Hello from task1!\r\n");
    uint32_t count = 0;
    while(1) {
        vTaskDelay(100);
        xQueueSend(*queue, &count, 0);
        count++;
    }
}

void task2(void *pvParameters)
{
    printf("Hello from task 2!\r\n");
    QueueHandle_t *queue = (QueueHandle_t *)pvParameters;
    while(1) {
        uint32_t count;
        if(xQueueReceive(*queue, &count, 1000)) {
            printf("Got %u\n", count);
        } else {
            printf("No msg :(\n");
        }
    }
}

void i2cScanTask(void *pvParameters) {
  uint8_t i;
  printf("i2cScanTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);
  //i2c_init(SCL_PIN,SDA_PIN);
  printf("i2c_init() complete\n");

  while(1) {
    i = ADXL345_init(SCL_PIN,SDA_PIN);
    printf("ADXL found at address %x\n",i);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  } 
}




void gpio_intr_handler(uint8_t gpio_num)
{
  //printf("ISR - GPIO_%d\n",gpio_num);
  uint32_t now = xTaskGetTickCountFromISR();
  xQueueSendToBackFromISR(tsqueue, &now, NULL);
}


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

  //printf("Setting 2G range\n");
  //ADXL345_setRange(ADXL345_RANGE_2G);

  //printf("Setting to 100Hz data rate\n");
  ADXL345_setDataRate(ADXL345_DATARATE_100HZ);
  ADXL345_setDataRate(ADXL345_DATARATE_1_56HZ);

  /*printf("Setting Data Format - interupts active low.\n");
  if (ADXL345_writeRegister8(ADXL345_REG_DATA_FORMAT,0x2B)==-1) {
    printf("*** Error setting Data Format ***\n");
  } else {
    printf("Data Format Set to 0x%02x\n",
  	   ADXL345_readRegister8(ADXL345_REG_DATA_FORMAT));
  }
  */
  // Set to 4g range, interrupts active high.
  ADXL345_writeRegister8(ADXL345_REG_DATA_FORMAT,0x01);

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
  printf("DEV_ID=     0x%02x\n",ADXL345_readRegister8(ADXL345_REG_DEVID));
  printf("INT_SOURCE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_SOURCE));
  printf("BW_RATE=    0x%02x\n",ADXL345_readRegister8(ADXL345_REG_BW_RATE));
  printf("DATA_FORMAT=0x%02x\n",ADXL345_readRegister8(ADXL345_REG_DATA_FORMAT));
  printf("INT_ENABLE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_ENABLE));
  printf("INT_MAP=    0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_MAP));
  printf("POWER_CTL=  0x%02x\n",ADXL345_readRegister8(ADXL345_REG_POWER_CTL));
  printf("************************************\n");
  


}



void monitorAdxl345Task(void *pvParameters) {
  //uint8_t devAddr;
  ADXL345_Vector r;
  
  //printf("monitorAdxl345AccelTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);
  //setup_adxl345();

  while(1) {
    printf("*****************************************\n");
    printf("*        Periodic Monitoring            *\n");
    printf("INT_ENABLE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_ENABLE));
    printf("INT_MAP=    0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_MAP));
    printf("INT_SOURCE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_SOURCE));
    printf("FIFO_STATUS= %d\n",ADXL345_readRegister8(ADXL345_REG_FIFO_STATUS));
    printf("Interrupt Pin GPIO%d value = %d\n",INTR_PIN,gpio_read(INTR_PIN));
    //for (int i=0;i<33;i++) {
    r = ADXL345_readRaw();
    printf("r.x=%7.0f, r.y=%7.0f, r.z=%7.0f\n",r.XAxis,r.YAxis,r.ZAxis);
    //}
    printf("INT_SOURCE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_SOURCE));
    printf("*****************************************\n");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}


/* 
 *  based on button.c example in the esp-open-rtos sdk.
*/
void receiveAccelDataTask(void *pvParameters)
{
  ADXL345_Vector r;
  printf("receiveAccelDataTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);

  // Set the Interrupt pin to be an input.
  gpio_enable(INTR_PIN, GPIO_INPUT);
  gpio_set_pullup(INTR_PIN, false, false);
  gpio_set_interrupt(INTR_PIN, GPIO_INTTYPE_EDGE_POS, gpio_intr_handler);
  //gpio_set_interrupt(INTR_PIN, GPIO_INTTYPE_LEVEL_HIGH, gpio_intr_handler);


  // Put the axl345 into the correct mode of operation.
  setup_adxl345();
  r = ADXL345_readRaw();
  printf("Initial Reading: r.x=%7.0f, r.y=%7.0f, r.z=%7.0f\n",
	   r.XAxis,r.YAxis,r.ZAxis);

  printf("Starting ADXL345 Monitoring task\n");
  xTaskCreate(monitorAdxl345Task,"monitorAdxl345",256,NULL,2,NULL);

  
  printf("Waiting for accelerometer data ready interrupt on gpio %d...\r\n", INTR_PIN);
  QueueHandle_t *tsqueue = (QueueHandle_t *)pvParameters;
  
  while(1) {
    uint32_t data_ts;
    xQueueReceive(*tsqueue, &data_ts, portMAX_DELAY);
    data_ts *= portTICK_PERIOD_MS;

    int i=0;
    bool finished = false;
    while (!finished) {
      r = ADXL345_readRaw();
      i++;
      printf("%d: receiveAccelDataTask: %dms r.x=%7.0f, r.y=%7.0f, r.z=%7.0f\n",
	     i,data_ts,r.XAxis,r.YAxis,r.ZAxis);
      // has the interrupt input cleared yet?
      finished = !gpio_read(INTR_PIN);
    }
  }
}




void user_init(void)
{
  //uart_set_baud(0, 115200);
  // we just do this so we can use the same serial monitor to look at the
  // boot messages as the debug ones - otherwise the boot messages are mangled..
  uart_set_baud(0, 74880);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());

  //mainqueue = xQueueCreate(10, sizeof(uint32_t));
  //xTaskCreate(task1, "tsk1", 256, &mainqueue, 2, NULL);
  //xTaskCreate(task2, "tsk2", 256, &mainqueue, 2, NULL);
  //xTaskCreate(LEDBlinkTask,"Blink",256,NULL,2,NULL);
  //xTaskCreate(i2cScanTask,"i2cScan",256,NULL,2,NULL);


  //xTaskCreate(monitorAdxl345Task,"monitorAdxl345",256,NULL,2,NULL);

  tsqueue = xQueueCreate(2, sizeof(uint32_t));
  xTaskCreate(receiveAccelDataTask, "receiveAccelDataTask", 256, &tsqueue, 2, NULL);


}
