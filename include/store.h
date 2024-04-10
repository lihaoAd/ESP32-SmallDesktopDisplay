#ifndef STORE_H
#define STORE_H

#define ROM_SIZE 1024
// EEPROM参数存储地址位
#define BL_addr 1    // 被写入数据的EEPROM地址编号  1亮度
#define Ro_addr 2    // 被写入数据的EEPROM地址编号  2 旋转方向
#define DHT_addr 3   // 3 DHT使能标志位
#define UPDATEWEATHER_addr 4   // 3 更新天气的时间
#define CC_addr 5   // 被写入数据的EEPROM地址编号  10城市
#define wifi_ssid_addr 15 // 被写入数据的EEPROM地址编号  20wifi-ssid-psw
#define wifi_pawd_addr 47 // 被写入数据的EEPROM地址编号  20wifi-ssid-psw

class Store
{
public:
    uint8_t LCD_Rotation = 0;    // LCD屏幕方向
    uint8_t LCD_BL_PWM = 50;     // 屏幕亮度0-100，默认50
    uint8_t UpdateWeater_en = 0; // 更新时间标志位
    uint8_t prevTime = 0;        // 滚动显示更新标志位
    uint8_t DHT_img_flag = 0;    // DHT传感器使用标志位
    char cityCode[10] = "101090609";
    char stassid[32]; // 定义配网得到的WIFI名长度(最大32字节)
    char stapsw[64];  // 定义配网得到的WIFI密码长度(最大64字节)

    // 更新天气时间，单位:分钟
    uint8_t updateweatherTime = 5;

public:
    Store(/* args */);
    void init();
    void storeValue(int address, uint8_t value);
    void storeValue(int address, String value);
    void storeValue(int address, const char *value,int size);
    void deleteValue(int address);
    void deleteValue(int address, int size);
    bool commit();
    ~Store();
};
extern  Store  AppStore;
#endif

