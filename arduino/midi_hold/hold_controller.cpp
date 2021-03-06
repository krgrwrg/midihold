// hold_controller.cpp
// (cc) by krgrWrgkmn 2021
// all rights w dupie

#include "hold_controller.h"
#include <stdio.h>

#ifndef _countof
#define _countof(x) (sizeof(x) / sizeof (x[0]))
#endif

// - - - - - - - - - - - - - - - - - - - -
// implementation
// - - - - - - - - - - - - - - - - - - - -

const char* hold_controller::keys[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "B#"
};

const char* hold_controller::log_levels[] = {
 "DEBUG", "INFO", "WARNING", "ERROR"
};

bool hold_controller::set_key_octave(uint8_t k, uint8_t o)
{
  if(k < 12 && o <= max_octave ) {
    d_key = k;
    d_octave = o;
    d_note = d_key + ( 12 * d_octave );
    change_note();
    return true;
  }
  return false;
}

bool hold_controller::set_note(uint8_t n)
{
  if (n <= max_note ) {
    d_note = n;
    d_octave = d_note / 12;
    d_key =  d_note % 12;
    change_note();
    return true;
  }
  return false;
}

const char * hold_controller::get_status()
{
  sprintf(d_status, "key: %s [%d] oct: %d [%d] note: [%d] is_on: [%d]",
    key_human(), key(), octave_human(), octave(), note(), is_on());
  return d_status;
}

void hold_controller::play_midi(uint8_t note)
{
  if (logger_callback) {
    char buf[256];
    sprintf(buf, "play_midi: %d", note);
    logger_callback(DEBUG, buf);
  }

  uint8_t message[3];
  message[0] = 144;
  message[1] = note;
  message[2] = 127;
  if(midi_send_callback)
    midi_send_callback(message, _countof(message));
}

void hold_controller::stop_midi(uint8_t note)
{
  if (logger_callback) {
    char buf[256];
    sprintf(buf, "stop_midi: %d", note);
    logger_callback(DEBUG, buf);
  }

  uint8_t message[3];
  message[0] = 128;
  message[1] = note;
  message[2] = 127;
  if(midi_send_callback)
    midi_send_callback(message, _countof(message));
}

void hold_controller::reset_midi()
{
  if (logger_callback) {
    logger_callback(DEBUG, "reset_midi");
  }

  uint8_t message[3];
  message[0] = 128;
  message[2] = 40;
  for (uint8_t i=0; i<=127; i++){
    message[1] = i;
    if(midi_send_callback)
      midi_send_callback(message, _countof(message));
  }
}

void hold_controller::on()
{
  if(logger_callback)
    logger_callback(DEBUG, "on");

  d_on = true;
  d_active_note = d_note;
  play_midi(d_active_note);

  if(logger_callback)
    logger_callback(INFO, get_status());
}

void hold_controller::off()
{
  if(logger_callback)
    logger_callback(DEBUG, "off");

  stop_midi(d_active_note);
  d_on = false;

  if(logger_callback)
    logger_callback(INFO, get_status());
}

void hold_controller::change_note()
{
  if(logger_callback)
    logger_callback(DEBUG, "change_note");
  
  if(is_on())
  {
    //to avoid silence start playing before stopping previous note
    play_midi(d_note);
    stop_midi(d_active_note);
    d_active_note = d_note;
  }

  if(logger_callback)
    logger_callback(INFO, get_status());
}

void hold_controller::reset()
{
  if(logger_callback)
    logger_callback(DEBUG, "reset");

  off();
  set_note(0);
  d_active_note = d_note;
  reset_midi();

  if(logger_callback)
    logger_callback(DEBUG, get_status());
}
