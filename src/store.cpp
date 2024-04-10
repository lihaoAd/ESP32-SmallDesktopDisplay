#include <EEPROM.h>
#include "store.h"

void Store::init()
{
    EEPROM.begin(ROM_SIZE);

    // 从eeprom读取背光亮度设置
    LCD_BL_PWM = EEPROM.read(BL_addr);
    if (LCD_BL_PWM < 0 || LCD_BL_PWM > 100)
        LCD_BL_PWM = 50; // 给一个默认值

    // 从eeprom读取屏幕方向设置
    LCD_Rotation = EEPROM.read(Ro_addr);
    if (LCD_Rotation < 0 || LCD_Rotation > 3)
        LCD_Rotation = 0;

    updateweatherTime = EEPROM.read(UPDATEWEATHER_addr);

    DHT_img_flag = EEPROM.read(DHT_addr);

    for (unsigned int i = 0; i < sizeof(cityCode); i++)
    {
        cityCode[i] = EEPROM.read(i + CC_addr);
    }

    for (unsigned int i = 0; i < sizeof(stassid); i++)
    {
        stassid[i] = EEPROM.read(i + wifi_ssid_addr);
    }
    for (unsigned int i = 0; i < sizeof(stapsw); i++)
    {
        stapsw[i] = EEPROM.read(i + wifi_pawd_addr);
    }

    Serial.printf("LCD_BL_PWM:%d\r\n", LCD_BL_PWM);
    Serial.printf("LCD_Rotation:%d\r\n", LCD_Rotation);
    Serial.printf("updateweatherTime:%d\r\n", updateweatherTime);
    Serial.printf("DHT_img_flag:%d\r\n", DHT_img_flag);
    Serial.printf("cityCode:%s\r\n", cityCode);
    Serial.printf("Read WiFi Config.....\r\n");
    Serial.printf("SSID:%s\r\n", stassid);
    Serial.printf("PSW:%s\r\n", stapsw);
}

void Store::storeValue(int address, uint8_t value)
{
    EEPROM.write(address, value);
}

void Store::storeValue(int address, String value)
{
    storeValue(address, value.c_str(), value.length());
}

void Store::storeValue(int address, const char *value, int size)
{
    for (size_t i = 0; i < size; i++)
    {
        EEPROM.write(address + i, *(value + i));
    }
}

void Store::deleteValue(int address, int size)
{
    for (unsigned int i = 0; i < size; i++)
    {
        EEPROM.write(address + i, 0);
    }
}

void Store::deleteValue(int address)
{
    EEPROM.write(address, 0);
}

bool Store::commit()
{
    return EEPROM.commit();
}

void restoreValue()
{
    // 从eeprom读取背光亮度设置
    //   if (EEPROM.read(BL_addr) > 0 && EEPROM.read(BL_addr) < 100)
    //     LCD_BL_PWM = EEPROM.read(BL_addr);
    //   // 从eeprom读取屏幕方向设置
    //   if (EEPROM.read(Ro_addr) >= 0 && EEPROM.read(Ro_addr) <= 3)
    //     LCD_Rotation = EEPROM.read(Ro_addr);
}
Store::Store()
{
}
Store::~Store()
{
}

// 全局定义一个
Store AppStore;
