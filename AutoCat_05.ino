//AutoCat_05: final shipping version 5.0 written by Jasen B.W. Levoy

//Functions/Features:
//Read door open and closed time from pots and display
//Blink open/closed time displays when open >= closed
//RTC create tod and display
//Door closed check with hall effect sensor  
//Servo lock actuator sequence
//TOD clock set feature
//Adjust brightness using photo sensor
//Support functions: set displays

//See Rinky-Dink Electronics by Henning Karlsen for details about DS3231 time module. 
//web: http://www.RinkyDinkElectronics.com/

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

long int oldPotVal_1 = 0;
long int potTimer_1 = 0;
bool potHolder_1 = false;
int open = 0;

long int oldPotVal_2 = 0;
long int potTimer_2 = 0;
bool potHolder_2 = false;
int close = 0;

long int blink_timer = 0; //time blink to half second for impossible open/close times
bool blink = false; //bool for improper timing if's

long int close_timer = 0;
bool closed = false;

int open_angle = 28 ; //open
int lock_angle = 128; //lock 
int servo_angle = 5; //unlock on program start
long int servoTimer = 0;
bool detached = true;

long int counter = 720; //TOD clock set counter

long int upTimer1 = 0; //TOD clock set count up
long int upTimer2 = 0; 
long int upTimer3 = 0; 
bool upLongPress = 0;

long int downTimer1 = 0; //TOD clock set count down
long int downTimer2 = 0; 
long int downTimer3 = 0; 
bool downLongPress = 0;
long int setClockTimer = 0;
bool setClock = false;
bool upRelease = true;
bool downRelease = true;

float photoVoltage = 0; //photo sensor
long int brightTimer = 0;
int bright[10];
int brightSum = 0;
int brightCount = 0;
int brightAve = 0;

void setup() {
  Serial.begin(115200);
  display1.begin();
  display1.setBrightness(7); //set 0:min - 7:max
  display2.begin();
  display2.setBrightness(7);
  display3.begin();
  display3.setBrightness(7);

  rtc.begin(); //Initialize the rtc object
  rtc.setTime(12, 0, 0);     //set time. Reset at noon to calibrate
  pinMode(12, INPUT_PULLUP);    //hall sensor pin
  servo_angle = open_angle;  //open_angle seems to be 5
  lock_servo.attach(servo_pwm); //attach servo
  lock_servo.write(servo_angle);
  delay(2000);
  lock_servo.detach(); //un-attach servo. stops power going to servo  
  close_timer = millis(); //start timer for three seconds, make sure door is closed before lock moves 1st time (except initial unlock above)
  pinMode(11, INPUT_PULLUP); //TOD clock set count up
  pinMode(10, INPUT_PULLUP); //TOD clock set count down
}

