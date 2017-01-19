////////////////////////////////////////////////////////////////////////////
//                                                                        //
//  ROTARY PHONE GSM CONVERTER WITH SUBSCRIBER LINE INTERFACE CIRCUIT     //
//                                                                        //
//  By Johan Berglund, November 2016                                      //
//                                                                        //
//  For Arduino, QCX601 SLIC board and SIM900 or SIM800 breakout board    //
//                                                                        // 
////////////////////////////////////////////////////////////////////////////

#define GSM_MODULE
//#define SLIC_TEST

#if defined(GSM_MODULE)
#include "SIM900.h"
#include <SoftwareSerial.h>
#include "call.h"
#endif

// states of state machine 
#define IDLE_WAIT 1
#define RINGING 2
#define ACTIVE_CALL 3
#define GETTING_NUMBER 4


#define rflPin 6        // RFL pin to QCX601 pin J1-1, reverse signal (LOW)
#define hzPin 7         // 25Hz pin to QCX601 pin J1-2 (25Hz signal for ringing) _-_-_-_-_
#define rcPin 8         // RC pin to QCX601 pin J1-3, ring control, HIGH when ringing --___--___--___
#define shkPin 9        // switch hook pin from QCX601 pin J1-4

                        // Change the pins above as is suits your project
                        // For connection to GSM board, pin 2 goes to TX and pin 3 to RX
                        // To change this, edit GSM.cpp in the GSM library
                        
#if defined(SLIC_TEST)
#define ringTestPin 12 
#endif                       

#define oscInterval 20
#define ringInterval 6000
#define ringDuration 1200
#define statusCheckInterval 1000

#define tNewDig 500     // time since last SHK rising edge before counting for next digit
#define tHup 2000       // time since last SHK falling edge before hanging up/flushing number
#define tComplete 6000  // time since last SHK rising edge before starting call
#define tDebounce 15    // debounce time

unsigned long currentMillis = 0L;
unsigned long oscPreviousMillis = 0L;
unsigned long ringPreviousMillis = 0L;
unsigned long statusPreviousMillis = 0L;
unsigned long lastShkDebounce = 0L;
unsigned long lastShkRise = 0L;
unsigned long lastShkFall = 0L;

int shkState = 0;           // LOW is on hook, HIGH is off hook
int edge;
int lastShkReading = 0;
int currentShkReading = 0;

#if defined(SLIC_TEST)
int ringTest; 
#endif  

byte pulses = 0;
byte digit = 0;
String number = "";
byte digits = 0;            // the number of digits we have collected
char numArray[11];
byte gsmStatus;
byte state;

#if defined(GSM_MODULE)
CallGSM call;
#endif

////////////////////////////////////////////////////////////////////////////
// 
//  SETUP
//

void setup() {
  pinMode(shkPin, INPUT);
  pinMode(hzPin, OUTPUT);
  pinMode(rcPin, OUTPUT);
  pinMode(rflPin, OUTPUT);
  digitalWrite(rflPin, 0);
  #if defined(SLIC_TEST)
  pinMode(ringTestPin, INPUT_PULLUP);
  #endif

  Serial.begin(9600); // start serial for debug monitor
  Serial.println("\nHello, I am your telephone.");
  #if defined(GSM_MODULE)
  if (gsm.begin(2400)) {
    Serial.println("\nstatus=READY");
  } else {
    Serial.println("\nstatus=IDLE");
  }
  #endif
  state = IDLE_WAIT;
}

////////////////////////////////////////////////////////////////////////////
// 
//  MAIN LOOP
//

