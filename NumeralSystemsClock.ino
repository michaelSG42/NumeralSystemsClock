/* Clock for several numeral systems, inspired by the new binary clock at SBB rail station St.Gallen, Switzerland. */

/* Initialize DS1307 V1.4 Real Time Clock. */
#include <Wire.h>
#include <DS1307RTC.h>
#define DS1307_I2C 0x68

/* Soft clock */
#include <TimeLib.h>
#include <Time.h>
uint8_t lastSecond;
uint8_t setSecond;
uint8_t setMinute;
uint8_t setHour;
uint8_t stopCenti;
uint8_t stopDeci;
uint8_t stoppedTime;
unsigned long lastEventMillis;
unsigned long startStopClock;
bool stopClockRunning;
bool isChanged;
const uint8_t STOP_BASE_CENTI = 5; /* Numeral base to start with centiseconds: */

/* Initialize LedControl/MAX7219. */
#include <LedControl.h>
#define PIN_DIN  12
#define PIN_CLK  11
#define PIN_LOAD 10  
const uint8_t DRIVERS = 3;
LedControl lc=LedControl(PIN_DIN, PIN_CLK, PIN_LOAD, DRIVERS);

/* Seven-Segment Displays */
const uint8_t DIGITS[DRIVERS] = {6, 6, 5};
uint8_t brightness = 1;

/* Buttons INPUT_PULLUP */
const uint8_t BUTTON[3] = {5, 6, 7};
bool buttonLastStatus[3];
bool buttonIsPressed[3];
bool buttonIsHold[3];
bool buttonEvaluated[3];
uint8_t buttonIsClicked[3];
uint8_t buttonClicks[3];
unsigned long buttonMillis[3];

