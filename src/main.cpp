#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TM1637Display.h>
#include <ESP32Encoder.h>
#include <WiFiHandler.h>
#include <HandlerRemote.h>
#include "main.h"
#include <RadioStations.h>
#include <HTTPClient.h>

#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25
#define BUTTON_PIN 0   // Przycisk
#define CLK 22         // CLK pin dla TM1637
#define DIO 21         // DIO pin dla TM1637
#define ENCODER_CLK 32 // CLK pin enkodera
#define ENCODER_DT 33  // DT pin enkodera
#define ENCODER_SW 34  // SW (przycisk) enkodera
// #define IR_RECEIVER_PIN 15 // Pin dla odbiornika IR

Audio audio;
TM1637Display display(CLK, DIO);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200, 60000);
ESP32Encoder volumeEncoder;
// IRrecv irrecv(IR_RECEIVER_PIN); // Utworzenie instancji odbiornika IR
// decode_results results;          // Struktura do przechowywania wyników dekodowania

// String ssid = "BS_NET";
// String password = "28062024";

// String radioStations[] = {
//     "http://ml01.cdn.eurozet.pl/mel-ldz.mp3",
//     "http://stream.rcs.revma.com/ypqt40u0x1zuv",
//     "http://s2.radioparty.pl:8000",
//     "https://stream.radioparadise.com/mp3-192",
//     "http://31.192.216.5/rmf_fm",
//     "http://stream4.nadaje.com:15274/live",
//     "https://rs103-krk-cyfronet.rmfstream.pl/rmf_maxxx_knn",
//     "http://ic1.smcdn.pl:8000/2140-1.mp3",
//     "http://stream4.nadaje.com:15274/live",
//     "http://waw.ic.smcdn.pl/4100-1.mp3",
//     "https://radiostream.pl/tuba8942-1.mp3",
//     "https://ic2.smcdn.pl/3990-1.mp3"
// };

const int numberOfStations = sizeof(radioStations) / sizeof(radioStations[0]);
int currentStationIndex = 0;
int currentVolume = 3;

unsigned long lastInteractionTime = 0;
bool volumeMode = true; // true: regulacja głośności, false: przełączanie programów

unsigned long lastDisplay = 0;
bool VolOrProgDisplay = false;

bool apiTriggeredToday = false;

static long lastPosition = 0;

void connectToStation(int index);
void showChannelNumber();
void updateVolume();
void updateStation();
void handleButtonInput();
void triggerAPI(String url);
// void handleRemoteInput(); // Dodanie funkcji do obsługi pilota
void blinkDots();

void setup()
{
    Serial.begin(115200);
    Serial.println("Rozpoczynanie programu...");

    // WiFi.mode(WIFI_STA);
    // wifiMulti.addAP(ssid.c_str(), password.c_str());

    // Serial.println("Łączenie z siecią Wi-Fi...");
    // while (wifiMulti.run() != WL_CONNECTED) {
    //     Serial.println("Oczekiwanie na połączenie z Wi-Fi...");
    //     delay(1000);
    // }

    // Serial.println("Połączono z Wi-Fi!");
    // Serial.print("Adres IP: ");
    // Serial.println(WiFi.localIP());

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW, INPUT_PULLUP);
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(currentVolume);

    display.setBrightness(0x0f);
    // timeClient.begin();
    // timeClient.update();

    volumeEncoder.attachHalfQuad(ENCODER_CLK, ENCODER_DT);
    volumeEncoder.clearCount();

    // irrecv.enableIRIn(); // Uruchomienie odbiornika IR
    setupWiFi();
    timeClient.begin();
    timeClient.setTimeOffset(7200);
    timeClient.update();
    handleRemoteSetup();
    connectToStation(currentStationIndex);
}

void loop()
{
    audio.loop();
    // timeClient.update();

    handleButtonInput();

    handleRemoteInput(); // Sprawdzenie wejścia z pilota

    if (volumeMode)
    {
        updateVolume();
    }
    else
    {
        updateStation(); // Dodaj tę linię do wywołania funkcji updateStation()
    }

    if (millis() - lastDisplay >= 3000)
    {
        VolOrProgDisplay = false;
        volumeMode = true;
    }

    if (!VolOrProgDisplay)
    {
        blinkDots();
    }

    delay(2);

    timeClient.update();
    if (timeClient.isTimeSet())
    {
        int hours = timeClient.getHours();
        int minutes = timeClient.getMinutes();
        if (hours == 4 && minutes == 45)
        {
            if (!apiTriggeredToday)
            {
                triggerAPI();
                apiTriggeredToday = true;
            }
        }
        else
        {
            if (minutes != 45)
            {
                apiTriggeredToday = false;
            }
        }
    }
    delay(5); // Sprawdzenie co minutę
}

// KONIEC LOOPA - PONIŻEJ FUNKCJE POMOCNICZE

void connectToStation(int index)
{
    Serial.print("Łączenie ze stacją: ");
    Serial.println(radioStations[index]);
    audio.connecttohost(radioStations[index].c_str());
}

