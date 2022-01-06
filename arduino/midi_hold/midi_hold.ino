// midi_hold
// (cc) by krgrWrgkmn 2022
// all rights w dupie

#include <MIDI.h>
#include <TM1637Display.h>
#include <OneButton.h>

#include "hold_controller.h"

#define VERSION HOLD_CONTROLLER_VERSION

// uncoment if you want to use serial for debug
// midi can't work together with serial for debug
//#define DEBUG_SERIAL

// - - - - - - - - - - - - - - - - - - - -
// global defs / vars
// - - - - - - - - - - - - - - - - - - - -

MIDI_CREATE_DEFAULT_INSTANCE();

// hold controller lib
#define HC hold_controller
hold_controller CTRL;

// display
#define PIN_TM_CLK A0
#define PIN_TM_DIO A1
#define PIN_TM_5V  11

// brightness A number from 0 (lowes brightness) to 7 (highest brightness)
#define TM_BRIGHTNESS 0

TM1637Display TM(PIN_TM_CLK, PIN_TM_DIO);

// rotary controller
// Define the IO Pins Used
#define PIN_ROTARY_1_CLK    2   // Used for generating interrupts using CLK signal
#define PIN_ROTARY_1_DAT    3   // Used for reading DT signal
#define PIN_ROTARY_1_SW     4   // Used for the Rotary push button switch
#define PIN_ROTARY_1_5V     5   // Set to HIGH to be the 5V pin for the Rotary Encoder
#define PIN_ROTARY_1_GND    6   // Set to LOW to be the GND pin for the Rotary Encoder

#define PIN_ROTARY_2_CLK    7   // Used for generating interrupts using CLK signal
#define PIN_ROTARY_2_DAT    8   // Used for reading DT signal
#define PIN_ROTARY_2_SW    12   // Used for the Rotary push button switch
#define PIN_ROTARY_2_5V     9   // Set to HIGH to be the 5V pin for the Rotary Encoder
#define PIN_ROTARY_2_GND   10   // Set to LOW to be the GND pin for the Rotary Encoder

#define ROTARY_PRESS_TICKS 2000


// - - - - - - - - - - - - - - - - - - - -
// callbacks
// - - - - - - - - - - - - - - - - - - - -

void arduino_midi_send(const uint8_t m[], uint8_t size) 
{
#ifndef DEBUG_SERIAL
    MIDI.send(m[0],m[1],m[2], 1);
#else
  char buf[128];
  if(size == 3) {

    sprintf(buf,"midi_send: %02X %02X %02X", m[0], m[1], m[2]);
    arduino_log(HC::DEBUG, buf);
  } else {
    sprintf(buf, "Invalid midi message size:%s", size);
    arduino_log(HC::ERROR, buf);
  }
#endif
}

void arduino_log(hold_controller::log_levels_t priority, const char * message)
{ 
#ifdef DEBUG_SERIAL
  const char * p = hold_controller::log_levels[priority];
  char buf[256];
  sprintf(buf, "> %8s :: %s", p, message);
  Serial.println(buf);
#endif
}

// - - - - - - - - - - - - - - - - - - - -
// display
// - - - - - - - - - - - - - - - - - - - -

// data for each digit
uint8_t TM_DT[] = {0,0,0,0,0};

void clear_display()
{
  TM.clear();
  delay(1000);
  TM.setBrightness(TM_BRIGHTNESS);
}

void init_display()
{
  // Set the 5V pin for display
  pinMode(PIN_TM_5V, OUTPUT);
  digitalWrite(PIN_TM_5V, HIGH);

  clear_display();
}

void update_display()
{
  uint8_t k[] = { 0xC, 0xC, 0xD, 0xD, 0xE, 0xF, 0xF, 0x6, 0x6, 0xA, 0xA, 0xB, 0xB };
  uint8_t dot = CTRL.is_on() ? 0x80 : 0;  
  int o = CTRL.octave_human();
  TM_DT[0] = (TM.encodeDigit(k[CTRL.key()])) | dot;
  TM_DT[1] = (CTRL.key_sharp() ? (SEG_A | SEG_B | SEG_F | SEG_G) : 0) | dot;
  TM_DT[2] = (o < 0 ? SEG_G : 0) | dot;
  TM_DT[3] = (TM.encodeDigit(o < 0 ? (o * -1 ): o)) | dot;
  
}
void refresh_display()
{
  TM.setBrightness(TM_BRIGHTNESS);
  TM.setSegments(TM_DT);
}

// - - - - - - - - - - - - - - - - - - - -
// rotary
// - - - - - - - - - - - - - - - - - - - -

