/*
 * HTTP server example.
 *
 * This sample code is in the public domain.
 */
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <string.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <ssid_config.h>
#include <httpd/httpd.h>
#include "osd_app.h"

#define LED_PIN 2

enum {
    SSI_UPTIME,
    SSI_FREE_HEAP,
    SSI_LED_STATE
};


char *gpio_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  APP_LOG(APP_LOG_LEVEL_DEBUG,"gpio_cgi_handler\n");
  for (int i = 0; i < iNumParams; i++) {
    if (strcmp(pcParam[i], "on") == 0) {
      uint8_t gpio_num = atoi(pcValue[i]);
      gpio_enable(gpio_num, GPIO_OUTPUT);
      gpio_write(gpio_num, true);
    } else if (strcmp(pcParam[i], "off") == 0) {
      uint8_t gpio_num = atoi(pcValue[i]);
      gpio_enable(gpio_num, GPIO_OUTPUT);
      gpio_write(gpio_num, false);
    } else if (strcmp(pcParam[i], "toggle") == 0) {
      uint8_t gpio_num = atoi(pcValue[i]);
      gpio_enable(gpio_num, GPIO_OUTPUT);
      gpio_toggle(gpio_num);
    }
  }
  return "/index.html";
}

char *about_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"about_cgi_handler()\n");
  return "/index.html";
}

char *data_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  APP_LOG(APP_LOG_LEVEL_DEBUG,"data_cgi_handler()\n");
  return "{'key':'val'}";
}


void httpd_task(void *pvParameters)
{
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"httpd_task()\n");
  tCGI pCGIs[] = {
    {"/data", (tCGIHandler) data_cgi_handler},
    {"/gpio", (tCGIHandler) gpio_cgi_handler},
    {"/about", (tCGIHandler) about_cgi_handler},
  };
  
  /* register handlers and start the server */
  http_set_cgi_handlers(pCGIs, sizeof (pCGIs) / sizeof (pCGIs[0]));
  httpd_init();
  
  for (;;);
}

void httpd_server_init(void)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG,"httpd_server_init()\n");
  struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);
    sdk_wifi_station_connect();

    /* turn off LED */
    gpio_enable(LED_PIN, GPIO_OUTPUT);
    gpio_write(LED_PIN, true);

    /* initialize tasks */
    xTaskCreate(&httpd_task, "HTTP Daemon", 128, NULL, 2, NULL);
}
