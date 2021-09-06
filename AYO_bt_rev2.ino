#include <Wire.h>
#include <CheapStepper.h>
// #include <LiquidCrystal_I2C.h>
#include "BluetoothSerial.h"
/**
 * GPIO 15 - BUTTON
 * GPIO 2 - BUTTON
 * GPIO 4 - BUTTON
 * GPIO 16 - BUTTON
 * GPIO 36 - BUTTON
 * GPIO 39 - BUTTON (Limit Switch)
 * 
 * GPIO 17 - LED
 * GPIO 5 - LED
 * GPIO 18 - LED
 * GPIO 19 - LED
 * 
 * GPIO 36 - RELAY
 * GPIO 39 - RELAY
 * GPIO 34 - RELAY
 * 
 * GPIO 35 - STEPPER
 * GPIO 32 - STEPPER
 * GPIO 33 - STEPPER
 * GPIO 25 - STEPPER
 * 
 * GPIO 21 - SDA
 * GPIO 22 - SCL
 * 
 * GPIO 23 - SENSOR
 * 
 **/
 

#define INSTILL_MOTOR 22 //32    TIP 12O
#define PUMP_MOTOR 23 //33 TIP 120
#define BACK_UP_MOTOR 27 //25 TIP 120
#define KIT 13 //Voltage Sensor

#define LIMIT_SWITCH 15 //39
#define SENSOR 36 //Pressure

#define BUTTON1 21 //36

#define LED1 19
#define BUZZER 18

#define STEPPER1 33 //26
#define STEPPER2 32 //27                                                
#define STEPPER3 25 //14
#define STEPPER4 26 //13

 #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
 #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
 #endif

BluetoothSerial SerialBT;

CheapStepper stepper (STEPPER1,STEPPER2,STEPPER3,STEPPER4);
// LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

int pressure = 0;
int sensorValue = 0;
int mod = 6;
int prevMode;
int deg = 5;
int limitState = 0;
int acState = 0;

const int freq = 1000;
const int BUZZChannel = 0;
const int resolution = 8;

long longInterval = 300000;
long longDuration = 120000;
long millisIntermittentOn, millisIntermittentOff;
long debounceDelay = 500;
long readingStarted;
long millisLastPressed;
long millisLastChanged;

boolean reading = false;

String modd, interval, len, duration, message;

boolean intPump = false;
boolean changed = true;
boolean pressureSet = false;
boolean intermittentOn = false;
boolean setPres = true;
boolean leakageDetected = false;
boolean onBackUp = false;
boolean pumpOn = false;
boolean instillOn = false;

void button1() {
    if (mod != 1){
      changed = true;
      mod = 1;
      setScreen();
  }
}

void button2() {
    if (mod != 2){
      changed = true;
      mod = 2;
      setScreen();
    }
}

void button3() {
    if (mod != 3){
      changed = true;
      mod = 3;
      setScreen();
  }
}

void button4() {
    if (mod != 4){
      changed = true;
      mod = 4;
      setScreen();
  }
}


void button5() {
  if(mod != 5){
    prevMode = mod;  
    changed = true;
    mod = 5;
    setScreen();
  }
}
  
void buttonF(){  
    mod = prevMode;
    changed = true;
    digitalWrite(INSTILL_MOTOR, LOW);
    setScreen();
    }


void button6(){
mod = 6;
setScreen();
}

void Buzz(){
    
    ledcWrite(BUZZChannel, 255);
    pinMode(LED1, INPUT);
    delay(500);
    ledcWrite(BUZZChannel, 0);
    pinMode(LED1, OUTPUT);
    digitalWrite(LED1, LOW);
  
}

void hardButton(){
  if ((millis()-millisLastChanged) > 2000){
    mod = mod+1;
    if(mod > 6){
      mod = 1;
    }
    millisLastChanged = millis();
    blinkLED();
    setScreen();
  }
}

