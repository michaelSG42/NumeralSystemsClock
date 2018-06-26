/* Clock for several numeral systems, inspired by the new binary clock at SBB rail station St.Gallen, Switzerland. */

/*  Initialize LedControl/MAX7219. */
#include <LedControl.h>
#define PIN_DIN  12
#define PIN_CLK  11
#define PIN_LOAD 10  
const uint8_t NUM_DRIVERS = 3;

LedControl lc=LedControl(PIN_DIN, PIN_CLK, PIN_LOAD, NUM_DRIVERS);

/* Seven-Segment Displays */
const uint8_t NUM_DIGITS[NUM_DRIVERS] = {6, 6, 5};
uint8_t brightness = 1;

/* Fake clock */
uint8_t seconds;
uint8_t minutes;
uint8_t hours;
float trigger_micros;

/* Buttons INPUT_PULLUP */
const uint8_t BUTTON[3] = {5, 6, 7};
long button_status[3];

/* Numeral systems */
uint8_t base = 2;
uint8_t previous_base;
char representation[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
const uint8_t SHOW_BASE_SEC = 4;
int show_base = SHOW_BASE_SEC;

void setup() {

  /* Start and setup displays. */
  for (int i = 0; i < NUM_DRIVERS; i++) {
    /* Wake up: */
    lc.shutdown(i, false);
    /* Brightness: */
    lc.setIntensity(i, brightness);
    /* Clear display: */
    lc.clearDisplay(i); 
  }

  /* Setup clock. */
  seconds = 53;
  minutes = 45;
  hours   = 21;
  trigger_micros = micros() + 1000000;

  /* Setup buttons. */
  for (int i = 0; i < 3; i++) {
    pinMode(BUTTON[i], INPUT_PULLUP);
  }

  //Serial.begin(9600);
  //display_test(50);

}

void loop() {

  button_commands();

  if (micros() >= trigger_micros) {

    trigger_micros += 1000000.0;

    seconds++;
    if (seconds >= 60) {
      minutes++;
      if (minutes >= 60) {
        hours++;
        if (hours >= 24) {
          hours = 0;
        }
        minutes = 0;
      }
      seconds = 0;
    }

    if (show_base > -1) {
      show_base--;
    }

    /* Sometimes a driver seems to get down, even with external 5V 2A.
     * But not needed since two capacitors (10nF and 100nF).

    for (int i = 0; i < NUM_DRIVERS; i++) {
    lc.shutdown(i, false);
    }*/

    print_clock();
    
  }

}

void button_commands() {

  for (int i = 0; i < 3; i++) {
    if (digitalRead(BUTTON[i]) == LOW) {
      button_status[i]++;
    }
    else {
      button_status[i] = 0;
    }
  }

  if (button_status[2] > 50) {
    if (base < 16) {
      base++;
      show_base = SHOW_BASE_SEC;
      print_clock();
    }
    button_status[2] = -15000;
  }

  if (button_status[0] > 50) {
    if (base > 2) {
      base--;
      show_base = SHOW_BASE_SEC;
      print_clock();
    }
    button_status[0] = -15000;
  }

  if (button_status[1] > 50) {
    show_base = SHOW_BASE_SEC;
    print_clock();
  }

}

void display_test(int blink_delay) {

  bool m = true;

  clear_displays();
  
  for (int i = 0; i < NUM_DRIVERS; i++) {
    for (int j = 0; j < NUM_DIGITS[i]; j++) {
      for (int k = 0; k < 2; k++) {
        for (int l = 0; l < 8; l++) {
          lc.setLed(i, j, l, m);
          delay(blink_delay);
        }
        m = !m;          
      }
    }  
  }

  for (int l = 0; l < 8; l++){
    for (int k = 0; k < 2; k++) {
      for (int i = 0; i < NUM_DRIVERS; i++) {
        for (int j = 0; j < NUM_DIGITS[i]; j++) {
          lc.setLed(i, j, l, m);
          delay(blink_delay);
        }  
      }
    m = !m;
    }
  }
  
}

void binary(int number, int row, int first_digit) {

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

  for (int i = 0; i < NUM_DIGITS[row] - first_digit; i++) {
    lc.setDigit(row, first_digit + i, !!(number & 1<<i), false);
  }
  
}

void print_clock() {

  int digits;

  if (base != previous_base) {
    clear_displays();
    previous_base = base;
  }

  if (show_base > 0) {
    if (base > 2) {
      if (base < 10) {
        digits = 1;
      }
      else {
        digits = 2;
      }
      print_digits(base, 0, NUM_DIGITS[0] - digits, 16, 10);
    }
  }
  else if (show_base == 0) {
      clear_displays();
  }

  print_digits(seconds, 0, 0, 60, base);
  print_digits(minutes, 1, 0, 60, base);
  print_digits(hours, 2, 0, 24, base);

  /*
  binary(seconds, 0, 0);
  binary(minutes, 1, 0);
  binary(hours, 2, 0);
  */

}

void print_digits(int number, int row, int first_digit, int maximum, int numeral_base) {

  unsigned long power[NUM_DIGITS[row] - first_digit];
  int digits_max;
  int digit;

  /* pow() calculates with floats and gives wrong integers, i.e. 3^3 = 26. */
  power[0] = 1;
  for (int i = 1; i < NUM_DIGITS[row] - first_digit; i++) {
    power[i] = power[i - 1] * numeral_base;
  }

  for (int i = NUM_DIGITS[row] - first_digit - 1; i >= 0; i--) {
    if (maximum > power[i]) {
      digit = 0;
      while (number >= power[i]) {
        digit++;
        number -= power[i];
      }
      if (digit < numeral_base) {
        lc.setChar(row, first_digit + i, digit, false);
      }
      else {
        error(row, first_digit + i);
        break;
      }
    }
  }

}

void error(int row, int digit) {

  for (int i = 0; i < 8; i++){
    lc.setLed(row, digit, i, false);
  }

  lc.setLed(row, digit, 7, true);

}

void clear_displays() {

  for (int i = 0; i < NUM_DRIVERS; i++) {
   lc.clearDisplay(i);
  }

}

