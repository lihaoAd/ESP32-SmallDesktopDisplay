#ifndef CONFIG_H
#define CONFIG_H

#define ON 1
#define OFF 0
#define DEBUG 1

#define Animate_Choice 1      //动图选择：1,太空人图片 2,胡桃
#define TMS 1000              //一千毫秒
#define FPS_ANIM  10          // gif帧率
#define WM_EN 1               // WEB配网使能标志位----WEB配网打开后会默认关闭smartconfig功能
#define DHT_EN 1              //设定DHT11温湿度传感器使能标志
#define SD_FONT_YELLOW 0xD404 // 黄色字体颜色
#define SD_FONT_WHITE 0xFFFF  // 黄色字体颜色
#define timeY 82 // 定义高度
#define UDP_LOCAL_PORT  8000
// 配置热点
#define WEB_AP_NAME "ESP32AP"
#define WEB_AP_PASSWORD "393592730"

#define SERIAL_DEBUG_EN 1
#endif
