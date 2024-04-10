#include <ArduinoJson.h>
#include <TimeLib.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <EEPROM.h>  //内存
#include <Button2.h> //按钮库
#include <Thread.h>
#include <StaticThreadController.h>
#include "weather.h"
#include "config.h"                //配置文件
#include "weatherNum/weatherNum.h" //天气图库
#include "anim.h"                  // 动画模块
#include "store.h"
#include "ntp_time.h"
#include "font/ZdyLwFont_20.h"  //字体库
#include "font/timeClockFont.h" //字体库
#include "img/temperature.h"    //温度图标
#include "img/humidity.h"       //湿度图标
#include <esp_wifi.h>
#if WM_EN
#include <Web.h>
#endif

#define Version "SDD V1.4.3"

// DHT11温湿度传感器
#if DHT_EN
#include "DHT.h"
#define DHTPIN 4 // 定义引脚
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
#endif

// 定义按钮引脚
Button2 Button_sw1 = Button2(25);

// LCD屏幕相关设置
TFT_eSPI tft = TFT_eSPI(); // 引脚请自行配置tft_espi库中的 User_Setup.h文件
TFT_eSprite clk = TFT_eSprite(&tft);
int currentIndex = 0;
TFT_eSprite clkb = TFT_eSprite(&tft);

void weatherData(void *data);
void LCD_reflash();
void saveWificonfig();   // wifi ssid，psw保存到eeprom
void deletewificonfig(); // 删除原有eeprom中的信息
void refresh_AnimatedImage();
void reflashTime();
void openWifi();
void reflashBanner();
void wifi_reset(Button2 &btn); // WIFI重设
void setupUdp();
void esp_restart(Button2 &btn);

// LCD背光引脚
#define LCD_BL_PIN 16
// 背景色
#define bgColor TFT_BLACK

uint8_t Wifi_EN = 1;         // WIFI模块启动  1：打开    0：关闭
uint8_t UpdateWeater_en = 0; // 更新时间标志位
int prevTime = 0;            // 滚动显示更新标志位
bool updateTime_en = true;
time_t prevDisplay = 0;       // 显示时间显示记录
int Amimate_reflash_Time = 0; // 更新时间记录

WeatherNum wrat;

uint32_t targetTime = 0;

uint8_t tempnum = 0;  // 温度百分比
int huminum = 0;      // 湿度百分比
int tempcol = 0xffff; // 温度显示颜色
int humicol = 0xffff; // 湿度显示颜色

float duty = 0;

// 创建时间更新函数线程
Thread reflash_time = Thread();
// 创建副标题切换线程
Thread reflash_Banner = Thread();
// 创建恢复WIFI链接
Thread reflash_openWifi = Thread();
// 创建动画绘制线程
Thread reflash_Animate = Thread();
// 创建协程池
StaticThreadController<4> controller(&reflash_time, &reflash_Banner, &reflash_openWifi, &reflash_Animate);
// 联网后所有需要更新的数据
Thread WIFI_reflash = Thread();

// 星期
String week()
{
  String wk[7] = {"日", "一", "二", "三", "四", "五", "六"};
  String s = "周" + wk[weekday() - 1];
  return s;
}

// 月日
String monthDay()
{
  String s = String(month());
  s = s + "月" + day() + "日";
  return s;
}

void saveWificonfig()
{
  AppStore.storeValue(wifi_ssid_addr, AppStore.stassid, sizeof(AppStore.stassid));
  AppStore.storeValue(wifi_pawd_addr, AppStore.stapsw, sizeof(AppStore.stapsw));
  AppStore.commit();
}

// TFT屏幕输出函数
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
  if (y >= tft.height())
    return 0;
  tft.pushImage(x, y, w, h, bitmap);
  // Return 1 to decode next block
  return 1;
}

// 进度条函数
byte loadNum = 6;

void loading(byte delayTime) // 绘制进度条
{
  clk.setColorDepth(8);
  clk.createSprite(200, 100);
  clk.fillSprite(TFT_BLACK);
  clk.drawRoundRect(0, 0, 200, 16, 8, TFT_WHITE);
  clk.fillRoundRect(3, 3, loadNum, 10, 5, TFT_WHITE);
  clk.setTextDatum(CC_DATUM); // 设置文本数据
  clk.setTextColor(TFT_GREEN, TFT_BLACK);
  clk.drawString("Connecting to WiFi......", 100, 40, 2);
  clk.setTextColor(TFT_WHITE, TFT_BLACK);
  clk.pushSprite(20, 120); // 窗口位置
  clk.deleteSprite();
  loadNum += 1;
  delay(delayTime);
}

