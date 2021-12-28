#!/bin/bash
g++ -Wall -D__LINUX_ALSA__ -o midihold midihold.cpp -lasound -lpthread -lrtmidi
