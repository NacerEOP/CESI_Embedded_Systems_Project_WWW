#include "sd_logger.h"
// Inclusion du fichier d'en-tête pour la gestion de l'enregistrement sur carte SD

#define BASE_NAME_LEN 16
#define LINE_BUF_SIZE 96
// Définition de la longueur maximale pour les noms de fichiers et les lignes CSV

char* getActiveFileName(char* outName, size_t outSize) {
  // Trouve le fichier actif pour enregistrer les données, crée un nouveau fichier si le précédent est plein
  char baseName[BASE_NAME_LEN];
  snprintf_P(baseName, sizeof(baseName),
             PSTR("%02u%02u%02u_"),
             (uint8_t)(currentData.timestamp.year() % 100),
             (uint8_t)currentData.timestamp.month(),
             (uint8_t)currentData.timestamp.day());
  // Génère la base du nom de fichier à partir de la date actuelle (YYMMDD_)

  uint8_t fileIndex = 0;
  char fileName[BASE_NAME_LEN];
  const char *ext = ".log";

  while (true) {
    snprintf_P(fileName, sizeof(fileName), PSTR("%s%u%s"), baseName, fileIndex, ext);
    if (!SD.exists(fileName)) break;
    fileIndex++;
  }
  // Cherche l'index le plus élevé de fichiers existants pour éviter d'écraser

  if (fileIndex > 0) {
    snprintf_P(fileName, sizeof(fileName), PSTR("%s%u%s"), baseName, fileIndex - 1, ext);
    File f = SD.open(fileName, FILE_READ);
    if (f) {
      if (f.size() >= FILE_MAX_SIZE) {
        f.close();
        snprintf_P(fileName, sizeof(fileName), PSTR("%s%u%s"), baseName, fileIndex, ext);
      } else {
        f.close(); // Réutilise le fichier si pas plein
      }
    }
  }

  strncpy(outName, fileName, outSize);
  return outName;
}
// Retourne le nom du fichier actif pour l'enregistrement

static inline int appendVal(char *line, int n, size_t size,
                            const __FlashStringHelper *fmt,
                            float val, float invalid,
                            const __FlashStringHelper *naText) {
  // Ajoute une valeur à une ligne CSV, ou un texte "NA" si valeur invalide
  if (val == invalid)
    return n + snprintf_P(line + n, size - n, PSTR(",%S"), naText);
  else
    return n + snprintf_P(line + n, size - n, (const char*)fmt, val);
}

void WriteLineToSD(File &dataFile) {
  // Écrit une ligne CSV complète avec timestamp et données capteurs
  char line[LINE_BUF_SIZE];
  int n = snprintf_P(line, sizeof(line),
                     PSTR("%u-%02u-%02u %02u:%02u:%02u"),
                     currentData.timestamp.year(),
                     currentData.timestamp.month(),
                     currentData.timestamp.day(),
                     currentData.timestamp.hour(),
                     currentData.timestamp.minute(),
                     currentData.timestamp.second());

  n = appendVal(line, n, sizeof(line), F(",%.1f"), currentData.temperature, -200, F("NA"));
  n = appendVal(line, n, sizeof(line), F(",%.1f"), currentData.humidity, -200, F("NA"));
  n = appendVal(line, n, sizeof(line), F(",%.0f"), currentData.lux, -1, F("NA"));
  n = appendVal(line, n, sizeof(line), F(",%.4f"), currentData.latitude, -1, F("Unknown Location"));
  n = appendVal(line, n, sizeof(line), F(",%.4f"), currentData.longitude, -1, F("Unknown Location"));

  dataFile.println(line);
}

void Stocker_Donnees_SD() {
  // Fonction principale pour stocker les données sur la carte SD
  if (!SD.begin(SDPins[0])) {
    Serial.println(F("SD card not detected!"));
    CurrentMode = 4;
    ErrorState = 4;
    return;
  }

  char fileName[BASE_NAME_LEN];
  getActiveFileName(fileName, sizeof(fileName));

  File dataFile = SD.open(fileName, FILE_WRITE);
  if (!dataFile) {
    Serial.println(F("Error opening file!"));
    CurrentMode = 4;
    ErrorState = 5;
    return;
  }

  if (dataFile.size() == 0)
    dataFile.println(F("Timestamp,Temperature,Humidity,Lux,Latitude,Longitude"));
  // Écrit l'entête CSV si fichier nouveau

  WriteLineToSD(dataFile);
  dataFile.close();

  Serial.print(F("Logged: "));
  Serial.println(fileName);
  // Affiche le nom du fichier où les données ont été enregistrées
}
