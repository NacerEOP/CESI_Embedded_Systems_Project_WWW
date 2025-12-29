#include "buttons.h"  // Inclusion du fichier contenant les déclarations et variables des boutons

void InterruptButton1() {
  // Interruption pour le bouton 1
  if (digitalRead(ButtonsPINS[0])) {  // Si le bouton est pressé
    Button1PressTime = millis();       // Enregistrer le temps de pression
    Button1State = true;               // Mettre l'état à true
  } else Button1State = false;         // Sinon, état à false
}

void InterruptButton2() {
  // Interruption pour le bouton 2
  if (digitalRead(ButtonsPINS[1])) {  // Si le bouton est pressé
    Button2PressTime = millis();       // Enregistrer le temps de pression
    Button2State = true;               // Mettre l'état à true
  } else Button2State = false;         // Sinon, état à false
}
