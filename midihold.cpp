// midihold.cpp
// (cc) by krgrWrgkmn 2021
// all rights w dupie

#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <time.h>
#include <rtmidi/RtMidi.h>

#include "arduino/midi_hold/hold_controller.h"

using namespace std;

// - - - - - - - - - - - - - - - - - - - -
// global singletons
// - - - - - - - - - - - - - - - - - - - -
hold_controller g_ctrl;
RtMidiOut g_midiout;


// - - - - - - - - - - - - - - - - - - - -
// callbacks
// - - - - - - - - - - - - - - - - - - - -

const std::string gui_dtm()
{
  time_t     now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);

  // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
  // for more information about date/time format
  strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

  return buf;
}

void ui_midi_send(const uint8_t message[], uint8_t size)
{
  std::vector<uint8_t> m(size);
  for(uint8_t i=0; i<size; i++) {
    m[i] = message[i];
  }
  g_midiout.sendMessage(&m);
}

void ui_log(hold_controller::log_levels_t priority, const char * message)
{
  const char * p = hold_controller::log_levels[priority];
  printf("> [ %s ]%8s :: %s\n", gui_dtm().c_str(), p, message);
}


// - - - - - - - - - - - - - - - - - - - -
// user interface
// - - - - - - - - - - - - - - - - - - - -
bool parseMidiNote(uint8_t * out_note, const string & key, const string & oct)
{

  {
    stringstream ss;
    ss << "parseMidiNote: key:" << key << " oct: " << oct;
    ui_log(hold_controller::DEBUG, ss.str().c_str());
  }

  if(key == "") {
    cerr << "Key not set !" << endl;
    return false;
  }

  if(oct == "") {
    cerr << "Octave not set !" << endl;
    return false;
  }

  //parse octave
  int o;
  try {
    o = stol(oct.c_str());
  } catch (invalid_argument const& ex) {
    cerr << "Invalid octave: " << oct << endl;
    return false;
  }

  o = o+2; // shift octave number by 2 ( -2 is first octave )
  if (o < 0 || o > 10) {
    cerr << "Invalid octave: " << oct << endl;
    return false;
  }

  //parse key
  uint8_t n = 0;
  char ch = key[0];
  bool sharp = (key[1] == '#') ? true : false;

  if (ch == 'E' && sharp) {
    cerr << "Invalid key: " << key << endl;
    return false;
  }
  if (ch >= 'C' && ch <= 'G')
    n = ch - 'C' ;
  else if(ch == 'A' || ch == 'B')
    n = (ch == 'A') ? 9 : 11;
  else {
    cerr << "Invalid key: " << key << endl;
    return false;
  }

  if (sharp)
    n++;

  uint8_t n0 = n;

  if (o > 0)
    n += (o * 12);

  {
    stringstream ss;
    ss << "parseMidiNote :: key: " << key 
       << " [" << (int)n0 << "]"
       << " oct: " << oct
       << " [" << o << "]"
       << " note: [" << (int)n << "]"
    ;
    ui_log(hold_controller::DEBUG, ss.str().c_str());
  }

  *out_note = n;

  return true;
}

void doNote(const string & key, const string & oct)
{
  stringstream ss;
  ss << "doNote: key:" << key << " oct: " << oct;
  ui_log(hold_controller::DEBUG, ss.str().c_str());

  uint8_t n;
  if(parseMidiNote(&n, key, oct)){
    g_ctrl.set_note(n);
  }
}

void doNoteUp()
{
  if (!g_ctrl.increment_note())
    ui_log(hold_controller::WARNING, "doNoteUp = MAX");
}

void doNoteDown()
{
  if (!g_ctrl.decrement_note())
    ui_log(hold_controller::WARNING, "doNoteDown = MIN");
}

void doKey(uint8_t key)
{
  if (!g_ctrl.set_key(key)) {
    stringstream ss;
    ss << "doKey - invalid value: " << (int)key;
    ui_log(hold_controller::WARNING, ss.str().c_str());
  }
}


void doKeyUp()
{
  if (!g_ctrl.increment_key())
    ui_log(hold_controller::WARNING, "doKeyUp = MAX");
}

void doKeyDown()
{
  if (!g_ctrl.decrement_key())
    ui_log(hold_controller::WARNING, "doKeyDown = MIN");
}

void doOct(uint8_t oct)
{
  if (!g_ctrl.set_octave(oct)) {
    stringstream ss;
    ss << "doOct - invalid value: " << (int)oct << endl;
    ui_log(hold_controller::WARNING, ss.str().c_str());
  }
}

void doOctUp()
{
  if (!g_ctrl.increment_octave())
    ui_log(hold_controller::WARNING, "doOctUp = MAX");
}

void doOctDown()
{
  if (!g_ctrl.decrement_octave())
    ui_log(hold_controller::WARNING, "doOctDown = MIN");
}


