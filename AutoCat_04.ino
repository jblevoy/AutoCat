//AutoCat_04: final shipping version 4.0

//See Rinky-Dink Electronics by Henning Karlsen for details about DS3231 time module. 
// web: http://www.RinkyDinkElectronics.com/

#include <DS3231.h> //RTC module library

// Init the DS3231 using the hardware interface
// DS3231:  SDA pin   -> Analog 4
//          SCL pin   -> Analog 5
DS3231  rtc(SDA, SCL);

#include <TM1637TinyDisplay.h> //TM1637 library. Get displays ready, associate with pins
#define CLK_1 6 //Digital 6 display 1 'time open'
#define DIO_1 7 //Digital 7 
TM1637TinyDisplay display1(CLK_1, DIO_1);

#define CLK_2 4 //Digital 4 display 2 'time close'
#define DIO_2 5 //Digital 5 
TM1637TinyDisplay display2(CLK_2, DIO_2);

#define CLK_3 2 //Digital 2 display 3 'RTC'
#define DIO_3 3 //Digital 3 
TM1637TinyDisplay display3(CLK_3, DIO_3);

Time t; //create object t of class Time, member of DS3231.h

uint8_t blank[] = { 0x0, 0x0, 0x0, 0x0 }; //will use this later to clear the "2" that remains after 2359 rollover

#include <Servo.h> // include servo library to use its related functions
#define servo_pwm 9 // A descriptive name for digital 9 pin of Arduino to provide PWM signal --> angle (0=0%, 180=100% duty cycle)
Servo lock_servo;  // Define an instance of Servo object with the name of "lock_servo"

long int blink_timer = 0; //time blink to half second for impossible open/close times
bool blink = false; //bool for improper timing if's

int open_angle = 5; //open
int lock_angle = 105; //lock 
int servo_angle = 5; //unlock on program start

long int close_timer = 0;
bool closed = false;

void setup() {
  Serial.begin(115200);
  display1.begin();
  display1.setBrightness(0); //set 0:min - 7:max
  display2.begin();
  display2.setBrightness(0);
  display3.begin();
  display3.setBrightness(0);

  rtc.begin(); //Initialize the rtc object
  rtc.setTime(12, 0, 0);     //set time. Reset at noon to calibrate
  pinMode(12, INPUT_PULLUP);    //hall sensor pin
  servo_angle = open_angle;  //open_angle seems to be 5
  lock_servo.attach(servo_pwm); //attach servo
  lock_servo.write(servo_angle);
  delay(2000);
  lock_servo.detach(); //un-attach servo. stops power going to servo  
  close_timer = millis(); //start timer for three seconds, make sure door is closed before lock moves 1st time (except initial unlock above)
}

