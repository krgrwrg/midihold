#!/bin/bash
g++ -Wall -D__LINUX_ALSA__ -o \
  midihold \
  midihold.cpp \
  arduino/midi_hold/hold_controller.cpp \
  -lasound -lpthread -lrtmidi
