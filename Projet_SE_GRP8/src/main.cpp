#include "system.h"
#include "rgb.h"
#include "sensors.h"
#include "buttons.h"
#include "sd_logger.h"
#include <EEPROM.h>
#include <Wire.h>
#include <system.h>
// Inclusion des librairies et fichiers nécessaires : gestion du système, des LEDs RGB, des capteurs, des boutons,
// du stockage SD, de l'EEPROM et de la communication I2C (Wire)

#define ADDR_MAGIC       0
#define ADDR_LOGINT      1
#define ADDR_FILEMAX     3
#define ADDR_TIMEOUT     5
#define ADDR_VERSION     7
// Adresses EEPROM pour stocker différentes configurations

#define MAGIC_VAL 0xAB
// Valeur magique pour vérifier si l'EEPROM a été initialisée

uint16_t LOG_INTERVAL_EE;
uint16_t FILE_MAX_SIZE_EE;
uint16_t TIMEOUT_EE;
char VERSION_EE[6] = "1.0";
// Variables pour stocker temporairement les valeurs lues depuis l'EEPROM

static char serialBuffer[40];
static uint8_t bufPos = 0;
// Tampon pour la lecture série et position actuelle dans le buffer

int freeMemory() {
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
// Fonction pour estimer la mémoire libre RAM disponible

void loadFromEEPROM() {
  EEPROM.get(ADDR_LOGINT, LOG_INTERVAL);
  EEPROM.get(ADDR_FILEMAX, FILE_MAX_SIZE);
  EEPROM.get(ADDR_TIMEOUT, TIMEOUT);
}
// Lecture des valeurs de configuration depuis l'EEPROM vers les variables globales

static void writeDefaultsToEEPROM() {
  EEPROM.write(ADDR_MAGIC, MAGIC_VAL);
  EEPROM.put(ADDR_LOGINT, (uint16_t)10);
  EEPROM.put(ADDR_FILEMAX, (uint16_t)4096);
  EEPROM.put(ADDR_TIMEOUT, (uint16_t)30);
  for (uint8_t i = 0; i < sizeof(VERSION_EE); i++)
    EEPROM.write(ADDR_VERSION + i, VERSION_EE[i]);
}
// Écrit les valeurs par défaut dans l'EEPROM (utilisé si EEPROM non initialisée ou reset)

void initEEPROMIfNeeded() {
  if (EEPROM.read(ADDR_MAGIC) != MAGIC_VAL) {
    Serial.println(F("EEPROM not initialized, setting defaults..."));
    writeDefaultsToEEPROM();
  }
  EEPROM.get(ADDR_LOGINT, LOG_INTERVAL_EE);
  EEPROM.get(ADDR_FILEMAX, FILE_MAX_SIZE_EE);
  EEPROM.get(ADDR_TIMEOUT, TIMEOUT_EE);
  for (uint8_t i = 0; i < sizeof(VERSION_EE); i++)
    VERSION_EE[i] = EEPROM.read(ADDR_VERSION + i);
}
// Initialise l'EEPROM si elle n'a pas été configurée et charge les valeurs dans les variables temporaires

void resetEEPROM() {
  writeDefaultsToEEPROM();
  initEEPROMIfNeeded();
}
// Réinitialise l'EEPROM aux valeurs par défaut

static uint16_t parseValue(const char *cmd, uint8_t startIdx) {
  return atoi(cmd + startIdx);
}
// Convertit une chaîne de caractères en entier à partir d'un index donné (pour traitement des commandes)

void processCommand(const char *cmd) {
  // Traite une commande série reçue
  if (strncmp(cmd, "LOG_INTERVAL=", 13) == 0) {
    uint16_t val = parseValue(cmd, 13);
    EEPROM.put(ADDR_LOGINT, val);
    LOG_INTERVAL = (uint32_t)val * 1000;
    LOG_INTERVAL_EE = val;
    Serial.println(F("OK LOG_INTERVAL updated."));
  }
  else if (strncmp(cmd, "FILE_MAX_SIZE=", 14) == 0) {
    uint16_t val = parseValue(cmd, 14);
    EEPROM.put(ADDR_FILEMAX, val);
    FILE_MAX_SIZE = val;
    FILE_MAX_SIZE_EE = val;
    Serial.println(F("OK FILE_MAX_SIZE updated."));
  }
  else if (strncmp(cmd, "TIMEOUT=", 8) == 0) {
    uint16_t val = parseValue(cmd, 8);
    EEPROM.put(ADDR_TIMEOUT, val);
    TIMEOUT = (uint32_t)val * 1000;
    TIMEOUT_EE = val;
    Serial.println(F("OK TIMEOUT updated."));
  }
  else if (strcmp(cmd, "RESET") == 0) {
    resetEEPROM();
    Serial.println(F("EEPROM reset to defaults."));
  }
  else if (strcmp(cmd, "VERSION") == 0) {
    Serial.print(F("Version: "));
    Serial.println(VERSION_EE);
  }
  else if (strcmp(cmd, "SHOW") == 0) {
    Serial.print(F("LOG_INTERVAL = "));
    Serial.println(LOG_INTERVAL);
    Serial.print(F("FILE_MAX_SIZE = "));
    Serial.println(FILE_MAX_SIZE);
    Serial.print(F("TIMEOUT = "));
    Serial.println(TIMEOUT);
  }
  else {
    Serial.println(F("Unknown command."));
  }
}

void handleSerialInput() {
  // Lit les caractères reçus sur le port série et traite les commandes complètes
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuffer[bufPos] = '\0';
      processCommand(serialBuffer);
      bufPos = 0;
    } else if (c != '\r' && bufPos < sizeof(serialBuffer) - 1) {
      serialBuffer[bufPos++] = toupper(c);
    }
  }
}

