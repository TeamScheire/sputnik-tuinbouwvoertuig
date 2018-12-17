/* Arduino Code voor de beide arduino nano in de tuinbouwmachine van Bart. Deze code kan voor andere closed loop DC motoren gebruikt worden
 * pin 2 en 3 optische encoder op de motor
 * pin4 +5V voor de encoder
 * pin8 +5V voor de H-brug logica 
 * pin 13 led pin en test pin
 * pin 9 en pin 10: PWM uitgangen naar de H-brug
 
 */

int Control = 0;            // Interne waarde van de potentiometer voor de snelheid
int ControlPower = 0;       // 
int ExtraPower = 0;
int Gear = 0;               // 1 tot 5 voor de versnellingen: 0 = achteruit, 1 = neutraal, 2 = 1ste, 3 = 2de, 4 = 3de vooruit

int PWMout1 = 0;        // Interne waarde voor PWM1 (analog out)
int PWMout2 = 0;        // Interne waarde voor PWM2 (analog out)

#define EncPinA  2  // interrupt pin
#define EncPinB  3  // een andere pin, zou ook interrupt kunnen geven
volatile int EncPos = 512;

void setup() {

  Serial.begin(115200);                                         // Start de seriele communicatie
  pinMode(EncPinA, INPUT);   digitalWrite(EncPinA, HIGH);       // encoder pinnen input en pull-up weerstand
  pinMode(EncPinB, INPUT);   digitalWrite(EncPinB, HIGH);       // encoder pinnen input en pull-up weerstand

  pinMode(4, OUTPUT);   digitalWrite(4, HIGH);                  // 5V voor de Encoder
  pinMode(8, OUTPUT);   digitalWrite(8, HIGH);                  // 5V voor de H-Bridge
  pinMode(13, OUTPUT);  digitalWrite(13, LOW);                  // Test LED 
  
  pinMode(9, OUTPUT);                                           // PWM Pin 1
  pinMode(10, OUTPUT);                                          // PWM Pin 2
  ControlPower = 127;

  //Setup Timer2 overflow to 16ms as watchdog timer - Zie internet od atmega datasheet
  TCCR2B = 0x00;        //Disbale Timer2 while we set it up
  TCNT2  = 0;           //Reset Timer Count to 0 out of 255
  TIFR2  = 0x00;        //Timer2 INT Flag Reg: Clear Timer Overflow Flag
  TIMSK2 = 0x01;        //Timer2 INT Reg: Timer2 Overflow Interrupt Enable
  TCCR2A = 0x00;        //Timer2 Control Reg A: Wave Gen Mode normal
  TCCR2B = 0x07;        //Timer2 Control Reg B: Timer Prescaler set to 1024

  interrupts();  // zet de interrupts hoofdschakelaar aan
}

void loop() {
  TCNT2 = 0;  // reset timer 2 = onze eigen watchdogtimer
  digitalWrite(13, HIGH);   // Om te tijd van 1 controle lus te kunnen checken met een oscilloscoop
  digitalWrite(13, LOW);    // Genereren we hier een puls

  Control = analogRead(A7);               // Lees de gewenste snelheid uit, potentiometer  van Bart
  //           if (Control < 10) { delay(10);}       // **** Watchdog Timer (timer2) Test code
  
  Gear = map(analogRead(A6),0,1024,0,5);   // verdeel de versnelingspotentiometer in 5 levels
  switch (Gear) {

     case 0:
       Serial.print("R ");     // Achteruit     -  geen encoder feedback - differentieel 
       Control = map(analogRead(A7),0,1023,256,512);
       break;
     case 1:
       Serial.print("N ");     // Neutraal - geen aansturing - differentieel 
       Control = 512;
       break; 
     case 2:
       Serial.print("1 ");      //  1st versnelling -  met ecoder feedback - starre achteras
       Control = map(analogRead(A7),0,1023,512+64,512);
       break;
     case 3:
       Serial.print("2 ");     //  2de versnelling -  met encoder feedback - starre achteras
       Control = map(analogRead(A7),0,1023,512+128,512); 
       break;   
     case 4:
       Serial.print("3 ");     //  3de versnelling - zonder encoder feedback- differentieel 
       Control = map(analogRead(A7),0,1023,512+512,512); 
       break;
    }
     

  PWMout1 = 1;      // We gaan er van uit dat de motor moet stilstaan, indien niet wordt dit in de volgende lijnen aangepast  
  PWMout2 = 1;  


  // Volgende lijnen: zet de PWM uitgangen afhankelijk van gevraagde snelheid en keuze van de versnelling en richting
  if (Gear == 0) { PWMout1 = 1; PWMout2 = map(Control,512,256,0,128); }  // Geen encoder feedback  
  if (Gear == 4) { PWMout1 = map(Control,512,1024,0,255); PWMout2 = 1; } // Geen encoder feedback
  if ((Gear == 2) or (Gear == 3)) {          
    ControlPower = (Control + ExtraPower) / 4 ;                          // met encoder feedback
    if (ControlPower <   0) {ControlPower =   0;}
    if (ControlPower > 255) {ControlPower = 255;}
    if (ControlPower > 127) {PWMout1 = map(ControlPower,128,255,5,255); PWMout2 = 1; }  
    if (ControlPower < 127) {PWMout2 = map(ControlPower,126,  0,5,255); PWMout1 = 1; }
    }
  // In neutraal laten we ze beide op 1 staan

  analogWrite(9, PWMout1);  // PWM1 effectief naar de H-Brug sturen
  analogWrite(10, PWMout2); // PWM2 effectief naar de H-Brug sturen 
  
  EncPos = 512; // 512 = nul, higher is one direction, lower than 512 is the other direction
  attachInterrupt(0, doEncoder0, CHANGE);  // encoder pin on interrupt 0 - pin 2
  delay(10);                               // Tel pulsen gedurende 10 ms  
  detachInterrupt(0);                      // Zet interrupt terug af


  ExtraPower = ExtraPower + 1 * (Control - EncPos);
  if (ExtraPower < -250) {ExtraPower = -250;}
  if (ExtraPower >  250) {ExtraPower =  250;}

  /*  // Om te debuggen kan je data doorsturen naar de PC
  Serial.print(EncPos);
  Serial.print("\t  ");
  Serial.print(Control);
  Serial.print("\t  ");
  Serial.print(ExtraPower );
  
  Serial.print("\t 1L ");
  Serial.print(PWMout1 );
  Serial.print("\t 2L ");
  Serial.print(PWMout2 );
  Serial.println( );
  */ 
}
// Einde Hoofdprogramma

// Interrupt Service Routine voor de Encoder uit te lezen en om te zetten naar snelheid met 512 als stilstand
void doEncoder0() {
  if (digitalRead(EncPinA) == digitalRead(EncPinB)) {
    if (EncPos < 1024)  EncPos=EncPos+1;      // We zorgen dat de positie steeds tussen 0 en 1024 blijft
  } else {
    if (EncPos > 0)    EncPos=EncPos-1;       // We zorgen dat de positie steeds tussen 0 en 1024 blijft
  }
}

// Volgende code is voor onze eigen watchdog functie. Als de timer overflowed (dan loopt het programma niet zoals verwacht) resetten we de Arduino
void(* resetFunc) (void) = 0; //declare reset function @ address 0

ISR(TIMER2_OVF_vect) { // Timer2 = Watchdog interrupt
  TIFR2 = 0x00;        // reset timer 2 = onze eigen watchdogtimer
  Serial.print("**");
  resetFunc();         // call reset, herstart de Arduino
}
