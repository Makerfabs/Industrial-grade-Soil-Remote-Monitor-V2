#include "web_server.h"

extern char sensor_id[NVS_DATA_LENGTH];
extern char sleep_time[NVS_DATA_LENGTH];

extern float humidity_value;
extern float tem_value;
extern float ph_value;
extern int P_value;
extern int N_value;
extern int K_value;

// Web page
const char *mainPage =
    "<title>Makerfabs</title>"
    "<h1>Makerfabs</h1><br><h2>RS485-LoRa Wireless Station</h2><br>"
    "Click <a href=\"/setPage\">Set Page.</a><br>"
    "Click <a href=\"/over\">Finish Set.</a><br>";

const char *setFailPage =
    "<title>Makerfabs</title>"
    "<h1>Makerfabs</h1><br><h2>Set Fail, input NULL</h2><br>"
    "<a href=\"/\">Back to main page.</a><br>";

const char *setOverPage =
    "<title>Makerfabs</title>"
    "<h1>Makerfabs</h1><br><h2>Set Success</h2><br><h2>Restarting...</h2><br>";

WiFiServer server(80);

void wifi_init()
{

    // 3秒内拉低BUTTON_PIN则恢复出场设置并重启
    Serial.println("Check BUTTON_PIN");

    long start_time = millis();
    while ((millis() - start_time) < 3000)
    {
        if (digitalRead(BUTTON_PIN) == LOW)
        // if (1)
        {
            config_check();

            WiFi.disconnect();

            String AP_name = "";
            AP_name = AP_name + AP_SSID + get_uid();
            ap_init(AP_name, AP_PWD);

            Serial.printf("Please connect :");
            Serial.println(AP_name);
            Serial.printf("Password is :");
            Serial.println(AP_PWD);
            Serial.println("And visit 192.168.4.1 to set sensor.");

            while (wifi_config_server() != SUCCESS)
            {
            }

            delay(3000);
            esp_restart();
        }
        Serial.printf(".");
        delay(100);
    }

    Serial.println("WiFi init over.");

    // 无操作则检查存储
    config_check();
}

void ap_init(String ssid, String password)
{
    WiFi.softAP(ssid.c_str(), password.c_str());

    IPAddress myIP = WiFi.softAPIP();
    Serial.printf("AP IP address: ");
    Serial.println(myIP);
    server.begin();
}

