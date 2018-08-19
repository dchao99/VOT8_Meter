/*
 VOT-8 Voltmeter with 3 digit 7 segment display
 Created by: David Chao (07-29-2018)
 ADC Updates: 3/sec
 Display Refresh Rate: 100Hz
 Significant digits: 3
 
 Notes: Because the lower Vf (1.8V) of the red LED. When running on 5V, you should stil add 
  a small resistors in the DP segment in case too much peak current is dumped through it.  
  DP is the only segment which can be ON by itself. Every digits uses at least 2 segments, 
  and the peak current is shared.
  VOL + VOH = (Vcc-Vf) / 2 = (5.0 - 1.8) / 2
  VOH = VOL = 1.6V
  According to the datasheet (Fig: I/O Pin Current vs Output Voltage):
  Vcc=5.0 and VOH=1.6 => IOH is 60mA
  This is too close to the red LED's peak forward current of 40-60mA.
  When Vcc=3.3V, max current per I/O pin is only 25mA, there is no worry of over-driving a LED.
*/

//Don't turn on DEBUG if we're using the TX and RX pins to drive the display
//#define DEBUG

// Display Brightness
// Each digit is on for a certain amount of microseconds. Then it is off until we have 
// reached a total of 10ms for the function call displayNumber().
// (This gives a refresh rate of 100Hz and there will be no flickering to the eyes.)
// Because all 7 segments are tied to a common anode (or cathode), the LED's are brighter 
// if a digit uses less segments.  In this code, we will scale down the brightness 
// of every digit individually depending on the number of segments used.

//Let's define a constant called DISPLAY_BRIGHTNESS that varies from:
//3000 blindingly bright 
//2000 shockingly bright
//1000 high 
// 500 normal
// 200 low
// 100 dim but readable 
//  50 dim but readable 
//   5 very very dim but readable 
#define DISPLAY_BRIGHTNESS 2500 //us
#define DISPLAY_REFRESH      10 //ms

//LED type is Common Anode
#define DIGIT_ON    HIGH
#define DIGIT_OFF   LOW
#define SEGMENT_ON  LOW
#define SEGMENT_OFF HIGH

//Pin mapping from Arduino to the ATmega DIP28 if you need it
//http://www.arduino.cc/en/Hacking/PinMapping

const int digit1 = 1;  //PWM Display pin 12 (D1 is also the TX pin)
const int digit2 = 2;  //PWM Display pin 9
const int digit3 = 0;  //PWM Display pin 8  (D0 is also the RX pin)
const int segA = 20;   //Display pin 11 (UNO: Use D6 because D20 used for xtal)
const int segB = 8;    //Display pin 7
const int segC = 10;   //Display pin 4
const int segD = 12;   //Display pin 2
const int segE = 13;   //Display pin 1
const int segF = 21;   //Display pin 10 (UNO: Use D7 because D21 used for xtal)
const int segG = 9;    //Display pin 5
const int segDP = 11;  //Display pin 3

// Analog Hardware Constants
const int sensorPin = A0;
const double scaling = 24.0;      // maximum voltmeter range

//ADC Oversampling Variables
//http://ww1.microchip.com/downloads/en/AppNotes/doc8003.pdf
//f(oversamp) = 4^n * f(nyquist)
const int overSamples = 32;     //oversample size (oversamp)
const int additionalBits = 2;   //increased bits in resolution (n), 
int adcCount = 0;
unsigned long adcTotal = 0UL;
double milliVolts = 0.0;

/*
 * Initiallization code
 */
void setup() {
  #ifdef DEBUG
  Serial.begin(19200);  // initialize serial communication at 19200 bps
  #endif
  
  pinMode(segA, OUTPUT);
  pinMode(segB, OUTPUT);
  pinMode(segC, OUTPUT);
  pinMode(segD, OUTPUT);
  pinMode(segE, OUTPUT);
  pinMode(segF, OUTPUT);
  pinMode(segG, OUTPUT);
  pinMode(segDP, OUTPUT);
  
  lightNumber(-1);    //Turn off all segments
  
  pinMode(digit1, OUTPUT);
  pinMode(digit2, OUTPUT);
  pinMode(digit3, OUTPUT);

  //The DEFAULT analog reference is Vcc (5V ~ 3.3v)
  //The INTERNAL reference, equal to 1.1V on the ATmega168 or ATmega328P,
  //and 2.56 volts on the ATmega8
  analogReference(INTERNAL);  

  //Initiallize ADC and oversampling variables
  adcCount = 0;
  adcTotal = 0UL;
  milliVolts = 0.0;
}

/*
 * Main Program, run repeatedly
 */
