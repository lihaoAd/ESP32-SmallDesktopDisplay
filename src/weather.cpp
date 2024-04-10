#include <HTTPClient.h>
#include <TimeLib.h>
#include "config.h"
#include "weather.h"
#include <ArduinoJson.h>
#include "store.h"

WiFiClient wificlient;
static Weather weather;

void _parseWeaterResponse(const String &str)
{
  if (str == nullptr)
  {
    return;
  }

  DynamicJsonDocument doc(1024);

  int indexStart = str.indexOf("weatherinfo\":");
  int indexEnd = str.indexOf("};var alarmDZ");

  String jsonCityDZ = str.substring(indexStart + 13, indexEnd);
#if DEBUG
  Serial.println(jsonCityDZ);
#endif
  deserializeJson(doc, jsonCityDZ);
  JsonObject dz = doc.as<JsonObject>();
  // 今日天气情况
  weather.weatherToday = dz["weather"].as<String>();

  indexStart = str.indexOf("dataSK =");
  indexEnd = str.indexOf(";var dataZS");
  String jsonDataSK = str.substring(indexStart + 8, indexEnd);
#if DEBUG
  Serial.println(jsonDataSK);
#endif
  deserializeJson(doc, jsonDataSK);
  JsonObject sk = doc.as<JsonObject>();
  // 今日摄氏度
  weather.temp = sk["temp"].as<u8_t>();
  // 今日湿度
  weather.huminum = sk["SD"].as<String>();
  // 城市名称
  weather.cityName = sk["cityname"].as<String>();
  // PM2.5空气指数
  weather.aqi = sk["aqi"].as<uint8_t>();
  // 实时天气
  weather.weather = sk["weather"].as<String>();
  // 风向
  weather.WD = sk["WD"].as<String>();
  // 风速
  weather.WS = sk["WS"].as<String>();

  weather.weathercode = atoi((sk["weathercode"].as<String>()).substring(1, 3).c_str());

  // 今天的天气温度
  indexStart = str.indexOf("\"f\":[");
  indexEnd = str.indexOf(",{\"fa");
  String jsonFC = str.substring(indexStart + 5, indexEnd);
#if DEBUG
  Serial.println(jsonFC);
#endif
  deserializeJson(doc, jsonFC);
  JsonObject fc = doc.as<JsonObject>();
  // 今日最高温度
  weather.fd = fc["fd"].as<String>();
  // 今日最低温度
  weather.fc = fc["fc"].as<String>();

#if DEBUG
  Serial.printf("weatherToday %s\r\n",weather.weatherToday.c_str());
  Serial.printf("temp %d\r\n",weather.temp);
  Serial.printf("huminum %d\r\n",weather.huminum);
  Serial.printf("cityName %s\r\n",weather.cityName.c_str());
   Serial.printf("aqi %d\r\n",weather.aqi);
   Serial.printf("weather %s\r\n",weather.weather.c_str());
   Serial.printf("WD %s\r\n",weather.WD.c_str());
   Serial.printf("WS %s\r\n",weather.WS.c_str());
   Serial.printf("weathercode %d\r\n",weather.weathercode);
   Serial.printf("fd %s\r\n",weather.fd.c_str());
   Serial.printf("fc %s\r\n",weather.fc.c_str());
#endif
 
}

// 发送HTTP请求并且将服务器响应通过串口输出
void getCityCode(Callback callback)
{
  String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(now());
  // 创建 HTTPClient 对象
  HTTPClient httpClient;

  // 配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(wificlient, URL);

  // 设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

#if DEBUG
  Serial.printf("Send GET request to URL: %s", URL.c_str());
  Serial.println();
#endif

  // 启动连接并发送HTTP请求
  int httpCode = httpClient.GET();

  // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {
    String str = httpClient.getString();

#if DEBUG
    Serial.printf("Reponsed GET request code: 200,String:%s", str.c_str());
    Serial.println();
#endif

    int aa = str.indexOf("id=");
    if (aa > -1)
    {
      strcpy(AppStore.cityCode, str.substring(aa + 4, aa + 4 + 9).c_str());
#if DEBUG
      Serial.printf("CityCode: %s", AppStore.cityCode);
      Serial.println();
#endif
      getCityWeater(callback);
    }
    else
    {
#if DEBUG
      Serial.printf("获取城市代码失败");
      Serial.println();
#endif
    }
  }
  else
  {
    Serial.println("请求城市代码错误：");
    Serial.println(httpCode);
    if (callback != NULL)
    {
      callback(&weather);
    }
  }

  // 关闭与服务器连接
  httpClient.end();
}

// 获取城市天气
void getCityWeater(Callback callback)
{
  // String URL = "http://d1.weather.com.cn/dingzhi/" + cityCode + ".html?_="+String(now());//新
  String URL = "http://d1.weather.com.cn/weather_index/" + String(AppStore.cityCode) + ".html?_=" + String(now()); // 原来
  // 创建 HTTPClient 对象
  HTTPClient httpClient;

  // httpClient.begin(URL);
  httpClient.begin(wificlient, URL); // 使用新方法

  // 设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

#if DEBUG
  Serial.printf("Send GET request: %s", URL.c_str());
  Serial.println();
#endif

  // 启动连接并发送HTTP请求
  int httpCode = httpClient.GET();

  // 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {
    String str = httpClient.getString();
#if DEBUG
    //Serial.printf("Reponsed GET String : %s", str.c_str());
    Serial.println();
#endif

    _parseWeaterResponse(str);
    if (callback != NULL)
    {
      callback(&weather);
    }
  }
  else
  {
#if DEBUG
    Serial.printf("请求城市天气错误,Code:%d", httpCode);
    Serial.println();
#endif
    if (callback != NULL)
    {
      callback(&weather);
    }
  }

  // 关闭与服务器连接
  httpClient.end();
}