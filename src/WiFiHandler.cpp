#include "WiFiHandler.h"
#include <WiFiMulti.h>
#include <time.h>

WiFiMulti wifiMulti;

String ssid = "BS_NET";
String password = "28062024";

void setupWiFi()
{
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(ssid.c_str(), password.c_str());

    Serial.println("Łączenie z siecią Wi-Fi...");
    while (wifiMulti.run() != WL_CONNECTED)
    {
        Serial.println("Oczekiwanie na połączenie z Wi-Fi...");
        delay(1000);
    }

    Serial.println("Połączono z Wi-Fi!");
    Serial.print("Adres IP: ");
    Serial.println(WiFi.localIP());

    configTime(0, 0, "pool.ntp.org");              // Bez offsetu, użyjemy TZ
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // TZ dla Europy Środkowej z DST
    tzset();
}
