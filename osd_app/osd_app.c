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

#define ACC_BUF_LEN 50

/* GLOBAL VARIABLES */
Sd_Settings sdS;        // SD setings structure.
Wifi_Settings wifiS;    // Wifi Settings structure.

// Settings (obtained from default constants or persistent storage)
int debug = DEBUG_DEFAULT;    // enable or disable logging output
int sampleFreq = SAMPLE_FREQ_DEFAULT;      // Sampling frequency in Hz (must be one of 10,25,50 or 100)
int freqCutoff = FREQ_CUTOFF_DEFAULT;      // Frequency above which movement is ignored.
int nFreqCutoff;     // Bin number of cutoff frequency.
int samplePeriod = SAMPLE_PERIOD_DEFAULT;    // Sample period in seconds
int nSamp;           // number of samples in sampling period
                     //  (rounded up to a power of 2)
int fftBits;         // size of fft data array (nSamp = 2^(fftBits))

int dataUpdatePeriod = DATA_UPDATE_PERIOD_DEFAULT; // number of seconds between sending data to the phone.
int sdMode = SD_MODE_DEFAULT;          // Seizure Detector mode 0=normal, 1=raw, 2=filter
//int sampleFreq;      // Sample frequency in Hz
int alarmFreqMin = ALARM_FREQ_MIN_DEFAULT;    // Bin number of lower boundary of region of interest
int alarmFreqMax = ALARM_FREQ_MAX_DEFAULT;    // Bin number of higher boundary of region of interest
int warnTime = WARN_TIME_DEFAULT;        // number of seconds above threshold to raise warning
int alarmTime = ALARM_TIME_DEFAULT;       // number of seconds above threshold to raise alarm.
int alarmThresh = ALARM_THRESH_DEFAULT;     // Alarm threshold (average power of spectrum within
                     //       region of interest.
int alarmRatioThresh = ALARM_RATIO_THRESH_DEFAULT;
int nMax = 0;
int nMin = 0;
int nMins[4];
int nMaxs[4];

int fallActive = 0;     // fall detection active (0=inactive)
int fallThreshMin = 0;  // fall detection minimum (lower) threshold (milli-g)
int fallThreshMax = 0;  // fall detection maximum (upper) threshold (milli-g)
int fallWindow = 0;     // fall detection window (milli-seconds).
int fallDetected = 0;   // flag to say if fall is detected (<>0 is fall)

int isManAlarm = 0;     // flag to say if a manual alarm has been raised.
int manAlarmTime = 0;   // time (in sec) that manual alarm has been raised
int manAlarmPeriod = 0; // time (in sec) that manual alarm is raised for

int isMuted = 0;        // flag to say if alarms are muted.
int muteTime = 0;       // time (in sec) that alarms have been muted.
int mutePeriod = 0;     // Period for which alarms are muted (sec)

ADXL345_IVector latestAccelData;  // Latest accelerometer readings received.
int maxVal = 0;       // Peak amplitude in spectrum.
int maxLoc = 0;       // Location in output array of peak.
int maxFreq = 0;      // Frequency corresponding to peak location.
long specPower = 0;   // Average power of whole spectrum.
long roiPower = 0;    // Average power of spectrum in region of interest
long roiPowers[4];
int roiRatio = 0;     // 10xroiPower/specPower
int roiRatios[4];
int freqRes = 0;      // Actually 1000 x frequency resolution

int alarmState = 0;    // 0 = OK, 1 = WARNING, 2 = ALARM
int alarmRoi = 0;
int alarmCount = 0;    // number of seconds that we have been in an alarm state.

static QueueHandle_t tsqueue;
static QueueHandle_t commsQueue;

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
    printf("Interrupt Pin GPIO%d value = %d\n",INTR_PIN,gpio_read(INTR_PIN));
    //for (int i=0;i<33;i++) {
    r = ADXL345_readRaw();
    printf("r.x=%7d, r.y=%7d, r.z=%7d\n",r.XAxis,r.YAxis,r.ZAxis);
    //}
    printf("INT_SOURCE= 0x%02x\n",ADXL345_readRegister8(ADXL345_REG_INT_SOURCE));
    printf("*****************************************\n");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}


