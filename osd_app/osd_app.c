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

void read_reg(int *result, uint8_t reg_addr);

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


void adxl_init() {
  int response;
  printf("adxl_init() - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);
  i2c_init(SCL_PIN,SDA_PIN);
  printf("i2c_init() complete\n");
  // read device id.
  read_reg(&response, 0x00);
  printf("device: %x\n", response);
}


void read_reg(int *result, uint8_t reg_addr) {
	uint8_t ack;
	i2c_start();
	ack = i2c_write((DEV_ADDR << 1) + 0);
	if (!ack) {
		printf("addr not ack when tx write command \n");
		i2c_stop();
	}
	ack = i2c_write(reg_addr);
	if (!ack) {
		printf("register addr not ack \n");
		i2c_stop();
	}
	//os_delay_us(40000);
	vTaskDelay(40 / portTICK_PERIOD_MS);
	i2c_stop();
	i2c_start();
	ack = i2c_write((DEV_ADDR << 1) + 1);
	if (!ack) {
		printf("read device addr not ack when tx write command \n");
		i2c_stop();
	}
	//os_delay_us(40000);
	vTaskDelay(40 / portTICK_PERIOD_MS);
	int res = i2c_read(false);
	i2c_stop();
	*result = res;
	printf("reading %x succeed\n", res);
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
  printf("i2cScanTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);
  i2c_init(SCL_PIN,SDA_PIN);
  printf("i2c_init() complete\n");

  while(1) {
    uint8_t id;
    uint8_t reg = 0;
    uint8_t i;

    printf("i2c scan...\n");

    for (i=0;i<127;i++) {
      if (i2c_slave_read(i, &reg, &id, 1)) {
	// non-zero response = error
      } else {
	printf("Response received from address %d (0x%x) - id=%d (0x%x)id\n",i,i,id,id);
      }
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
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
  xTaskCreate(LEDBlinkTask,"Blink",256,NULL,2,NULL);
  xTaskCreate(i2cScanTask,"i2cScan",256,NULL,2,NULL);
  //adxl_init();
}