void showChannelNumber()
{
    const uint8_t SEG_P = 0b01110011;     // Segmenty dla "P"
    const uint8_t SEG_MINUS = 0b01000000; // Segmenty dla "-"
    display.clear();
    display.setSegments(&SEG_P, 1, 0);
    display.setSegments(&SEG_MINUS, 1, 1);

    display.showNumberDecEx(currentStationIndex + 1, 0b01000000, false, 2, 2);
    // display.showNumberDec(currentStationIndex +1);
    lastInteractionTime = millis();
}

void showVolumeValue()
{
    const uint8_t SEG_P = 0b01110011;     // Segmenty dla "P"
    const uint8_t SEG_U = 0b00111110;     // Segmenty dla "U"
    const uint8_t SEG_MINUS = 0b01000000; // Segmenty dla "-"
    display.clear();
    display.setSegments(&SEG_U, 1, 0);
    display.setSegments(&SEG_MINUS, 1, 1);
    display.showNumberDecEx(currentVolume, 0b01000000, false, 2, 2);
}

void showoNoF()
{
    const uint8_t SEG_O = 0b00111101;        // Segmenty dla "o" (a,b,c,d)
    const uint8_t SEG_N = 0b00110111;        // Segmenty dla "N" (a,b,c,e,f)
    const uint8_t SEG_F_CUSTOM = 0b01100011; // Segmenty dla "F" (a,e,f,g)
    uint8_t segments[4] = {SEG_O, SEG_N, SEG_O, SEG_F_CUSTOM};
    display.clear();
    // display.setSegments(segments);

    display.setSegments(&SEG_O, 1, 0);
    display.setSegments(&SEG_N, 1, 1);
    display.setSegments(&SEG_F_CUSTOM, 1, 2);
    display.setSegments(&SEG_F_CUSTOM, 1, 3);
    // display.showNumberDecEx(currentVolume, 0b01000000, false, 2, 2);
}

void updateVolume()
{
    // const uint8_t SEG_U = 0b00111110;  // Segmenty dla "U"
    // const uint8_t SEG_MINUS = 0b01000000;  // Segmenty dla "-"

    long newPosition = volumeEncoder.getCount() / 2;
    if (newPosition != lastPosition)
    {
        VolOrProgDisplay = true;
        lastDisplay = millis();
        int change = newPosition - lastPosition;
        currentVolume = constrain(currentVolume + change, 0, 21);
        lastPosition = newPosition;
        showVolumeValue();
        audio.setVolume(currentVolume);
        Serial.print("Ustawiona głośność: ");
        Serial.println(currentVolume);
        lastInteractionTime = millis();
    }
}

void updateStation()
{ // Dodana funkcja do aktualizacji stacji
    long newPosition = volumeEncoder.getCount() / 2;
    if (newPosition != lastPosition)
    {
        VolOrProgDisplay = true;
        lastDisplay = millis();
        int change = newPosition - lastPosition;
        currentStationIndex = (currentStationIndex + change + numberOfStations) % numberOfStations;
        lastPosition = newPosition;
        showChannelNumber();
        connectToStation(currentStationIndex);
    }
}

void handleButtonInput()
{
    static bool buttonPressed = false;
    static unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 200;

    int buttonState = digitalRead(ENCODER_SW);
    if (buttonState == LOW && !buttonPressed && (millis() - lastDebounceTime > debounceDelay))
    {
        lastDebounceTime = millis();
        buttonPressed = true;
        volumeMode = !volumeMode;
        if (!volumeMode)
        {
            showChannelNumber();
        }
        else
        {
            showVolumeValue();
        }

        Serial.println(volumeMode ? "Tryb regulacji głośności." : "Tryb przełączania programów.");
    }
    else if (buttonState == HIGH)
    {
        buttonPressed = false;
    }
}

void triggerAPI(String url)
{
    const int maxRetries = 3;
    const int retryDelay = 2000; // 2 sekundy między próbami

    HTTPClient http;

    for (int attempt = 1; attempt <= maxRetries; attempt++)
    {
        Serial.print("Próba wywołania API #");
        Serial.println(attempt);

        http.begin(url);
        http.setTimeout(3000); // timeout 3 sekundy

        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
            Serial.print("API OK, kod: ");
            Serial.println(httpResponseCode);
            http.end();
            return; // sukces → kończymy
        }
        else
        {
            Serial.print("Błąd API: ");
            Serial.println(httpResponseCode);
        }

        http.end();

        if (attempt < maxRetries)
        {
            Serial.println("Ponawianie...");
            delay(retryDelay);
        }
    }

    Serial.println("Nie udało się wywołać API po kilku próbach.");
}


void blinkDots()
{
    static unsigned long lastBlinkTime = 0;
    static bool dotsOn = true;

    if (millis() - lastBlinkTime >= 1000)
    {
        dotsOn = !dotsOn;
        if (!timeClient.isTimeSet())
        {
            Serial.println("NTP time not set");
            return;
        }
        int hours = timeClient.getHours();
        int minutes = timeClient.getMinutes();
        display.showNumberDecEx(hours, dotsOn ? 0b01000000 : 0, true, 2, 0);
        display.showNumberDec(minutes, true, 2, 2);
        lastBlinkTime = millis();
    }
}