/* Numeral systems */
uint8_t base = 2;
uint8_t preBase;
const uint8_t SHOW_BASE_SECS = 4;
int showBase = SHOW_BASE_SECS;
const char REPRESENTATION[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

/* Clock modes:
 * 0: clock
 * 1: set time
 * 2: stopwatch
 */
uint8_t clockMode = 0;
uint8_t preClockMode;
uint8_t currentRow;

void setup() {

  //Serial.begin(9600);

  /* Get settings from RTC RAM */
  readSettings();

  /* Start and setup displays. */
  for (int i = 0; i < DRIVERS; i++) {
    /* Wake up: */
    lc.shutdown(i, false);
    /* Brightness: */
    lc.setIntensity(i, brightness);
    /* Clear display: */
    lc.clearDisplay(i); 
  }

  /* Setup clock. */
  readRTC();

  /* Setup buttons. */
  for (int i = 0; i < 3; i++) {
    pinMode(BUTTON[i], INPUT_PULLUP);
  }

}

void loop() {

  readButtons();
  buttonActions();

  if (second() != lastSecond) {

    if (clockMode == 0) {
      if (showBase > -1) {
        showBase--;
      }
      lastSecond = second();
      printClock(second(), minute(), hour());
    }

    if (clockMode == 1) {
      lastSecond = second();
      lastEventMillis = millis();
      printClock(setSecond, setMinute, setHour);
    }

  }

  if (clockMode == 1 && (millis() - lastEventMillis) >= 500) {

    lastEventMillis = millis();

    /* If BUTTON[0] and BUTTON[2] not pressed: */
    if (buttonIsPressed[0] == buttonIsPressed[2]) {
      lc.clearDisplay(currentRow);
    }

  }

  if (clockMode == 2 && stopClockRunning) {
    if (millis() - lastEventMillis >= 10) {
      giveStopClock(now() - startStopClock, true);
    }
  }

}

void readButtons() {

  for (int i = 0; i < 3; i++) {

    if (digitalRead(BUTTON[i]) == LOW) {

      if (!buttonLastStatus[i]) {
        buttonLastStatus[i] = true;
        buttonMillis[i] = millis();
      }

      if (millis() - buttonMillis[i] >= 20) {
        buttonIsPressed[i] = true;
      }

      if (millis() - buttonMillis[i] >= 1000) {
        buttonIsHold[i] = true;
      }

    }

    else {

      buttonIsHold[i] = false;

      if (buttonLastStatus[i]) {
        buttonMillis[i] = millis();
        buttonLastStatus[i] = false;
      }

      if (buttonIsPressed[i] && (millis() - buttonMillis[i] >= 50)) {
        /* Double clicks only for middle button: */
        if (i == 1) {
          buttonClicks[i]++;
        }
        else {
          buttonIsClicked[i] = 1;
        }
        buttonIsPressed[i] = false;
      }

      if (buttonClicks[i] > 0 && (millis() - buttonMillis[i] >= 100)) {
        buttonIsClicked[i] = buttonClicks[i];
        buttonClicks[i] = 0;
      }

    }

  }

}

void buttonActions() {

  /* Left and right button actions: */

  for (int i = 0; i < 3; i = i + 2) {

    if (buttonIsHold[i]) {
      buttonIsHold[i] = false;
      buttonMillis[i] = millis() - 750;
      buttonIsClicked[i] = 1;
    }

    if (buttonIsClicked[i] == 1) {

      buttonIsClicked[i] = 0;

      if (buttonIsPressed[1]) {
        buttonEvaluated[1] = true;
        changeBright(i);
        setBright();
      }

      else {

        if (clockMode == 1) {
          changeSetTime(i);
          printClock(setSecond, setMinute, setHour);
        }
        
        else {
          changeBase(i);
          showBase = SHOW_BASE_SECS;
          if (clockMode == 0 || clockMode == 1) {
            printClock(second(), minute(), hour());
          }
          if (clockMode == 2 && !stopClockRunning) {
            giveStopClock(stoppedTime, false);
          }
        }

      }

    }

  }

  /* If middle button was used for combination with left or right button, do not interpret as click, too. */

  if (buttonEvaluated[1] && buttonIsClicked[1] == 1) {
    buttonEvaluated[1] = false;
    buttonIsClicked[1] = 0;
  }

  /* Middle button actions: */

  if (buttonIsClicked[1] == 1) {

    buttonIsClicked[1] = 0;

    if (clockMode == 0) {
      showBase = SHOW_BASE_SECS;
      printClock(second(), minute(), hour());
    }

    if (clockMode == 1) {
      if (currentRow == 0) {
        currentRow = DRIVERS - 1;
      }
      else {
        currentRow--;
      }
    }

    if (clockMode == 2) {
      if (stopClockRunning) {
        stoppedTime = now() - startStopClock;
        giveStopClock(stoppedTime, true);
      }
      else {
        lastEventMillis = millis();
        startStopClock = now() - stoppedTime;
        giveStopClock(stoppedTime, true);
      }
      stopClockRunning = !stopClockRunning;
    }

  }

  /* Multiple clicks only from middle button: */

  if (buttonIsClicked[1] == 2) {

    buttonIsClicked[1] = 0;

    if (clockMode == 0) {
      stopClockRunning = false;
      stoppedTime = 0;
      stopDeci = 0;
      stopCenti = 0;
      clockMode = 2;
      showBase = 1;
      printClock(0, 0, 0);
    }
    else if (clockMode == 2) {
      clockMode = 0;
    }

  }

  if (buttonIsClicked[1] > 2) {
    buttonIsClicked[1] = 0;
    displayTest(40);
  }

  /* Button holds for middle button: */

  if (buttonIsHold[1] && !buttonEvaluated[1] && !buttonIsPressed[0] && !buttonIsPressed[2]) {

    buttonEvaluated[1] = true;

    if (clockMode == 0) {
      clockMode = 1;
      currentRow = DRIVERS - 1;
      setSecond = second();
      setMinute = minute();
      setHour = hour();
    }
    else if (clockMode == 1) {
      clockMode = 0;
      writeRTC();
      readRTC();
    }

    printClock(setSecond, setMinute, setHour);

  }

}

void displayTest(int blinkDelay) {

  bool m = true;

  clearDisplays();

  for (int i = 0; i < DRIVERS; i++) {
    for (int j = 0; j < DIGITS[i]; j++) {
      for (int k = 0; k < 2; k++) {
        for (int l = 0; l < 8; l++) {
          lc.setLed(i, j, l, m);
          delay(blinkDelay);
        }
        m = !m;          
      }
    }  
  }

  for (int l = 0; l < 8; l++){
    for (int k = 0; k < 2; k++) {
      for (int i = 0; i < DRIVERS; i++) {
        for (int j = 0; j < DIGITS[i]; j++) {
          lc.setLed(i, j, l, m);
          delay(blinkDelay);
        }  
      }
    m = !m;
    }
  }

}

/* void binary(int number, int row, int firstDigit) { */

  /* (number & 1<<i) gives 0 or number,
   * ! negates => 1 or 0), but need 1 for true => !!(number & i<<i).
   * 
   * Alternative: 
   *  for (int i = 0; i < 6; i++) {
   *    if (number & 1<<i) {
   *      lc.setChar(0, i, '1', false);
   *    }
   *    else {
   *      lc.setChar(0, i, '0', false);
   *    }
   *  } */

/*  for (int i = 0; i < DIGITS[row] - firstDigit; i++) {
    lc.setDigit(row, firstDigit + i, !!(number & 1<<i), false);
  }

} */

void printClock(uint8_t bottom, uint8_t middle, uint8_t top) {

  int digits;

  if (base != preBase) {
    clearDisplays();
    preBase = base;
  }

  if (clockMode != preClockMode) {
    clearDisplays();
    preClockMode = clockMode;
  }

  if (base > 2) {
    if (showBase > 0) {
      if (base < 10) {
        digits = 1;
      }
      else {
        digits = 2;
      }
      printDigits(base, 0, DIGITS[0] - digits, 16, 10);
    }
    else if (showBase == 0) {
      lc.clearDisplay(0);
    }
  }

  if (clockMode == 0 || clockMode == 1) {
    printDigits(bottom, 0, 0, 59, base);
    printDigits(middle, 1, 0, 59, base);
    printDigits(top, 2, 0, 23, base);
  }

  if (clockMode == 2) {
    if (base >= STOP_BASE_CENTI) {
      printDigits(bottom, 0, 0, 99, base);
    }
    else {
      printDigits(bottom, 0, 0, 9, base);
    }
    printDigits(middle, 1, 0, 59, base);
    printDigits(top, 2, 0, 59, base);
  }

}

void printDigits(int number, int row, int firstDigit, int maximum, int numeralBase) {

  unsigned long power[DIGITS[row] - firstDigit];
  int digit;

  /* pow() calculates with floats and gives wrong integers, i.e. 3^3 = 26. */
  power[0] = 1;
  for (int i = 1; i < DIGITS[row] - firstDigit; i++) {
    power[i] = power[i - 1] * numeralBase;
  }

  for (int i = DIGITS[row] - firstDigit - 1; i >= 0; i--) {
    if (maximum > power[i]) {
      digit = 0;
      while (number >= power[i]) {
        digit++;
        number -= power[i];
      }
      if (digit < numeralBase) {
        lc.setChar(row, firstDigit + i, digit, false);
      }
      else {
        error(row, firstDigit + i);
        break;
      }
    }
  }

}

void giveStopClock(int stopTime, bool update) {

  if (update) {

    if (lastSecond != second(stopTime)) {
      lastSecond = second(stopTime);
      stopCenti = 0;
      stopDeci = 0;
      isChanged = true;
    }

    while (millis() - lastEventMillis >= 10) {
      lastEventMillis += 10;
      if (stopCenti < 99) {
        stopCenti++;
      }
    }

    if (stopDeci != stopCenti / 10) {
      stopDeci = stopCenti / 10;
      isChanged = true;
    }

  }

  if (base >= STOP_BASE_CENTI) {
    printClock(stopCenti, second(stopTime), minute(stopTime));
  }
  else if (isChanged || !stopClockRunning) {
    isChanged = false;
    printClock(stopDeci, second(stopTime), minute(stopTime));
  }

}

void changeBright(int upDown) {

  /* 0: left button = down
   * 2: right button = up
   */

  if (upDown == 0 && brightness > 0) {
    brightness--;
    writeSettings();
  }

  if (upDown == 2 && brightness < 15) {
    brightness++;
    writeSettings();
  }

}

void setBright() {

  for (int i = 0; i < DRIVERS; i++) {
    lc.setIntensity(i, brightness);
  }

}

void changeBase(int upDown) {

  /* 0: left button = down
   * 2: right button = up
   */

  if (upDown == 0) {
    base--;
  }

  if (upDown == 2) {
    base++;
  }

  if (base < 2) {
    base = 16;
  }

  if (base > 16) {
    base = 2;
  }

  writeSettings();

}

void changeSetTime(int upDown) {

  /* 0: left button = down
   * 2: right button = up
   */

  if (upDown == 0) {
    
    if (currentRow == 2) {
      
      if (setHour > 0) {
        setHour--;
      }
      else {
        setHour = 23;
      }
      
    }
    
    if (currentRow == 1) {
      
      if (setMinute > 0) {
        setMinute--;
      }
      else {
        setMinute = 59;
      }
      
    }
    
    if (currentRow == 0) {
      
      if (setSecond > 0) {
        setSecond--;
      }
      else {
        setSecond = 59;
      }
      
    }
    
  }

  if (upDown == 2) {
    if (currentRow == 2) {
      setHour++;
    }
    if (currentRow == 1) {
      setMinute++;
    }
    if (currentRow == 0) {
      setSecond++;
    }

    if (setHour == 24) {
      setHour = 0;
    }
    if (setMinute == 60) {
      setMinute = 0;
    }
    if (setSecond == 60) {
      setSecond = 0;
    }
  }

}

void error(int row, int digit) {

  for (int i = 0; i < 8; i++){
    lc.setLed(row, digit, i, false);
  }

  lc.setLed(row, digit, 7, true);

}

void clearDisplays() {

  for (int i = 0; i < DRIVERS; i++) {
   lc.clearDisplay(i);
  }

}

void readRTC() {

  setSyncProvider(RTC.get);

}

void writeRTC() {

  byte bSecond;
  byte bMinute;
  byte bHour;

  bSecond = (setSecond/10*16) + (setSecond%10) & 0x7f;
  bMinute = (setMinute/10*16) + (setMinute%10);
  bHour = (setHour/10*16) + (setHour%10) & 0x3f;

  Wire.beginTransmission(DS1307_I2C);
  Wire.write((byte)0x00);
  Wire.write(bSecond);
  Wire.write(bMinute);
  Wire.write(bHour);
  Wire.endTransmission();

}

void readSettings() {

  Wire.beginTransmission(DS1307_I2C);
  Wire.write((byte)0x08);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C, 2);
  base = int(Wire.read());
  brightness = int(Wire.read());

}

void writeSettings() {

  Wire.beginTransmission(DS1307_I2C);
  Wire.write((byte)0x08);
  Wire.write(byte(base));
  Wire.write(byte(brightness));
  Wire.endTransmission();

}
