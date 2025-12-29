#pragma once
// Empêche les inclusions multiples du fichier

#include <Arduino.h>
#include <SPI.h>
//#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <DHTNew.h>
#include <Wire.h>
#include <RTClib.h>
#include <BH1750.h>
#include <SdFat.h>
#include <NeoSWSerial.h>
// Inclusion des librairies nécessaires : capteurs, communication série, RTC, SD, SPI, etc.

#define DHTPIN 4
#define DHTTYPE DHT11
// Définition de la broche du capteur DHT et du type de capteur

const short RGBpins[3] = {5, 6, 9};
// Broches des LED RGB (R, G, B)
const short ButtonsPINS[2] = {2, 3};
// Broches des boutons
const short SDPins[4] = {10, 13, 11, 12}; // CS, SCK, MOSI, MISO
// Broches de la carte SD

extern uint16_t LOG_INTERVAL;
extern uint16_t FILE_MAX_SIZE;
extern uint16_t TIMEOUT;
// Paramètres globaux d'intervalle de log, taille max fichier, timeout

extern DHTNEW dht;
extern RTC_DS3231 rtc;
extern BH1750 lightMeter;
//extern TinyGPSPlus gps;
extern SdFat SD;
extern NeoSWSerial gpsSerial;
// Déclarations externes des instances de capteurs et périphériques

extern short CurrentMode;
extern short ErrorState;
extern short LastMode;
extern short AquisitionRepeat[2];
extern bool ConfirmationFlag[4];
extern bool SDsave;
extern int currentFileIndex;
// Variables globales pour gérer les modes, erreurs, répétitions, confirmation et fichiers

extern volatile bool Button1State;
extern volatile bool Button2State;
extern volatile unsigned long Button1PressTime;
extern volatile unsigned long Button2PressTime;
// États et temps des boutons (interruption)

extern unsigned long LastAquisitionTime;
extern unsigned long SensorsAquisitionTime[4];
extern int currentRGB[3];
// Temps de la dernière acquisition et valeurs RGB actuelles

struct SensorData {
  float temperature;
  float humidity;
  float lux;
  float latitude;
  float longitude;
  DateTime timestamp;
};
// Structure pour stocker les données des capteurs à chaque acquisition

extern SensorData currentData;
// Déclaration de l'instance globale pour stocker les données actuelles
