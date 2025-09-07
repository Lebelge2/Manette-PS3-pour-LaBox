//--------------------------------- Contrôle d'un train électrique avec une manette PS3 SONY (Originale obligatoire) ----------------------------
// Dernière modif: 07-09-25
// lebelge2@yahoo.fr

//#include "EXComm.h"

//#ifdef ENABLE_PS3
#include "PS3.h"
#include <Ps3Controller.h>
#include "DCC.h"
#include <EEPROM.h>

int battery = 0;
int Batterie;
bool DirL1 = true;     // Diection loco 1
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
byte AdrDefLoco1 = 3;  // Adresse par défaut loco 1
byte AdrDefLoco2 = 4;  // Adresse par défaut loco 2
byte FoncL1 = 0;       // Registre fonctions loco 1
byte FoncL2 = 0;
byte EtatLED;
bool Prog = false;
bool Select = false;
unsigned long curTime;
unsigned long prevTime;
unsigned int TimeLoop = 100;

//PS3::PS3() : EXCommItem("PS3") {}

void notify() {
  //--- Digital cross/square/triangle/circle button events ---
  if ( Ps3.event.button_down.cross ) {                // F2 Loco 2
    FoncL2 = FoncL2 ^ 2;
    DCC::setFn(AdrL2, 2, bitRead(FoncL2, 1));
  }
  if ( Ps3.event.button_down.square ) {               // F3 Loco 2
    FoncL2 = FoncL2 ^ 4;
    DCC::setFn(AdrL2, 3, bitRead(FoncL2, 2));
  }
  if ( Ps3.event.button_down.triangle ) {             // F0 Loco 2
    FoncL2 = FoncL2 ^ 16;
    DCC::setFn(AdrL2, 0, bitRead(FoncL2, 4));
  }
  if ( Ps3.event.button_down.circle ) {               // F1 Loco 2
    FoncL2 = FoncL2 ^ 1;
    DCC::setFn(AdrL2, 1, bitRead(FoncL2, 0));
  }
  //--------------- Digital D-pad button events --------------
  if ( Ps3.event.button_down.up ) {                   // F0 Loco 1
    FoncL1 = FoncL1 ^ 16;
    DCC::setFn(AdrL1, 0, bitRead(FoncL1, 4));
  }
  if ( Ps3.event.button_down.right ) {                // F1 Loco 1
    FoncL1 = FoncL1 ^ 1;
    DCC::setFn(AdrL1, 1, bitRead(FoncL1, 0));
  }
  if ( Ps3.event.button_down.down ) {                 // F2 Loco 1
    FoncL1 = FoncL1 ^ 2;
    DCC::setFn(AdrL1, 2, bitRead(FoncL1, 1));
  }
  if ( Ps3.event.button_down.left ) {                // F3 Loco 1
    FoncL1 = FoncL1 ^ 4;
    DCC::setFn(AdrL1, 3, bitRead(FoncL1, 2));
  }
  //------------- Digital shoulder button events -------------
  if ( Ps3.event.button_down.l1 )
    //SpeedL1 = 128;                                  // Vitesse loco 1 max
    DCC::setThrottle(AdrL1, 127, DirL1);              // Vitesse loco 1 max
  if ( Ps3.event.button_down.r1 ) 
    //SpeedL2 = 128;                                  // Vitesse loco 2 max.
    DCC::setThrottle(AdrL2, 127, DirL2);              // Vitesse loco 2 max.
  if ( Ps3.event.button_up.l1 )
    //SpeedL1 = 0;                                    // Vitesse loco 1 à 0
    DCC::setThrottle(AdrL1, 0, DirL1);                // Vitesse loco 1 à 0
  if ( Ps3.event.button_up.r1 )
    DCC::setThrottle(AdrL2, 0, DirL2);                // Vitesse loco 2 à 0
  // SpeedL2 = 0;                                     // Vitesse loco 2 à 0
  //-------------- Digital trigger button events -------------
  if ( Ps3.event.button_down.l2 ) {                   // Arrêt urgence loco 1
    SpeedL1 = 0;                                      // Vitesse loco 1 à 0
    DCC::setThrottle(AdrL1, 1, 1);                    // Arrêt loco 1
    Ps3.setRumble(100.0, 500);                        // Vibration manette 500 ms.
    Serial.println("Stop loco 1");
  }
  if ( Ps3.event.button_down.r2 ) {                   // Arrêt urgence loco 2
    SpeedL2 = 0;
    DCC::setThrottle(AdrL2, 1, 1);
    Ps3.setRumble(100.0, 500);                        // Vibration manette 500 ms.
    Serial.println("Stop loco 2");
  }
  //--------------- Digital stick button events --------------
  if ( Ps3.event.button_down.l3 ) {                   // Changement de direction loco 1
    if (DirL1 == true) DirL1 = false;
    else DirL1 = true;
  }
  if ( Ps3.event.button_down.r3 ) {                   // Changement de direction loco 2
    if (DirL2 == true) DirL2 = false;
    else DirL2 = true;
  }
  //---------- Digital select/start/ps button events ---------
  if ( Ps3.event.button_down.select )               // Bouton select enfoncé
    Select = true;
  if ( Ps3.event.button_up.select ) {               // Bouton select relaché
    Select = false;
    Ps3.setPlayer(Batterie);                        // Affiche niveau batterie (LED)
  }
  if ((Ps3.event.button_down.start ) && Select == true) { // Bouton START
    Ps3.setRumble(100.0, 500);                            // Vibration manette 500 ms.
    if (Prog == true) {                             
      Prog = false;                                       // Mode normal
      TimeLoop = 100;                                     // Incrémentation vitesse normal
      if (SpeedL1 < 1) SpeedL1 = 1;
      if (SpeedL2 < 1) SpeedL2 = 1;
      EEPROM.write(1, SpeedL1);
      EEPROM.write(2, SpeedL2);
      EEPROM.commit();
      AdrL1 =  SpeedL1;       // La vitesse affichée sur le display devient l'adresse loco
      AdrL2 =  SpeedL2;
      Serial.print("Adresses loco 1 mémorisée: "); Serial.println(AdrL1);
      Serial.print("Adresses loco 2 mémorisée: "); Serial.println(AdrL2);
    }
    else {                                           
      Prog = true;                                   // Mode programmation
      TimeLoop = 300;                                // Ralenti incrémentation vitesse
    }
  }
  if ( Ps3.event.button_down.ps ) {                  // Bouton PS
  }

  //---------------- Analog stick value events ---------------
  if ( abs(Ps3.event.analog_changed.stick.lx) + abs(Ps3.event.analog_changed.stick.ly) > 4 ) {
    AcDecL1 = (abs(Ps3.data.analog.stick.ly)) ;
    if (AcDecL1 < 20)  AcDecL1 = 0;
    else if (AcDecL1 < 50)  AcDecL1 = 1;
    else if (AcDecL1 < 80)  AcDecL1 = 2;
    else if (AcDecL1 < 100)  AcDecL1 = 3;
    else AcDecL1 = 4;
    if ((Ps3.data.analog.stick.ly) < 0)                // Position Stick ?
      AvL1 = true;                                     // Accélération loco 1 (Stick avant)
    else
      AvL1 = false;                                    // Décélération loco 1 (Stick arrière)
  }
  if ( abs(Ps3.event.analog_changed.stick.rx) + abs(Ps3.event.analog_changed.stick.ry) > 4 ) {
    AcDecL2 = abs(Ps3.data.analog.stick.ry);
    if (AcDecL2 < 20)  AcDecL2 = 0;
    else if (AcDecL2 < 50)  AcDecL2 = 1;
    else if (AcDecL2 < 80)  AcDecL2 = 2;
    else if (AcDecL2 < 100)  AcDecL2 = 3;
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
    if ((SpeedL1 != 0)||(SpeedL2 != 0)) {           // Arrête les locos si en mouvements
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
    if (Prog == true)  LEDCli(Batterie);            // Clignotement LED  (Mode programmation)
    if (AvL1) {
      SpeedL1 += AcDecL1 ;                          // Accélération loco 1
      if (SpeedL1 > 127) SpeedL1 = 127 ;
    }
    else {
      SpeedL1 -= AcDecL1;                           // Décélération loco 1
      if (SpeedL1 < 0) SpeedL1 = 0;
    }
    if (MemoSpeed1 != SpeedL1)                      // Si changement de vitesse
      DCC::setThrottle(AdrL1, SpeedL1, DirL1);      // Envoi commande
    MemoSpeed1 = SpeedL1;                           // Mémorise vitesse

    if (AvL2) {
      SpeedL2 += AcDecL2 ;                          // Accélération loco 2
      if (SpeedL2 > 127) SpeedL2 = 127 ;
    }
    else {
      SpeedL2 -= AcDecL2;                           // Décélération loco 2
      if (SpeedL2 < 0) SpeedL2 = 0;
    }
    if (MemoSpeed2 != SpeedL2)                      // Si changement de vitesse
      DCC::setThrottle(AdrL2, SpeedL2, DirL2);      // Envoi commande
    MemoSpeed2 = SpeedL2;                           // Mémorise vitesse
  }
  return true;
}
void LEDCli(byte NumLED) {
   EtatLED = EtatLED ^ NumLED;
  Ps3.setPlayer(EtatLED);                           // Clignotement LED
}
//#endif
