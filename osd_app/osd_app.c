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



void monitorAdxl345AccelTask(void *pvParameters) {
  uint8_t devAddr;
  ADXL345_Vector r;
  
  printf("monitorAdxl345AccelTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);
  devAddr = ADXL345_init(SCL_PIN,SDA_PIN);
  printf("ADXL345 found at address 0x%x\n",devAddr);
  
  while(1) {
    r = ADXL345_readRaw();
    printf("r.x=%7.0f, r.y=%7.0f, r.z=%7.0f\n",r.XAxis,r.YAxis,r.ZAxis);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void gpio_intr_handler(uint8_t gpio_num)
{
  //printf("ISR - GPIO_%d\n",gpio_num);
  uint32_t now = xTaskGetTickCountFromISR();
  xQueueSendToBackFromISR(tsqueue, &now, NULL);
}


/* 
 *  based on button.c example in the esp-open-rtos sdk.
*/
void receiveAccelDataTask(void *pvParameters)
{
  uint8_t devAddr;
  ADXL345_Vector r;
  //uint32_t last = 0;
  
  printf("receiveAccelDataTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);
  devAddr = ADXL345_init(SCL_PIN,SDA_PIN);
  printf("ADXL345 found at address 0x%x\n",devAddr);
  //ADXL345_enableFifo();
  printf("Waiting for accelerometer data ready interrupt on gpio %d...\r\n", INTR_PIN);
  QueueHandle_t *tsqueue = (QueueHandle_t *)pvParameters;
  // Set the Interrupt pin to be an input.
  gpio_enable(INTR_PIN, GPIO_INPUT);
  gpio_set_interrupt(INTR_PIN, GPIO_INTTYPE_EDGE_NEG, gpio_intr_handler);
  
  while(1) {
    uint32_t data_ts;
    xQueueReceive(*tsqueue, &data_ts, portMAX_DELAY);
    data_ts *= portTICK_PERIOD_MS;
    r = ADXL345_readRaw();
    printf("receiveAccelDataTask: %d ms, int_source=0x%02x,  r.x=%7.0f, r.y=%7.0f, r.z=%7.0f\n",
	   data_ts,
	   //ADXL345_readRegister8(ADXL345_REG_INT_SOURCE),
	   0x00,
	   r.XAxis,r.YAxis,r.ZAxis);

    //if(last < button_ts-200) {
    //  printf("Interrupt fired at %dms\r\n", button_ts);
    //  last = button_ts;
    //}
  }
}

void user_init(void)
{
  // we just do this so we can use the same serial monitor to look at the
  // boot messages as the debug ones - otherwise the boot messages are mangled..
  uart_set_baud(0, 74880);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());

  //mainqueue = xQueueCreate(10, sizeof(uint32_t));
  //xTaskCreate(task1, "tsk1", 256, &mainqueue, 2, NULL);
  //xTaskCreate(task2, "tsk2", 256, &mainqueue, 2, NULL);
  //xTaskCreate(LEDBlinkTask,"Blink",256,NULL,2,NULL);
  //xTaskCreate(i2cScanTask,"i2cScan",256,NULL,2,NULL);


  //xTaskCreate(monitorAdxl345AccelTask,"monitorAdxl345Accel",256,NULL,2,NULL);

  tsqueue = xQueueCreate(2, sizeof(uint32_t));
  xTaskCreate(receiveAccelDataTask, "receiveAccelDataTask", 256, &tsqueue, 2, NULL);


}
