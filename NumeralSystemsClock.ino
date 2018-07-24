/* Clock for several numeral systems, inspired by the new binary clock at SBB rail station St.Gallen, Switzerland. */

/* Initialize DS1307 V1.4 Real Time Clock. */
#include <Wire.h>
#define DS1307_I2C 0x68

/* Initialize LedControl/MAX7219. */
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
uint8_t second_reached;
uint8_t decisecond_reached;
uint8_t stop_centiseconds;
uint8_t stop_deciseconds;
uint8_t stop_seconds;
uint8_t stop_minutes;

/* Buttons INPUT_PULLUP */
const uint8_t BUTTON[3] = {5, 6, 7};
long button_pressed[3];
long button_status[3];
float last_pressed_time[3];
float pre_last_pressed_time[3];
bool button_evaluated[3] = {false, false, false};

/* Numeral systems */
uint8_t base = 2;
uint8_t previous_base;
char representation[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
const uint8_t SHOW_BASE_SECS = 4;
int show_base = SHOW_BASE_SECS;

/* Clock modes:
 * 0: clock
 * 1: set time
 * 2: stopwatch
 */
uint8_t clock_mode = 0;
uint8_t previous_clock_mode;
uint8_t current_row;
bool current_on;

void setup() {

  //Serial.begin(9600);

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
  Wire.begin();
  read_rtc();
  trigger_micros = micros() + 1000000;

  /* Setup buttons. */
  for (int i = 0; i < 3; i++) {
    pinMode(BUTTON[i], INPUT_PULLUP);
  }

  //display_test(50);

}

void loop() {

  button_commands();

  if (micros() >= trigger_micros) {

    trigger_micros += 10000.0;

    second_reached++;
    decisecond_reached++;

    if (second_reached >= 100) {
      if (clock_mode != 1) {
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
      }
      if (show_base > -1) {
        show_base--;
      }
      second_reached = 0;
      if (clock_mode == 0) {
        print_clock(seconds, minutes, hours);
      }
    }

    /* Sometimes a driver seems to get down, even with external 5V 2A.
     * But not needed since two capacitors (10nF and 100nF).

    for (int i = 0; i < NUM_DRIVERS; i++) {
    lc.shutdown(i, false);
    }*/

    if (clock_mode == 1) {
      if (second_reached == 50 || second_reached == 0) {
        print_clock(seconds, minutes, hours);
        /* If BUTTON[0] and BUTTON[2] not pressed: */
        if (button_pressed[0] == button_pressed[2]) {
          if (!current_on) {
            lc.clearDisplay(current_row);
          }
          current_on = !current_on;
        }
      }
    }

    if (clock_mode == 2) {
      stop_centiseconds++;
      if (stop_centiseconds >= 100) {
        stop_seconds++;
        if (stop_seconds >= 60) {
          stop_minutes++;
          if (stop_minutes >= 60) {
            stop_minutes = 0;
          }
          stop_seconds = 0;
        }
        stop_centiseconds = 0;
      }
      if (base > 4) {
        print_clock(stop_centiseconds, stop_seconds, stop_minutes);
      }
      else {
        if (decisecond_reached >= 10) {
          stop_deciseconds++;
          if (stop_deciseconds >= 10) {
            stop_deciseconds = 0;
          }
          print_clock(stop_deciseconds, stop_seconds, stop_minutes);
          decisecond_reached = 0;
        }
      }
    }

  }

}

void button_commands() {

  for (int i = 0; i < 3; i++) {
    if (digitalRead(BUTTON[i]) == LOW) {
      button_pressed[i]++;
    }
    else {
      button_status[i] = button_pressed[i];
      button_pressed[i] = 0;
      button_evaluated[i] = false;
      if (button_status[i] > 50) {
        pre_last_pressed_time[i] = last_pressed_time[i];
        last_pressed_time[i] = micros();
      }
    }
  }

  if (button_pressed[2] > 50) {
    if (clock_mode == 0 || clock_mode == 2) {
      if (button_pressed[1] > 50) {
        /* Do not interpret as long klick (time setting). */
        button_evaluated[1] = true;
        /* Do not interpret as short klick (show numeral base). */
        button_pressed[1] = 30000;
        if (brightness < 15) {
          brightness++;
          set_brightness();
          button_pressed[2] = -5000;
        }
      }
      else {
        if (base < 16) {
          base++;
          show_base = SHOW_BASE_SECS;
          print_clock(seconds, minutes, hours);
          button_pressed[2] = -11000;
        }
      }
    }
    if (clock_mode == 1) {
      if (current_row == 2 && hours < 24) {
        hours++;
        print_clock(seconds, minutes, hours);
      }
      if (current_row == 1 && minutes < 60) {
        minutes++;
        print_clock(seconds, minutes, hours);
      }
      if (current_row == 0 && seconds < 60) {
        seconds++;
        print_clock(seconds, minutes, hours);
      }
      button_pressed[2] = -5000;
    }
  }

  if (button_pressed[0] > 50) {
    if (clock_mode == 0 || clock_mode == 2) {
      if (button_pressed[1] > 50) {
        /* Do not interpret as long klick (time setting). */
        button_evaluated[1] = true;
        /* Do not interpret as short klick (show numeral base). */
        button_pressed[1] = 30000;
        if (brightness > 0) {
          brightness--;
          set_brightness();
          button_pressed[0] = -5000;
        }
      }
      else {
        if (base > 2) {
          base--;
          show_base = SHOW_BASE_SECS;
          print_clock(seconds, minutes, hours);
          button_pressed[0] = -11000;
        }
      }
    }

    if (clock_mode == 1) {
      if (current_row == 2 && hours > 0) {
        hours--;
        print_clock(seconds, minutes, hours);
      }
      if (current_row == 1 && minutes > 0) {
        minutes--;
        print_clock(seconds, minutes, hours);
      }
      if (current_row == 0 && seconds > 0) {
        seconds--;
        print_clock(seconds, minutes, hours);
      }
      button_pressed[0] = -5000;
    }
  }

  if (button_pressed[1] > 30000) {
    if (!button_evaluated[1]) {
      if (clock_mode == 0) {
        clock_mode = 1;
        current_row = NUM_DRIVERS - 1;
      }
      else {
        clock_mode = 0;
        write_rtc();
      }
      print_clock(seconds, minutes, hours);
      button_evaluated[1] = true;
    }
  }

  if (button_status[1] > 50 && button_status[1] < 30000) {
    if (last_pressed_time[1] - pre_last_pressed_time[1] < 500000.0) {
      if (clock_mode == 0) {
        clock_mode = 2;
      }
      else if (clock_mode == 2) {
        clock_mode = 0;
      }
    }
    else {
      if (clock_mode == 0) {
        show_base = SHOW_BASE_SECS;
        print_clock(seconds, minutes, hours);
      }
      if (clock_mode == 1) {
        if (current_row == 0) {
          current_row = NUM_DRIVERS - 1;
        }
        else {
          current_row--;
        }
      }
    }
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

void print_clock(uint8_t bottom, uint8_t middle, uint8_t top) {

  int digits;

  if (base != previous_base) {
    clear_displays();
    previous_base = base;
  }

  if (clock_mode != previous_clock_mode) {
    clear_displays();
    previous_clock_mode = clock_mode;
  }

  if (base > 2) {
    if (show_base > 0) {
      if (base < 10) {
        digits = 1;
      }
      else {
        digits = 2;
      }
      print_digits(base, 0, NUM_DIGITS[0] - digits, 16, 10);
    }
    else if (show_base == 0) {
      lc.clearDisplay(0);
    }
  }

  if (clock_mode == 0 || clock_mode == 1) {
    print_digits(bottom, 0, 0, 60, base);
    print_digits(middle, 1, 0, 60, base);
    print_digits(top, 2, 0, 24, base);
  }
  if (clock_mode == 2) {
    if (base > 4) {
      print_digits(bottom, 0, 0, 100, base);
    }
    else {
      print_digits(bottom, 0, 0, 10, base);
    }
    print_digits(middle, 1, 0, 60, base);
    print_digits(top, 2, 0, 60, base);
  }

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

void set_brightness() {

  for (int i = 0; i < NUM_DRIVERS; i++) {
    lc.setIntensity(i, brightness);
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

void read_rtc() {

  byte b_seconds;
  byte b_minutes;
  byte b_hours;

  Wire.beginTransmission(DS1307_I2C);
  Wire.write((byte)0x00);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_I2C, 3);
  b_seconds = Wire.read();
  b_minutes = Wire.read();
  b_hours = Wire.read();

  seconds = (b_seconds/16*10) + (b_seconds%16) & 0x7f;
  minutes = (b_minutes/16*10) + (b_minutes%16);
  hours = (b_hours/16*10) + (b_hours%16) & 0x3f;

}

void write_rtc() {

  byte b_seconds;
  byte b_minutes;
  byte b_hours;

  b_seconds = (seconds/10*16) + (seconds%10) & 0x7f;
  b_minutes = (minutes/10*16) + (minutes%10);
  b_hours = (hours/10*16) + (hours%10) & 0x3f;

  Wire.beginTransmission(DS1307_I2C);
  Wire.write((byte)0x00);
  Wire.write(b_seconds);
  Wire.write(b_minutes);
  Wire.write(b_hours);
  Wire.endTransmission();

}
