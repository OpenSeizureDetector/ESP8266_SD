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
#include <dhcpserver.h>

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

  sdk_wifi_set_opmode(SOFTAP_MODE);
  struct ip_info ap_ip;
  IP4_ADDR(&ap_ip.ip, 192, 168, 1, 1);
  IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
  IP4_ADDR(&ap_ip.netmask, 255, 255, 255, 0);
  sdk_wifi_set_ip_info(1, &ap_ip);
  
  struct sdk_softap_config ap_config = {
    .ssid = SETUP_WIFI_SSID,
    .ssid_hidden = 0,
    .channel = 3,
    .ssid_len = strlen(SETUP_WIFI_SSID),
    .authmode = AUTH_WPA_WPA2_PSK,
    .password = SETUP_WIFI_PASSWD,
    .max_connection = 3,
    .beacon_interval = 100,
  };

  printf("configuring SSID %s\n",ap_config.ssid);
  sdk_wifi_softap_set_config(&ap_config);

  ip_addr_t first_client_ip;
  IP4_ADDR(&first_client_ip, 192, 168, 1, 2);
  dhcpserver_start(&first_client_ip, 4);

  
  
  /* turn off LED */
  gpio_enable(LED_PIN, GPIO_OUTPUT);
  gpio_write(LED_PIN, true);
  

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