int wifi_config_server()
{

    WiFiClient client = server.available(); // listen for incoming clients

    if (client) // if you get a client,
    {
        Serial.println("---------------------------------------------------");
        Serial.println("New Client.");
        String currentLine = "";
        while (client.connected())
        { // loop while the client's connected
            if (client.available())
            {
                char c = client.read();
                Serial.write(c);

                // PAGE:192.168.4.1
                if (c == '\n')
                { // if the byte is a newline character

                    if (currentLine.length() == 0)
                    {
                        String temp;
                        temp = sensor_read();

                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        main_page_html(&client);

                        client.stop();

                        return ERROR;
                    }
                    else
                    {
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {
                    currentLine += c;
                }

                // API:保存设置
                if (currentLine.endsWith("GET /saveConfig"))
                {
                    String get_request = "";

                    // read GET next line
                    while (1)
                    {
                        char c_get = client.read();
                        Serial.write(c_get);
                        if (c_get == '\n')
                        {
                            break;
                        }
                        else
                        {
                            get_request += c_get;
                        }
                    }
                    Serial.println(get_request);

                    char id_temp[NVS_DATA_LENGTH] = "";
                    char sleep_temp[NVS_DATA_LENGTH] = "";

                    if (parse_request(get_request, id_temp, sleep_temp) == ERROR)
                    {
                        Serial.println("Wrong input");
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        client.println(setFailPage);
                        client.stop();

                        return ERROR;
                    }

                    else
                    {
                        record_id(id_temp);
                        check_id(sensor_id);
                        record_sleep_time(sleep_temp);
                        check_sleep_time(sleep_time);

                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        client.println(setOverPage);
                        client.stop();

                        return SUCCESS;
                    }
                }
            }
        }
        // close the connection:
        client.stop();
        Serial.println("Client Disconnected.");
    }
    return ERROR;
}

// 检测存储，如果没有默认配置则自动写入
void config_check()
{
    Serial.println("Check Saved Config");

    if (check_id(sensor_id) == SUCCESS)
    {
        Serial.println("Got sensor id");
    }
    else
    {
        Serial.println("No sensor id, set default");
        record_id(DEFAULT_SENSOR_ID);
        check_id(sensor_id);
        Serial.println("Set over");
    }

    if (check_sleep_time(sleep_time) == SUCCESS)
    {
        Serial.println("Got sleep time");
    }
    else
    {
        Serial.println("No sleep time, set default");
        record_sleep_time(DEFAULT_SLEEP_TIME);
        check_sleep_time(sleep_time);
        Serial.println("Set over");
    }

    Serial.println("Now config:");
    Serial.println("ID:");
    Serial.println(sensor_id);
    Serial.println("Sleep Time:");
    Serial.println(sleep_time);
}

int parse_request(String request, char *id, char *sleep)
{
    //?id=111&sleep=222 HTTP/1.1

    int id_add = 0;
    int sleep_add = 0;
    int end_add = 0;

    id_add = request.indexOf("id");
    sleep_add = request.indexOf("&sleep");
    end_add = request.indexOf(' ');

    if ((sleep_add - id_add) == 3)
        return ERROR;
    if ((end_add - sleep_add) == 7)
        return ERROR;

    strcpy(id, request.substring(id_add + 3, sleep_add).c_str());
    strcpy(sleep, request.substring(sleep_add + 7, end_add).c_str());

    return SUCCESS;
}

void main_page_html(WiFiClient *client)
{
    client->print("<title>Makerfabs</title>");
    client->print("<h1>Makerfabs</h1><br><h2>RS485-LoRa Wireless Station</h2><br>");
    client->print("<h2>Current CONFIG</h2><br>");
    client->print("<table><tr><td>Sensor id</td><td>");
    client->print(sensor_id);
    client->print("</td></tr>");
    client->print("<td>Interval time(Sec)</td><td>");
    client->print(sleep_time);
    client->print("</td></tr></table>");
    client->print("<h2>New CONFIG</h2><br>");
    client->print("<form action=\"/saveConfig\">");
    client->print("<table><tr><td>Sensor ID</td>");
    client->print("<td><input type=\"text\" name=\"id\"></td></tr>");
    client->print("<tr><td>Interval Time(Sec)</td>");
    client->print("<td><input type=\"text\" name=\"sleep\"></td></tr></table>");
    client->print("<input type=\"submit\" value=\"Setup and Restart\"></form>");

    client->print("<h2>Data</h2><br>");
    client->print("<table border=\"2\"><tr><td>Temp(F)</td><td>RH(%)</td><td>PH</td><td>N(mg/Kg)</td>");
    client->print("<td>P(mg/Kg)</td><td>K(mg/Kg)</td></tr><tr><td>");
    client->print((int)tem_value);
    client->print("</td><td>");
    client->print((int)humidity_value);
    client->print("</td><td>");
    client->print((int)ph_value);
    client->print("</td><td>");
    client->print((int)N_value);
    client->print("</td><td>");
    client->print((int)P_value);
    client->print("</td><td>");
    client->print((int)K_value);
    client->print("</td></tr></table>");

    client->print("<br>The time from starting to sending is about 24 seconds.");
    client->print("\"Interval time\" refers to the time between the last transmission and the next startup.<br>");
    // client->print();
    client->println();
}

String get_uid()
{
    // uint64_t chipid;
    uint32_t chipid = 0;
    char c[20];

    // chipid = ESP.getEfuseMac();
    for (int i = 0; i < 17; i = i + 8)
    {
        chipid |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    sprintf(c, "%08X", (uint32_t)chipid);

    return (String)c;
}