#ifndef WEATHER_H
#define WEATHER_H
#include <WString.h>


struct Weather
{
    /**
     * {
    "weatherinfo": {
       "city": "淮南",
       "cityname": "huainan",
       "temp": "15",
       "tempn": "12",
       "weather": "多云转阴",
       "wd": "东北风转东南风",
       "ws": "<3级",
       "weathercode": "d1",
       "weathercoden": "n2",
       "fctime": "202404080800"
    }
    }



    {
    "nameen": "huainan",
    "cityname": "淮南",
    "city": "101220401",
    "temp": "19.4",
    "tempf": "66.9",
    "WD": "东风",
    "wde": "E",
    "WS": "2级",
    "wse": "9km\/h",
    "SD": "65%",
    "sd": "65%",
    "qy": "1009",
    "njd": "24km",
    "time": "13:35",
    "rain": "0",
    "rain24h": "0",
    "aqi": "38",
    "aqi_pm25": "38",
    "weather": "多云",
    "weathere": "Cloudy",
    "weathercode": "d01",
    "limitnumber": "",
    "date": "04月08日(星期一)"
    }
     *
     *
     *
    */
    // 今日天气
    String weatherToday;

    // 当前摄氏度
    uint8_t temp;
    // 湿度
    String huminum;

    // 城市名
    String cityName;

    // PM2.5空气指数
    uint8_t aqi;

    // 实时天气
    String weather;

    // 风向
    String WD;

    // 风速
    String WS;

    uint8_t weathercode;

    // 最高温度
    String fd;

    // 最低温度
    String fc;
};

typedef void (*Callback)(void *);

void getCityCode(Callback callback);
void getCityWeater(Callback callback);

#endif