void setpressure(){
  String command = "tSate.txt=\""+String(pressure)+"\"";
  Serial2.print(command);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}

 void setup() {
    Serial.begin(115200);
    Serial2.begin(9600);
    SerialBT.begin("ayo_device");
    Wire.begin();
    pinMode(PUMP_MOTOR, OUTPUT);
    // pinMode(BACK_UP_MOTOR, OUTPUT);
    pinMode(INSTILL_MOTOR, OUTPUT);

    pinMode(STEPPER1, OUTPUT);
    pinMode(STEPPER2, OUTPUT);
    pinMode(STEPPER3, OUTPUT);
    pinMode(STEPPER4, OUTPUT);  
    //buzzer  
    ledcSetup(BUZZChannel, freq, resolution);
    ledcAttachPin(BUZZER, BUZZChannel);
    
    pinMode(LIMIT_SWITCH, INPUT);
    pinMode(KIT, INPUT);

    pinMode(BUTTON1, INPUT_PULLUP);

    digitalWrite(PUMP_MOTOR, LOW);
    digitalWrite(INSTILL_MOTOR, LOW);
    // digitalWrite(BACK_UP_MOTOR, LOW);

    stepper.setRpm(10);    

}

void loop() {
  checkButtons();
  processMode();
}  


void readPressure(){
 int x = 0;
 for (int i = 0; i < 100; i++){
    sensorValue = analogRead(SENSOR);
    x = x + sensorValue; 
    delay(2);
 }
 x = x/100;
 pressure = map (x, 3100, 0, 0, 300);
 Serial.println(pressure); 
 sendpressureToNextion();
 String toSend = String(pressure) + ".";
 sendData(toSend);
}  

void processMode(){
  if (mod == 6){    
    pinMode(LED1, INPUT);
  }else{
    pinMode(LED1, OUTPUT);
    digitalWrite(LED1, LOW);
  }
  if (mod == 2){
    digitalWrite(INSTILL_MOTOR, LOW);
//    if(!onBackUp){
      digitalWrite(PUMP_MOTOR, HIGH);
//    }else{
//      digitalWrite(PUMP_MOTOR, LOW);
//      digitalWrite(BACK_UP_MOTOR, HIGH); 
//    }
    delay(1000);
    intermittentOn = false;
    setPressure (122, 118);
  }else if (mod == 1){
    digitalWrite(INSTILL_MOTOR, LOW);
//    if(!onBackUp){
      digitalWrite(PUMP_MOTOR, HIGH);
//    }else{
//      digitalWrite(PUMP_MOTOR, LOW);
//      digitalWrite(BACK_UP_MOTOR, HIGH); 
//    }
    delay(1000);
    intermittentOn = false;
    setPressure (72, 68);
  }else if(mod==4){
    digitalWrite(INSTILL_MOTOR, LOW);
    if(!intermittentOn){
      millisIntermittentOn = millis();
      intermittentOn = true;
//    if(!onBackUp){
      digitalWrite(PUMP_MOTOR, HIGH);
//    }else{
//      digitalWrite(PUMP_MOTOR, LOW);
//      digitalWrite(BACK_UP_MOTOR, HIGH); 
//    }
      delay(1000);
      setPressure(122, 118);
      setPres = true;
      intPump = true;
    }else{
      if(((millis() - millisIntermittentOn) > longInterval) && intermittentOn == true && intPump == true){
//    if(!onBackUp){
      digitalWrite(PUMP_MOTOR, LOW);
//    }else{
//      digitalWrite(BACK_UP_MOTOR, LOW); 
//    }
        millisIntermittentOff = millis();
        setPres = false;
        intPump = false;
      }else if(((millis() - millisIntermittentOff) > longDuration) && intPump == false){
//      if(!onBackUp){
          digitalWrite(PUMP_MOTOR, HIGH);
//        }else{
//          digitalWrite(PUMP_MOTOR, LOW);
//          digitalWrite(BACK_UP_MOTOR, HIGH); 
//        }
        delay(1000);
        millisIntermittentOn = millis();
        setPres = true;
        intPump = true;
      }
    }
    if(setPres == true){
        setPressure(122, 118);
      }else{
        readPressure();
      }  
  }else if(mod==3){    if(!intermittentOn){
    digitalWrite(INSTILL_MOTOR, LOW);
      millisIntermittentOn = millis();
      intermittentOn = true;
//      if(!onBackUp){
        digitalWrite(PUMP_MOTOR, HIGH);
//      }else{
//        digitalWrite(PUMP_MOTOR, LOW);
//        digitalWrite(BACK_UP_MOTOR, HIGH); 
//      }
      setPressure (72, 68);
      setPres = true;
      intPump = true;
    }else{
      if(((millis() - millisIntermittentOn) > longInterval) && intermittentOn == true && intPump == true){
//      if(!onBackUp){
        digitalWrite(PUMP_MOTOR, LOW);
//      }else{
//        digitalWrite(BACK_UP_MOTOR, LOW); 
//      }
        millisIntermittentOff = millis();
        setPres = false;
        intPump = false;
      }else if(((millis() - millisIntermittentOff) > longDuration) && intPump == false){
//      if(!onBackUp){
        digitalWrite(PUMP_MOTOR, HIGH);
//      }else{
//        digitalWrite(PUMP_MOTOR, LOW);
//        digitalWrite(BACK_UP_MOTOR, HIGH); 
//      }
        delay(1000);
        millisIntermittentOn = millis();
        setPres = true;
        intPump = true;
      }
    }
    if(setPres == true){
        setPressure (72, 68);
      }else{
        readPressure();
      }
  }else if(mod == 5){
      digitalWrite(PUMP_MOTOR, LOW);
      digitalWrite(BACK_UP_MOTOR, LOW);
      digitalWrite(INSTILL_MOTOR, HIGH);
  }else if (mod == 6){
      digitalWrite(PUMP_MOTOR, LOW); 
      digitalWrite(BACK_UP_MOTOR, LOW);
      digitalWrite(INSTILL_MOTOR, LOW);
  }
}

