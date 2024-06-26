
#include <Udp.h>
#include <WiFiUdp.h>
#include "config.h"
#include "ntp_time.h"

// NTP服务器参数
static const char ntpServerName[] = "ntp6.aliyun.com";
#define timeZone 8 // 东八区
WiFiUDP Udp;

void sendNTPpacket(IPAddress &address);

#ifndef NTP_PACKET_SIZE
#define NTP_PACKET_SIZE 48        // NTP时间在消息的前48字节中
#endif

byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing packets

void setupUdp()
{
    Udp.begin(UDP_LOCAL_PORT);
    Serial.println("等待同步...");
    setSyncProvider(getNtpTime);
    setSyncInterval(300);
}

time_t getNtpTime()
{
    IPAddress ntpServerIP; // NTP server's ip address

    while (Udp.parsePacket() > 0)
        ; // discard any previously received packets
    // Serial.println("Transmit NTP Request");
    //  get a random server from the pool
    WiFi.hostByName(ntpServerName, ntpServerIP);
    // Serial.print(ntpServerName);
    // Serial.print(": ");
    // Serial.println(ntpServerIP);
    sendNTPpacket(ntpServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500)
    {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE)
        {
            Serial.println("Receive NTP Response");
            Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
            unsigned long secsSince1900;
            // convert four bytes starting at location 40 to a long integer
            secsSince1900 = (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            // Serial.println(secsSince1900 - 2208988800UL + timeZone * 3600UL);
            return secsSince1900 - 2208988800UL + timeZone * 3600UL;
        }
    }
    Serial.println("No NTP Response :-(");
    return 0; // 无法获取时间时返回0
}

// 向NTP服务器发送请求
void sendNTPpacket(IPAddress &address)
{
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); // NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}