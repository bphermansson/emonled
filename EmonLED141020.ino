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
int value, counter, statemachine, displaystatemachine=0, whattoshow, x;
int values[5];
int ones;
int tens; 
int hundreds;
int thousands;
  
/* 
Digits 0-9, c, -, (blank)
The 8 segments in each digit is represented by a bit in numbers array. A 1 lights the segment, a zero keeps it off. 
The bits are named A-G and 'DP' and are represented in that order. This means that 'number[x]=11000000' makes the segments A and B light up.
*/
byte numbers[13]={B11111100, B01100000, B11011010, B11110010, B01100110, B10110110, B10111110, B11100000, B11111110, B11110110, B00011010, B00000010, B00000000};
byte numbers_w_dots[11]={B11111101, B01100001, B11011011, B11110011, B01100111, B10110111, B10111111, B11100001, B11111111, B11110111, B00011011};
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
int outtemp, belowzero;

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

typedef struct { int temperature; } PayloadGLCD;
PayloadGLCD emonglcd;
//-------------------------------------------------------------------------------------------- 
// Flow control
//-------------------------------------------------------------------------------------------- 
unsigned long last_emontx;                   // Used to count time from last emontx update
unsigned long last_emonbase;                   // Used to count time from last emontx update

// Time
String shour, smin;


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
  temp = (sensors.getTempCByIndex(0));     // get inital temperature reading
  mintemp = temp; maxtemp = temp;          // reset min and max
  Serial.println(temp);
  
  // LED display
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
  
  // Welcome message on display
  int welcome = 1234;
  ones = (welcome%10);
  tens = ((welcome/10)%10);
  hundreds = ((welcome/100)%10);
  thousands = (welcome/1000);
    
  values[3]=ones;
  values[2]=tens;
  values[1]=hundreds;
  values[0]=thousands;
  setDigit(values);
}
//--------------------------------------------------------------------------------------------
// Loop
//--------------------------------------------------------------------------------------------
void loop()
{
  
  if (statemachine < 75) {
    statemachine++;
  }
  else {
    // Update led display
    statemachine=0;
    //Serial.println(displaystatemachine);
    
    // Toggle between display of time and temp
    if (displaystatemachine<5) {
      displaystatemachine++;
    }
    else {
      if (whattoshow<1) {
        whattoshow=1;
      }
      else if (whattoshow>0) {
        whattoshow=0;
      }
      //Serial.print("Whattoshow: ");
      //Serial.println (whattoshow);
      displaystatemachine=0;
    }
  
    if (whattoshow==0) {
    // Display time
      //Serial.print("Display time: ");
      String time = shour + smin;
      int itime = time.toInt();
      //Serial.println(itime);
      
      ones = (itime%10);
      tens = ((itime/10)%10);
      hundreds = ((itime/100)%10);
      thousands = (itime/1000);
    }
    else { 
      // Display outdoor temp
      //Serial.print("Outdoor temp: ");      
      //Serial.println(outtemp);   
      // outtemp = emontx.temp;
      
      // Test negative values
      //outtemp = -2755;
      
      if (outtemp < 0) {
        // Set flag for value below zero
        belowzero = 1;
        outtemp = abs(outtemp); // Remove '-' sign
      }
      else {
        belowzero = 0; 
      }
      
      ones = ((outtemp/10)%10);
      tens = ((outtemp/100)%10);
      hundreds = (outtemp/1000);
      
      if (belowzero == 1) {  // Negative value
         // Below 10 degress, adjust display 
          if (outtemp < 1000){
            thousands = 12;  // Blank
            hundreds = 11; // -
          }
          else {
            thousands = 11;  // '-' sign
          }
      }
      else if (outtemp < 1000){
          hundreds = 12; // Blank   
      }
      else {
          //Serial.println("test"); 
          thousands = 12;  // Blank
      }
      
      /*
      ones = (outtemp%10);
      tens = ((outtemp/10)%10);
      hundreds = ((outtemp/100)%10);
      thousands = (outtemp/1000);
      */
      //ones = (outtemp%10);

      
    }
    /*
    int ones = (outtemp%10);
    int tens = ((outtemp/10)%10);
    int hundreds = ((outtemp/100)%10);
    int thousands = (outtemp/1000);
    */
    values[3]=ones;
    values[2]=tens;
    values[1]=hundreds;
    values[0]=thousands;
  }
  
  // Show digits
  setDigit(values);

  // Signal from the emonTx received  
  if (rf12_recvDone())
  {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no rf errors
    {
      int node_id = (rf12_hdr & 0x1F);
      // Node 18 is my outdoor Emontx
      if (node_id == 18) {
        Serial.println("Got message from emonTx-------------");
        emontx = *(PayloadTX*) rf12_data; 
        // Store value
        outtemp = emontx.temp;
        Serial.print("Data:");
        Serial.println(emontx.temp);        
        last_emontx = millis();
      }  
      
      // Node 16 is the emonBase. It sends out some date, for example the current time. 
      if (node_id == 16)			//Assuming 15 is the emonBase node ID
      {
        Serial.println("Got message from emonBase");
        //RTC.adjust(DateTime(2012, 1, 1, rf12_data[1], rf12_data[2], rf12_data[3]));
        emontx = *(PayloadTX*) rf12_data; 
        
        // Get time from server
        // Hour
        int hour = rf12_data[1];
        //shour = (char)hour;
        //Serial.print ("hour: ");
        //Serial.println (hour);

        if (hour<10) {
          //Serial.println ("Less than 10");
          shour = "0" + String(hour);
        }  
        else {
          shour = String(hour);
        }
        //Serial.println (shour);
        // Minute
        int min = rf12_data[2];

        //Serial.print ("M: ");        
        //Serial.println (min);
        if (min<10) {
          smin = "0"+String(min);
          //Serial.print ("SM: ");        
          //Serial.println (smin);          
        }
        else {
          smin = String(min);
          //Serial.print ("SM: ");        
          //Serial.println (smin);        
        }
        String time = shour + smin;
        int itime = time.toInt();
        Serial.println(itime);
        last_emonbase = millis();
      } 
      //Serial.print("node_id: ");
      //Serial.println(node_id);
      
      // Send local temperature to emonBase
      sensors.requestTemperatures();
      temp = (sensors.getTempCByIndex(0));
      emonglcd.temperature = (int) (temp * 100);                          // set emonglcd payload
      rf12_sendNow(0, &emonglcd, sizeof emonglcd);                     //send temperature data via RFM12B using new rf12_sendNow wrapper -glynhudson
      rf12_sendWait(2); 
    }
  }
}

void setDigit(int values[5]) {
  for (x=0; x<4; x++) {
    chars=digit[x];
    value=values[x];
    showDigit();
    delay(3);
  }
}

void showDigit() {
   digitalWrite(latchPin, LOW);
   // The second chars shall have the dot lit if time is showing
   if (x==1 && whattoshow==0) {  // Show time
      shiftOut(dataPin, clockPin, LSBFIRST, numbers_w_dots[value]);   
   }
   // If temp is on display, the third digit shall have a dot
   else if (x==2 && whattoshow==1) {  // Show out temp
      shiftOut(dataPin, clockPin, LSBFIRST, numbers_w_dots[value]);   
   }
   else {
      shiftOut(dataPin, clockPin, LSBFIRST, numbers[value]);  
      //Serial.println(numbers[value]);   

   }

   
   // Last char is 'c'
   //if (x==3) {
   //   shiftOut(dataPin, clockPin, LSBFIRST, numbers_w_dots[11]);   
   //}
   shiftOut(dataPin, clockPin, LSBFIRST, chars);
   digitalWrite(latchPin, HIGH);
}
