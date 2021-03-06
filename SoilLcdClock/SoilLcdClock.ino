/*
*took some code from here
*https://github.com/Makuna/Rtc/tree/master/examples/DS1307_Simple
*http://pdacontrolenglish.blogspot.com.co  
*http://pdacontrol.blogspot.com.co
*and some from here
*
* SCHEMATIC http://i.imgur.com/plwZbFa.png
* Connecting ESP8266 (ESP-12) and Nokia 5110 LCD
* www.KendrickTabi.com
* http://www.kendricktabi.com/2015/08/esp8266-and-nokia-5110-lcd.html
* 
*connected it all together and here it is a simple clock
*/


#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <FS.h>

#define SLEEP_DELAY_IN_SECONDS 60

//SDA - D3 -- GPIO0
//SCL - D4 -- GPIO2

#define PIN_SCE   D8//3 //Pin 3 on LCD
#define PIN_RESET D9//4 //Pin 4 on LCD
#define PIN_DC    D7//2 //Pin 5 on LCD
#define PIN_SDIN  D6//1 //Pin 6 on LCD
#define PIN_SCLK  D5//0 //Pin 7 on LCD

//The DC pin tells the LCD if we are sending a command or data
#define LCD_COMMAND 0
#define LCD_DATA  1

//You may find a different size screen, but this one is 84 by 48 pixels
#define LCD_X     84
#define LCD_Y     48

