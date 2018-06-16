/* Clock for several numeral systems, inspired by the new binary clock at SBB rail station St.Gallen, Switzerland. */

/*  Initialize LedControl/MAX7219. */
#include <LedControl.h>
#define PIN_DIN  12
#define PIN_CLK  11
#define PIN_LOAD 10  
const uint8_t NUM_DRIVERS = 3;

LedControl lc=LedControl(PIN_DIN,PIN_CLK,PIN_LOAD,NUM_DRIVERS);

/* Seven-Segment Displays */
const uint8_t NUM_DIGITS[NUM_DRIVERS] = {6,6,5};
uint8_t brightness = 1;

/* Fake clock */
uint8_t seconds;
uint8_t minutes;
uint8_t hours;
float trigger_micros;

uint8_t base = 2;
char representation[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

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

  /* Setup Clock. */
  seconds = 53;
  minutes = 45;
  hours   = 21;
  trigger_micros = micros() + 1000000;

  //Serial.begin(9600);
  //display_test(50);

}

void loop() {

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

    /* Sometimes a driver seems to get down, even with external 5V 2A. */
    for (int i = 0; i < NUM_DRIVERS; i++) {
      lc.shutdown(i, false);
    }

    print_clock();
    
  }

}

void display_test(int blink_delay) {

  bool m = true;

  lc.clearDisplay(0);
  
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

  print_digits(seconds, 0, 0, 60);
  print_digits(minutes, 1, 0, 60);
  print_digits(hours, 2, 0, 24);

  /*
  binary(seconds, 0, 0);
  binary(minutes, 1, 0);
  binary(hours, 2, 0);
  */

}

void print_digits(int number, int row, int first_digit, int maximum) {

  unsigned long power[NUM_DIGITS[row] - first_digit];
  int digits_max;
  int digit;

  /* pow() calculates with floats and gives wrong integers, i.e. 3^3 = 26. */
  power[0] = 1;
  for (int i = 1; i < NUM_DIGITS[row] - first_digit; i++) {
    power[i] = power[i - 1] * base;
  }

  for (int i = NUM_DIGITS[row] - first_digit - 1; i >= 0; i--) {
    if (maximum > power[i]) {
      digit = 0;
      while (number >= power[i]) {
        digit++;
        number -= power[i];
      }
      if (digit < base) {
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

