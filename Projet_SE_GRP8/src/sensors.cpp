#include "sensors.h"
#include "sd_logger.h"
#include <Arduino.h>
#include <NeoSWSerial.h>
#include <system.h>
// Inclusion des fichiers pour capteurs, stockage SD, Arduino de base, communication série GPS et système global

// ===== GPS interrupt helpers =====
static void disableGPSInterrupt() { gpsSerial.ignore(); }
static void enableGPSInterrupt() { gpsSerial.listen(); }
// Fonctions pour désactiver/activer l'écoute GPS afin d'éviter les interférences lors de lecture capteurs

static void setError(uint8_t err) {
  CurrentMode = 4;
  ErrorState = err;
}
// Déclenche un état d'erreur avec le code fourni

static bool timeout(uint8_t idx) {
  if (millis() - SensorsAquisitionTime[idx] > TIMEOUT) {
    SensorsAquisitionTime[idx] = 0;
    return true;
  }
  return false;
}
// Vérifie si un capteur a dépassé le temps limite d'acquisition

static float convertToDecimal(const char* nmea, char dir) {
  if (!nmea || !*nmea) return -1;
  float raw = atof(nmea);
  int deg = (int)(raw / 100);
  float minutes = raw - (deg * 100);
  float dec = deg + minutes / 60.0;
  if (dir == 'S' || dir == 'W') dec = -dec;
  return dec;
}
// Convertit une position GPS NMEA ddmm.mmmm en degrés décimaux avec prise en compte de la direction

// ================================================================
//                GPS MANUAL PARSER (MINIMIZED)
// ================================================================
float Lire_Module_GPS() {
  // Lit le GPS manuellement sans librairie externe et met à jour latitude/longitude
  unsigned long start = millis();
  bool gotData = false;
  float lat = -1.0, lon = -1.0;
  bool fix = false;

  char line[100];
  uint8_t idx = 0;
  bool collecting = false;

  while (millis() - start < 2000) {
    while (gpsSerial.available()) {
      char c = gpsSerial.read();
      if ((c < 32 || c > 126) && c != '\r' && c != '\n' && c != '$') continue;

      if (c == '$') { gotData = true; collecting = true; idx = 0; line[idx++] = c; continue; }
      if (!collecting) continue;
      if (c == '\r') continue;

      if (c == '\n') {
        line[idx] = '\0';
        collecting = false;
        if (!strstr(line, "RMC") && !strstr(line, "GGA")) continue;

        char *token, *ptr = line;
        uint8_t field = 0;
        char latBuf[16] = {0}, lonBuf[16] = {0};
        char latDir = 'N', lonDir = 'E';
        char fixStatus = 'V';
        int fixQuality = 0;

        while ((token = strsep(&ptr, ",")) != NULL) {
          field++;
          if (strstr(line, "RMC")) {
            if (field == 2 && *token) fixStatus = *token;
            if (field == 4 && *token) strncpy(latBuf, token, 15);
            if (field == 5 && *token) latDir = *token;
            if (field == 6 && *token) strncpy(lonBuf, token, 15);
            if (field == 7 && *token) lonDir = *token;
          } else {
            if (field == 3 && *token) strncpy(latBuf, token, 15);
            if (field == 4 && *token) latDir = *token;
            if (field == 5 && *token) strncpy(lonBuf, token, 15);
            if (field == 6 && *token) lonDir = *token;
            if (field == 7 && *token) fixQuality = atoi(token);
          }
        }

        if (fixStatus == 'A' || fixQuality > 0) fix = true;
        if (fix && strlen(latBuf) && strlen(lonBuf)) {
          lat = convertToDecimal(latBuf, latDir);
          lon = convertToDecimal(lonBuf, lonDir);
          goto done;
        }
      } else if (idx < sizeof(line) - 1) line[idx++] = c;
      else { collecting = false; idx = 0; }
    }
  }

done:
  if (lat != -1 && lon != -1 && fix) {
    currentData.latitude = lat;
    currentData.longitude = lon;
    return 1.0; // GPS fix OK
  }
  return gotData ? -1.0 : 0.0; // -1 = données reçues mais pas de fix, 0 = pas de données
}