void loop(){
  //#####################################################################
  int pot1_val = analogRead(A0); //opening time pot (0 to 1023)
  int tod1_100 = pot1_val*2.346;
  int mod1_min = tod1_100 % 100; //remainder of tod_100 (0 to 2399) after mod 100
  int hrs1_trunc = tod1_100 / 100; //eg 1034 -> 10
  int min1 = mod1_min * 0.596; //eg 99 -> 59, 50 -> 30
  int tod1_min = hrs1_trunc * 100; 
  int tod1 = (hrs1_trunc * 100) + min1; //exact time of day to minute from potentiometer
  int rem1_30 = min1 % 30;
  int open = (hrs1_trunc * 100) + (min1 - rem1_30); //put it all together..

  if (open < 100 && blink == false){
    int open1 = open*10;
    display1.showNumberDec(open1, 0b10000000, 1, 4, 1); //ShowNumberDec(number to display, colon 0b10000000=on (for this config only), leading zeroes T/F. length, start position)
  }

  if (open >= 100 && blink == false){
    display1.showNumberDec(open, 0b01000000, 0, 4, 0);
  }

  //#####################################################################
  int pot2_val = analogRead(A1); //closing time pot
  int tod2_100 = pot2_val*2.346;
  int mod2_min = tod2_100 % 100; //remainder of tod_100 (0 to 2399) after mod 100
  int hrs2_trunc = tod2_100 / 100; //eg 1034 -> 10
  int min2 = mod2_min * 0.596; //eg 99 -> 59, 50 -> 30
  int tod2_min = hrs2_trunc * 100; 
  int tod2 = (hrs2_trunc * 100) + min2; //exact time of day to minute from potentiometer
  int rem2_30 = min2 % 30;
  int close = (hrs2_trunc * 100) + (min2 - rem2_30); //put it all together..

  if (close < 100 && blink == false){ //display close time 
    int close1 = close*10;
    display2.showNumberDec(close1, 0b10000000, 1, 4, 1); //ShowNumberDec(number to display, colon 0b10000000=on (for this config only), leading zeroes T/F. length, start position)
  }

  if (close >= 100 && blink == false){
    display2.showNumberDec(close, 0b01000000, 0, 4, 0);
  }
  
  //#####################################################################
  //improper timing blink sequence

  if (open >= close){ //blink display1 & 2 if close time occurs before open time
    if (millis() - blink_timer > 500 && blink == false) {
      display1.setSegments(blank);
      display2.setSegments(blank); 
      blink_timer = millis();
      blink = true;
    }
    
    if (millis() - blink_timer > 500 && blink == true) {
      if (open < 100){ //display open time
        int open1 = open*10;
        display1.showNumberDec(open1, 0b10000000, 1, 4, 1); //ShowNumberDec(number to display, colon 0b10000000=on (for this config only), leading zeroes T/F. length, start position)
      }
      if (open >= 100){
        display1.showNumberDec(open, 0b01000000, 0, 4, 0);      
      }

      if (close < 100){ //display close time
        int close1 = close*10;
        display2.showNumberDec(close1, 0b10000000, 1, 4, 1); //ShowNumberDec(number to display, colon 0b10000000=on (for this config only), leading zeroes T/F. length, start position)
      }
      if (close >= 100){
        display2.showNumberDec(close, 0b01000000, 0, 4, 0);
      }
      blink_timer = millis();
      blink = false;
    }
  }
  if (open < close) { //if negative time condition is fixed while outside above if condition, blink must be returned to false so time display works in other sections 
    blink = false;
  }

  //#####################################################################
  //RTC: create int disp_time using t.hour and t.min fields of t object
  t = rtc.getTime(); //populate t object fields
  int disp_sec = t.hour;
  int disp_min = t.min;
  int tod = t.hour*100+t.min; //put it all together..

  //display time on TM1637 (7-Seg) from 0:00 to 1:00 am: shift start pos to [1] and turn leading zeros on
  if (tod < 100){
    if (t.sec < 2){ //this clears the "2" that would otherwise remain in pos[0] after rollover from 2359. only happens once
      display3.setSegments(blank);
    }
    int tod1 = tod*10;
    display3.showNumberDec(tod1, 0b10000000, 1, 4, 1); //ShowNumberDec(number to display, colon 0b10000000=on (for this config only), leading zeroes T/F. length, start position)
  }
  
  //display time on TM1637 (7-Seg) from 1:00 to 23:59 pm: shift start pos to [0] and turn leading zeros off
  if (tod >= 100){
    display3.showNumberDec(tod, 0b01000000, 0, 4, 0); //ShowNumberDec(number to display, colon 0b01000000=on, leading zeroes T/F. length, start position)
  }

  //#####################################################################
  //3144 Hall effect sensor - door closed check sequence
  
  if (digitalRead(12) == 1){ //check if hall sensor outputting 1 meaning no proximity to magnet, indicating door open
    closed = false; //set closed to false 
    close_timer = millis(); //start timer for three seconds
  }  
  if (closed == false && millis() - close_timer > 3000 && digitalRead(12) == 0){ //if bool closed is false and it has been > three seconds, assume door closed is true
      closed = true;
  }

  //#####################################################################
  //servo sequence

  if (open < close && tod >= open && tod <= close && servo_angle != open_angle && closed == true){ //disengage lock to open
    servo_angle = open_angle;  //open_angle seems to be 5
    lock_servo.attach(servo_pwm); //attach servo
    lock_servo.write(servo_angle);
    delay(2000);
    lock_servo.detach(); //un-attach servo. stops power to servo
  }

  if (open < close && tod <= open || tod >= close && servo_angle != lock_angle && closed == true){ //engage lock
    servo_angle = lock_angle; //lock_angle seems to be 105
    lock_servo.attach(servo_pwm); //attach servo
    lock_servo.write(servo_angle);
    delay(2000);
    lock_servo.detach(); //un-attach servo. stops power to servo
  }

  if (open > close && servo_angle != lock_angle && closed == true){ //engage lock
    servo_angle = lock_angle; //lock_angle seems to be 105
    lock_servo.attach(servo_pwm); //attach servo
    lock_servo.write(servo_angle);
    delay(2000);
    lock_servo.detach(); //un-attach servo. stops power to servo
  }

}



