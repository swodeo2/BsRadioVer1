#include <Arduino.h>
#include "HandlerRemote.h"
#include "main.h"

#define IR_RECEIVER_PIN 15        // Pin dla odbiornika IR
#define DOUBLE_DIGIT_TIMEOUT 1000 // Timeout dla wyboru dwucyfrowego (ms)

IRrecv irrecv(IR_RECEIVER_PIN);
decode_results results;

// Zmienne dla obsługi wyboru dwucyfrowego (kanały 10, 11)
uint8_t firstDigit = 255; // 255 oznacza brak pierwszej cyfry
unsigned long lastDigitTime = 0;

void handleRemoteSetup()
{
    irrecv.enableIRIn();
}

void handleRemoteInput()
{
    if (irrecv.decode(&results))
    {
        VolOrProgDisplay = true;
        lastDisplay = millis();
        Serial.print("Odebrano kod: ");
        Serial.println(results.value, HEX); // Wyświetlenie odebranego kodu w formacie heksadecymalnym
        switch (results.value)
        {
        case 0xFFC23D: // Zastąp rzeczywistym kodem przycisku głośności +
            volumeMode = true;
            currentVolume = constrain(currentVolume + 1, 0, 21);
            audio.setVolume(currentVolume);
            showVolumeValue();
            Serial.print("Głośność zwiększona: ");
            Serial.println(currentVolume);
            break;
        case 0xFF22DD: // Zastąp rzeczywistym kodem przycisku głośności -
            volumeMode = true;
            currentVolume = constrain(currentVolume - 1, 0, 21);
            audio.setVolume(currentVolume);
            showVolumeValue();
            Serial.print("Głośność zmniejszona: ");
            Serial.println(currentVolume);
            break;
        case 0xFF629D: // kanału +
            volumeMode = false;
            currentStationIndex = (currentStationIndex + 1) % numberOfStations;
            showChannelNumber();
            connectToStation(currentStationIndex);
            //  showChannelNumber();
            Serial.print("Przełączono na stację: ");
            Serial.println(currentStationIndex + 1);
            break;

        case 0xFFA857: //  kanał -
            volumeMode = false;
            currentStationIndex = (currentStationIndex - 1 + numberOfStations) % numberOfStations;
            showChannelNumber();
            connectToStation(currentStationIndex);
            // showChannelNumber();
            Serial.print("Przełączono na stację: ");
            Serial.println(currentStationIndex + 1);
            break;

        case 0xFF6897: //  kanał 1
                       //   case 0xF38FDD93: //  kanał 1
        case 0xFF9867: //  kanał 2
        case 0xFFB04F: //  kanał 3
        case 0xFF30CF: //  kanał 4
        case 0xFF18E7: //  kanał 5
        case 0xFF7A85: //  kanał 6
        case 0xFF10EF: //  kanał 7
        case 0xFF38C7: //  kanał 8
        case 0xFF5AA5: //  kanał 9
        case 0xFF4AB5: //  kanał 0
        {
            // Określ cyfrę wciśniętego przycisku
            uint8_t digit = 0;
            switch (results.value)
            {
            case 0xFF6897:
            case 0xF38FDD93:
                digit = 1;
                break;
            case 0xFF9867:
                digit = 2;
                break;
            case 0xFFB04F:
                digit = 3;
                break;
            case 0xFF30CF:
                digit = 4;
                break;
            case 0xFF18E7:
                digit = 5;
                break;
            case 0xFF7A85:
                digit = 6;
                break;
            case 0xFF10EF:
                digit = 7;
                break;
            case 0xFF38C7:
                digit = 8;
                break;
            case 0xFF5AA5:
                digit = 9;
                break;
            case 0xFF4AB5:
                digit = 0;
                break;
            }

            // Sprawdź, czy czekamy na drugą cyfrę po wciśnięciu 1
            if (millis() - lastDigitTime < DOUBLE_DIGIT_TIMEOUT && firstDigit == 1)
            {
                // To druga cyfra po 1 - kanały 10-19
                volumeMode = false;
                currentStationIndex = 9 + digit; // 1+0=10 (idx 9), 1+1=11 (idx 10), ..., 1+9=19 (idx 18)
                showChannelNumber();
                connectToStation(9 + digit);
                Serial.print("Przełączono na stację: ");
                Serial.println(10 + digit);
                firstDigit = 255; // Zresetuj
            }
            else if (digit == 1)
            {
                // Pierwsza cyfra = 1, czekaj na drugą
                Serial.println("Czekam na drugą cyfrę... wciśnij 0-9 dla kanałów 10-19");
                volumeMode = false;
                currentStationIndex = 0; // Wyświetl cyfrę 1
                showChannelNumber();     // Pokaż "1" na wyświetlaczu
                firstDigit = 1;
                lastDigitTime = millis();
            }
            else if (digit == 0)
            {
                // Przycisk 0 - brak funkcji w normalnym trybie
                Serial.println("Przycisk 0 - wciśnij 1 aby wybrać kanały 10-19");
                firstDigit = 255; // Zresetuj
            }
            else
            {
                // Normalne kanały 2-9
                volumeMode = false;
                currentStationIndex = digit - 1;
                showChannelNumber();
                connectToStation(digit - 1);
                Serial.print("Przełączono na stację: ");
                Serial.println(digit);
                firstDigit = 255; // Zresetuj
            }
            break;
        }

        case 0xFF52AD: // API toggle
            volumeMode = false;
            showoNoF();
            triggerAPI("http://bs.zzux.com:5555/toggle");

            delay(100);
            VolOrProgDisplay = false;
            volumeMode = true;

            Serial.print("Przekaźnik toggled");
            break;

        default:
            Serial.print("Nieznany kod: ");
            Serial.println(results.value, HEX); // Wyświetlenie nieznanego kodu w formacie heksadecymalnym
            break;
        }
        irrecv.resume(); // Odbierz następny sygnał
    }
}
