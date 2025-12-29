#pragma once  
// Empêche les inclusions multiples du fichier

#include "system.h"  
// Inclusion du fichier système pour accéder aux variables et définitions globales

void Aquisition_Capteurs();  
// Déclaration de la fonction principale pour acquérir les données de tous les capteurs

float Lire_Capteur_Luminosite();  
// Déclaration de la fonction qui lit la luminosité via le capteur BH1750

float Lire_Capteur_Temperature_et_Humidite();  
// Déclaration de la fonction qui lit la température et l'humidité via le capteur DHT

float Lire_Module_GPS();  
// Déclaration de la fonction qui lit les données GPS et met à jour latitude/longitude

DateTime Horloge_RTC();  
// Déclaration de la fonction qui retourne l'heure actuelle du module RTC
