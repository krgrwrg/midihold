// midi_hold
// (cc) by krgrWrgkmn 2022
// all rights w dupie

#include <MIDI.h>
#include <TM1637Display.h>
#include <OneButton.h>

#include "hold_controller.h"

#define VERSION "0.0.2"

// uncoment if you want to use serial for debug
// midi can't work together with serial for debug
//#define DEBUG_SERIAL

// - - - - - - - - - - - - - - - - - - - -
// global defs / vars
// - - - - - - - - - - - - - - - - - - - -

#define HC hold_controller
MIDI_CREATE_DEFAULT_INSTANCE();

hold_controller CTRL;

// display
int CLK = 7;
int DIO = 8;

TM1637Display TM(CLK, DIO);

// rotary controller
// Define the IO Pins Used
#define PIN_ROTARY_1_CLK    2   // Used for generating interrupts using CLK signal
#define PIN_ROTARY_1_DAT    3   // Used for reading DT signal
#define PIN_ROTARY_1_SW     4   // Used for the Rotary push button switch
#define PIN_ROTARY_1_5V     5   // Set to HIGH to be the 5V pin for the Rotary Encoder
#define PIN_ROTARY_1_GND    6   // Set to LOW to be the GND pin for the Rotary Encoder

#define PIN_ROTARY_2_CLK   11   // Used for generating interrupts using CLK signal
#define PIN_ROTARY_2_DAT   12   // Used for reading DT signal
//#define PIN_ROTARY_2_SW    XX   // Used for the Rotary push button switch
#define PIN_ROTARY_2_5V     9   // Set to HIGH to be the 5V pin for the Rotary Encoder
//#define PIN_ROTARY_2_GND    X   // Set to LOW to be the GND pin for the Rotary Encoder


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

// brightness A number from 0 (lowes brightness) to 7 (highest brightness)
uint8_t TM_BRIGHTNESS = 0;

// data for each digit
uint8_t TM_DT[] = {0,0,0,0,0};

void init_display()
{
  TM.clear();
  TM.setBrightness(TM_BRIGHTNESS);
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

void rotary_1_click()
{
  if (CTRL.is_on())
    CTRL.off();
  else
    CTRL.on();

  update_display();
}


void rotary_1_long_press()
{
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
  rotary_1_btn.attachClick(&rotary_1_click);
  rotary_1_btn.attachLongPressStart(&rotary_1_long_press);
  rotary_1_btn.setPressTicks(2000);
}

void rotary_2_init()
{
  // Set the Directions of the I/O Pins
  pinMode(PIN_ROTARY_2_CLK, INPUT_PULLUP);
  pinMode(PIN_ROTARY_2_DAT, INPUT_PULLUP);
//pinMode(PIN_ROTARY_2_SW, INPUT_PULLUP);
//pinMode(PIN_ROTARY_2_GND, OUTPUT);
  pinMode(PIN_ROTARY_2_5V, OUTPUT);

  // Set the 5V and GND pins for the Rotary Encoder
//digitalWrite(PIN_ROTARY_2_GND, LOW);
  digitalWrite(PIN_ROTARY_2_5V, HIGH);

  rotary_2_A = digitalRead(PIN_ROTARY_2_CLK);
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
  //CTRL.reset();
  
  init_display();
  update_display();
  refresh_display();
  
  rotary_1_init();
  rotary_2_init();
  arduino_log(HC::INFO, "midi_hold v" VERSION " started");

}

int sec=0;
void loop() {
  // put your main code here, to run repeatedly:

  
  /*
  MIDI.sendNoteOn(60, 127, 1);
  delay(1000);
  MIDI.sendNoteOff(60, 0, 1);
  delay(1000);
  
  sec++;
  dp[0] = sec;
  dp[1] = sec+1;
  dp[2] = sec+2;
  dp[3] = sec+3;
  update_display();
  */
  
  
  rotary_1_read();
  rotary_2_read();
  refresh_display();
  
}
