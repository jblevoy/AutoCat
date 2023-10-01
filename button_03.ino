

long int counter = 0;

long int upTimer1 = 0; 
long int upTimer2 = 0; 
long int upTimer3 = 0; 
long int upMiddlePressTimer;
long int upLongPressTimer;
bool upMiddlePress = 0;
bool upLongPress = 0;

long int downTimer1 = 0; 
long int downTimer2 = 0; 
long int downTimer3 = 0; 
long int downMiddlePressTimer;
long int downLongPressTimer;
bool downMiddlePress = 0;
bool downLongPress = 0;


void setup() {
  Serial.begin(115200);
pinMode(11, INPUT_PULLUP);
pinMode(10, INPUT_PULLUP);
}

void loop() {

  //##################################################################################################################
  //count up

  if (digitalRead(11) == 1 && millis() - upTimer1 > 200 && upLongPress != true){
    counter++;
    //Serial.println(counter);
    upTimer1 = millis();
  }
  if (digitalRead(11) == 0){
    upTimer2 = millis();
    upLongPress = false;
  }
  if(millis() - upTimer2 > 500){
    upLongPress = true;
  }
  while (upLongPress == true){
    counter++;
    delay(10);
    if (digitalRead(11) == 0){
      upLongPress = false;
    }
    //Serial.println(counter);
  }

  //##################################################################################################################
  //count down

  if (digitalRead(10) == 1 && millis() - downTimer1 > 200 && downLongPress != true){
    counter--;
    //Serial.println(counter);
    downTimer1 = millis();
  }
  if (digitalRead(10) == 0){
    downTimer2 = millis();
    downLongPress = false;
  }
  if(millis() - downTimer2 > 500){
    downLongPress = true;
  } 
  while (downLongPress == true){
    counter--;
    delay(10);
    if (digitalRead(10) == 0){
      downLongPress = false;
    }
    //Serial.println(counter);
  }
/*
  int hours = (counter / 60);
  int min = counter % 60;
  Serial.print("min ");
  Serial.println(min);
  Serial.print("hours ");
  Serial.println(hours);
  Serial.print("counter ");
  Serial.println(counter);
  */
}


