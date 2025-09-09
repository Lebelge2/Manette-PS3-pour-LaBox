//--------------------------------- Contrôle d'un train électrique avec une manette PS3 SONY (Originale obligatoire) ----------------------------
// Première version:  07-09-25
// Deuxième version:  09-09-25  Ajout aiguillages.
// Dernière modif:    09-09-25
// lebelge2@yahoo.fr

//#include "EXComm.h"

//#ifdef ENABLE_PS3
#include "PS3.h"
#include <Ps3Controller.h>
#include "DCC.h"
#include <EEPROM.h>
#include "TrackManager.h"

byte AdrDefLoco1 = 3;  // Adresse par défaut loco 1
byte AdrDefLoco2 = 4;  // Adresse par défaut loco 2
byte NumAigui[4][4] =  // Liste des aiguillages (En 4 groupes de 4)
{ {1, 2, 3, 4},        // Groupe 1 (SELECT 1)
  {5, 6, 7, 8},        // Groupe 2 (SELECT 2)
  {9, 10, 11, 12},     // Groupe 3 (SELECT 3)
  {13, 14, 15, 16}     // Groupe 4 (SELECT 4)
};

byte NumGr = 0;
int battery;
int Batterie;
bool DirL1 = true;     // Direction loco 1
bool DirL2 = true;
bool AvL1 = true;      // Avance loco 1
bool AvL2 = true;
int SpeedL1;           // Vitesse loco 1
int SpeedL2;
int MemoSpeed1;
int MemoSpeed2;
byte AcDecL1;          // Accélération/décélération loco 1
byte AcDecL2;
byte AdrL1;            // Adresse loco 1
byte AdrL2;            // Adresse loco 2
byte FoncL1 = 0;       // Registre fonctions loco 1
byte FoncL2 = 0;       // Registre fonctions loco 2
byte EtatLED;
bool ProgL1 = false;
bool ProgL2 = false;
bool Start = false;
byte LocoActive;
byte Aigui;
byte IndexAigui;
byte LoopAigui = 0;
unsigned long curTime;
unsigned long prevTime;
unsigned int TimeLoop = 100;

//PS3::PS3() : EXCommItem("PS3") {}