void setup() {
  Serial.begin(9600);
  initEEPROMIfNeeded();
  loadFromEEPROM();

  for (uint8_t i = 0; i < 2; i++) pinMode(ButtonsPINS[i], INPUT);
  attachInterrupt(digitalPinToInterrupt(ButtonsPINS[0]), InterruptButton1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ButtonsPINS[1]), InterruptButton2, CHANGE);

  if (digitalRead(ButtonsPINS[0])) CurrentMode = 1;

  Wire.begin();
  if (!rtc.begin()) Serial.println(F("RTC"));
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(SDPins[0], OUTPUT);
  digitalWrite(SDPins[0], HIGH);

  for (uint8_t i = 0; i < 3; i++) pinMode(RGBpins[i], OUTPUT);
  gpsSerial.begin(9600);
  RGB_Control(1);
}

void loop() {
  // Gestion des appuis longs sur les boutons
  uint32_t now = millis();

  if (Button1State && now - Button1PressTime > 5000) {
    Button1PressTime = now;
    if (CurrentMode == 0 || CurrentMode == 3) {
      LastMode = CurrentMode;
      CurrentMode = 2;
    } else if (CurrentMode == 2) {
      CurrentMode = LastMode;
    }
  }

  if (Button2State && (CurrentMode == 0 || CurrentMode == 3) && now - Button2PressTime > 5000) {
    Button2PressTime = now;
    CurrentMode = (CurrentMode == 0) ? 3 : 0;
  }

  // Actions selon le mode actuel
  switch (CurrentMode) {
    case 0: // Mode Standard
      SDsave = true;
      RGB_Control(2);
      if (now - LastAquisitionTime > LOG_INTERVAL) Aquisition_Capteurs();
      break;

    case 1: // Mode Configuration
      RGB_Control(5);
      handleSerialInput();
      break;

    case 2: // Mode Maintenance
      SDsave = false;
      RGB_Control(3);
      if (now - LastAquisitionTime > LOG_INTERVAL) Aquisition_Capteurs();
      break;

    case 3: // Mode Economique
      SDsave = true;
      RGB_Control(4);
      if (now - LastAquisitionTime > LOG_INTERVAL * 2) Aquisition_Capteurs();
      break;

    case 4: // Mode erreur
      AquisitionRepeat[0] = 0;
      AquisitionRepeat[1] = 0;
      switch (ErrorState) {
        case 1: RGB_Control(6); break;
        case 2: RGB_Control(7); break;
        case 3: RGB_Control(8); break;
        case 4: RGB_Control(11); break;
        case 5: RGB_Control(10); break;
        case 9: RGB_Control(9); break; // Données capteurs incohérentes
      }
      break;
  }
}
