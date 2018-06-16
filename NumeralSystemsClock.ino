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
unsigned long seconds;
float trigger_time;

void setup() {

  /* Start and set up displays. */
  for (int i = 0; i < NUM_DRIVERS; i++) {
    /* Wake up: */
    lc.shutdown(i, false);
    /* Brightness: */
    lc.setIntensity(i, brightness);
    /* Clear display: */
    lc.clearDisplay(i); 
  }

  /* Setup Clock. */
  seconds = 49312;
  trigger_time = micros() + 1000000;
  
  //Serial.begin(9600);
  //display_test(50);

}

void loop() {

  if (micros() >= trigger_time) {
    
    trigger_time += 1000000.0;
    seconds++;
    
    /* Restart at midnight. */
    if (seconds >= 86400) {
      seconds = 0;
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
    /* Sometimes a driver seems to get down, even with external 5V 2A. */
    lc.shutdown(row,false);
    lc.setDigit(row, first_digit + i, !!(number & 1<<i), false);
  }
  
}

void print_clock() {

  int hours = seconds / 60 / 60;
  int minutes = seconds / 60 - (hours * 60);
  int secs = seconds - ((hours * 60 + minutes) * 60);

  binary(secs, 0, 0);
  binary(minutes, 1, 0);
  binary(hours, 2, 0);
  
}