void setPressure (int higherLimit, int lowerLimit){
  pressureSet = false;
  readPressure();
    if(pressure > higherLimit ){
      leakageDetected = false;
      deg = ((pressure - higherLimit))*3;
      if(deg < 30){
        deg = 30;
        }
     stepper.moveDegreesCW(deg);
      }else if (pressure < lowerLimit){
      if(!leakageDetected){
      deg = ((lowerLimit - pressure))*3;
      if(deg < 30){
        deg = 30;
        }
     stepper.moveDegreesCCW(deg);
      }else{
          Serial2.print("t5.txt=\"Leakage\"");
          Serial2.write(0xff);
          Serial2.write(0xff);
          Serial2.write(0xff);
          Serial2.print("t0.txt=\"Detected\"");
          Serial2.write(0xff);
          Serial2.write(0xff);
          Serial2.write(0xff);
          
          Buzz();
          Serial.println("Leakage!");
      }
    }
  checkButtons();    
}


void setScreen(){
  switch (mod){
  case 1:
  Serial2.print("t5.txt=\"Continuous\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("t0.txt=\"LOW\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCL.pic=5");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCH.pic=3");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIL.pic=9");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIH.pic=7");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("b0.pic=1");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.pic=2");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.txt=\"ON\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  break;

  case 2:  
  Serial2.print("t5.txt=\"Continuous\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("t0.txt=\"HIGH\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCL.pic=4");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCH.pic=6");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIL.pic=9");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIH.pic=7");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("b0.pic=1");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.pic=2");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.txt=\"ON\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  break;

  case 3:
  Serial2.print("t5.txt=\"Intermittent\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("t0.txt=\"LOW\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCL.pic=4");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCH.pic=3");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIL.pic=10");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIH.pic=7");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("b0.pic=1");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.pic=2");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.txt=\"ON\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  break;
    
  case 4:  
  Serial2.print("t5.txt=\"Intermittent\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("t0.txt=\"HIGH\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCL.pic=4");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCH.pic=3");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIL.pic=9");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIH.pic=8");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("b0.pic=1");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.pic=2");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.txt=\"ON\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  break;

  case 5:  
  Serial2.print("t5.txt=\"Instill\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("t0.txt=\"\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCL.pic=4");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCH.pic=3");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIL.pic=9");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIH.pic=7");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("b0.pic=2");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.pic=1");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.txt=\"OFF\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("b0.txt=\"ON\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  break;

  case 6:  
  Serial2.print("t5.txt=\"PUMP\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("t0.txt=\"OFF\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCL.pic=4");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bCH.pic=3");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIL.pic=9");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bIH.pic=7");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("b0.pic=1");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.pic=1");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.print("bP2.txt=\"OFF\"");
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
  break;

  default:
  break;
  } 
}

