#ifndef NTP_TIME_H
#define NTP_TIME_H
#include <TimeLib.h>
#include <WiFi.h>

time_t getNtpTime();

void sendNTPpacket(IPAddress &address);

#endif;