void loop() {

  currentMillis = millis(); // get snapshot of time for debouncing and timing

  // read and debounce hook pin
  currentShkReading = digitalRead(shkPin);
  if (currentShkReading != lastShkReading) {
    // reset debouncing timer
    lastShkDebounce = currentMillis;
  }
  if ((unsigned long)(currentMillis - lastShkDebounce) > tDebounce) {
    // debounce done, set shk state to debounced value if changed
    if (shkState != currentShkReading) {
      shkState = currentShkReading;
      if (shkState == HIGH) {
        lastShkRise = millis();
      } else {
        lastShkFall = millis();  
      }
    }
  }
  lastShkReading = currentShkReading;

  if ((unsigned long)(currentMillis - statusPreviousMillis) >= statusCheckInterval) {
    // Time to check status of GSM board
    #if defined(GSM_MODULE)
    if (!digitalRead(rcPin) && (state != GETTING_NUMBER)) { // or software serial will interfere with ringing and dialing
      gsmStatus = call.CallStatus();
    }
    #endif
    #if defined(SLIC_TEST)
    ringTest = !digitalRead(ringTestPin);
    #endif
    statusPreviousMillis = currentMillis;
  }


  ////////////////////////////////////////////////////////////////////////////
  // 
  //  STATE IDLE_WAIT
  //
  if (state == IDLE_WAIT) {
    // wait for incoming call or picking up reciever
    #if defined(GSM_MODULE)
    if (gsmStatus == CALL_INCOM_VOICE) {
      Serial.println("Incoming call. Ringing.");
      state = RINGING;
    }
    if (gsmStatus == CALL_ACTIVE_VOICE) {
      // this should not happen
      // hanging up must have failed, try again
      call.HangUp();
      delay(2000); 
    }
    #endif
    #if defined(SLIC_TEST)
    if (ringTest) {
      Serial.println("Incoming call. Ringing.");
      state = RINGING;
    }
    #endif    
    if (shkState == HIGH) {
      Serial.println("Off hook. Dial your number.");
      state = GETTING_NUMBER;
    }
  }
  ////////////////////////////////////////////////////////////////////////////
  // 
  //  STATE RINGING
  //
  if (state == RINGING) {  
    // ring until reciever is off hook or call disconnected
    // Ringing interval 
    // How much time has passed, accounting for rollover with subtraction!
    if ((unsigned long)(currentMillis - ringPreviousMillis) >= ringInterval) {
      digitalWrite (rcPin,1); // Ring
      // Use the snapshot to set track time until next event
      ringPreviousMillis = currentMillis;
    }
    if (digitalRead(rcPin) && ((unsigned long)(currentMillis - ringPreviousMillis) >= ringDuration)) {
        digitalWrite(rcPin, 0); // Silent after ring duration
    }
    // 25Hz oscillation      
    // How much time has passed, accounting for rollover with subtraction!
    if ((unsigned long)(currentMillis - oscPreviousMillis) >= oscInterval) {
      // It's time to do something!
      if (digitalRead(rcPin)) {
        digitalWrite(hzPin, !digitalRead(hzPin)); // Toggle the 25Hz pin
      }    
      // Use the snapshot to set track time until next event
      oscPreviousMillis = currentMillis;
    }
    if (shkState == HIGH) {
      digitalWrite(rcPin, 0); // stop ringing
      digitalWrite(hzPin, 0);
      // tell GSM board to pick up
      Serial.println("Picking up. Call initiated.");
      #if defined(GSM_MODULE)
      call.PickUp();
      #endif   
      state = ACTIVE_CALL; 
    }
    #if defined(GSM_MODULE)
    if (gsmStatus != CALL_INCOM_VOICE) {
      digitalWrite(rcPin, 0); // stop ringing
      digitalWrite(hzPin, 0);
      Serial.println("Caller gave up. Going back to idle.");
      state = IDLE_WAIT;
    }
    #endif
    #if defined(SLIC_TEST)
    if (!ringTest) {
      digitalWrite(rcPin, 0); // stop ringing
      digitalWrite(hzPin, 0);
      Serial.println("Caller gave up. Going back to idle.");
      state = IDLE_WAIT;
    }
    #endif
  }
  ////////////////////////////////////////////////////////////////////////////
  // 
  //  STATE ACTIVE_CALL
  //
  if (state == ACTIVE_CALL) {
    // keep connection until on-hook or call disconnected
    if ((shkState == LOW) && ((unsigned long)(currentMillis - lastShkFall) >= tHup)) {
      // tell GSM board to disconnect call, flush everything, then go idle
      Serial.println("Hanging up. Going idle.");
      #if defined(GSM_MODULE)
      call.HangUp();
      #endif
      flushNumber();
      delay(1000); // wait a sec before going to next state to make sure GSM module keeps up
      state = IDLE_WAIT;
    }
    #if defined(GSM_MODULE)
    if (gsmStatus == CALL_NONE) {
      Serial.println("Call disconnected. Going idle.");
      flushNumber();
      state = IDLE_WAIT;
    }
    #endif
    #if defined(SLIC_TEST)
    if (!ringTest) {
      Serial.println("Call disconnected. Going idle.");
      flushNumber();
      state = IDLE_WAIT;
    }
    #endif
  }
  ////////////////////////////////////////////////////////////////////////////
  // 
  //  STATE GETTING_NUMBER
  //
  if (state == GETTING_NUMBER) {
    // count groups of pulses on SHK (loop disconnect) until complete number
    // if single digit, fetch stored number
    // then make call

    if (pulses && (unsigned long)(currentMillis - lastShkRise) > tNewDig) {
      // if there are pulses, check rising edge timer for complete digit timeout
      digit = pulses - 1; // one pulse is zero, ten pulses is nine (swedish system)
      // for systems where ten pulses is zero, use code below instead:
      // digit = pulses % 10;
      Serial.println(digit); // just for debug
      // add digit to number string
      number += (int)digit;
      digits++;
      pulses = 0;
    }

    if ((shkState == LOW) && (edge == 0)) {
      edge = 1;
    } else if ((shkState == HIGH) && (edge == 1)) {
      pulses++;
      Serial.print(". "); // just for debug . . . . .
      edge = 0;
    }
    
    if ((digits && (shkState == HIGH) && ((unsigned long)(currentMillis - lastShkRise) > tComplete)) || digits == 10) {
      // if completed number (full 10 digits or timeout with at least one digit)
      // check if shortnumber/fave and then tell GSM board to initiate call 
      if (digits == 1) getFave();
      Serial.print("Number complete, calling: ");
      Serial.println(number);
      number.toCharArray(numArray, 11);
      #if defined(GSM_MODULE)
      call.Call(numArray);
      #endif
      delay(1000); // wait a sec before going to next state to make sure GSM module keeps up
      state = ACTIVE_CALL;
    }
    if ((shkState == LOW) && (unsigned long)(currentMillis - lastShkFall) > tHup) {
      // reciever on hook, flush everything and go to idle state
      flushNumber();
      Serial.println("On hook. Flushing everything. Going idle.");
      state = IDLE_WAIT;
    }
  }
}  // END OF MAIN LOOP


////////////////////////////////////////////////////////////////////////////
// 
//  FUNCTIONS
//

void getFave() {
  if (number == "1\0"){
    number = "9876543210"; // Significant other
    digits = 10;
  }
  if (number == "2\0"){
    number = "0123456789"; // Best friend
    digits = 10;
  }  
  if (number == "3\0"){
    number = "1234567890"; // fave #3... and so on...
    digits = 10;
  }
  // just add more faves here
}

void flushNumber() {
  digits = 0;
  number = "";
  pulses = 0;
  edge = 0;
}