void sendData(String textToSend){
  for (int i = 0; i < textToSend.length(); i++){
    SerialBT.write(textToSend[i]);
  }
}

String requestFromNode(){
  String pressureFromSensor;
  Wire.requestFrom(1, 3);
  delay(100);
  while (Wire.available()){
    char c = Wire.read();
    pressureFromSensor.concat(c);
    
   Serial.print(String(1));
   Serial.print(": ");
   Serial.print(c);
    }
    return pressureFromSensor;
  } 

void checkButtons(){
  if (Serial2.available() > 0){
    char comdata = char(Serial2.read());
    if (comdata == 'A'){
      button1();  
      Serial.println("A");
      }else if (comdata == 'B'){
      button2();  
      Serial.println("B");
      }else if (comdata == 'C'){
      button3(); 
      Serial.println("C");
      }else if (comdata == 'D'){
      button4();
      Serial.println("D");
      }else if (comdata == 'E'){
      button5();
      Serial.println("E");
      }else if(comdata == 'F'){
      buttonF();
      Serial.println("F");
      }else if (comdata == 'G'){
      button6();
      Serial.println("G");
      }
  }

    checkMessage();

    limitState = digitalRead(LIMIT_SWITCH);
    if (limitState == LOW){
      leakageDetected = true;
    }else{
      leakageDetected = false;
    }
    // acState = digitalRead(KIT);
    // if (acState == LOW){
    //   onBackUp = true;
    // }else{
    //   onBackUp = false;
    //   (digitalWrite(BACK_UP_MOTOR, LOW));
    // }

    if (digitalRead(BUTTON1) == LOW){
    hardButton();
    }  
  }

void checkMessage(){
  if (SerialBT.available()) {
    message = SerialBT.readString();
    Serial.println(message);
    modd = message.substring(0,1);
    if(modd == "A"){
      button1();  
      Serial.println("A");
    }else if(modd == "B"){
      button2();
      Serial.println("B");
    }else if(modd == "C"){
      button3();
      Serial.println("C");
    }else if(modd == "D"){
      button4();
      Serial.println("D");
    }else if(modd == "E"){
      button5();
      Serial.println("E");
    }else if(modd == "F"){
      buttonF();
      Serial.println("F");
    }else if(modd == "G"){
      button6();
      Serial.println("G");
    }else if(modd == "."){
      String toSend = String(mod) + ".";
      sendData(toSend);
    }
    interval = getValue(message, ',', 1);
    duration = getValue(message, ',', 2);
    longInterval = interval.toInt() * 60000;
    longDuration = duration.toInt() * 60000;
    message = "";

    Serial.print("Mode: ");
    Serial.println(mod);
    Serial.print("Interval: ");
    Serial.println(longInterval);
    Serial.print("Duration: ");
    Serial.println(longDuration);
  }
    Serial.print("Mode: ");
    Serial.println(mod);
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void blinkLED(){
    pinMode(LED1, INPUT);
    delay(500);
    pinMode(LED1, OUTPUT);
    digitalWrite(LED1, LOW);
    ledcWrite(BUZZChannel, 0);
    
  }

void sendpressureToNextion(){
  String command = "tSate.txt=\""+String(pressure)+"\"";
  Serial2.print(command);
  Serial2.write(0xff);
  Serial2.write(0xff);
  Serial2.write(0xff);
}
