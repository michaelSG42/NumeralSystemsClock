/* Clock for several numeral systems, inspired by the new binary clock at SBB rail station St.Gallen, Switzerland. */

/*  Initialize LedControl/MAX7219 */
#include <LedControl.h>
#define PIN_DIN  12
#define PIN_CLK  11
#define PIN_LOAD 10  
const uint8_t NUM_DRIVERS = 3;

LedControl lc=LedControl(PIN_DIN,PIN_CLK,PIN_LOAD,NUM_DRIVERS);

/* 7 Segment Displays */
const uint8_t NUM_DISPLAYS[NUM_DRIVERS] = {6,6,5};

void setup() {

  /* Start and set up displays */
  for (int i = 0; i < 3; i++) {
    /* Wake up: */
    lc.shutdown(i,false);
    /* Brightness: */
    lc.setIntensity(i,1);
    /* Clear display: */
    lc.clearDisplay(i); 
  }

}

void loop() {
  
  display_test(50);

}

void display_test(int blink_delay) {

  bool m = true;

  lc.clearDisplay(0);
  
  for (int i = 0; i < NUM_DRIVERS; i++) {
    for (int j = 0; j < NUM_DISPLAYS[i]; j++) {
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
        for (int j = 0; j < NUM_DISPLAYS[i]; j++) {
          lc.setLed(i, j, l, m);
          delay(blink_delay);
        }  
      }
    m = !m;
    }
  }
  
}

