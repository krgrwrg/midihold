#!/bin/bash
g++ -Wall -D__LINUX_ALSA__ -o \
  midihold \
  midihold.cpp \
  hold_controller.cpp \
  -lasound -lpthread -lrtmidi