// 湿度图标显示函数
void humidityWin()
{
  clk.setColorDepth(8);
  huminum = huminum / 2;
  clk.createSprite(52, 6);
  clk.fillSprite(TFT_BLACK);
  clk.drawRoundRect(0, 0, 52, 6, 3, TFT_WHITE);
  clk.fillRoundRect(1, 1, huminum, 4, 2, humicol);
  clk.pushSprite(45, 222);
  clk.deleteSprite();
}

// 温度图标显示函数
void tempWin()
{
  clk.setColorDepth(8);
  clk.createSprite(52, 6);
  clk.fillSprite(TFT_BLACK);
  clk.drawRoundRect(0, 0, 52, 6, 3, TFT_WHITE);
  clk.fillRoundRect(1, 1, tempnum, 4, 2, tempcol);
  clk.pushSprite(45, 192);
  clk.deleteSprite();
}

#if DHT_EN
// 外接DHT11温湿度传感器，显示数据
void indoorTem()
{
  // 摄氏温度
  float t = dht.readTemperature();
  // 湿度
  float h = dht.readHumidity();
  String s = "温湿";
  /***绘制相关文字***/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);

  // 位置
  clk.createSprite(58, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(s, 29, 16);
  clk.pushSprite(172, 150);
  clk.deleteSprite();

  // 温度
  clk.createSprite(60, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawFloat(t, 1, 20, 13);
  clk.drawString("℃", 50, 13);
  clk.pushSprite(170, 184);
  clk.deleteSprite();

  // 湿度
  clk.createSprite(60, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawFloat(h, 1, 20, 13);
  clk.drawString("%", 50, 13);
  clk.pushSprite(170, 214);
  clk.deleteSprite();
}
#endif

#if !WM_EN
// 微信配网函数
void smartConfig(void)
{
  WiFi.mode(WIFI_STA); // 设置STA模式
  tft.pushImage(0, 0, 240, 240, qr);
  Serial.println("\r\nWait for Smartconfig...");
  WiFi.beginSmartConfig(); // 开始SmartConfig，等待手机端发出用户名和密码
  while (true)
  {
    Serial.print(".");
    delay(100);                 // wait for a second
    if (WiFi.smartConfigDone()) // 配网成功，接收到SSID和密码
    {
#if DEBUG
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
#endif
      break;
    }
  }
  loadNum = 194;
}
#endif

// 强制屏幕刷新
void LCD_reflash()
{
  reflashTime();
  reflashBanner();
  openWifi();
}

bool serialRecvFlag = false;
void onSerialRecv()
{
  serialRecvFlag = true;
}
// 串口调试设置函数
void Serial_set()
{

  uint8_t SMOD = 0;
  if (Serial.available() <= 0)
    return;

  String incomingByte = "";
  while (Serial.available() > 0)
  {
    incomingByte += char(Serial.read());
  }

  SMOD = atoi(incomingByte.substring(0, 1).c_str());
  incomingByte = incomingByte.substring(2);
  if (SMOD == 1) // 设置1亮度设置
  {
    int LCDBL = atoi(incomingByte.c_str());
    if (LCDBL >= 0 && LCDBL <= 100)
    {
      AppStore.LCD_BL_PWM = LCDBL;
      AppStore.storeValue(BL_addr, LCDBL);
      AppStore.commit();
      Serial.printf("LCDBL %d\r\n", LCDBL);
      ledcWrite(0, pow(2, 8) * AppStore.LCD_BL_PWM / 100);
    }
  }
  else if (SMOD == 2) // 设置2地址设置
  {
    String cityCode = incomingByte;
    if (cityCode.length() == 9)
    {
      AppStore.storeValue(CC_addr, cityCode);
      getCityWeater(weatherData); // 更新城市天气
    }
    else
    {
      Serial.println("城市代码调整为：自动");
      getCityCode(weatherData); // 获取城市代码
    }
  }
  else if (SMOD == 3) // 设置3屏幕显示方向
  {
    int RoSet = atoi(incomingByte.c_str());
    if (RoSet >= 0 && RoSet <= 3)
    {
      // 保存旋转方向
      AppStore.storeValue(Ro_addr, RoSet);
      AppStore.commit();
      tft.setRotation(RoSet);
      tft.fillScreen(TFT_BLACK);
      updateTime_en = true;
      LCD_reflash(); // 屏幕刷新程序
      UpdateWeater_en = 1;

      TJpgDec.drawJpg(15, 183, temperature, sizeof(temperature)); // 温度图标
      TJpgDec.drawJpg(15, 213, humidity, sizeof(humidity));       // 湿度图标
    }
    else
    {
      Serial.println("屏幕方向值错误，请输入0-3内的值");
    }
  }
  else if (SMOD == 4) // 设置天气更新时间
  {
    int wtup = atoi(incomingByte.c_str());
    if (wtup >= 1 && wtup <= 60)
    {
      AppStore.updateweatherTime = wtup;
    }
    else
      Serial.println("更新时间太长，请重新设置（1-60）");
  }
  else
  {

    delay(2);
    if (SMOD == 1)
      Serial.println("请输入亮度值，范围0-100");
    else if (SMOD == 2)
      Serial.println("请输入9位城市代码，自动获取请输入0");
    else if (SMOD == 3)
    {
      Serial.println("请输入屏幕方向值，");
      Serial.println("0-USB接口朝下");
      Serial.println("1-USB接口朝右");
      Serial.println("2-USB接口朝上");
      Serial.println("3-USB接口朝左");
    }
    else if (SMOD == 4)
    {
      Serial.print("当前天气更新时间：");
      Serial.print(AppStore.updateweatherTime);
      Serial.println("分钟");
      Serial.println("请输入天气更新时间（1-60）分钟");
    }
    else if (SMOD == 5)
    {
      Serial.println("重置WiFi设置中......");
      delay(10);
#if WM_EN
      resetWifiSettings();
#endif
      deletewificonfig();
      delay(10);
      Serial.println("重置WiFi成功");

      ESP.restart();
    }
    else
    {
      Serial.println("");
      Serial.println("请输入需要修改的代码：");
      Serial.println("亮度设置输入        1");
      Serial.println("地址设置输入        2");
      Serial.println("屏幕方向设置输入    3");
      Serial.println("更改天气更新时间    4");
      Serial.println("重置WiFi(会重启)    5");
      Serial.println("");
    }
  }
}

#if WM_EN
// WEB配网LCD显示函数
void webWin()
{
  uint8_t spriteWidth = 200;
  clk.setColorDepth(8);
  clk.createSprite(spriteWidth, 60);
  clk.fillSprite(TFT_BLACK);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_GREEN, TFT_BLACK);
  clk.drawString("WiFi Connect Fail!", 100, 10, 2);
  clk.setTextDatum(CL_DATUM);
  int16_t ssidTextWidth = clk.textWidth("SSID : ");
  int16_t apTextWidth = clk.textWidth(WEB_AP_NAME);
  uint8_t half = (ssidTextWidth + apTextWidth) / 2;
  // 让文字居中而已
  if (apTextWidth > ssidTextWidth)
  {
    clk.drawString("SSID : ", spriteWidth / 2 - half, 40, 2);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString(WEB_AP_NAME, spriteWidth / 2 - (apTextWidth - half), 40, 2);
  }
  else
  {
    clk.drawString("SSID : ", spriteWidth / 2 - half, 40, 2);
    clk.setTextColor(TFT_WHITE, 0x0000);
    clk.drawString(WEB_AP_NAME, spriteWidth / 2 + ssidTextWidth - half, 40, 2);
  }
  clk.pushSprite(20, 50);
  clk.deleteSprite();
}

// 删除原有eeprom中的信息
void deletewificonfig()
{
  AppStore.deleteValue(wifi_ssid_addr, sizeof(AppStore.stassid));
  AppStore.deleteValue(wifi_pawd_addr, sizeof(AppStore.stapsw));
  AppStore.commit();
}
#endif

String scrollText[7];

// 天气信息写到屏幕上
void weatherData(void *data)
{
  struct Weather *weather = (struct Weather *)data;

  /***绘制相关文字***/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);

  // 当前温度
  clk.createSprite(58, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(String(weather->temp) + "℃", 28, 13);
  clk.pushSprite(100, 184);
  clk.deleteSprite();
  tempnum = weather->temp;
  tempnum = tempnum + 10;
  if (tempnum < 10)
    tempcol = 0x00FF;
  else if (tempnum < 28)
    tempcol = 0x0AFF;
  else if (tempnum < 34)
    tempcol = 0x0F0F;
  else if (tempnum < 41)
    tempcol = 0xFF0F;
  else if (tempnum < 49)
    tempcol = 0xF00F;
  else
  {
    tempcol = 0xF00F;
    tempnum = 50;
  }
  tempWin();

  // 湿度
  clk.createSprite(58, 24);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(weather->huminum, 28, 13);
  clk.pushSprite(100, 214);
  clk.deleteSprite();
  huminum = atoi(weather->huminum.substring(0, 2).c_str());
  if (huminum > 90)
    humicol = 0x00FF;
  else if (huminum > 70)
    humicol = 0x0AFF;
  else if (huminum > 40)
    humicol = 0x0F0F;
  else if (huminum > 20)
    humicol = 0xFF0F;
  else
    humicol = 0xF00F;

  humidityWin();

  // 城市名称
  clk.createSprite(94, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(weather->cityName, 44, 16);
  clk.pushSprite(15, 15);
  clk.deleteSprite();

  // PM2.5空气指数
  uint16_t pm25BgColor = tft.color565(156, 202, 127); // 优
  String aqiTxt = "优";
  int pm25V = weather->aqi;
  if (pm25V > 200)
  {
    pm25BgColor = tft.color565(136, 11, 32); // 重度
    aqiTxt = "重度";
  }
  else if (pm25V > 150)
  {
    pm25BgColor = tft.color565(186, 55, 121); // 中度
    aqiTxt = "中度";
  }
  else if (pm25V > 100)
  {
    pm25BgColor = tft.color565(242, 159, 57); // 轻
    aqiTxt = "轻度";
  }
  else if (pm25V > 50)
  {
    pm25BgColor = tft.color565(247, 219, 100); // 良
    aqiTxt = "良";
  }
  clk.createSprite(56, 24);
  clk.fillSprite(bgColor);
  clk.fillRoundRect(0, 0, 50, 24, 4, pm25BgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(0x0000);
  clk.drawString(aqiTxt, 25, 13);
  clk.pushSprite(104, 18);
  clk.deleteSprite();

  scrollText[0] = "实时天气 " + weather->weather;
  scrollText[1] = "空气质量 " + aqiTxt;
  scrollText[2] = "风向 " + weather->WD + weather->WS;
  // 天气图标
  wrat.printfweather(170, 15, weather->weathercode);

  // 左上角滚动字幕
  // 横向滚动方式
  scrollText[3] = "今日" + weather->weatherToday;

  scrollText[4] = "最低温度" + weather->fd + "℃";
  scrollText[5] = "最高温度" + weather->fc + "℃";
  clk.unloadFont();
}

void scrollBanner()
{
  // if(millis() - prevTime > 2333) //3秒切换一次
  //  if(second()%2 ==0&& prevTime == 0)
  //  {
  if (scrollText[currentIndex])
  {
    clkb.setColorDepth(8);
    clkb.loadFont(ZdyLwFont_20);
    clkb.createSprite(150, 30);
    clkb.fillSprite(bgColor);
    clkb.setTextWrap(false);
    clkb.setTextDatum(CC_DATUM);
    clkb.setTextColor(TFT_WHITE, bgColor);
    clkb.drawString(scrollText[currentIndex], 74, 16);
    clkb.pushSprite(10, 45);

    clkb.deleteSprite();
    clkb.unloadFont();
    (++currentIndex) %= 5;
  }
  prevTime = 1;
  //  }
}

// 用快速线方法绘制数字
void drawLineFont(uint32_t _x, uint32_t _y, uint32_t _num, uint32_t _size, uint32_t _color)
{
  uint32_t fontSize;
  const LineAtom *fontOne;
  // 小号(9*14)
  if (_size == 1)
  {
    fontOne = smallLineFont[_num];
    fontSize = smallLineFont_size[_num];
    // 绘制前清理字体绘制区域
    tft.fillRect(_x, _y, 9, 14, TFT_BLACK);
  }
  // 中号(18*30)
  else if (_size == 2)
  {
    fontOne = middleLineFont[_num];
    fontSize = middleLineFont_size[_num];
    // 绘制前清理字体绘制区域
    tft.fillRect(_x, _y, 18, 30, TFT_BLACK);
  }
  // 大号(36*90)
  else if (_size == 3)
  {
    fontOne = largeLineFont[_num];
    fontSize = largeLineFont_size[_num];
    // 绘制前清理字体绘制区域
    tft.fillRect(_x, _y, 36, 90, TFT_BLACK);
  }
  else
    return;

  for (uint32_t i = 0; i < fontSize; i++)
  {
    tft.drawFastHLine(fontOne[i].xValue + _x, fontOne[i].yValue + _y, fontOne[i].lValue, _color);
  }
}

int Hour_sign = 60;
int Minute_sign = 60;
int Second_sign = 60;
// 日期刷新
void digitalClockDisplay()
{
  // 时钟刷新,输入1强制刷新
  int now_hour = hour();     // 获取小时
  int now_minute = minute(); // 获取分钟
  int now_second = second(); // 获取秒针
  // 小时刷新
  if ((now_hour != Hour_sign) || updateTime_en)
  {
    drawLineFont(20, timeY, now_hour / 10, 3, SD_FONT_WHITE);
    drawLineFont(60, timeY, now_hour % 10, 3, SD_FONT_WHITE);
    Hour_sign = now_hour;
  }
  // 分钟刷新
  if ((now_minute != Minute_sign) || updateTime_en)
  {
    drawLineFont(101, timeY, now_minute / 10, 3, SD_FONT_YELLOW);
    drawLineFont(141, timeY, now_minute % 10, 3, SD_FONT_YELLOW);
    Minute_sign = now_minute;
  }
  // 秒针刷新
  if ((now_second != Second_sign) || updateTime_en) // 分钟刷新
  {
    drawLineFont(182, timeY + 30, now_second / 10, 2, SD_FONT_WHITE);
    drawLineFont(202, timeY + 30, now_second % 10, 2, SD_FONT_WHITE);
    Second_sign = now_second;
  }

  updateTime_en = false;

  /***日期****/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);

  // 星期
  clk.createSprite(58, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(week(), 29, 16);
  clk.pushSprite(102, 150);
  clk.deleteSprite();

  // 月日
  clk.createSprite(95, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(monthDay(), 49, 16);
  clk.pushSprite(5, 150);
  clk.deleteSprite();

  clk.unloadFont();
  /***日期****/
}

void esp_restart(Button2 &btn)
{
  ESP.restart();
}

void wifi_reset(Button2 &btn)
{
#if WM_EN
  resetWifiSettings();
#endif
  deletewificonfig();
  delay(10);
  Serial.println("重置WiFi成功");
  ESP.restart();
}

// 更新时间
void reflashTime()
{
  prevDisplay = now();
  digitalClockDisplay();
  prevTime = 0;
}

// 切换天气 or 空气质量
void reflashBanner()
{
#if DHT_EN
  if (AppStore.DHT_img_flag != 0)
    indoorTem();
#endif
  scrollBanner();
}

void configChange()
{
  tft.setRotation(AppStore.LCD_Rotation);
  tft.fillScreen(0x0000);
  webWin();
  loadNum--;
  loading(1);
  ledcWrite(0, pow(2, 8) * AppStore.LCD_BL_PWM / 100);
}

// 所有需要联网后更新的方法都放在这里
void WIFI_reflash_All()
{
  if (Wifi_EN == 1)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WIFI connected");
      getCityWeater(weatherData);
      getNtpTime();

      // todo
      // wifi休眠
      // 其他需要联网的方法写在后面
      Wifi_EN = 0;
    }
    else
    {
#if DEBUG
      Serial.println("WIFI unconnected");
#endif
    }
  }
}

// 打开WIFI
void openWifi()
{
  Serial.println("WIFI reset......");
  // todo
  // 唤醒wifi
  esp_wifi_start();
  Wifi_EN = 1;
}

void setup()
{
  Serial.begin(115200);

  AppStore.init();
  // pinMode(LCD_BL_PIN, OUTPUT);
  // 处理按钮事件
  // Button_sw1.setClickHandler(esp_restart);
  Button_sw1.setLongClickHandler(wifi_reset);

#if DHT_EN
  // 初始化DHT11温湿度传感器
  dht.begin();
#endif

  // PWM调光
  ledcSetup(0, 5000, 8);
  ledcAttachPin(LCD_BL_PIN, 0);
  // 最亮100，精度255
  ledcWrite(0, AppStore.LCD_BL_PWM * pow(2, 8) / 100);

  // analogWrite(LCD_BL_PIN, 1023 - (AppStore.LCD_BL_PWM * 10));

  // LCD屏幕初始化
  tft.begin();
  tft.invertDisplay(1); // 反转所有显示颜色：1反转，0正常
  tft.setRotation(AppStore.LCD_Rotation);
  tft.fillScreen(bgColor);
  tft.setTextColor(TFT_BLACK, bgColor);

  targetTime = millis() + 1000;
  Serial.print("正在连接WIFI:");
  Serial.println(AppStore.stassid);
  WiFi.begin(AppStore.stassid, AppStore.stapsw);

  while (WiFi.status() != WL_CONNECTED)
  {
    loading(30);
    if (loadNum >= 194)
    {
      // wifi连接失败
// 使能web配网后自动将smartconfig配网失效
#if WM_EN
      webWin();
      webconfig(configChange);
#endif

#if !WM_EN
      SmartConfig();
#endif
      break;
    }
  }

  delay(10);
  while (loadNum < 194) // 让动画走完
  {
    loading(1);
  }

  tft.fillScreen(TFT_BLACK); // 清屏
  if (WiFi.status() == WL_CONNECTED)
  {
#if DEBUG
    Serial.printf("SSID:%s", WiFi.SSID().c_str());
    Serial.println();
    Serial.printf("PASSWORD:%s", WiFi.psk().c_str());
    Serial.println();
#endif

    strcpy(AppStore.stassid, WiFi.SSID().c_str()); // 名称复制
    strcpy(AppStore.stapsw, WiFi.psk().c_str());   // 密码复制
    saveWificonfig();
  }
#if DEBUG
  Serial.printf("本地IP:%s", WiFi.localIP().toString().c_str());
  Serial.println();
  Serial.println("启动UDP");
#endif

  setupUdp();

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  if (sizeof(AppStore.cityCode) != 9)
  {
    getCityCode(weatherData);
  }
  else
  {
    getCityWeater(weatherData);
  }

  TJpgDec.drawJpg(15, 183, temperature, sizeof(temperature)); // 温度图标
  TJpgDec.drawJpg(15, 213, humidity, sizeof(humidity));       // 湿度图标

#if DHT_EN
  if (AppStore.DHT_img_flag != 0)
    indoorTem();
#endif

  // esp_wifi_stop();
  // WiFi.forceSleepBegin(); // wifi off
  // Serial.println("WIFI休眠......");
  // todo
  // wifi休眠
  Wifi_EN = 0;

  reflash_time.setInterval(1000); // 设置所需间隔 1000毫秒
  reflash_time.onRun(reflashTime);

  reflash_Banner.setInterval(2 * TMS); // 设置所需间隔 2秒
  reflash_Banner.onRun(reflashBanner);

  reflash_openWifi.setInterval(AppStore.updateweatherTime * 60 * TMS); // 设置所需间隔 ，分钟
  reflash_openWifi.onRun(openWifi);

  reflash_Animate.setInterval(1000 / FPS_ANIM); // 设置帧率
  reflash_openWifi.onRun(refresh_AnimatedImage);
  controller.run();

  // 串口数据回调
  Serial.onReceive(onSerialRecv, false);
}

void refresh_AnimatedImage()
{
#if Animate_Choice

  if (AppStore.DHT_img_flag == 0)
  {
    if (millis() - Amimate_reflash_Time > 100) // x ms切换一次
    {
      const uint8_t *Animate_value; // 指向关键帧的指针
      uint32_t Animate_size;        // 指向关键帧大小的指针
      Amimate_reflash_Time = millis();
      imgAnim(&Animate_value, &Animate_size);
      TJpgDec.drawJpg(160, 160, Animate_value, Animate_size);
    }
  }
#endif
}

void loop()
{
  if (controller.shouldRun())
  {
    controller.run();
  }

  if (serialRecvFlag)
  {
    Serial.println("Serial_set");
    // 串口有数据发过来了
    Serial_set();
    serialRecvFlag = false;
  }

  refresh_AnimatedImage(); // 更新右下角
  WIFI_reflash_All();      // WIFI应用
  Button_sw1.loop();       // 按钮轮询
}