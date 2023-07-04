/*
ESP32 Borad Version 1.0.6
RadioLib Version 4.6.0


*/
#include <SPI.h>
#include <LovyanGFX.hpp>
#include <RadioLib.h>
#include <WiFi.h>

#include "makerfabs_pin.h"
#include "log_save.h"
#include "Lora.h"

#define SSID "Makerfabs"
#define PASSWORD "20160704"

#define LOG_FILE_NAME "/my_log.txt"

#define RECEIVE_LEN 20

struct LGFX_Config
{
    static constexpr spi_host_device_t spi_host = VSPI_HOST;
    static constexpr int dma_channel = 1;
    static constexpr int spi_sclk = LCD_SCK;
    static constexpr int spi_mosi = LCD_MOSI;
    static constexpr int spi_miso = LCD_MISO;
};

static lgfx::LGFX_SPI<LGFX_Config> tft;
static LGFX_Sprite sprite(&tft);
static lgfx::Panel_ILI9488 panel;

SPIClass SPI_Lora = SPIClass(HSPI);
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, SPI_Lora, SPISettings());
Lora lora(&radio);

int net_flag = 1;

void setup()
{
    Serial.begin(115200);

    pinMode(LCD_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI_Lora.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    SD_init();
    set_tft();
    tft.begin();
    tft.setRotation(SCRENN_ROTATION);
    // Lora init must after HSPI init
    lora.init();

    checkFile(SD, LOG_FILE_NAME);

    page_title("WiFi Connecting...");

    WiFi.begin(SSID, PASSWORD);

    int connect_count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(500);
        Serial.print(".");
        connect_count++;
        if (connect_count > 20)
        {
            Serial.println("Wifi error");
            net_flag = 0;
            break;
        }
    }
}

void loop()
{

    receive_page();
}

//********************************page**************************

void receive_page()
{
    long runtime = millis();
    int pos[2] = {0, 0};

    String rec_list[RECEIVE_LEN];
    int rec_index = 0;
    int rec_num = 0;

    // Page refresh loop
    while (1)
    {
        // page init
        if (net_flag == 1)
            page_title("RECEIVE PAGE(WiFi)");
        else
            page_title("RECEIVE PAGE");

        tft.drawRect(9, 44, 462, 252, TFT_WHITE);

        // Prevent accidental touch.
        delay(1000);

        // Working loop
        while (1)
        {
            String rec_str = "";
            rec_str = lora.receive();

            // delay(5000);
            // rec_str = "ID:P2,SLEEEP:10,H:1.01,T:2.86,PH:3.03,N:4,P:5,K:6";
            if (!rec_str.equals(""))
            {
                lora_record(rec_str);

                if (net_flag == 1)
                    thingspeak(rec_str);

                rec_list[rec_index] = rec_str;
                int temp_index = rec_index;

                if (rec_index == RECEIVE_LEN - 1)
                    rec_index = 0;
                else
                    rec_index++;

                if (rec_num < RECEIVE_LEN)
                    rec_num++;

                tft.fillRect(10, 45, 460, 200, TFT_BLACK);
                tft.setTextColor(TFT_WHITE);
                tft.setTextSize(1);

                for (int i = 0; i < rec_num; i++)
                {
                    tft.setCursor(12, 50 + i * 10);
                    tft.println(rec_list[temp_index--]);
                    if (temp_index < 0)
                        temp_index = rec_num - 1;
                }
            }
        }
    }
    return;
}

//****************tft************************************

void set_tft()
{
    panel.freq_write = 40000000;
    panel.freq_fill = 40000000;
    panel.freq_read = 16000000;

    panel.spi_cs = LCD_CS;
    panel.spi_dc = LCD_DC;
    panel.gpio_rst = LCD_RST;
    panel.gpio_bl = LCD_BL;

    tft.setPanel(&panel);
}

void page_title(String title)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, 10);
    tft.setTextSize(4);
    tft.println(title);
}

//********************Lora Transform and Log Save******************

void lora_record(String msg)
{
    String time = "Time:";
    time = time + String(millis() / 1000) + "s    ";
    appendFile(SD, LOG_FILE_NAME, time.c_str());
    appendFile(SD, LOG_FILE_NAME, "\n");
    appendFile(SD, LOG_FILE_NAME, msg.c_str());
    appendFile(SD, LOG_FILE_NAME, "\n");
    // readFile(SD, LOG_FILE_NAME);
}

void thingspeak(String msg)
{

    // Parse
    // ID:P2,SLEEEP:10,H:0.00,T:81.86,PH:4.00,N:0,P:0,K:0
    int length = msg.length();
    int value_start = 0;
    String temp = "";
    String data[8] = {"", "", "", "", "", "", "", ""};
    int data_index = 0;
    for (int i = 0; i < length; i++)
    {
        char c = msg.c_str()[i];
        if (c == ':')
        {
            value_start = 1;
            continue;
        }
        if (value_start == 1)
        {
            temp = temp + c;
        }
        if (c == ',' || i == (length - 1))
        {
            value_start = 0;
            Serial.println(temp);

            if (data_index < 8)
            {
                data[data_index++] = temp;
            }
            else
            {
                Serial.println("Lora data that does not conform to the format");
                return;
            }

            temp = "";
            continue;
        }
    }

    // for (int i = 0; i < 8; i++)
    // {
    //     Serial.println(data[i]);
    // }

    // return;

    // Send to Thingspeak
    // https://api.thingspeak.com/update?api_key=9D8ACLLXDEGP6GZG&field1=0

    WiFiClient client;
    if (!client.connect("api.thingspeak.com", 80))
    {
        Serial.println("connection failed");
        return;
    }

    String req = "/update?api_key=9D8ACLLXDEGP6GZG&";

    for (int i = 0; i < 5; i++)
    {
        req = req + "field" + (i + 1) + "=" + data[i + 2] + "&";
    }
    req = req + "field6=" + data[7];

    client.print(String("GET ") + req + " HTTP/1.1\r\n" +
                 "Host: " + "api.thingspeak.com" + "\r\n" +
                 "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0)
    {
        if (millis() - timeout > 5000)
        {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    while (client.available())
    {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }

    Serial.println();
    Serial.println("closing connection");
}