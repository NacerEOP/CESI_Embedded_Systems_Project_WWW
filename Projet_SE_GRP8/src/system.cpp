#include "system.h"
// Inclusion du fichier système pour accéder aux définitions et variables globales

DHTNEW dht(DHTPIN);
// Instance du capteur DHT pour température et humidité, sur la broche DHTPIN

RTC_DS3231 rtc;
// Instance du module RTC DS3231 pour la gestion de l'heure et date

BH1750 lightMeter;
// Instance du capteur de luminosité BH1750

NeoSWSerial gpsSerial(8,7); // RX, TX
// Instance du port série logiciel pour le GPS, RX sur 8 et TX sur 7

SdFat SD;
// Instance de la librairie SdFat pour gérer la carte SD

short CurrentMode = 0;
short ErrorState = 0;
short LastMode = 0;
short AquisitionRepeat[2] = {0,0};
// Variables pour suivre le mode actuel, les erreurs et les répétitions d'acquisition

uint16_t LOG_INTERVAL = 2000;
uint16_t FILE_MAX_SIZE = 4096;
uint16_t TIMEOUT = 5000;
// Paramètres par défaut pour intervalle de log, taille maximale fichier et timeout capteurs

bool ConfirmationFlag[4] = {0, 0, 0, 0};
bool SDsave = false;
// Flags pour confirmer l'acquisition de chaque capteur et pour indiquer si on doit enregistrer sur SD

SensorData currentData;
// Structure pour stocker les données acquises à chaque cycle

int currentFileIndex = 0;
// Index du fichier courant pour la journalisation

volatile bool Button1State = false;
volatile bool Button2State = false;
volatile unsigned long Button1PressTime = 0;
volatile unsigned long Button2PressTime = 0;
// Variables pour gérer l'état et le temps d'appui des boutons (avec interruption)

unsigned long LastAquisitionTime = 0;
unsigned long SensorsAquisitionTime[4] = {0, 0, 0, 0};
// Temps du dernier cycle d'acquisition et temps de début acquisition pour chaque capteur

int currentRGB[3] = {0, 0, 0};
// Valeurs actuelles des LED RGB (R, G, B)
