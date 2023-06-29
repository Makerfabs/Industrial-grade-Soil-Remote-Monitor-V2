#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#include "config.h"
#include "nvs_func.h"

void ap_init(String ssid, String password);
void wifi_init();
int wifi_config_server();
void config_check();

int parse_request(String request, char *id, char *sleep);
String sensor_read();

void main_page_html(WiFiClient *client);

#endif