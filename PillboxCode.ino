#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

// — I²C LCD @0x27, 16×2 —
LiquidCrystal_I2C lcd(0x27, 16, 2);
// — DS3231 RTC via RTClib —
RTC_DS3231 rtc;

// — Single-button & alarm pin —
const int buttonPin = 4;
const int alarmPin  = 9;

// — Alarm time —
int alarmHour   = 0;
int alarmMinute = 0;

// — Button timing/state —
bool lastState      = HIGH;
unsigned long pressTime   = 0;
unsigned long releaseTime = 0;
const unsigned long longPressThreshold = 1000;  // 1 second

// — Setting state: 0=view, 1=set hour, 2=set minute —
int setStage = 0;
bool alarmTriggered = false;

void setup() {
  Wire.begin();
  rtc.begin();
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.init();
  lcd.backlight();

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(alarmPin,  OUTPUT);

  updateDisplay();
}

void loop() {
  // 1) Update the current time on row 2 when the minute changes
  static int lastMin = -1;
  DateTime now = rtc.now();
  if (now.minute() != lastMin) {
    lcd.setCursor(0, 1);
    if (now.hour()   < 10) lcd.print('0');
    lcd.print(now.hour());
    lcd.print(':');
    if (now.minute() < 10) lcd.print('0');
    lcd.print(now.minute());
    lcd.print("   ");
    lastMin = now.minute();
  }

  // 2) Alarm check **only in view mode** (setStage == 0)
  if (setStage == 0) {
    if (now.hour() == alarmHour && now.minute() == alarmMinute) {
      if (!alarmTriggered) {
        activateAlarm();
        alarmTriggered = true;
      }
    } else {
      alarmTriggered = false;
    }
  } else {
    // while editing, never trigger
    alarmTriggered = false;
  }

  // 3) Single-button handling
  bool curr = digitalRead(buttonPin);
  // detect press
  if (lastState == HIGH && curr == LOW) {
    pressTime = millis();
  }
  // detect release
  else if (lastState == LOW && curr == HIGH) {
    releaseTime = millis();
    unsigned long duration = releaseTime - pressTime;

    if (setStage == 0 && duration >= longPressThreshold) {
      // long-press in view → enter hour-set
      setStage = 1;
    }
    else if (setStage == 1) {
      if (duration < longPressThreshold) {
        // short-press in hour mode → +1 hour
        alarmHour = (alarmHour + 1) % 24;
      } else {
        // long-press → enter minute-set
        setStage = 2;
      }
    }
    else if (setStage == 2) {
      if (duration < longPressThreshold) {
        // short-press in minute mode → +1 minute
        alarmMinute = (alarmMinute + 1) % 60;
      } else {
        // long-press → back to view
        setStage = 0;
      }
    }

    updateDisplay();
  }

  lastState = curr;
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (setStage == 0) {
    // View mode: show alarm time
    lcd.print("Alarm: ");
    if (alarmHour   < 10) lcd.print('0');
    lcd.print(alarmHour);
    lcd.print(':');
    if (alarmMinute < 10) lcd.print('0');
    lcd.print(alarmMinute);
  }
  else if (setStage == 1) {
    // Setting hour
    lcd.print("Set Hr: ");
    if (alarmHour < 10) lcd.print('0');
    lcd.print(alarmHour);
  }
  else {
    // setStage == 2 → setting minute
    lcd.print("Set Mn: ");
    if (alarmMinute < 10) lcd.print('0');
    lcd.print(alarmMinute);
  }
}

void activateAlarm() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  !!! ALARM !!!");

  for (int i = 0; i < 10; i++) {
    // Play 650 Hz for 250 ms, then automatically stops
    tone(alarmPin, 650, 250);
    delay(300);  // give a 50 ms gap after the tone finishes
  }

  updateDisplay();
}