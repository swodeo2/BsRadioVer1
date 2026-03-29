#include "main.h"
#include <Arduino.h>
// #include "Arduino.h"
// #include "WiFiMulti.h"
// #include "Audio.h"
#include <TM1637Display.h>
// #include <WiFiUdp.h>
// #include <NTPClient.h>
#include <ESP32Encoder.h>
// #include <IRremoteESP8266.h> // Dodanie biblioteki IRremoteESP8266
// #include <IRrecv.h>          // Dodanie nagłówka dla odbiornika IR
// #include <IRutils.h>         // Przydatne funkcje do obsługi IR
#include <WiFiHandler.h>
#include <HandlerRemote.h>
#include <RadioStations.h>
// #include <IRsend.h>

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
// WiFiMulti wifiMulti;
//  WiFiUDP ntpUDP;
//  NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);
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

static long lastPosition = 0;

void connectToStation(int index);
void showChannelNumber();
void updateVolume();
void updateStation();
void handleButtonInput();
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

    if (millis() - lastDisplay >= 10000)
    {
        VolOrProgDisplay = false;
        volumeMode = true;
    }

    if (!VolOrProgDisplay)
    {
        blinkDots();
    }

    delay(2);
}

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

// void handleRemoteInput() { // Funkcja do obsługi pilota
//    if (irrecv.decode(&results)) {
//   VolOrProgDisplay = true;
//     lastDisplay = millis();
//        switch (results.value) {
//            case 0xFFC23D: // Zastąp rzeczywistym kodem przycisku głośności +
//                 volumeMode = true;
//               // VolOrProgDisplay = true;

//                currentVolume = constrain(currentVolume + 1, 0, 21);
//                audio.setVolume(currentVolume);
//                showVolumeValue();
//                Serial.print("Głośność zwiększona: ");
//                Serial.println(currentVolume);
//                break;
//            case 0xFF22DD: // Zastąp rzeczywistym kodem przycisku głośności -
//             volumeMode = true;
//                // VolOrProgDisplay = true;

//                currentVolume = constrain(currentVolume - 1, 0, 21);
//                audio.setVolume(currentVolume);
//                showVolumeValue();
//                Serial.print("Głośność zmniejszona: ");
//                Serial.println(currentVolume);
//                break;
//            case 0xFF629D: // Zastąp rzeczywistym kodem przycisku kanału +
//            volumeMode = false;

//                    //      VolOrProgDisplay = true;

//                currentStationIndex = (currentStationIndex + 1) % numberOfStations;
//                connectToStation(currentStationIndex);
//                showChannelNumber();
//                Serial.print("Przełączono na stację: ");
//                Serial.println(currentStationIndex + 1);
//                break;
//            case 0xFFA857: // Zastąp rzeczywistym kodem przycisku kanału -
//           volumeMode = false;
//                  //           VolOrProgDisplay = true;
//        lastDisplay = millis();
//                currentStationIndex = (currentStationIndex - 1 + numberOfStations) % numberOfStations;
//                connectToStation(currentStationIndex);
//                showChannelNumber();
//                Serial.print("Przełączono na stację: ");
//                Serial.println(currentStationIndex + 1);
//                break;
//            default:
//                Serial.print("Nieznany kod: ");
//                Serial.println(results.value, HEX); // Wyświetlenie nieznanego kodu w formacie heksadecymalnym
//                break;
//        }
//        irrecv.resume(); // Odbierz następny sygnał
//    }
// }

void blinkDots()
{
    static unsigned long lastBlinkTime = 0;
    static bool dotsOn = true;

    if (millis() - lastBlinkTime >= 1000)
    {
        dotsOn = !dotsOn;
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("Nie udało się pobrać czasu");
            return;
        }
        int hours = timeinfo.tm_hour;
        int minutes = timeinfo.tm_min;
        display.showNumberDecEx(hours, dotsOn ? 0b01000000 : 0, true, 2, 0);
        display.showNumberDec(minutes, true, 2, 2);
        lastBlinkTime = millis();
    }
}