void loop() 
{
  adcTotal += analogRead(sensorPin);
 
  adcCount++;
  if (adcCount == overSamples) {
    adcCount = 0;
    bool roundup = adcTotal && (B00000001 << (5-1-additionalBits));
    //Decimate useless extra LS bits
    unsigned int adcDecimated = adcTotal >> (5-additionalBits);  
    if (roundup)
      adcDecimated += 1;        
    adcTotal = 0UL;
    milliVolts = adcDecimated * scaling * 1000 / (0x3ff << additionalBits);
  }
  
  #ifdef DEBUG
  Serial.println("Accumulator:"+String(adcTotal)+"  Decimated:"+String(milliVolts)+"mV");
  #endif
  
  displayNumber(milliVolts/10);  //Display voltage in (V) and two decimal places
  //call will return after 10ms
}

/*
 * Display a number between 0 - 99.99, on a 3 bit 7-segment LED display. 
 * The number is scaled by 100 before calling, the default decimal places is 2.
 * To fit a number >= 10 on the display, we need to reduce the decimal place 
 * down to 1, so the display can accomodate two integer digits.
 */
void displayNumber(int toDisplay) 
{
  long beginTime = millis();
 
  //If actual number is less than 10, display to two decimal places
  //otherwise, display to just one decimal place
  //you can set decimalPlaces to -1 to turn off all DP LED's
  int decimalPlaces = 0;
  if (toDisplay < 1000)
    decimalPlaces = 2;
  else {
    decimalPlaces = 1;
    toDisplay /= 10;
  }

  for(int digit = 3 ; digit > 0 ; digit--) {

    //Scanning through each digit in turns
    switch(digit) {
    case 1:
      digitalWrite(digit1, DIGIT_ON);
      break;
    case 2:
      digitalWrite(digit2, DIGIT_ON);
      break;
    case 3:
      digitalWrite(digit3, DIGIT_ON);
      break;
    }

    //Turn on coresponding segments for this digit
    int us = lightNumber(toDisplay % 10);  
    delayMicroseconds(us); 

    //Turn off all segments and all digits
    lightNumber(-1);
    digitalWrite(digit1, DIGIT_OFF);
    digitalWrite(digit2, DIGIT_OFF);
    digitalWrite(digit3, DIGIT_OFF);
      
    toDisplay /= 10;
    
  } //end of for loop

  //Draw the decimal point
  switch(decimalPlaces) {
  case 0:
    digitalWrite(digit3, DIGIT_ON);
    break;  
  case 1:
    digitalWrite(digit2, DIGIT_ON);
    break;
  case 2:      
    digitalWrite(digit1, DIGIT_ON);
    break;
  }
  digitalWrite(segDP, SEGMENT_ON);

  if (decimalPlaces != -1)
    //Draw the decimal point 
    delayMicroseconds(DISPLAY_BRIGHTNESS/2.5); 

  //Turn off all segments
  lightNumber(-1); 

  //Turn off all digits
  digitalWrite(digit1, DIGIT_OFF);
  digitalWrite(digit2, DIGIT_OFF);
  digitalWrite(digit3, DIGIT_OFF);
    
  //Wait for 10ms to pass before we paint the display again
  while( (millis() - beginTime) < DISPLAY_REFRESH) ; 
}

/*
 * Given a number, turns on those segments
 * If number == -1, then turn off all segments
 * Returns: A modified display duration (microseconds), this makes all segments even brightness 
 * without the use of current limiting resistors.
 */
int lightNumber(int numberToDisplay) 
{
  switch (numberToDisplay){

  case 0:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_OFF);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1.2);
    break;

  case 1:
    digitalWrite(segA, SEGMENT_OFF);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_OFF);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_OFF);
    digitalWrite(segG, SEGMENT_OFF);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/2.0);
    break;

  case 2:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_OFF);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_OFF);
    digitalWrite(segG, SEGMENT_ON);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1.4);
    break;

  case 3:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_OFF);
    digitalWrite(segG, SEGMENT_ON);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1.4);
    break;

  case 4:
    digitalWrite(segA, SEGMENT_OFF);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_OFF);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1.6);
    break;

  case 5:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_OFF);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1.4);
    break;

  case 6:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_OFF);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1.2);
    break;

  case 7:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_OFF);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_OFF);
    digitalWrite(segG, SEGMENT_OFF);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1.8);
    break;

  case 8:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_ON);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1);
    break;

  case 9:
    digitalWrite(segA, SEGMENT_ON);
    digitalWrite(segB, SEGMENT_ON);
    digitalWrite(segC, SEGMENT_ON);
    digitalWrite(segD, SEGMENT_ON);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_ON);
    digitalWrite(segG, SEGMENT_ON);
    digitalWrite(segDP, SEGMENT_OFF);
    return(DISPLAY_BRIGHTNESS/1.1);
    break;

  case -1:
    digitalWrite(segA, SEGMENT_OFF);
    digitalWrite(segB, SEGMENT_OFF);
    digitalWrite(segC, SEGMENT_OFF);
    digitalWrite(segD, SEGMENT_OFF);
    digitalWrite(segE, SEGMENT_OFF);
    digitalWrite(segF, SEGMENT_OFF);
    digitalWrite(segG, SEGMENT_OFF);
    digitalWrite(segDP, SEGMENT_OFF);
    return(0);
    break;
  }
}

