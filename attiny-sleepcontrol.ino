/**************************************************************************************************************
  +
  + Quelle: https://github.com/8n1/ESP8266-Tiny-Door-and-Window-Sensor/blob/master/Firmware/ATtiny%20Arduino%20Sketch/tiny-vreg-controller/tiny-vreg-controller.ino
  +  Fuses:            -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m  (default value)

  +
  +  Getestet mit der Arduino IDE v1.6.13
  +  Voraussetzung zum kompilieren: ATtiny unterstützung ist installiert.
  +  (Am besten über den neuen Boards Manager machen: Tools->Board:...->Boards Manager->"attiny")
  +
  +  Pinout:
  +                   +-------------+
  +                 +-+ (PB5)   VCC +-+  BATT_LOW (Batteriespannung ueber Schottkyy-Diode)
  +                   | (Reset)     |
  +                   |             |
  +  DEBUG_LED      +-+ PB3     PB2 +-+  ESP_OFF (OUTPUT)
  +  (OUTPUT)         |             |
  +                   |             |
  +  SHUTDOWN       +-+ PB4     PB1 +-+  SWITCH_STATE (OUTPUT)
  +  (INPUT)          |             |
  +                   |             |
  +  GND            +-+ GND     PB0 +-+  SWITCH (INPUT)
  +                   +-------------+
  +
  +  Pin Funktionen:
  +  PB0 - SWITCH              - Umschalter, über den der ATtiny aus dem Tiefschlaf geweckt wird
  +  PB1 - SWITCH_STATE        - Der Zustand des Schalters wird für den ESP an diesem Pin ausgegeben
  +  PB2 - ESP_OFF             - Gate des MOSFET, High = ESP off, LOW = ESP on
  +  PB4 - SHUTDOWN            - Ein kurzes HIGH Signal an diesem Pin (vom ESP an den Tiny) bewirkt Abschalten des ESP und Sleep des Tiny
  +
  +  Ablauf:
  +  1. -> Der ATtiny wird durch einen PinChange(PC) Interrupt (Wechsel sowohl von HIGH nach LOW als auch umgekehrt möglich) am Pin PB0 geweckt.
  +  2. -> Der Tiny setzt PB2 auf LOW und aktiviert den MOSFET (und somit den ESP).
  +  3. -> Jetzt wird gewartet bis (vom ESP) ein Signal über PB4 kommt das den ATtiny veranlasst den ESP wieder abzuschalten,
         Ausserdem wird der Zustand des Schalters(PB0) immer wieder neu eingelesen und über PB1 ausgegeben.
  +  4. -> Ist nach "timerInterval" Sekunden noch kein Signal gekommen wird der ESP vom ATtiny wieder abgeschaltet -> PB2 geht auf HIGH
  +        Zusätzlich wird auch PB1 auf LOW gesetzt
  +  5. -> Zu guter letzt geht der ATtiny wieder in den Tiefschlaf(Power-down) und wartet auf den nächsten Interrupt.
  +        Der Stromverbrauch des ATtiny liegt während er schläft bei ca. ~300nA @3.8V. Im Datenblatt steht genauers.
  +
**************************************************************************************************************/

// Attiny-Sleepcontrol-0057.ino


#include <avr/sleep.h>

//------------------------------------------------------------------------------
// Pin Konfiguration
const int SWITCH = 0;               // PB.0
const int SWITCH_STATE = 1;         // PB.1
const int ESP_OFF = 2;              // PB.2
const int DEBUG_LED = 3;            // PB.3 
const int SHUTDOWN = 4;             // PB.4

// Anzahl Sekunden nach denen der ESP automatisch wieder abgeschaltet wird, wenn der ESP kein Abschaltsignal geliefert hat
const unsigned long timerInterval = 20;

// Arbeitsvariablen
unsigned long start_time;
boolean goto_sleep = true;
int switch_state_old;


//------------------------------------------------------------------------------
// PinChange ISR - Einstiegspunkt nach dem Tiefschlaf

ISR (PCINT0_vect) {
    PCMSK &= ~(1 << PCINT0);                                // Port Change Interrupts vorübergehend verbieten
    sleep_disable();                                        // Schlafmodus deaktivieren
    digitalWrite(DEBUG_LED, HIGH);                          // Timer und Funktionen, die Timer benutzen
    delayMicroseconds(500);                                 // können in der ISR nicht benutzt werden,
    digitalWrite(DEBUG_LED, LOW);                           // also z.B. delay() und serielle Schnittstelle
    delayMicroseconds(500);
    digitalWrite(DEBUG_LED, HIGH);
    delayMicroseconds(500);
    digitalWrite(DEBUG_LED, LOW);
    delayMicroseconds(500);
  }