void help()
{
  cout << "Available commands:\n"
       << "  note [A,A#,B,B#... G#] [-1,9] ( example : note A# 0)\n"
       << "  note+\n"
       << "  note-\n"
       << "  key [0-11]\n"
       << "  key+\n"
       << "  key-\n"
       << "  oct+\n"
       << "  oct-\n"
       << "  oct [0-9]\n"
       << "  on ( hold on )\n"
       << "  off ( hold off )\n"
       << "  print ( print current settings)\n"
       << "  reset ( reset all values, stop all midi notes)\n"
       << "  help\n"
       << "  exit\n"
  ;
}

int listPorts()
{
  // Check available ports.
  uint nPorts = g_midiout.getPortCount();
  if ( nPorts == 0 ) {
    cerr << "No ports available!" << endl;
    return 1;
  }
  for(uint i=0; i < nPorts; i++) {
    cout << "port[" << i << "]" << g_midiout.getPortName(i) << endl;
  }

  return 0;
}

void tokTok(const string & str, vector<string> & vect, char delimiter)
{
  stringstream ss(str);

  for (string i; ss >> i;) {
    vect.push_back(i);    
    if (ss.peek() == delimiter)
      ss.ignore();
  }
}

bool doCmd(const string & cmd)
{
  if(cmd == "")
    return false;

  vector<string> args;
  tokTok(cmd, args, ' ');

  if(cmd == "exit")
  {
    cout << "Exit program..." << endl;
    return true;
  }
  else if(cmd == "help")
  {
    help();
  }
  else if(cmd == "print")
  {
    cout << g_ctrl.get_status() << endl;
  }
  else if(cmd == "reset")
  {
    g_ctrl.reset();
  }
  else if(cmd == "on")
  {
    g_ctrl.on();
  }
  else if(cmd == "off")
  {
    g_ctrl.off();
  }
  else if(cmd == "note+")
  {
    doNoteUp();
  }
  else if(cmd == "note-")
  {
    doNoteDown();
  }
  else if(cmd == "key+")
  {
    doKeyUp();
  }
  else if(cmd == "key-")
  {
    doKeyDown();
  }
  else if(cmd == "oct+")
  {
    doOctUp();
  }
  else if(cmd == "oct-")
  {
    doOctDown();
  }
  else if(args.size() > 0 && args[0] == "note")
  {
    const string note = (args.size() > 1) ? args[1] : "";
    const string oct = (args.size() > 2) ? args[2] : "";
    doNote(note, oct);
  }
  else if(args.size() > 1 && args[0] == "key")
  {
    try {
      uint key = stoul(args[1]);
      if (key <= 0xFF) {
        doKey(key);
      } else {
        cerr << "Argument  '" << args[1] << "' is > 0xFF" << endl;
      }
    } catch (invalid_argument const& ex) {
      cerr << "Argument  '" << args[1] << "' is not uinteger" << endl;
    }
  }
  else if(args.size() > 1 && args[0] == "oct")
  {
    try {
      uint oct = stoul(args[1]);
      if (oct <= 0xFF) {
        doOct(oct);
      } else {
        cerr << "Argument  '" << args[1] << "' is > 0xFF" << endl;
      }
    } catch (invalid_argument const& ex) {
      cerr << "Argument  '" << args[1] << "' is not uinteger" << endl;
    }
  }
  else
  {
    cout << "Unknown command '" << cmd << "' Type 'help' for command list" << endl;
  }
  return false;
}

int readStdIn(uint port) {
  // Check available ports.
  uint nPorts = g_midiout.getPortCount();
  if ( port >= nPorts ) {
    cerr << "Selected port [" << port << "] nPorts[ " << nPorts << "]" << endl;
    return 1;
  }
  cout << " Open port [" << port << "] :"
       << g_midiout.getPortName( port )
       << endl;
  g_midiout.openPort( port );

  for (string line; getline(cin, line);) {
    if (doCmd(line))
      break;
  }

  return 0;

}

int main(int argc, char **argv)
{
  g_ctrl.set_midi_send_callback(ui_midi_send);
  g_ctrl.set_logger_callback(ui_log);

  cout << "starting midihold v"
       << HOLD_CONTROLLER_VERSION << endl << endl;

  try {
    if(argc == 1) {
      cout << "Please select one from available controllers and run as command:" << endl
           << "./midihold [n]" << endl
           << endl;
      return listPorts();
    } else {
      try {
        uint port = stoul(argv[1]);
        return readStdIn(port);
      } catch (invalid_argument const& ex) {
        cerr << "Argument  '" << argv[1] << "' is not integer" << endl;
        return 1;
      }
    }
  } catch (RtMidiError &error) {
    // Handle the exception here
    error.printMessage();
  }
  return 0;
}