void loop(){

  //#####################################################################
  //read and display door opening time

  if (millis() - potTimer_1 > 5000){ //chose 5 seconds as wait time to see if user is adjusting timer 1
    oldPotVal_1 = analogRead(A0);
    potTimer_1 = millis();
  }
  if (abs(oldPotVal_1 - analogRead(A0)) < 20){ //chose 20 to balance sensitivity to new activity vs. resiliance to pot jitter
    potHolder_1 = true;
  }
  if (abs(oldPotVal_1 - analogRead(A0)) > 20){
    potHolder_1 = false;
  }
  if (potHolder_1 == false){
    int pot1_val = analogRead(A0); //opening time pot (0 to 1023)
    int tod1_100 = pot1_val*2.346;
    int mod1_min = tod1_100 % 100; //remainder of tod_100 (0 to 2399) after mod 100
    int hrs1_trunc = tod1_100 / 100; //eg 1034 -> 10
    int min1 = mod1_min * 0.596; //eg 99 -> 59, 50 -> 30
    int tod1_min = hrs1_trunc * 100; 
    int tod1 = (hrs1_trunc * 100) + min1; //exact time of day to minute from potentiometer
    int rem1_30 = min1 % 30;
    open = (hrs1_trunc * 100) + (min1 - rem1_30); //put it all together..
  }
  if (blink == false){ //display close time if blink flag is false
    setTimerDisplay1(open);
  }

  //#####################################################################
  //read and display door closing time

  if (millis() - potTimer_2 > 5000){ //chose 5 seconds as wait time to see if user is adjusting timer 2
    oldPotVal_2 = analogRead(A1);
    potTimer_2 = millis();
  }
  if (abs(oldPotVal_2 - analogRead(A1)) < 20){ //chose 20 to balance sensitivity to new activity vs. resiliance to pot jitter
    potHolder_2 = true;
  }
  if (abs(oldPotVal_2 - analogRead(A1)) > 20){
    potHolder_2 = false;
  }
  if (potHolder_2 == false){
    int pot2_val = analogRead(A1); //closing time pot (0 to 1023) 
    int tod2_100 = pot2_val*2.346;
    int mod2_min = tod2_100 % 100; //remainder of tod_100 (0 to 2399) after mod 100
    int hrs2_trunc = tod2_100 / 100; //eg 1034 -> 10
    int min2 = mod2_min * 0.596; //eg 99 -> 59, 50 -> 30
    int tod2_min = hrs2_trunc * 100; 
    int tod2 = (hrs2_trunc * 100) + min2; //exact time of day to minute from potentiometer
    int rem2_30 = min2 % 30;
    close = (hrs2_trunc * 100) + (min2 - rem2_30); //put it all together..
  }
  if (blink == false){ //display close time if blink flag is false
    setTimerDisplay2(close);
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
      setTimerDisplay1(open);
      setTimerDisplay2(close);
      blink_timer = millis();
      blink = false;
    }
  }
  if (open < close) { //if negative time condition is fixed while outside above if condition, blink must be returned to false so time display works in other sections 
    blink = false;
  }

  //#####################################################################
  //RTC: create int tod using t.hour and t.min fields of t object
  t = rtc.getTime(); //populate t object fields
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
  if (digitalRead(12) == 0 && closed == false && millis() - close_timer > 3000){ //if bool closed is false and it has been > three seconds, assume door closed is true
      closed = true;
  }

  //#####################################################################
  //servo sequence

  if (closed == 1 && open < close && tod > open && tod < close && servo_angle != open_angle){ //disengage lock to open
    servo_angle = open_angle; //this is to keep it from reassigning state and constantly resetting timer
    lock_servo.attach(servo_pwm); //attach servo
    lock_servo.write(servo_angle); //open_angle seems to be 5
    servoTimer = millis();
    detached = false;
  }
  if (closed == 1 && open < close && (tod <= open || tod >= close) && servo_angle != lock_angle){ //engage lock
    servo_angle = lock_angle;
    lock_servo.attach(servo_pwm); //attach servo
    lock_servo.write(servo_angle); //lock_angle seems to be 105
    servoTimer = millis();
    detached = false;
  }
  if (closed == 1 && open >= close && servo_angle != lock_angle){ //engage lock
    servo_angle = lock_angle;
    lock_servo.attach(servo_pwm); //attach servo
    lock_servo.write(servo_angle); //lock_angle seems to be 105
    servoTimer = millis();
    detached = false;
  }
  if (millis() - servoTimer > 3000 && detached == false){
    lock_servo.detach(); //un-attach servo. stops power to servo
    detached = true;
  }

  //##################################################################################################################
  //TOD clock set

  if (digitalRead(11) == 1 || digitalRead(10) == 1){
    setClockTimer = millis();
    upTimer1 = millis();
    upTimer2 = millis();
    downTimer1 = millis();
    downTimer2 = millis();
    setClock = true;
    t = rtc.getTime(); //populate t object fields
    counter = t.hour*60+t.min;
  }
  while (setClock == true){
    if (digitalRead(11) || digitalRead(10)){
      setClockTimer = millis();
    }
    if (digitalRead(11) == 0){
      upTimer1 = millis();
      upTimer2 = millis();
      upRelease = true;
    }
    if (digitalRead(10) == 0){
      downTimer1 = millis();
      downTimer2 = millis();
      downRelease = true;
    }

    //##########################
    //TOD clock set - count up

    if (digitalRead(11) == 1 && millis() - upTimer1 > 50 && upLongPress != true && upRelease){
      counter++;
      int setHours = (counter / 60);
      int setMin = counter % 60;
      if (setHours > 23){
        setHours = 23;
      }
      rtc.setTime(setHours, setMin, 0);
      int tod = setHours*100+setMin; //put it all together..
      setClockDisplay(tod);
      upTimer1 = millis();
      upRelease = false;
    }
    if (millis() - upTimer2 > 800){
      upLongPress = true;
    }
    while (upLongPress == true){
      counter++;
      delay(5);
      int setHours = (counter / 60);
      int setMin = counter % 60;
      if (setHours > 23){
        setHours = 23;
      }    
      rtc.setTime(setHours, setMin, 0);
      int tod = setHours*100+setMin; //put it all together..
      setClockDisplay(tod);
      if (digitalRead(11) == 0){
        upLongPress = false;
      }
    }

    //##########################
    //TOD clock set - count down
    if (digitalRead(10) == 1 && millis() - downTimer1 > 50 && downLongPress != true && downRelease){
      counter--;
      if (counter < 0){
        counter = 0;
      }
      int setHours = (counter / 60);
      int setMin = counter % 60;
      rtc.setTime(setHours, setMin, 0);
      int tod = setHours*100+setMin; //put it all together..
      setClockDisplay(tod);
      downTimer1 = millis();
      downRelease = false;
    }
    if (digitalRead(10) == 0){
      downTimer2 = millis();
    }
    if(millis() - downTimer2 > 800){
      downLongPress = true;
    } 
    while (downLongPress == true){
      counter--;
      if (counter < 0){
        counter = 0;
      }
      delay(5);
      int setHours = (counter / 60);
      int setMin = counter % 60;
      rtc.setTime(setHours, setMin, 0);
      int tod = setHours*100+setMin; //put it all together..
      setClockDisplay(tod);
      if (digitalRead(10) == 0){
        downLongPress = false;
      }
    }
    if (millis() - setClockTimer > 5000){
      setClock = false;
    }
  }

  //################################################################################################
  //adjust display brightness using photo sensor

  if (millis() - brightTimer > 800){
    photoVoltage = analogRead(A2);
    photoVoltage = photoVoltage - 300;
    int brightness = photoVoltage / 80;
    if (brightness > 7){
      brightness = 7;
    }
    if (brightness < 0){
      brightness = 0;
    }
    brightness = 7 - brightness;
    brightCount++; //should be 1 after this line, to begin with
    bright[brightCount] = brightness;
     
    if (brightCount == 10){
      for (int i = 1; i < 11; i++){
        brightSum += bright[i]; 
      }
      brightAve = brightSum / 10;
      brightCount = 0;
      brightSum = 0;
      display1.setBrightness(brightAve);
      display2.setBrightness(brightAve);
      display3.setBrightness(brightAve);
    }
    brightTimer = millis();
  }
}

void setTimerDisplay1(int time){
  if (time < 100){
    int time1 = time*10;
    display1.showNumberDec(time1, 0b10000000, 1, 4, 1); //ShowNumberDec(number to display, colon 0b10000000=on (for this config only), leading zeroes T/F. length, start position)
  }
  if (time >= 100){
    display1.showNumberDec(time, 0b01000000, 0, 4, 0);
  }
}

void setTimerDisplay2(int time){
  if (time < 100){
    int time1 = time*10;
    display2.showNumberDec(time1, 0b10000000, 1, 4, 1); //ShowNumberDec(number to display, colon 0b10000000=on (for this config only), leading zeroes T/F. length, start position)
  }
  if (time >= 100){
    display2.showNumberDec(time, 0b01000000, 0, 4, 0);
  }
}

void setClockDisplay(int time){
  if (time < 100){ //display open time
    int time1 = time*10;
    display3.showNumberDec(time1, 0b10000000, 1, 4, 1); //ShowNumberDec(number to display, colon 0b10000000=on (for this config only), leading zeroes T/F. length, start position)
  }
  if (time >= 100){
    display3.showNumberDec(time, 0b01000000, 0, 4, 0);      
  }
}