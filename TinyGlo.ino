#include <avr/sleep.h>
#include <avr/wdt.h>

#define fiveVolt 1
#define led  0
#define analogIn 2
#define battChgStat 4
#define photo 3
#define numBlinks 75
#define photoThresh 100

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int blinkCtr = numBlinks; //number of blinks before sleeping for the night
byte lightStat=1;
volatile boolean f_wdt = 1;
byte chgLed=0;
byte phStat=0;

void setup() {                
  pinMode(analogIn, INPUT);  
  setup_watchdog(8);
  pinMode(photo, INPUT);
  pinMode(led, INPUT);
  pinMode(battChgStat, INPUT);
  randomSeed(analogRead(analogIn) + analogRead(photo));
}

// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep() {
  //f_wdt=0;                             // reset flag
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9=8sec
void setup_watchdog(int ii) {

  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}
  
// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
  //f_wdt=1;  // set global flag
}

/////////////////////////////////////////////////////////////

void fadeLED(void){
  byte ctr=0;
  
  pinMode(led, OUTPUT);     
  for(ctr=0;ctr<9;ctr++){
    analogWrite(led,ctr);
    delay(150);
    }

  delay(100 * random(20,35));

  for(ctr=8;ctr>0;ctr--){
    analogWrite(led,ctr);
    delay(150);
    }
  digitalWrite(led,LOW);
  pinMode(led, INPUT);     
  }

/////////////////////////////////////////////////////////////
// Checks phototransistor light detected.  
// Hysteresis incorporated to help avoid false detections

byte chkPhoto(void){
  phStat=analogRead(photo);

  if(phStat == 0){
    if(lightStat != 0){
      naptime(3);
      phStat=analogRead(photo);
      }
    if(phStat == 0){
      lightStat = 0;
      return(0);
      }
    }

  else if(phStat > photoThresh){
    if(lightStat != 2){
      naptime(3);
      phStat=analogRead(photo);
      }
    if(phStat > photoThresh){
      lightStat = 2;
      return(2);
      }
    }

  else{
    lightStat = 1;
    return(1);
    }
  }

/////////////////////////////////////////////////////////////

void naptime(byte hcount){
  byte hctr=0;
  for(hctr=0;hctr<hcount;hctr++)
    system_sleep();
  }

////////////////////////////////////////////////////////////

void blinkLed(int freq){
  chgLed = chgLed ^ 1;
  if(chgLed){
    pinMode(led, OUTPUT);  
    analogWrite(led,20);
    delay(freq);
    digitalWrite(led,LOW);
    pinMode(led, INPUT);  
    }
   else
     delay(freq);
   }


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void loop() {
    
  if (digitalRead(fiveVolt) == 0) {   // Check if not plugged into a 5V USB power source
    switch( chkPhoto() ){  //Measure ambient light level
      case 0:    // Darkness detected
        if(blinkCtr > 0){
          blinkCtr--;
          naptime( random(2,16) );
          fadeLED();
          }
        else
          naptime(3);
        break;
      case 1:  // Ambient light is medium
        naptime(3);
        break;
      case 2:  // Ambient light is bright
        blinkCtr=numBlinks;  // Reset blink counter
        naptime(3);
        break;
      default:
        naptime(3);
      }
    }
  else{
    if(digitalRead(battChgStat) == 1)   // See if battery is being charged or not
      system_sleep();  // If battery is charged, sleep
    else{
      blinkLed(100);   // If battery is being charged, blink to indicate charging in progress
      system_sleep();
      }
    } 
  }