//------------------------------------------------------------------------------
// SETUP FUNCTION
void setup()
{
  // GPIOs konfigurieren
  pinMode(ESP_OFF, OUTPUT);
  pinMode(SHUTDOWN, INPUT);
  pinMode(SWITCH_STATE, OUTPUT);
  pinMode(SWITCH, INPUT);                                   // SWITCH input bekommen keinen pullup, da Wechselschalter angeschlossen wird. Pullup würde unnoetigen Strom ziehen
  pinMode(DEBUG_LED, OUTPUT);
  digitalWrite(DEBUG_LED, LOW);                             // Pullup Widerstand für den unbenutzen Pin: Floating vermeiden (Stromverbrauch!)

  ADCSRA &= ~(1 << ADEN);                                   // AD Wandler abschalten (benötigt nur unnötig Strom, ~230µA)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);                      // Schlafmodus(Power-down) konfigurieren
  GIMSK |= (1 << PCIE);                                     // PinChange(PC) Interrupts erlauben
                                                            // GIMSK = General Interrupt Mask Register
                                                            // Bit    7   6   5   4   3   2   1   0
                                                            //        - INT0 PCIE -   -   -   -   -   
                                                            // GIMSK = GIMSK OR 32 = PCIE wird "1" gesetzt
}


//------------------------------------------------------------------------------
// MAIN LOOP
void loop()
{
  if (goto_sleep == true)                                   // Flag für schlafen gehen gesetzt?
  {
    digitalWrite(SWITCH_STATE, LOW);                        // SWITCH_STATE beim Schlafen auf LOW setzen, weil dieser Pin einen spannungslosen ESP
                                                            // treibt, bei HIGH => Spannung am IO-Port > Vcc, das ist nicht erlaubt
    digitalWrite(ESP_OFF, HIGH);                            // ESP abschalten


    digitalWrite(DEBUG_LED, HIGH);                          
    delayMicroseconds(100);                                
    digitalWrite(DEBUG_LED, LOW);                          
    delayMicroseconds(100);
    digitalWrite(DEBUG_LED, HIGH);
    delayMicroseconds(100);
    digitalWrite(DEBUG_LED, LOW);
    delayMicroseconds(100);
    digitalWrite(DEBUG_LED, HIGH);
    delayMicroseconds(100);
    digitalWrite(DEBUG_LED, LOW);
    delayMicroseconds(100);

    
    PCMSK |= (1 << PCINT0);                                 // Port Change Interrupt an PB0erlauben
    sleep_enable();                                         // Schlafmodus aktivieren
    sleep_mode();                                           // Tiefschlaf aktivieren und auf den Port Change Interrupt warten

//------------------------------------------------------------------------------
// STARTPUNKT nach Interrupt

    // Kommt der Port Change Interrupt (Schalter wurde betätigt), wird die Port Change ISR ausgeführt, und der ATtiny ist wieder wach,
    // anschließend geht es hier weiter
    digitalWrite(DEBUG_LED, HIGH);                          
    delayMicroseconds(250);                                
    digitalWrite(DEBUG_LED, LOW);                          
    delayMicroseconds(250);
    digitalWrite(DEBUG_LED, HIGH);
    delayMicroseconds(250);
    digitalWrite(DEBUG_LED, LOW);
    delayMicroseconds(250);
   
    digitalWrite(ESP_OFF, LOW);                             // ESP einschalten
    start_time = millis();                                  // Startzeit einlesen    
    delay(100);                                             // Entprellen
    digitalWrite(SWITCH_STATE, digitalRead(SWITCH));        // Zustand des Schalters(PB0) auf PB1 für ESP ausgeben
    switch_state_old = digitalRead(SWITCH);                 // Schalterstellung beim Aufwecken merken        
    delay(400);                                             // Nötig, weil der Pullup am esp anfangs die shutdown Leitung noch auf HIGH zieht. (Schaltungs-Design-Fehler)
                                                            // wenn der ESP gestartet ist, dann zieht er shutdown auf LOW                                                    
    goto_sleep = false;                                     // Tiny wurde gerade geweckt, also jetzt nicht schlafen, sondern mit ESP arbeiten
  }

  if (millis() - start_time >= timerInterval * 1000)        // Verfügbare Zeit abgelaufen (Weil ESP keinen Shutdown angefordert hat)?
  {
    goto_sleep = true;                                      // Im nächsten Durchlauf von loop wieder schlafen gehen
  }

  if (digitalRead(SHUTDOWN) == 1)                           // Shutdown Anforderung vom ESP erkannt ?
  {
    if (switch_state_old == digitalRead(SWITCH))            // Schalterstellung ist unverändert, übertragener Status ist gültig
    {
      goto_sleep = true;
    }
    else                                                    // Schalterstellung hat sich während der übertragung des Statusses geändert, neu senden 
    {
      digitalWrite(ESP_OFF, HIGH);                          // ESP ausschalten 
      delay(5000);     
      start_time = millis();                                // Startzeit einlesen    
      digitalWrite(ESP_OFF, LOW);                           // ESP einschalten, und geänderte Schalterstellung übertragen
      delay(500);                                           // Nötig, weil der pullup am esp anfangs die shutdown leitung noch auf HIGH zieht. (Schaltungs-Design-Fehler)
      digitalWrite(SWITCH_STATE, digitalRead(SWITCH));      // Zustand des Schalters(PB0) auf PB1 ausgeben
      switch_state_old = digitalRead(SWITCH);               // Aktuelle Schalterstellung merken

    }
  }
}



