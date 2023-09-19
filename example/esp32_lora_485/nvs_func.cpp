#include "nvs_func.h"

void record_id(const char *id)
{
    write_nvs("id", id);
}

int check_id(char *id)
{
    return read_nvs("id", id);
}

void record_sleep_time(const char *time)
{
    write_nvs("sleep", time);
}

int check_sleep_time(char *time)
{
    return read_nvs("sleep", time);
}

void record_count(int count)
{
    char temp[20];
    sprintf(temp, "%d", count);
    write_nvs("count", temp);
}

int check_count(int *count)
{
    char temp[20];
    String temp_str = "";

    if (read_nvs("count", temp) == SUCCESS)
    {
        temp_str += temp;
        *count = temp_str.toInt();
        return SUCCESS;
    }
    else
    {
        *count = 0;
        return ERROR;
    }
}
//----------------------------------
// ESP32 写NVS   DEBUG LOG
// 验证过，一般不会失败
void write_nvs(const char *key, const char *value)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    Serial.printf("Opening NVS handle... ");
    nvs_handle my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        Serial.printf("Done\n");

        Serial.printf("Updating %s:%s in NVS ... ", key, value);
        err = nvs_set_str(my_handle, key, value);
        Serial.printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        Serial.printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        Serial.printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        nvs_close(my_handle);
    }
}

// ESP32 读NVS   DEBUG LOG
// 验证过，一般不会失败
int read_nvs(const char *key, char *value)
{
    char saved_value[NVS_DATA_LENGTH];
    size_t save_value_length = NVS_DATA_LENGTH;
    int check_status = 0;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    Serial.printf("Opening NVS handle... \n");
    nvs_handle my_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        Serial.printf("Done\n");
        Serial.printf("Reading %s from NVS ... \n", key);

        err = nvs_get_str(my_handle, key, saved_value, &save_value_length);
        switch (err)
        {
        case ESP_OK:
            Serial.printf("Done\n");
            Serial.printf("Value: %s\n", saved_value);
            Serial.printf("Value length= %d\n", save_value_length);
            strcpy(value, saved_value);
            check_status++;
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            Serial.printf("The value is not initialized yet!\n");
            break;
        default:
            Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        nvs_close(my_handle);
    }

    if (check_status == 1)
        return SUCCESS;
    else
        return ERROR;
}