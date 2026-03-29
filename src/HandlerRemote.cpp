
#include "HandlerRemote.h"
#include "main.h"

#define IR_RECEIVER_PIN 15 // Pin dla odbiornika IR

IRrecv irrecv(IR_RECEIVER_PIN);
decode_results results; 
void handleRemoteSetup()
{
   irrecv.enableIRIn();  
   
}

void handleRemoteInput() { // Funkcja do obsługi pilota
   if (irrecv.decode(&results)) {
  VolOrProgDisplay = true;
    lastDisplay = millis(); 
       switch (results.value) {
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
           case 0xFF629D: // Zastąp rzeczywistym kodem przycisku kanału +
           volumeMode = false;  
               
               currentStationIndex = (currentStationIndex + 1) % numberOfStations;
               connectToStation(currentStationIndex);
               showChannelNumber(); 
               Serial.print("Przełączono na stację: ");
               Serial.println(currentStationIndex + 1);
               break;
           case 0xFFA857: // Zastąp rzeczywistym kodem przycisku kanału -
          volumeMode = false;
        
       lastDisplay = millis();
               currentStationIndex = (currentStationIndex - 1 + numberOfStations) % numberOfStations;
               connectToStation(currentStationIndex);
               showChannelNumber(); 
               Serial.print("Przełączono na stację: ");
               Serial.println(currentStationIndex + 1);
               break;
           default:
               Serial.print("Nieznany kod: ");
               Serial.println(results.value, HEX); // Wyświetlenie nieznanego kodu w formacie heksadecymalnym
               break;
       }
       irrecv.resume(); // Odbierz następny sygnał
   }
}