//This table contains the hex values that represent pixels
//for a font that is 5 pixels wide and 8 pixels high
static const byte ASCII[][5] = {
  {0x00, 0x00, 0x00, 0x00, 0x00} // 20
  , {0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
  , {0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
  , {0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
  , {0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
  , {0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
  , {0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
  , {0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
  , {0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
  , {0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
  , {0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
  , {0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
  , {0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
  , {0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
  , {0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
  , {0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
  , {0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
  , {0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
  , {0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
  , {0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
  , {0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
  , {0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
  , {0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
  , {0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
  , {0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
  , {0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
  , {0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
  , {0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
  , {0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
  , {0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
  , {0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
  , {0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
  , {0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
  , {0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
  , {0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
  , {0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
  , {0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
  , {0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
  , {0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
  , {0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
  , {0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
  , {0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
  , {0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
  , {0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
  , {0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
  , {0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
  , {0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
  , {0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
  , {0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
  , {0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
  , {0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
  , {0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
  , {0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
  , {0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
  , {0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
  , {0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
  , {0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
  , {0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
  , {0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
  , {0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
  , {0x02, 0x04, 0x08, 0x10, 0x20} // 5c
  , {0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
  , {0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
  , {0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
  , {0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
  , {0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
  , {0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
  , {0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
  , {0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
  , {0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
  , {0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
  , {0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
  , {0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
  , {0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
  , {0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j
  , {0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
  , {0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
  , {0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
  , {0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
  , {0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
  , {0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
  , {0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
  , {0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
  , {0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
  , {0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
  , {0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
  , {0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
  , {0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
  , {0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
  , {0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
  , {0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
  , {0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
  , {0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
  , {0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
  , {0x10, 0x08, 0x08, 0x10, 0x08} // 7e ~
  , {0x78, 0x46, 0x41, 0x46, 0x78} // 7f DEL
};

RTC_DS1307 RTC;
void setup() {
  pinMode(D2,OUTPUT);
  //tiny RTC
  Wire.begin(0, 2); // Inicia el puerto I2C
  RTC.begin(); // Inicia la comunicación con el RTC

  /*
  // Check to see if the RTC is keeping time.  If it is, load the time from your computer.
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // This will reflect the time that your sketch was compiled
    //setup RTC time
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  */
  Serial.begin(115200); // Establece la velocidad de datos del puerto serie

  //LCD SCREEN
  LCDInit(); //Init the LCD
  digitalWrite(D2,HIGH);
  gotoXY(0,3);
  LCDString("Loading ...");
  delay(1000);
}

//GLOBAL VARIABLES TIME/DATE/HUMIDITY
//HUMIDITY
    float humRead = 0; //sensor data
    float humRead2 = 0; //data mapped to sensor ranges
    double humValue = 0.0; //percentage value
    String humStr = "Humidity";
//TIME
    int nHour = 1;
    int nMin = 1;
    int nSec = 1;
//DATE
    int nDay = 1;
    int nMonth = 1;
    int nYear = 1;
    DateTime now = RTC.now();
    
void loop(){
  //TURN ON/OFF SOIL SENSOR
  powerSoilSensor();
  //print time and humidity
  printHora();
  //read soil sensor
  readHumidity();
  //desactivar descanso mientras se prueban otras cosas
  //sleep();
}

//CYCLE - 4 seconds
//turn soil sencor during 2 seconds
//of each cycle
void powerSoilSensor(){
  if(nSec%4==0 || nSec%4==1){
      digitalWrite(D2,HIGH);
    }else{
      digitalWrite(D2,LOW);
    }
  }
int retry = 0;
//IF AN ERROR OCCURED LOGGIN DATA INTO THE ESP FILE SYSTEM
//THIS WILL TRY TO RELOG THE DATA.
void retryLog(){
  if(retry < 5){
        SPIFFS.begin();
        retry++;
        logData();
      }else{
        //no more tryes 
      }
 }

//LOG DATA INTO ESP8266 MEMORY
void logData(){
  //mount file system
  if( SPIFFS.begin() ){
  String folder = "/" + String(nYear) + int2String(nMonth);
  String file = "/" + String(nYear) + int2String(nMonth) + int2String(nDay) + ".csv";
  String percent = String(humValue);
  percent.replace("." , ",");
  String logN = String(nYear) + "-" + int2String(nMonth) + "-" + int2String(nDay) + ";" //Date
                  + int2String(nHour) + ":" + int2String(nMin) + ":" + int2String(nSec) + ";" //Time
                  + String(humRead) + ";"//Sensor value
                  + percent ; //Percentage value
                  
  //we want to append data to existing file or create a new one.
  File f = SPIFFS.open(folder + file, "a");
  if(f){
    //appending new log          
    f.println(logN);
    retry = 0;    
    }else{
      //file creation failed
      retryLog();
    }
    f.close();
  }else{
    //file system is not mounted
    retryLog();
  }
  
}

//READ SOIL SENSOR ONCE PER EACH 4 SECONDS
//READ EACH [NOW.SECOND%4] == 1
void readHumidity(){
  if(nSec%4==1){
    humRead = analogRead(A0);
    humRead2 = map(humRead,1000,100,0,1024);
    humValue = map(humRead2,0,1024,0,100);
    if(humValue < 0){
      humValue = 0.0;
    }else if(humValue > 100.0){
      humValue = 100.0;
    }
    //log data after reading it
    logData();
  }
 }
 
 //SEND ESP8266 to sleep 
 void sleep(){
  if((nMin % 3) == 0){
    //turn humidity sensor OFF
    digitalWrite(D2,LOW);
    LCDClear();
    gotoXY(0,2);
    LCDString("SLEEPING");
    gotoXY(0,3);
    LCDString("Z-Z-Z-Z-Z-Z");
    ESP.deepSleep((SLEEP_DELAY_IN_SECONDS * 1000000), WAKE_RF_DEFAULT);
    delay(400);
    
    LCDClear();
    gotoXY(0,2);
    LCDString("GOOD MORNING!");
    delay(1500);
    //turn humidity sensor ON
    digitalWrite(D2,HIGH);
  }
}

//used for seconds count
static const String secs[10] = {"*","**","***","****","*****","******","*******","********","*********","**********"};
void printHora(){
  LCDClear();
  
  //GET TIME/DATE FROM REAL TIME CLOCK
  now = RTC.now(); 
  
  //Put TIME/DATE into String  
  nHour = now.hour();
  nMin = now.minute();
  nSec = now.second();
  nDay = now.day();
  nMonth = now.month();
  nYear = now.year();
  String tempFecha = int2String(now.day()) + "/" + int2String(now.month()) + "/" + String(now.year());
  String tempHora = int2String(nHour) + ":" + int2String(nMin);

  //--------TRANSFORM STRING INTO CHAR* FOR LCD PRINTING ------------------------------------------
  char tempF [tempFecha.length() + 1];
  char tempH [tempHora.length() + 1];
  tempHora.toCharArray(tempH, tempHora.length() + 1);
  tempFecha.toCharArray(tempF, tempFecha.length() + 1);
  //-----------------------------------------------------------------------------------------------
  
  //PRINT TIME TO LCD 
  gotoXY(25, 0);
  LCDString(tempH);

  //PRINT SECONDS TO LCD
  gotoXY(0,1);
  String sec = String(nSec / 10) + " ";
  String str = ""; 
  str = sec;
  str += secs[(nSec % 10)];
  char ch[str.length()+1];
  str.toCharArray(ch, str.length() + 1);
  LCDString(ch);
  
  //PRINT REAL VALUE
  gotoXY(0,2);
  String rVal = "real " + String(humRead);
  char rv[rVal.length() + 1];
  rVal.toCharArray(rv,rVal.length() + 1);
  LCDString(rv);
  
  //PRINT HUMIDITY
  humStr = "Hum " + String(humValue) + "%";
  char humPrint[humStr.length() + 1];
  humStr.toCharArray(humPrint,humStr.length() + 1);
  gotoXY(0,3);
  LCDString(humPrint);
  
  //PRINT DATE TO LCD
  gotoXY(5, 5);
  LCDString(tempF);

  delay(1000);  
}

/**
 * Transform an int to string with zero padding
*/
String int2String(int num){
  String str = ""; 
  if(num < 10){
    str+="0";
  }
  str+=String(num);
  return str;  
}

//There are two memory banks in the LCD, data/RAM and commands. This
//function sets the DC pin high or low depending, and then sends
//the data byte
void LCDWrite(byte data_or_command, byte data) {
  digitalWrite(PIN_DC, data_or_command); //Tell the LCD that we are writing either to data or a command

  //Send the data
  digitalWrite(PIN_SCE, LOW);
  shiftOut(PIN_SDIN, PIN_SCLK, MSBFIRST, data);
  digitalWrite(PIN_SCE, HIGH);
}


void gotoXY(int x, int y) {
  LCDWrite(0, 0x80 | x);  // Column  x - range: 0 to 84
  LCDWrite(0, 0x40 | y);  // Row     y - range: 0 to 5
}

//This takes a large array of bits and sends them to the LCD
void LCDBitmap(char my_array[]) {
  for (int index = 0 ; index < (LCD_X * LCD_Y / 8) ; index++)
    LCDWrite(LCD_DATA, my_array[index]);
}

//This function takes in a character, looks it up in the font table/array
//And writes it to the screen
//Each character is 8 bits tall and 5 bits wide. We pad one blank column of
//pixels on each side of the character for readability.
void LCDCharacter(char character) {
  LCDWrite(LCD_DATA, 0x00); //Blank vertical line padding

  for (int index = 0 ; index < 5 ; index++)
    LCDWrite(LCD_DATA, ASCII[character - 0x20][index]);
  //0x20 is the ASCII character for Space (' '). The font table starts with this character

  LCDWrite(LCD_DATA, 0x00); //Blank vertical line padding
}


//Given a string of characters, one by one is passed to the LCD
void LCDString(char *characters) {
  while (*characters)
    LCDCharacter(*characters++);
}



//Clears the LCD by writing zeros to the entire screen
void LCDClear(void) {
  for (int index = 0 ; index < (LCD_X * LCD_Y / 8) ; index++)
    LCDWrite(LCD_DATA, 0x00);

  gotoXY(0, 0); //After we clear the display, return to the home position
}


//This sends the magical commands to the PCD8544
void LCDInit(void) {

  //Configure control pins
  pinMode(PIN_SCE, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_SDIN, OUTPUT);
  pinMode(PIN_SCLK, OUTPUT);

  //Reset the LCD to a known state
  digitalWrite(PIN_RESET, LOW);
  digitalWrite(PIN_RESET, HIGH);

  LCDWrite(LCD_COMMAND, 0x21); //Tell LCD that extended commands follow
  LCDWrite(LCD_COMMAND, 0xB0); //Set LCD Vop (Contrast): Try 0xB1(good @ 3.3V) or 0xBF if your display is too dark
  LCDWrite(LCD_COMMAND, 0x04); //Set Temp coefficent
  LCDWrite(LCD_COMMAND, 0x14); //LCD bias mode 1:48: Try 0x13 or 0x14

  LCDWrite(LCD_COMMAND, 0x20); //We must send 0x20 before modifying the display control mode
  LCDWrite(LCD_COMMAND, 0x0C); //Set display control, normal mode. 0x0D for inverse
}


