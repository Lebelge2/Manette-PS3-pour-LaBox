//--------------------------------- Contrôle d'un train électrique avec une manette PS3 SONY (Originale obligatoire) ----------------------------
// Première version:  07-09-25
// Dernière modif:    10-11-25

// lebelge2@yahoo.fr

#include "EXComm.h"
#ifdef ENABLE_PS3
#include "PS3.h"
#include <Ps3Controller.h>
#include "DCC.h"
#include <EEPROM.h>
#include "TrackManager.h"
#include "hmi.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern hmi boxHMI;    // from .ino !

Adafruit_SSD1306 display(128, 64, &Wire, -1);

byte AdrDefLoco = 3;      // Adresse par défaut loco 1
int battery;
int Batterie;
bool Dir = true;           // Direction loco
int Val = 1;
int MemoVal = 1;
int Speed;                 // Vitesse loco
int MemoSpeed;
int MemoFonc;
int DeltaVG;               // Accélération/décélération vitesse loco
int DeltaVD;               // Incrémentation/décrémentation
int DeltaHD;
byte Adr;                  // Adresse loco
unsigned long RegFonc = 0; // Registre fonctions
bool Start;
byte NumMenu = 1;
bool StickDroitEn;
bool StickDroitHori;
byte PosCurs;
byte Aigui;
byte PosAigui;
byte AdrCV;
byte ValCV;
bool enservice;
unsigned long curTime;
unsigned long prevTime;
unsigned int TimeLoop = 100;
String messageRecu;

PS3::PS3() : EXCommItem("PS3") {}