int rotary_1_A = 0;
int rotary_1_B = 0;
int rotary_2_A = 0;
int rotary_2_B = 0;

// OneButton class handles Debounce and detects button press
OneButton rotary_1_btn(PIN_ROTARY_1_SW, HIGH);      // Rotary Select button
OneButton rotary_2_btn(PIN_ROTARY_2_SW, HIGH);      // Rotary Select button

void rotary_click()
{
  if (CTRL.is_on())
    CTRL.off();
  else
    CTRL.on();

  update_display();
}


void rotary_long_press()
{
  clear_display();
  CTRL.reset();
  update_display();
}

void rotary_1_init()
{
  // Set the Directions of the I/O Pins
  pinMode(PIN_ROTARY_1_CLK, INPUT_PULLUP);
  pinMode(PIN_ROTARY_1_DAT, INPUT_PULLUP);
  pinMode(PIN_ROTARY_1_SW, INPUT_PULLUP);
  pinMode(PIN_ROTARY_1_GND, OUTPUT);
  pinMode(PIN_ROTARY_1_5V, OUTPUT);

  // Set the 5V and GND pins for the Rotary Encoder
  digitalWrite(PIN_ROTARY_1_GND, LOW);
  digitalWrite(PIN_ROTARY_1_5V, HIGH);

  rotary_1_A = digitalRead(PIN_ROTARY_1_CLK);

  // Define the functions for Rotary Encoder Click and Long Press
  rotary_1_btn.attachClick(&rotary_click);
  rotary_1_btn.attachLongPressStart(&rotary_long_press);
  rotary_1_btn.setPressTicks(ROTARY_PRESS_TICKS);
}

void rotary_2_init()
{
  // Set the Directions of the I/O Pins
  pinMode(PIN_ROTARY_2_CLK, INPUT_PULLUP);
  pinMode(PIN_ROTARY_2_DAT, INPUT_PULLUP);
  pinMode(PIN_ROTARY_2_SW, INPUT_PULLUP);
  pinMode(PIN_ROTARY_2_GND, OUTPUT);
  pinMode(PIN_ROTARY_2_5V, OUTPUT);

  // Set the 5V and GND pins for the Rotary Encoder
  digitalWrite(PIN_ROTARY_2_GND, LOW);
  digitalWrite(PIN_ROTARY_2_5V, HIGH);

  rotary_2_A = digitalRead(PIN_ROTARY_2_CLK);

  // Define the functions for Rotary Encoder Click and Long Press
  rotary_2_btn.attachClick(&rotary_click);
  rotary_2_btn.attachLongPressStart(&rotary_long_press);
  rotary_2_btn.setPressTicks(ROTARY_PRESS_TICKS);
}

void rotary_1_read()
{
  rotary_1_btn.tick();
  rotary_1_B = digitalRead(PIN_ROTARY_1_CLK);
  if (rotary_1_A != rotary_1_B) {
    if (digitalRead(PIN_ROTARY_1_DAT) != rotary_1_B) {
      if (CTRL.note() < CTRL.max_note)
        CTRL.increment_note();
    } else {
      if (CTRL.note() > 0)
        CTRL.decrement_note();
    }
    rotary_1_A = rotary_1_B;
    update_display();
  }
}

void rotary_2_read()
{
  rotary_2_btn.tick();
  rotary_2_B = digitalRead(PIN_ROTARY_2_CLK);
  if (rotary_2_A != rotary_2_B) {
    if (digitalRead(PIN_ROTARY_2_DAT) != rotary_2_B) {
      if (CTRL.octave() < CTRL.max_octave)
        CTRL.increment_octave();
    } else {
      if (CTRL.octave() > 0)
        CTRL.decrement_octave();
    }
    rotary_2_A = rotary_2_B;
    update_display();
  }
}

// - - - - - - - - - - - - - - - - - - - -
// MAIN
// - - - - - - - - - - - - - - - - - - - -

void setup() {
  
#ifdef DEBUG_SERIAL
  Serial.begin(9600);
#else
  MIDI.begin(MIDI_CHANNEL_OFF);
#endif

  arduino_log(HC::INFO, "midi_hold v" VERSION " preparing");
  
  CTRL.set_midi_send_callback(arduino_midi_send);
  CTRL.set_logger_callback(arduino_log);
  
  init_display();
  update_display();
  refresh_display();
  
  rotary_1_init();
  rotary_2_init();
  arduino_log(HC::INFO, "midi_hold v" VERSION " started");
}

void loop() {
  rotary_1_read();
  rotary_2_read();
  refresh_display();
}
