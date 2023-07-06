#ifndef CONFIG_H
#define CONFIG_H

#define AP_SSID "Makerfabs-"
#define AP_PWD "12345678"

#define SENSOR_5_PIN
// #define SENSOR_3_PIN

#define DEFAULT_SENSOR_ID "0x1234"
#define DEFAULT_SLEEP_TIME "10"

#define SUCCESS 1
#define ERROR 0

#define NVS_DATA_LENGTH 20

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */

#define BUTTON_PIN 1

// Lora
#define LORA_RST 5
#define LORA_CS 4

#define DIO0 6
#define DIO1 7

#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_SCK 12

#define FREQUENCY 868.0
#define BANDWIDTH 125.0
#define SPREADING_FACTOR 9
#define CODING_RATE 7
#define OUTPUT_POWER 10
#define PREAMBLE_LEN 8
#define GAIN 0

// RS485
#define MYSerialRX 18
#define MYSerialTX 17

// Power Control
#define POWER_LORA 21
#define POWER_485 48

#endif