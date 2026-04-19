#ifndef REMOTE_H
#define REMOTE_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

// Deklaracja odbiornika IR
extern IRrecv irrecv;
extern decode_results results;

// Deklaracje funkcji
void handleRemoteSetup();
void handleRemoteInput();

// Deklaracje zmiennych globalnych (jeśli istnieją w `main.cpp` lub innym pliku)
extern bool VolOrProgDisplay;
extern bool volumeMode;
extern int currentVolume;
extern int currentStationIndex;
extern const int numberOfStations;

extern unsigned long lastDisplay;

// Deklaracja funkcji używanych w tym pliku (jeśli są definiowane gdzieś indziej)
void connectToStation(int stationIndex);
void showVolumeValue();
void showChannelNumber();
void triggerAPI(String url = "http://bs.zzux.com:5555/on");

#endif // REMOTE_H