// ================================================================
//                   ACQUISITION CAPTEURS
// ================================================================
void Aquisition_Capteurs() {
  // Lit tous les capteurs et met à jour currentData
  for (uint8_t i = 0; i < 4; i++) {
    if (ConfirmationFlag[i]) continue;
    if (!SensorsAquisitionTime[i]) SensorsAquisitionTime[i] = millis();

    switch (i) {
      case 1: { // Luminosité
        int lux = Lire_Capteur_Luminosite();
        if (lux) { currentData.lux = lux; ConfirmationFlag[i] = true; AquisitionRepeat[0] = 0; }
        else if (timeout(i)) { if (AquisitionRepeat[0]++) setError(1); currentData.lux = -1; ConfirmationFlag[i] = true; }
      } break;

      case 3: { // Température + humidité
        disableGPSInterrupt();
        float t = Lire_Capteur_Temperature_et_Humidite();
        enableGPSInterrupt();
        gpsSerial.begin(9600);

        if (t != -1) {
          disableGPSInterrupt();
          currentData.temperature = t;
          currentData.humidity = dht.getHumidity();
          enableGPSInterrupt();
          gpsSerial.begin(9600);
          ConfirmationFlag[i] = true;
          AquisitionRepeat[1] = 0;
        } else if (timeout(i)) { if (AquisitionRepeat[1]++) setError(1); currentData.temperature = -200; currentData.humidity = -200; ConfirmationFlag[i] = true; }
      } break;

      case 0: { // GPS
        float gpsVal = Lire_Module_GPS();
        if (gpsVal > 0) ConfirmationFlag[i] = true;
        else if (gpsVal == -1) { currentData.latitude = -1; currentData.longitude = -1; ConfirmationFlag[i] = true; }
        else if (timeout(i)) setError(3);
      } break;

      case 2: { // RTC
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        if (rtc.begin()) {
          DateTime now = rtc.now();
          if (now.year() >= 2020 && now.year() <= 2100) { currentData.timestamp = now; ConfirmationFlag[i] = true; break; }
        }
        if (timeout(i)) setError(2);
      } break;
    }
  }

  // Vérifie si tous les capteurs ont confirmé
  for (uint8_t i = 0; i < 4; i++) if (!ConfirmationFlag[i]) return;

  if (SDsave) Stocker_Donnees_SD();
  else {
    Serial.println(F("Data acquisition complete (not saved to SD)."));
    if (currentData.lux == -1) Serial.println(F("Lux: NA"));
    else { Serial.print(F("Lux: ")); Serial.println(currentData.lux); }

    if (currentData.temperature == -200) Serial.println(F("Temperature/Humidity: NA"));
    else { Serial.print(F("Temperature: ")); Serial.println(currentData.temperature); Serial.print(F("Humidity: ")); Serial.println(currentData.humidity); }

    if (currentData.latitude == -1) Serial.println(F("GPS: No fix."));
    else { Serial.print(F("Latitude: ")); Serial.println(currentData.latitude, 6); Serial.print(F("Longitude: ")); Serial.println(currentData.longitude, 6); }

    DateTime t = currentData.timestamp;
    Serial.print(F("Timestamp: ")); Serial.print(t.year()); Serial.print('/'); Serial.print(t.month()); Serial.print('/'); Serial.print(t.day()); Serial.print(' '); Serial.print(t.hour()); Serial.print(':'); Serial.print(t.minute()); Serial.print(':'); Serial.println(t.second());
    Serial.println(F("----------------------------------------------------------"));
  }

  for (uint8_t j = 0; j < 4; j++) { ConfirmationFlag[j] = false; SensorsAquisitionTime[j] = 0; }
  LastAquisitionTime = millis();
}

// ================================================================
//                    SENSOR HELPERS
// ================================================================
float Lire_Capteur_Luminosite() {
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) return 0;
  return lightMeter.readLightLevel();
}
// Lit le capteur de luminosité BH1750 et retourne la valeur en lux, ou 0 si erreur

float Lire_Capteur_Temperature_et_Humidite() {
  return (dht.read() == 0) ? dht.getTemperature() : -1;
}
// Lit le capteur DHT pour température et humid