void AlarmCheckTask(void *pvParam) {
  static int dataUpdateCount = 0;
  static int lastAlarmState = 0;
  printf("AlarmCheckTask() - running every second\n");
  QueueHandle_t *commsQueue = (QueueHandle_t *)pvParam;
  while(1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("AlarmCheckTask() - heap=%d.\n", xPortGetFreeHeapSize());
    // Do FFT analysis if we have filled the buffer with data.
    if (accDataFull) {
      do_analysis();
      if (fallActive) check_fall();  // sets fallDetected global variable.
      // Check the alarm state, and set the global alarmState variable.
      alarm_check();
    
      // If no seizure detected, modify alarmState to reflect potential fall
      // detection
      if ((alarmState == ALARM_STATE_OK) && (fallDetected==1))
	alarmState = ALARM_STATE_FALL;
    
      //  Display alarm message on screen.
      if (alarmState == ALARM_STATE_OK) {
	APP_LOG(APP_LOG_LEVEL_DEBUG,"OK\n");
      }
      if (alarmState == ALARM_STATE_WARN) {
	APP_LOG(APP_LOG_LEVEL_DEBUG,"WARNING\n");
      }
      if (alarmState == ALARM_STATE_ALARM) {
	APP_LOG(APP_LOG_LEVEL_DEBUG,"**ALARM**\n");
      }
      if (alarmState == ALARM_STATE_FALL) {
	APP_LOG(APP_LOG_LEVEL_DEBUG,"**FALL**\n");
      }
      if (isManAlarm) {
	alarmState = ALARM_STATE_MAN_ALARM;
	APP_LOG(APP_LOG_LEVEL_DEBUG,"** MAN ALARM**\n");
      }
      if (isMuted) {
	alarmState = ALARM_STATE_MUTE;
	APP_LOG(APP_LOG_LEVEL_DEBUG,"** MUTE **\n");
      }    
      
      // Send data to phone if we have an alarm condition.
      // or if alarm state has changed from last time.
      if ((alarmState != ALARM_STATE_OK && !isMuted) ||
	  (alarmState != lastAlarmState)) {
	//sendSdData();
        xQueueSend(*commsQueue, &dataUpdateCount, 0);
      }
      lastAlarmState = alarmState;
    }
  
    // See if it is time to send data to the phone.
    dataUpdateCount++;
    if (dataUpdateCount>=dataUpdatePeriod) {
      //sendSdData();
      xQueueSend(*commsQueue, &dataUpdateCount, 0);
      dataUpdateCount = 0;
    } 
  }
}

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

/* 
 *  based on button.c example in the esp-open-rtos sdk.
*/
void receiveAccelDataTask(void *pvParameters)
{
  ADXL345_IVector r;
  ADXL345_IVector buf[ACC_BUF_LEN];
  
  printf("receiveAccelDataTask - SCL=GPIO%d, SDA=GPIO%d\n",SCL_PIN,SDA_PIN);
  setup_adxl345();
  ADXL345_enableFifo();
  printf("Waiting for accelerometer data ready interrupt on gpio %d...\r\n", INTR_PIN);
  QueueHandle_t *tsqueue = (QueueHandle_t *)pvParameters;

  // Set the Interrupt pin to be an input.
  gpio_enable(INTR_PIN, GPIO_INPUT);
  gpio_set_interrupt(INTR_PIN, GPIO_INTTYPE_EDGE_POS, gpio_intr_handler);

  // Start the routine monitoring task
  //xTaskCreate(monitorAdxl345Task,"monitorAdxl345",256,NULL,2,NULL);

  while(1) {
    uint32_t data_ts;
    xQueueReceive(*tsqueue, &data_ts, portMAX_DELAY);
    data_ts *= portTICK_PERIOD_MS;

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
    //printf("receiveAccelDataTask: read %d points from fifo, %dms r.x=%7d, r.y=%7d, r.z=%7d\n",
    //	   i,
    //	   data_ts,r.XAxis,r.YAxis,r.ZAxis);
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


/**
 * This is the freeRTOS equivalent of main()!!!
 */
void user_init(void)
{
  // we just do this so we can use the same serial monitor to look at the
  // boot messages as the debug ones - otherwise the boot messages are mangled..
  uart_set_baud(0, 74880);
  printf("SDK version:%s\n", sdk_system_get_sdk_version());

  //xTaskCreate(LEDBlinkTask,"Blink",256,NULL,2,NULL);
  //xTaskCreate(i2cScanTask,"i2cScan",256,NULL,2,NULL);

  gpio_enable(SETUP_PIN,GPIO_INPUT);
  gpio_set_pullup(SETUP_PIN,true,false);
  if (gpio_read(SETUP_PIN)) {
    printf("Starting in Run Mode\n");
    settings_init();
    comms_init();
    analysis_init();
    
    tsqueue = xQueueCreate(2, sizeof(uint32_t));
    commsQueue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreate(receiveAccelDataTask, "receiveAccelDataTask",
		256, &tsqueue, 2, NULL);
    xTaskCreate(AlarmCheckTask, "AlarmCheckTask",
		512, &commsQueue, 2, NULL);
    xTaskCreate(commsTask, "commsTask",
		256, &commsQueue, 2, NULL);
  } else {
    printf("Starting in Setup Mode\n");
    xTaskCreate(&httpd_task, "HTTP Daemon", 256, NULL, 2, NULL);
  }
}
