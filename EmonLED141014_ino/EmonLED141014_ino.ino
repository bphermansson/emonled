/*
Emon4Led
A 4 digit led display connected to a Arduino Mini Pro and RFM12B. 
Used as a disply for Openenergymonitor, http://openenergymonitor.org/emon/. 

Based on https://github.com/openenergymonitor/EmonGLCD/blob/master/HomeEnergyMonitor/HomeEnergyMonitor.ino
Display connected as of http://www.instructables.com/id/74HC595-digital-LED-Display-Based-on-Arduino-Code-/?ALLSTEPS

Schematic also in source folder. 

©Patrik Hermansson 2014
*/

// Connections to the shiftregisters/display. 
int latchPin = 3;
int clockPin = 5;
int dataPin = 4;
// For display
byte chars = 0;
int value, counter, statemachine;
int values[5];
/* 
Digits 0-9, c
The 8 segments in each digit is represented by a bit in numbers array. A 1 lights the segment, a zero keeps it off. 
The bits are named A-G and 'DP' and are represented in that order. This means that 'number[x]=11000000' makes the segments A and B light up.
*/
byte numbers[11]={B11111100, B01100000, B11011010, B11110010, B01100110, B10110110, B10111110, B11100000, B11111110, B11110110, B00011010};
/*
Digits are selected by setting bits in 'digit[x]'. 
To select a digit, fetch one item in digit array.
*/
byte digit[5]={B01111111, B10111111, B11011111, B11101111}; 

#include <JeeLib.h>   

#include <OneWire.h>		    // http://www.pjrc.com/teensy/td_libs_OneWire.html
#include <DallasTemperature.h>      // http://download.milesburton.com/Arduino/MaximTemperature/ (3.7.2 Beta needed for Arduino 1.0)

// For RFM12B
#define MYNODE 20            // Should be unique on network, node ID 30 reserved for base station
#define RF_freq RF12_433MHZ     // frequency - match to same frequency as RFM12B module (change to 868Mhz or 915Mhz if appropriate)
#define group 210 
int outtemp;

// DS18B20 for temperature readings
#define ONE_WIRE_BUS 6              // temperature sensor connection - hard wired 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double temp,maxtemp,mintemp;

//---------------------------------------------------
// Data structures for transfering data between units
//---------------------------------------------------
typedef struct { int temp, power2, power3, Vrms; } PayloadTX;         // neat way of packaging data for RF comms
PayloadTX emontx;


//-------------------------------------------------------------------------------------------- 
// Flow control
//-------------------------------------------------------------------------------------------- 
unsigned long last_emontx;                   // Used to count time from last emontx update
unsigned long last_emonbase;                   // Used to count time from last emontx update

void setup()
{
  Serial.begin(19200);
  delay(500); 				   //wait for power to settle before firing up the RF
  rf12_initialize(MYNODE, RF_freq,group);
  delay(100);	
  //wait for RF to settle befor turning on display
  Serial.println("Hello!");
  
  // Temp sensor
  sensors.begin();                         // start up the DS18B20 temp sensor onboard  
  sensors.requestTemperatures();
  temp = (sensors.getTempCByIndex(0));     // get inital temperture reading
  mintemp = temp; maxtemp = temp;          // reset min and max
  Serial.println(temp);
  
  // LED display
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
}
//--------------------------------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------------------------------
void loop()
{
  
  // Show counter on LED display
  if (statemachine < 75) {
    statemachine++;
  }
  else {
    // Update led display
    statemachine=0;
    outtemp = emontx.temp;
    
    
    //Serial.println("overflow");
    //counter++;
    //Serial.println(counter);
    
    int ones = (outtemp%10);
    int tens = ((outtemp/10)%10);
    int hundreds = ((outtemp/100)%10);
    int thousands = (outtemp/1000);
    
    values[3]=ones;
    values[2]=tens;
    values[1]=hundreds;
    values[0]=thousands;
  }
  setDigit(values);

  // Signal from the emonTx received  
  if (rf12_recvDone())
  {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
    {
      Serial.println("Got message");
      int node_id = (rf12_hdr & 0x1F);
      // Node 18 is my outdoor Emontx
      if (node_id == 18) {
        emontx = *(PayloadTX*) rf12_data; 
        Serial.print("Data:");
        Serial.println(emontx.temp);
        
        last_emontx = millis();
      }  
      
      if (node_id == 15)			//Assuming 15 is the emonBase node ID
      {
        //RTC.adjust(DateTime(2012, 1, 1, rf12_data[1], rf12_data[2], rf12_data[3]));
        last_emonbase = millis();
      } 
      Serial.print("node_id: ");
      Serial.println(node_id);
    }
  }
}

void setDigit(int values[5]) {
  for (int x=0; x<4; x++) {
    chars=digit[x];
    value=values[x];
    showDigit();
    delay(3);
  }
}

void showDigit() {
   digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, LSBFIRST, numbers[value]);
   shiftOut(dataPin, clockPin, LSBFIRST, chars);
   digitalWrite(latchPin, HIGH);
}
