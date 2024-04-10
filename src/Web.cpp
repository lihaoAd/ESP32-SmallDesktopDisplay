
#include "WiFi.h"
#include <WiFiManager.h>
#include "config.h"
#include "store.h"
#include "Web.h"

WiFiManager wm;
void (*configChangeCallback)(void);

void saveParamCallback()
{

#if DEBUG
    Serial.println("[CALLBACK] saveParamCallback fired");
    Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
    Serial.println("PARAM CityCode = " + getParam("CityCode"));
    Serial.println("PARAM LCD BackLight = " + getParam("LCDBL"));
    Serial.println("PARAM WeaterUpdateTime = " + getParam("WeaterUpdateTime"));
    Serial.println("PARAM Rotation = " + getParam("set_rotation"));
    Serial.println("PARAM DHT11_en = " + getParam("DHT11_en"));
#endif

#if DHT_EN
    AppStore.DHT_img_flag = getParam("DHT11_en").toInt();
#endif

    AppStore.updateweatherTime = getParam("WeaterUpdateTime").toInt();
    AppStore.LCD_Rotation = getParam("set_rotation").toInt();
    AppStore.LCD_BL_PWM = getParam("LCDBL").toInt();

    AppStore.storeValue(BL_addr, AppStore.LCD_BL_PWM);
    AppStore.commit();

    AppStore.storeValue(UPDATEWEATHER_addr, AppStore.updateweatherTime);
    AppStore.commit();

    delay(5);

    String cityCode = getParam("CityCode");
    AppStore.storeValue(CC_addr, cityCode);
    AppStore.commit();

    // 屏幕方向
    AppStore.storeValue(Ro_addr, AppStore.LCD_Rotation);
    AppStore.commit();
    delay(5);

    if (configChangeCallback != nullptr)
    {
        configChangeCallback();
        configChangeCallback = nullptr;
    }

#if DHT_EN
    // 是否使用DHT11传感器
    AppStore.storeValue(DHT_addr, AppStore.DHT_img_flag);
    AppStore.commit(); // 保存更改的数据
#endif
}

void resetWifiSettings()
{
    wm.resetSettings(); // wipe settings
}

// WEB配网函数
void webconfig(void (*callback)())
{
    configChangeCallback = callback;
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

    delay(3000);
    wm.resetSettings(); // wipe settings

    // add a custom input field
    // int customFieldLength = 40;

    // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");

    // test custom html input type(checkbox)
    //  new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type

    // test custom html(radio)
    // const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    // new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input

    const char *set_rotation = "<br/><label for='set_rotation'>显示方向设置</label>\
                              <input type='radio' name='set_rotation' value='0' checked> USB接口朝下<br>\
                              <input type='radio' name='set_rotation' value='1'> USB接口朝右<br>\
                              <input type='radio' name='set_rotation' value='2'> USB接口朝上<br>\
                              <input type='radio' name='set_rotation' value='3'> USB接口朝左<br>";
    WiFiManagerParameter custom_rot(set_rotation); // custom html input
    WiFiManagerParameter custom_bl("LCDBL", "屏幕亮度（1-100）", "10", 3);
#if DHT_EN
    WiFiManagerParameter custom_DHT11_en("DHT11_en", "Enable DHT11 sensor", "0", 1);
#endif
    WiFiManagerParameter custom_weatertime("WeaterUpdateTime", "天气刷新时间（分钟）", "10", 3);
    WiFiManagerParameter custom_cc("CityCode", "城市代码", "0", 9);
    WiFiManagerParameter p_lineBreak_notext("<p></p>");

    // wm.addParameter(&p_lineBreak_notext);
    // wm.addParameter(&custom_field);
    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&custom_cc);
    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&custom_bl);
    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&custom_weatertime);
    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&custom_rot);
#if DHT_EN
    wm.addParameter(&p_lineBreak_notext);
    wm.addParameter(&custom_DHT11_en);
#endif
    wm.setSaveParamsCallback(saveParamCallback);

    // custom menu via array or vector
    //
    // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
    // const char* menu[] = {"wifi","info","param","sep","restart","exit"};
    // wm.setMenu(menu,6);
    std::vector<const char *> menu = {"wifi", "restart"};
    wm.setMenu(menu);

    // set dark theme
    wm.setClass("invert");

    // set static ip
    //  wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
    //  wm.setShowStaticFields(true); // force show static ip fields
    //  wm.setShowDnsFields(true);    // force show dns field always

    // wm.setConnectTimeout(20); // how long to try to connect for before continuing
    //  wm.setConfigPortalTimeout(30); // auto close configportal after n seconds
    // wm.setCaptivePortalEnable(false); // disable captive portal redirection
    // wm.setAPClientCheck(true); // avoid timeout if client connected to softap

    // wifi scan settings
    // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
    wm.setMinimumSignalQuality(20); // set min RSSI (percentage) to show in scans, null = 8%
    // wm.setShowInfoErase(false);      // do not show erase button on info page
    // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons

    // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect(WEB_AP_NAME, WEB_AP_PASSWORD); // password protected ap

    while (!res)
        ;
}

String getParam(String name)
{
    // read parameter from server, for customhmtl input
    String value;
    if (wm.server->hasArg(name))
    {
        value = wm.server->arg(name);
    }
    return value;
}