void notify() {
  //--- Digital cross/square/triangle/circle button events ---
  if ( Ps3.event.button_down.triangle )
    IndexAigui = 0;
  if ( Ps3.event.button_down.square )
    IndexAigui = 1;
  if ( Ps3.event.button_down.circle )
    IndexAigui = 2;
  if ( Ps3.event.button_down.cross )
    IndexAigui = 3;
  //--------------- Digital D-pad button events --------------
  if ( Ps3.event.button_down.up ) {                   // F0 Loco 1
    if (LocoActive == 1) {
      FoncL1 = FoncL1 ^ 16;
      DCC::setFn(AdrL1, 0, bitRead(FoncL1, 4));
    }
    if (LocoActive == 2) {                            // F0 Loco 2
      FoncL2 = FoncL2 ^ 16;
      DCC::setFn(AdrL2, 0, bitRead(FoncL2, 4));
    }
  }
  if ( Ps3.event.button_down.left ) {                // F1 Loco 1
    if (LocoActive == 1) {
      FoncL1 = FoncL1 ^ 1;
      DCC::setFn(AdrL1, 1, bitRead(FoncL1, 0));
    }
    if (LocoActive == 2) {                            // F1 Loco 2
      FoncL2 = FoncL2 ^ 1;
      DCC::setFn(AdrL2, 1, bitRead(FoncL2, 0));
    }
  }
  if ( Ps3.event.button_down.right ) {                 // F2 Loco 1
    if (LocoActive == 1) {
      FoncL1 = FoncL1 ^ 2;
      DCC::setFn(AdrL1, 2, bitRead(FoncL1, 1));
    }
    if (LocoActive == 2) {                            // F2 Loco 2
      FoncL2 = FoncL2 ^ 2;
      DCC::setFn(AdrL2, 2, bitRead(FoncL2, 1));
    }
  }
  if ( Ps3.event.button_down.down ) {                 // F3 Loco 1
    if (LocoActive == 1) {
      FoncL1 = FoncL1 ^ 4;
      DCC::setFn(AdrL1, 3, bitRead(FoncL1, 2));
    }
    if (LocoActive == 2) {                            // F3 Loco 2
      FoncL2 = FoncL2 ^ 4;
      DCC::setFn(AdrL2, 3, bitRead(FoncL2, 2));
    }
  }
  //------------- Digital shoulder button events -------------
  if ( Ps3.event.button_down.l1 ) {                   // Changement de direction loco 1
    if (DirL1 == true) DirL1 = false;
    else DirL1 = true;
  }
  if ( Ps3.event.button_down.r1 ) {                   // Changement de direction loco 2
    if (DirL2 == true) DirL2 = false;
    else DirL2 = true;
  }
  //-------------- Digital trigger button events -------------
  if ( Ps3.event.button_down.l2 ) {                     // Arrêt urgence loco 1
    Ps3.setRumble(100.0, 500);                          // Vibration manette 500 ms.
    if (ProgL1 == true) {
      ProgL1 = false;                                   // Mode normal
      TimeLoop = 100;                                   // Incrémentation vitesse normal
      if (SpeedL1 < 1) SpeedL1 = 1;                     // Si vitesse = 0 ==> Adr = 1
      AdrL1 = SpeedL1;                                  // La vitesse affichée sur le display devient l'adresse loco
      EEPROM.write(1, AdrL1);                           // Mémo nouvelle adresse
      EEPROM.commit();
      Serial.print("Adresses loco 1 mémorisée: "); Serial.println(AdrL1);
      Ps3.setPlayer(NumGr + 1);                         // Affiche n° groupe aiguillages
      return;
    }
    if (Start == false) {
      SpeedL1 = 0;                                      // Vitesse loco 1 à 0
      DCC::setThrottle(AdrL1, 1, 1);                    // Stop loco 1
      Serial.println("Stop loco 1");
    }
    else {                                              // Appui START
      ProgL1 = true;
      TimeLoop = 400;                                   // Ralenti incrémentation vitesse
      Serial.println("Mode programmation");
    }
  }
  if ( Ps3.event.button_down.r2 ) {                     // Arrêt urgence loco 2
    Ps3.setRumble(100.0, 500);                          // Vibration manette 500 ms.
    if (ProgL2 == true) {
      ProgL2 = false;                                   // Mode normal
      TimeLoop = 100;                                   // Incrémentation vitesse normal
      if (SpeedL2 < 1) SpeedL2 = 1;                     // Si vitesse = 0 ==> Adr = 1
      AdrL2 = SpeedL2;                                  // La vitesse affichée sur le display devient l'adresse loco
      EEPROM.write(2, AdrL2);                           // Mémo nouvelle adresse
      EEPROM.commit();
      Serial.print("Adresses loco 2 mémorisée: "); Serial.println(AdrL2);
      Ps3.setPlayer(Batterie);                          // Affiche niveau Batterie  (LED)
      return;
    }
    if (Start == false) {
      SpeedL2 = 0;                                      // Vitesse loco 2 à 0
      DCC::setThrottle(AdrL2, 1, 1);                    // Stop loco 2
      Serial.println("Stop loco 2");
    }
    else {                                              // Appui START
      ProgL2 = true;
      TimeLoop = 400;                                   // Ralenti incrémentation vitesse
      Serial.println("Mode programmation");
    }
  }
  //--------------- Digital stick button events --------------
  if ( Ps3.event.button_down.l3 ) {                   // Appui Stick gauche
  }
  if ( Ps3.event.button_down.r3 ) {                   // Appui Stick gauche
  }
  //---------- Digital select/start/ps button events ---------
  if ( Ps3.event.button_down.select ) {                 // Bouton select enfoncé
    NumGr++;
    if (NumGr > 3) NumGr = 0 ;
    Ps3.setPlayer(NumGr + 1);
  }
  if ( Ps3.event.button_down.start )                      // Bouton START relac
    Start = true; ;
  if ( Ps3.event.button_up.start )                        // Bouton START relac
    Start = false;
  if ( Ps3.event.button_down.ps ) {                       // Bouton PS
  }
  //---------------- Analog stick value events ---------------
  if ( abs(Ps3.event.analog_changed.stick.lx) + abs(Ps3.event.analog_changed.stick.ly) > 4 ) {
    LocoActive = 1;
    if ( abs(Ps3.data.analog.stick.lx) > 20 ) {
      if ( abs(Ps3.data.analog.stick.lx) > 120 ) {
        Aigui = NumAigui[NumGr][IndexAigui];                                 // Sélection de l'aiguillage
        Serial.print("N° aiguillage: "); Serial.println(Aigui);
        if ((Ps3.data.analog.stick.lx) < -120)                               // Stick 1 à gauche
          DCC::setAccessory((Aigui - 1) / 4 + 1, (Aigui - 1)  % 4, 0, 2);    // Aiguillage X position 1
        if ((Ps3.data.analog.stick.lx) > 120)                                // Stick 1 à droite
          DCC::setAccessory((Aigui - 1) / 4 + 1, (Aigui - 1)  % 4, 1, 2);    // Aiguillage X position 2
      }
      return;
    }
    AcDecL1 = abs(Ps3.data.analog.stick.ly) ;
    if (AcDecL1 < 20)  AcDecL1 = 0;
    else if (AcDecL1 < 50)  AcDecL1 = 1;
    else if (AcDecL1 < 80)  AcDecL1 = 2;
    else if (AcDecL1 < 100) AcDecL1 = 3;
    else AcDecL1 = 4;
    if ((Ps3.data.analog.stick.ly) < 0)
      AvL1 = true;                                     // Accélération loco 1 (Stick avant)
    else
      AvL1 = false;                                    // Décélération loco 1 (Stick arrière)
  }
  if (abs(Ps3.event.analog_changed.stick.rx) + abs(Ps3.event.analog_changed.stick.ry) > 4 ) {
    LocoActive = 2;
    AcDecL2 = abs(Ps3.data.analog.stick.ry);
    if (AcDecL2 < 20)  AcDecL2 = 0;
    else if (AcDecL2 < 50)  AcDecL2 = 1;
    else if (AcDecL2 < 80)  AcDecL2 = 2;
    else if (AcDecL2 < 100) AcDecL2 = 3;
    else AcDecL2 = 4;
    if ((Ps3.data.analog.stick.ry) < 0)                // Position Stick ?
      AvL2 = true;                                     // Accélération loco 2 (Stick avant)
    else
      AvL2 = false;                                    // Décélération loco 2 (Stick arrière)
  }

  //---------------------- Battery events ---------------------
  if ( battery != Ps3.data.status.battery ) {
    battery = (Ps3.data.status.battery);
    Batterie = battery - 1;
    Ps3.setPlayer(Batterie);
  }
}
//-------------------------------------------------------
void onConnect() {
  Serial.println("PS3 connectée.");
  TrackManager::setMainPower(POWERMODE::ON);         // Power Tracks ON
  TrackManager::setJoin(true);
}
//-------------------------------------------------------
//void PS3::setup() {
bool PS3::begin() {
  Ps3.attach(notify);
  Ps3.attachOnConnect(onConnect);
  Ps3.begin("E0: B9: A5: BD: 19: 26");               // Adresse MAC de votre PS3 SONY  /!\/!\/!\
  EEPROM.begin(3);
  AdrL1 = EEPROM.read(1);                            // Lecture adresse loco 1
  if ((AdrL1 < 1) || (AdrL1 > 128)) AdrL1 = AdrDefLoco1;
  AdrL2 = EEPROM.read(2);                            // Lecture adresse loco 2
  if ((AdrL2 < 1) || (AdrL2 > 128)) AdrL2 = AdrDefLoco2;
  Serial.print("Adresse loco 1: "); Serial.println(AdrL1);
  Serial.print("Adresse loco 2: "); Serial.println(AdrL2);
  return true;
}
//-------------------------------------------------------------
//void PS3::loop() {
bool PS3::loop() {
  if (!Ps3.isConnected()) {                         // Perte de connection
    if ((SpeedL1 != 0) || (SpeedL2 != 0)) {         // Arrête les locos si en mouvements
      SpeedL1 = 0;
      DCC::setThrottle(AdrL1 , 1, 1);               // Arrêt d'urgence loco 1
      SpeedL2 = 0;
      DCC::setThrottle(AdrL2 , 1, 1);               // Arrêt d'urgence loco 2
      Serial.println("PS3 déconnectée.");
    }
    return false;
  }
  curTime = millis();
  if ((curTime - prevTime) > TimeLoop) {            // Mise à jour vitesse toutes les 100 ms.
    prevTime = curTime;
    if (AvL1) {
      SpeedL1 += AcDecL1 ;                          // Accélération loco 1
      if (SpeedL1 > 127) SpeedL1 = 127 ;
    }
    else {
      SpeedL1 -= AcDecL1;                           // Décélération loco 1
      if (SpeedL1 < 0) SpeedL1 = 0;
    }
    if (AvL2) {
      SpeedL2 += AcDecL2 ;                          // Accélération loco 2
      if (SpeedL2 > 127) SpeedL2 = 127 ;
    }
    else {
      SpeedL2 -= AcDecL2;                           // Décélération loco 2
      if (SpeedL2 < 0) SpeedL2 = 0;
    }
    if (ProgL1 == true) LEDCli(1);                  // Clignotement LED 1 (Mode programmation)
    if (ProgL2 == true) LEDCli(2);                  // Clignotement LED 2 (Mode programmation)
    if (MemoSpeed1 != SpeedL1)                      // Si changement de vitesse
      DCC::setThrottle(AdrL1, SpeedL1, DirL1);      // Envoi commande
    if (MemoSpeed2 != SpeedL2)                      // Si changement de vitesse
      DCC::setThrottle(AdrL2, SpeedL2, DirL2);      // Envoi commande
    MemoSpeed1 = SpeedL1;                           // Mémorise vitesse
    MemoSpeed2 = SpeedL2;                           // Mémorise vitesse
  }
  return true;
}
void LEDCli(byte NumLED) {
  EtatLED = EtatLED ^ NumLED;
  Ps3.setPlayer(EtatLED);                           // Clignotement LED
}
//#endif
