#ifndef NVS_FUNC_H
#define NVS_FUNC_H

#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <Arduino.h>

#include "config.h"

#define NVS_NAMESPACE "MyConfig"

void write_nvs(const char *key, const char *value);
int read_nvs(const char *key, char *value);

void record_id(const char *id);
int check_id(char *id);
void record_sleep_time(const char *time);
int check_sleep_time(char *time);

void record_count(int count);
int check_count(int *count);

#endif