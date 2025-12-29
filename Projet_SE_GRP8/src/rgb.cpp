#include "rgb.h"
#include <Sensors.h>
// Inclusion des fichiers pour gérer les LEDs RGB et accéder aux capteurs si nécessaire

static void blink(uint8_t pinA, uint8_t pinB, uint16_t durA, uint16_t durB) {
  // Fait clignoter alternativement deux LEDs sur les broches pinA et pinB
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(pinA, HIGH); digitalWrite(pinB, LOW); delay(durA);
    digitalWrite(pinA, LOW);  digitalWrite(pinB, HIGH); delay(durB);
  }
  ErrorState = CurrentMode = 0; // Reset des états après le clignotement
}

static void blinkRedWhite(uint16_t durR, uint16_t durW) {
  // Fait clignoter la LED rouge puis blanche sur le RGB
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(RGBpins[0], HIGH); digitalWrite(RGBpins[1], LOW); digitalWrite(RGBpins[2], LOW); delay(durR);
    digitalWrite(RGBpins[0], HIGH); digitalWrite(RGBpins[1], HIGH); digitalWrite(RGBpins[2], HIGH); delay(durW);
    digitalWrite(RGBpins[0], LOW);  digitalWrite(RGBpins[1], LOW); digitalWrite(RGBpins[2], LOW);
  }
  ErrorState = CurrentMode = 0; // Reset des états après clignotement
}

void RGB_Control(uint8_t preset, uint8_t R, uint8_t G, uint8_t B) {
  // Éteint toutes les LEDs avant de changer l'état
  for (uint8_t i = 0; i < 3; i++) digitalWrite(RGBpins[i], LOW);

  switch (preset) {
    case 1: // Cycle de montée/descente pour chaque couleur
      for (uint8_t j = 0; j < 3; j++) {
        for (uint8_t i = 0; i < 255; i++) analogWrite(RGBpins[j], i);
        for (int i = 255; i >= 0; i--) analogWrite(RGBpins[j], i);
      }
      return;

    case 2:  R = 0;   G = 255; B = 0;   break; // vert
    case 3:  R = 255; G = 180; B = 0;   break; // orange
    case 4:  R = 0;   G = 0;   B = 255; break; // bleu
    case 5:  R = 255; G = 255; B = 0;   break; // jaune

    case 6: blink(RGBpins[0], RGBpins[1], 1000, 1000); return;
    case 7: blink(RGBpins[0], RGBpins[2], 1000, 1000); return;

    case 8: // Séquence spéciale clignotement rouge et vert
      for (uint8_t i = 0; i < 4; i++) {
        digitalWrite(RGBpins[0], HIGH); digitalWrite(RGBpins[1], LOW); delay(1000);
        digitalWrite(RGBpins[1], HIGH); delay(1000);
        digitalWrite(RGBpins[0], LOW);
      }
      ErrorState = CurrentMode = 0;
      return;

    case 9:  blink(RGBpins[0], RGBpins[1], 1000, 2000); return;
    case 10: blinkRedWhite(1000, 1000); return;
    case 11: blinkRedWhite(1000, 2000); return;
  }

  // Transition douce vers la couleur demandée
  const uint8_t steps = 40;
  int dR = (int)R - currentRGB[0];
  int dG = (int)G - currentRGB[1];
  int dB = (int)B - currentRGB[2];

  for (uint8_t i = 1; i <= steps; i++) {
    analogWrite(RGBpins[0], currentRGB[0] + (dR * i) / steps);
    analogWrite(RGBpins[1], currentRGB[1] + (dG * i) / steps);
    analogWrite(RGBpins[2], currentRGB[2] + (dB * i) / steps);
    delay(2);
  }

  // Met à jour les valeurs courantes
  currentRGB[0] = R;
  currentRGB[1] = G;
  currentRGB[2] = B;
}
