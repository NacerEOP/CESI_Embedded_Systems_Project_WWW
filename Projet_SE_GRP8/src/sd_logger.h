#pragma once  
// Empêche les inclusions multiples de ce fichier

#include "system.h"  
// Inclusion du fichier système pour accéder aux variables et définitions globales nécessaires

void Stocker_Donnees_SD();  
// Déclaration de la fonction principale d'enregistrement des données sur la carte SD

void WriteLineToSD(File &dataFile);  
// Déclaration de la fonction qui écrit une seule ligne CSV dans un fichier SD

String getActiveFileName();  
// Déclaration de la fonction qui retourne le nom du fichier actif pour l'enregistrement
