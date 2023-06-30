
#include <RadioLib.h>
#include <SPI.h>
#include <HardwareSerial.h>

#include "config.h"
#include "web_server.h"

SX1276 radio = new Module(LORA_CS, DIO0, LORA_RST, DIO1, SPI, SPISettings());
HardwareSerial MySerial(1);

// Sensor Config
char sensor_id[NVS_DATA_LENGTH];
char sleep_time[NVS_DATA_LENGTH];

unsigned char resp[80] = {0};

float humidity_value = 0.0;
float tem_value = 0.0;
float ph_value = 0.0;
int P_value = 0;
int N_value = 0;
int K_value = 0;

RTC_DATA_ATTR int bootCount = 0;

void setup()
{
    pin_init();

    Serial.begin(115200);

    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    print_wakeup_reason();

    MySerial.begin(4800, SERIAL_8N1, MYSerialRX, MYSerialTX);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    lora_init();

    // Button Check
    wifi_init();

    delay(5000);
}

void loop()
{
    // Send three time and sleep
    for (int i = 0; i < 4; i++)
    {
        // Serial.println(lora_msg_create());

        String temp = "";

        temp = sensor_read();
        value_log();
        if (i > 2)
        {
            temp = lora_msg_create(temp);
            lora_send_task(temp);
        }

        delay(1000);
    }

    int sleep_s = atoi(sleep_time);
    esp_sleep_enable_timer_wakeup(sleep_s * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(sleep_s) + " Seconds");

    // esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    // Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

    Serial.println("Going to sleep now");
    Serial.flush();
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
}

// Hardware init

void pin_init()
{
    pinMode(POWER_485, OUTPUT);
    pinMode(POWER_LORA, OUTPUT);
    pinMode(LORA_CS, OUTPUT);

    digitalWrite(POWER_485, HIGH);
    digitalWrite(POWER_LORA, HIGH);
    delay(100);
}

void lora_init()
{
    Serial.print(F("[SX1276] Initializing ... "));

    int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SX127X_SYNC_WORD, OUTPUT_POWER, PREAMBLE_LEN, GAIN);
    if (state == ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
            ;
    }
}

// Lora send task
String lora_msg_create(String sensor_data)
{
    String temp = "ID:";
    temp = temp + sensor_id + ",SLEEEP:" + sleep_time + "," + sensor_data;

    return temp;
}

void lora_send_task(String data)
{
    Serial.println(data);
    int state = radio.transmit(data.c_str());
    if (state == ERR_NONE)
    {
        // the packet was successfully transmitted
        Serial.println(F(" success!"));

        // print measured data rate
        Serial.print(F("[SX1276] Datarate:\t"));
        Serial.print(radio.getDataRate());
        Serial.println(F(" bps"));
    }
    else if (state == ERR_PACKET_TOO_LONG)
    {
        // the supplied packet was longer than 256 bytes
        Serial.println(F("too long!"));
    }
    else if (state == ERR_TX_TIMEOUT)
    {
        // timeout occurred while transmitting packet
        Serial.println(F("timeout!"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);
    }
}

// Soil Sensor
String sensor_read()
{
#ifdef SENSOR_5_PIN
    sensor_read_5pin();
#endif
#ifdef SENSOR_3_PIN
    sensor_read_3pin();
#endif

    // humidity_value = 8;
    // tem_value = 8;
    // ph_value = 8;
    // ph_value = 8;
    // N_value = 8;
    // P_value = 8;
    // K_value = 8;

    String str = "H:";
    str = str + humidity_value + ",T:" + tem_value + ",PH:" + ph_value + ",N:" + N_value + ",P:" + P_value + ",K:" + K_value;

    return str;
}

void sensor_read_5pin()
{
    int humidity = 0;
    int tem = 0;
    int ph = 0;

    unsigned char ask_cmd[8] = {0X01, 0X04, 0X00, 0X00, 0X00, 0X07, 0XB1, 0XC8};
    MySerial.write(ask_cmd, 8);
    int i = 0;

    while (MySerial.available() > 0 && i < 80)
    {
        resp[i] = MySerial.read();
        i++;

        yield();
    }

    Serial.print("Answer Length:");
    Serial.println(i);

    char temp[20];
    for (int j = 0; j < 19; j++)
    {
        sprintf(temp, "%02X ", (int)resp[j]);
        Serial.printf(temp);
    }

    Serial.print("\n");

    humidity = CaculateValue((int)resp[3], (int)resp[4]);
    humidity_value = humidity * 0.1;

    tem = CaculateValue((int)resp[5], (int)resp[6]);
    tem_value = tem * 0.1;
    tem_value = c2f(tem_value);

    ph = CaculateValue((int)resp[9], (int)resp[10]);
    ph_value = ph * 0.1;
    N_value = CaculateValue((int)resp[11], (int)resp[12]);
    P_value = CaculateValue((int)resp[13], (int)resp[14]);
    K_value = CaculateValue((int)resp[15], (int)resp[16]);
}

void sensor_read_3pin()
{
    int humidity = 0;
    int tem = 0;

    unsigned char ask_cmd[8] = {0X01, 0X03, 0X00, 0X00, 0X00, 0X02, 0XC4, 0X0B};
    MySerial.write(ask_cmd, 8);
    int i = 0;

    while (MySerial.available() > 0 && i < 80)
    {
        resp[i] = MySerial.read();
        i++;

        yield();
    }
    Serial.print("Answer Length:");
    Serial.println(i);

    for (int j = 0; j < 19; j++)
    {
        Serial.print((int)resp[j]);
        Serial.print("  ");
    }
    Serial.print("\n");

    humidity = CaculateValue((int)resp[3], (int)resp[4]);
    humidity_value = humidity * 0.1;

    tem = CaculateValue((int)resp[5], (int)resp[6]);
    tem_value = tem * 0.1;
    tem_value = c2f(tem_value);
}

void value_log()
{
    Serial.print("humidity_value:");
    Serial.println(humidity_value);
    Serial.print("tem_value:");
    Serial.println(tem_value);

#ifdef SENSOR_5_PIN
    Serial.print("ph_value:");
    Serial.println(ph_value);

    Serial.print("N= ");
    Serial.print(N_value);
    Serial.println(" mg/kg");
    Serial.print("P= ");
    Serial.print(P_value);
    Serial.println(" mg/kg");
    Serial.print("K= ");
    Serial.print(K_value);
    Serial.println(" mg/kg");

#endif
}

int CaculateValue(int x, int y)
{
    int t = 0;
    t = x * 256;
    t = t + y;
    return t;
}

void print_wakeup_reason()
{
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        Serial.println("Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        Serial.println("Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        Serial.println("Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        Serial.println("Wakeup caused by ULP program");
        break;
    default:
        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
        break;
    }
}

float c2f(float c_temp)
{
    return c_temp * 9.0 / 5.0 + 32;
}