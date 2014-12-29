#define HC_DATA_H digitalWrite(data, HIGH) // data line output high
#define HC_DATA_L digitalWrite(data, LOW) //date line output low
#define HC_RCK_H digitalWrite(rck, HIGH) // rck output high
#define HC_RCK_L digitalWrite(rck, LOW) // rck output low
#define HC_SCK_H digitalWrite(sck, HIGH) // sck output high
#define HC_SCK_L digitalWrite(sck, LOW) // sck output low



unsigned char LED_BCD[16] ={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0x88,0x83,0xc6,0xa1,0x86,0x8e }; //common anode digital tube BCD code
//Define the pin
int data =2; 
int rck =3;
int sck =5;
// the setup routine runs once when you press reset:
void setup() { 
// initialize the digital pin as an output.
pinMode(data, OUTPUT); //initial data 
pinMode(rck, OUTPUT); //initial rck
pinMode(sck, OUTPUT); //initial sck
//pinMode(sclr, OUTPUT); //initial sclr


}

// the loop routine runs over and over again forever:
void loop() {

unsigned char dopp =0;
for( unsigned char i=0; i < 4; ++i)
{
if(i ==3) dopp =1; 
else dopp =0;
LED_display(i,i,dopp); //Nixie Tube display
}



}

void LED_display(char LED_number,unsigned char LED_display,unsigned char LED_dp)
{
// data analyse
unsigned int hc_disp = 0,hc_ledcode,hc_ledcode_temp=0;

if(LED_display > 15) LED_display = 0;
hc_ledcode = LED_BCD[LED_display] ; //get BCD code
for(unsigned char i=0; i < 8;++i)
{
hc_ledcode_temp <<=1;
if(hc_ledcode&0x01) hc_ledcode_temp |= 0x01;
hc_ledcode >>=1;

}
if(LED_dp) hc_ledcode_temp &= 0xfe; 
hc_disp = hc_ledcode_temp;

switch(LED_number)
{
case 0: hc_disp |= 0x8000;break;
case 1: hc_disp |= 0x4000;break;
case 2: hc_disp |= 0x2000;break;
case 3: hc_disp |= 0x1000;break;
}

write_74HC595_ShiftOUTPUT(hc_disp); //74HC595 shifting register data transfer


}

//shift output to 74HC595
void write_74HC595_ShiftOUTPUT( unsigned int data_a) //communication with 74HC595
{
char look =0;
HC_RCK_L; //latch open
HC_SCK_L;



for (;look < 16; ++look)
{
if(data_a&0x0001) {HC_DATA_H;}
else {HC_DATA_L;}
HC_SCK_H;

HC_SCK_L;
data_a >>= 1;
}
HC_RCK_H;
}
