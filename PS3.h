
#include "EXComm.h"

#ifdef ENABLE_PS3
#ifndef __PS3_h
#define __PS3_h
#include <arduino.h>


void notify();
void onConnect();
void iconeBatterie();
void MainMenu();
void Fonction(byte NumFonc);
void Affiche();
void TexteBas1();
void TexteBas2();
void TexteBas3();
//void entradaComandos ();
class PS3: public EXCommItem {
  public:
    PS3();
    // static void setup();
    bool begin()override;
    //static void loop();
    bool loop()override;
};

#endif
#endif
