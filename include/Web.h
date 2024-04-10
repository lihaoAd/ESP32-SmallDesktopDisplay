#ifndef WEB_H
#define WEB_H
#include <WString.h>

extern void resetWifiSettings();
extern void webconfig(void (*fun)());
extern String getParam(String name);

#endif
