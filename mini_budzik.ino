//Libraries:
//for DS53231 timer I2C https://github.com/jarzebski/Arduino-DS3231

//https://forum.arduino.cc/t/using-millis-for-timing-a-beginners-guide/483573
//https://forum.arduino.cc/t/how-to-make-hold-button-function/622665
//http://gammon.com.au/interrupts
//https://www.arduino.cc/en/Tutorial/BuiltInExamples/toneMelody

#include <Wire.h>
#include <DS3231.h>
#include "pitches.h"

DS3231 clock;
RTCDateTime dt;

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

int cheerMelody[] = {
  NOTE_C4, 0, NOTE_C4, 0, NOTE_C4, NOTE_C4, NOTE_C4, 0, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, 0, NOTE_C4, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

int noteDurationsCheer[] = {
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

enum Pin
{
  MorningLed = 9, //PWM
  NightLed = 10, //PWM
  Button = 2, //Interrupt
  Buzzer = 5 //PWM
};

bool buttonPressed = 0;

bool morningTaskState = 0;
bool nightTaskState = 0;
bool alarmTriggered = 0;

void buttonDown(){
  buttonPressed = 1;
  return;
}

bool isMorning(RTCDateTime dtNow){
  if(dtNow.hour>=6 && dtNow.hour<=10) return true;
  return false;
}

bool isNight(RTCDateTime dtNow){
  if(dtNow.hour>=19 && dtNow.hour<=24) return true;
  return false;
}

bool isNewDay(RTCDateTime dtNow){
  if(dtNow.hour==0 && dtNow.minute==0 && dtNow.second==0) return true;
  return false;
}

void ledFade(Pin ledPin){
  for (int fadeValue = 0; fadeValue <= 250; fadeValue += 5) {
    analogWrite(ledPin, fadeValue);
    delay(10);
  }
  for (int fadeValue = 250; fadeValue >= 0; fadeValue -= 5) {
    analogWrite(ledPin, fadeValue);
    delay(10);
  }
}

void ledBlink(Pin ledPin){
  for (int counter = 0; counter <= 3; counter++) {
    digitalWrite(ledPin, 1);
    delay(100);
    digitalWrite(ledPin, 0);
    delay(100);
  }
}

void ledReset(){
  digitalWrite(Pin(MorningLed),0);
  digitalWrite(Pin(NightLed),0);
}

void handleAlarm(){
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(Pin(Buzzer), melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(Pin(Buzzer));
  }
}

void handleButton(){
  unsigned long startMillis = millis();
  unsigned long currentMillis = millis();
  unsigned long period = 1000;

  while(digitalRead(Pin(Button)) && currentMillis - startMillis <= period){
    currentMillis = millis();
  }

  if(currentMillis - startMillis >= period){
    
    //TODO : gra rytmiczna
    demo();

  }else{
    if(alarmTriggered){
      alarmTriggered = 0;
      digitalWrite(MorningLed,morningTaskState);
      digitalWrite(NightLed,nightTaskState);
      noTone(Pin(Buzzer));
    }
    else if(isMorning(dt) && !morningTaskState)
    {
      morningTaskState = 1;
      ledBlink(Pin(MorningLed));
      digitalWrite(MorningLed,1);

    }else if(isNight(dt) && !nightTaskState){
      nightTaskState = 1;
      ledBlink(Pin(NightLed));
      digitalWrite(NightLed,1);
    }
  }
}

void peep(){
    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / 4;
    tone(Pin(Buzzer), NOTE_B3, noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(Pin(Buzzer));
}

void demo(){
  ledFade(Pin(MorningLed));
  ledFade(Pin(NightLed));
  peep();

  for (int thisNote = 0; thisNote < 15; thisNote++) {
    int noteDuration = 1000 / noteDurationsCheer[thisNote];
    tone(Pin(Buzzer), cheerMelody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(Pin(Buzzer));
  }
}

void setup() {
  
  Serial.begin(9600);

  pinMode(Pin(Buzzer), OUTPUT);

  clock.begin();
  // alarm everyday 08:00:00
  clock.setAlarm1(0, 8, 0, 0, DS3231_MATCH_H_M_S);
  // Disarm extra alarm and clear alarm, because alarms is battery backed.
  clock.armAlarm2(false);
  clock.clearAlarm2();
  // Manual (Year, Month, Day, Hour, Minute, Second)
  // TODO: changing hour
  //clock.setDateTime(2024, 9, 20, 2, 47, 0);

  pinMode(Pin(Button), INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Pin(Button)), buttonDown, RISING);
}

void loop() {
  dt = clock.getDateTime();

  if (clock.isAlarm1())
  {
    Serial.println("ALARM TRIGGERED!");
    alarmTriggered = 1;
  }

  if(alarmTriggered){
    handleAlarm();
  }
  else{
    if(isMorning(dt) && !morningTaskState){
      ledFade(Pin(MorningLed));
    }else if(isNight(dt) && !nightTaskState){
      ledFade(Pin(NightLed));
    }
  }
  
  if(buttonPressed){
    peep();
    handleButton();
    buttonPressed = 0;
  }

  if(isNewDay(dt)){
    ledReset();
  }
}