void notify() {
  //----------------- Digital cross/square/triangle/circle button events --------------------------
  if ( Ps3.event.button_down.triangle ) {
    NumMenu = 1;
    TimeLoop = 100;
    Affiche();                        // Num adresse Loco
  }
  if ( Ps3.event.button_down.circle ) {
    NumMenu = 2;
    TimeLoop = 500;
    Affiche();                         // Groupe Fonction
  }
  if ( Ps3.event.button_down.cross ) {
    NumMenu = 3;
    TimeLoop = 150;
    Affiche();                         // Num aiguillage
  }
  if ( Ps3.event.button_down.square ) {
    NumMenu = 4;
    TimeLoop = 200;
    Affiche();                         // CV POM
  }
  //--------------- Digital D-pad button events -----PAD GAUCHE --------------------------------------
  if ( Ps3.event.button_down.up )                    // F0 Loco
    Fonction(0);
  if ( Ps3.event.button_down.left )                 // F1 Loco
    Fonction(1);
  if ( Ps3.event.button_down.right )                 // F2 Loco
    Fonction(2);
  if ( Ps3.event.button_down.down )                  // F3 Loco
    Fonction(3);
  //------------- Digital shoulder button events -------------
  if ( Ps3.event.button_down.l1 ) {                   // Changement de direction loco
    if (Dir == true) Dir = false;
    else Dir = true;
  }
  if ( Ps3.event.button_down.r1 ) {
    if (enservice == true) {
      enservice = false;
      TrackManager::setMainPower(POWERMODE::OFF);         // Power Tracks OFF
    }
    else {
      enservice = true;
      TrackManager::setMainPower(POWERMODE::ON);         // Power Tracks ON
    }
  }
  //-------------- Digital trigger button events -------------
  if ( Ps3.event.button_down.l2 ) {                     // Arrêt urgence loco active
    DCC::setThrottle(Adr, 1, 1);                        // Stop loco
    Speed = 0;                                          // Vitesse loco à 0
    Ps3.setRumble(100.0, 500);                          // Vibration manette 500 ms.
    Serial.println("Stop loco");
  }
  if ( Ps3.event.button_down.r2 ) {                     // Arrêt urgence toutes loco
    DCC::setThrottle(0, 1, 1);                          // Stop toutes loco
    Speed = 0;                                          // Vitesse loco à 0
    Ps3.setRumble(100.0, 500);                          // Vibration manette 500 ms.
  }
  //--------------- Digital stick button events --------------
  if ( Ps3.event.button_down.l3 ) {}                     // Appui Stick gauche
  if ( Ps3.event.button_down.r3 ) {}                     // Appui Stick droit

  //---------- Digital select/start/ps button events ---------
  if ( Ps3.event.button_down.select ) {                 // Bouton select enfoncé MEMORISATION
    display.clearDisplay();
    if (NumMenu == 1) {
      Adr = Val;
      EEPROM.write(0, Adr);                             // Mémo nouvelle adresse loco dans EEPROM
      EEPROM.commit();
    }
    if (NumMenu == 2) {                                  // Mémo groupe fonction (Pas dans EEPROM)
      MemoFonc = Val - 1;
    }
    display.setTextSize(2);
    display.setCursor(15, 20);
    if (NumMenu == 4) {                                  // Envoi POM
      DCC::writeCVByteMain(Adr, AdrCV, ValCV);
      display.print("Envoy");
      display.print((char)130);  // é
    }
    else {
      display.print("M");
      display.print((char)130);  // é
      display.print("moris");
      display.print((char)130);  // é
    }
    display.display();
    display.setTextSize(1);
    delay(2000);
    Start = false;
    TimeLoop = 100;
    StickDroitEn = false;
    boxHMI.stopStateMachine = false;
  }
  if ( Ps3.event.button_down.start ) {                     // Bouton START  enfoncé
    if (Start == false) {
      Start = true;
      boxHMI.stopStateMachine = true;
      MainMenu();
    }
    else {
      Start = false;
      TimeLoop = 100;
      StickDroitEn = false;
      boxHMI.stopStateMachine = false;
    }
  }

  //-------------------------------- STICK GAUCHE -------------- ---------------
  if ( abs(Ps3.event.analog_changed.stick.lx) + abs(Ps3.event.analog_changed.stick.ly) > 4 ) {
    DeltaVG = (Ps3.data.analog.stick.ly) / 50 ;
  }
  //--------------------------------- STICK DROIT -------------------------------
  if (abs(Ps3.event.analog_changed.stick.rx) + abs(Ps3.event.analog_changed.stick.ry) > 4 ) {
    if (StickDroitEn == false)
      return;
    DeltaVD = (Ps3.data.analog.stick.ry) / 100;                       // Vertical
    if ((Ps3.data.analog.stick.rx) < -100) {
      Aigui = Val;                                                    // Sélection de l'aiguillage
      Serial.print("N° aiguillage: "); Serial.println(Aigui);
      StickDroitHori = false;                                         // Horizontal gauche
      Affiche();
      DCC::setAccessory((Aigui - 1) / 4 + 1, (Aigui - 1)  % 4, 0, 2); // Aiguillage X position 1
    }
    if ((Ps3.data.analog.stick.rx) > 100) {
      Aigui = Val;                                                    // Sélection de l'aiguillage
      Serial.print("N° aiguillage: "); Serial.println(Aigui);
      StickDroitHori = true;                                          // Horizontal droit
      Affiche();
      DCC::setAccessory((Aigui - 1) / 4 + 1, (Aigui - 1)  % 4, 1, 2); // Aiguillage X position 2
    }
  }
  //---------------------- Battery events ---------------------
  if ( battery == Ps3.data.status.battery )
    return;
  else {
    battery = Ps3.data.status.battery;
    Batterie = battery * 20;
  }
}   // Fin des Notify
//-------------------------------------------------------
void onConnect() {
  Serial.println("PS3 connectée.");
  TrackManager::setMainPower(POWERMODE::ON);         // Power Tracks ON
  TrackManager::setJoin(true);
}
//-------------------------------------------------------
bool PS3::begin() {
  Serial.println("PS3 Begin.");
  Ps3.attach(notify);
  Ps3.attachOnConnect(onConnect);
  Ps3.begin("E0: B9: A5: BD: 19: 26");                // Adresse MAC de votre PS3 SONY  /!\/!\/!\/
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(HMI_Rotation);                  // 180°
  display.setTextSize(1);
  display.setTextColor(WHITE);
  EEPROM.begin(3);
  Adr = EEPROM.read(0);                               // Lecture adresse loco
  if ((Adr < 1) || (Adr > 128)) Adr = AdrDefLoco;     // 3
  Serial.print("Adresse loco: "); Serial.println(Adr);
  return true;
}
//-------------------------------------------------------------
bool PS3::loop() {
  if (!Ps3.isConnected()) {                       // Perte de connection
    if (Speed != 0) {                             // Arrête loco si en mouvements
      Speed = 0;
      DCC::setThrottle(Adr , 1, 1);               // Arrêt d'urgence loco
      Serial.println("PS3 déconnectée.");
    }
    return false;
  }
  curTime = millis();
  if ((curTime - prevTime) > TimeLoop) {          // Mise à jour valeur toutes les 100 ms.
    prevTime = curTime;
    Speed = constrain(Speed - DeltaVG, 0, 127);   // Valeur Stick verti gauche
    if (MemoSpeed != Speed) {                     // Si changement de vitesse
      DCC::setThrottle(Adr, Speed, Dir);          // Envoi commande
      MemoSpeed = Speed;                          // Mémorise nouvelle vitesse
      return true;
    }
    Val = constrain(Val - DeltaVD, 1, 99);        // Valeur Stick verti droit

    if (NumMenu == 4) {                           // CV POM
      if (StickDroitHori == false) AdrCV = Val;
      if (StickDroitHori == true) ValCV = Val;
    }
    if (MemoVal != Val) {                         // Si changement valeur
      Affiche();                                  // Mise à jour OLED
      MemoVal = Val;                              // Mémorise valeur
    }
  }

  return true;
}
//---------------------------------------------------------------------------
void iconeBatterie() {
  display.drawRect(104, 53, 21, 11, WHITE);    // Icone Batterie
  display.fillRect(125, 56, 3, 5, WHITE);      // Coté positif à droite
  display.setCursor(106, 55);
  display.println(Batterie);                   // Charge bat.
}
//--------------------------------------------------------------
void MainMenu() {                            // Menu général
  delay(1);
  display.setTextSize(1);
  display.clearDisplay();
  display.drawLine(6, 2, 0, 12, WHITE);      // Triangle
  display.drawLine(0, 12, 12, 12, WHITE);    // Triangle
  display.drawLine(12, 12, 6, 2, WHITE);     // Triangle
  display.setCursor(20, 5);
  display.println("Selec Adresses");

  display.drawCircle(6, 23, 6, WHITE);       // Cercle
  display.setCursor(20, 21);
  display.println("Selec Groupe Fonct");

  display.drawLine(1, 35, 11, 45, WHITE);    // Croix
  display.drawLine(1, 45, 11, 35, WHITE);    // Croix
  display.setCursor(20, 37);
  display.println("Selec Aiguillages");

  display.drawRect(1, 50, 11, 11, WHITE);    // Carré
  display.setCursor(20, 53);
  display.println("Ecrit CV (POM)");
  iconeBatterie();
  display.display();
  delay(5);
}
//-------------------------------------------------------------------
void  Fonction(byte NumFonc) {           // 28 fonctions
  NumFonc += (MemoFonc) * 4;
  unsigned long Decal = 1;
  RegFonc = RegFonc  ^ (Decal << NumFonc);
  DCC::setFn(Adr, NumFonc, bitRead(RegFonc, NumFonc));
}
//------------------------------------------------------------------
void Affiche() {
  if (Start == false)
    return;
  StickDroitEn = true;
  display.clearDisplay();
  display.setTextSize(1);
  if (NumMenu == 1) {                     // Num adresse Loco
    display.setCursor(20, 0);
    display.println("- Adresse Loco -");
    display.setTextSize(2);
    display.setCursor(50, 15);
    display.println(Val);
    TexteBas1();
  }
  if (NumMenu == 2) {                    // Num Groupe Fonction
    Val = min(Val, 7);
    byte offset = (Val - 1) << 2;
    display.setCursor(0, 0);
    display.print("Fonctions");
    display.setTextSize(2);
    display.setCursor(75, 0);
    display.print(offset);
    display.setCursor(50, 10);
    display.print(offset + 1);
    display.setCursor(100, 10);
    display.print(offset + 2);
    display.setCursor(75, 20);
    display.print(offset + 3);
    TexteBas1();
  }
  if (NumMenu == 3) {                   // Num aiguillage
    display.setCursor(20, 00);
    display.println("- Aiguillages -");
    display.setTextSize(2);
    display.setCursor(10, 15);
    display.print(Val);
    if (StickDroitHori == false)
      display.print(" Droit");
    else {
      display.print(" D");
      display.print((char)130);       // é
      display.print("riv");
      display.print((char)130);       // é
    }
    TexteBas2();
  }
  if (NumMenu == 4) {                   // CV POM
    display.setCursor(0, 0);
    display.println("- Ecriture CV (POM) -");
    display.setCursor(0, 20);
    display.print("Loco ");
    display.print(Adr);
    display.print(" CV ");
    display.print(AdrCV);
    display.print(" Data ");
    display.print(ValCV);
    TexteBas3();
  }
  display.display();
  display.setTextSize(1);
}
//---------------------------------------------------------------------
void TexteBas1() {             // Adresse Loco et Groupes fonctions
  display.setTextSize(1);
  display.setCursor(0, 37);
  display.println("H-B    pour changer");       // Haut / Bas
  display.setCursor(0, 47);
  display.println("START  pour sortir");
  display.setCursor(0, 57);
  display.print  ("SELECT pour m");
  display.print((char)130);  // é
  display.print("moriser");
}
//---------------------------------------------------------------------
void TexteBas2() {               // Aiguillages
  display.setTextSize(1);
  display.setCursor(0, 37);
  display.println("H-B   pour changer");         // Haut / Bas
  display.setCursor(0, 47);
  display.println("G-D   pour commuter");        // Gauche / Droite
  display.setCursor(0, 57);
  display.println("START pour sortir");
}
//-------------------------------------------------------------------
void TexteBas3() {             // CV POM
  display.setTextSize(1);
  display.setCursor(0, 36);
  display.println("H-B G-D pour changer");
  display.setCursor(0, 46);
  display.println("START   pour sortir");
  display.setCursor(0, 56);
  display.print  ("SELECT  pour envoyer");
}
//------------------------------------------------------------------------
#endif
