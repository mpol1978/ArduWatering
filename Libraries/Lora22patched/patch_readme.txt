
Patch alla libreria per poterla utilizzare insieme alla libreria RTCLibMod.
Nella versione originale la definizione del simbolo "ss" era in conflitto con un membro di una classe della libreria RTCLib

In particolare modificati i due sorgenti: SX1278.h, SX1278.cpp

  SX1278.h

#if defined (__AVR_ATmega32U4__)
 #define ss 17
#else
 #define ss 10            // pin for chip select
#endif

to:

#if defined (__AVR_ATmega32U4__)
 #define cs_pin 17
#else
 #define cs_pin 10            // pin for chip select
#endif

  SX1278.cpp

Sostituite tutte le occorrenze di ss con cs_pin