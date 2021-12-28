// midihold.cpp
// (cc) by krgrWrgkmn 2021
// all rights w dupie

#include <unistd.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <time.h>
#include <rtmidi/RtMidi.h>

using namespace std;

// - - - - - - - - - - - - - - - - - - - -
// interface
// - - - - - - - - - - - - - - - - - - - -

class hold_controller {
 public:
  static const string keys[];

  typedef enum { DEBUG, INFO, WARNING, ERROR } log_levels_t;
  static const string log_levels[];

  // limits to make all posible values as square
  // highest midi note is G8 ( 127 ) but higher tones missing
  // to avoid disfunction of controller, last octave is disabled
  static const uint max_octave = 9;
  static const uint max_note = 119;

  typedef void (*f_midi_send_t) (vector<uint8_t> * message);
  typedef void (*f_logger_t) (log_levels_t priority, const char * message);

 private:
  uint8_t d_active_note = 0;   // midi note which is playing just now
  uint8_t d_note = 0;          // midi note which is selected
  uint8_t d_key = 0;           // key which is selected ( C=0 )
  uint8_t d_octave = 0;        // ocatave which is selected  ( -2 octave = 0 )
  bool d_on = false;           // on-off switch

  f_midi_send_t midi_send_callback = nullptr;
  f_logger_t    logger_callback = nullptr;

  // midi control private methods
  // - - - - - - - - - - - -
  void play_midi(uint8_t note);  // start selected note playback
  void stop_midi(uint8_t note);  // stop selected note playback
  void reset_midi();             // stop all notes on device

  // interface private methods
  // - - - - - - - - - - - -
  void change_note();
  bool set_key_octave(uint8_t k, uint8_t o);

 public:

  void set_midi_send_callback(f_midi_send_t cb) { midi_send_callback = cb; }
  void set_logger_callback(f_logger_t cb) { logger_callback = cb; }


  // interface methods
  // - - - - - - - - - - - -
  void reset();               // use only for debug purpose
  bool set_note(uint8_t n);

  void on();
  void off();
  bool is_on() const { return d_on; }

  bool set_key(uint8_t k) { return set_key_octave(k, d_octave); }
  bool set_octave(uint8_t o) { return set_key_octave(d_key, o); }

  bool increment_note() { return set_note(d_note+1); }
  bool decrement_note() { return (d_note > 0 ) ? set_note(d_note-1) : false; }
  bool increment_key() { return set_key(d_key+1); }
  bool decrement_key() { return (d_key > 0 ) ? set_key(d_key-1) : false; }
  bool increment_octave() { return set_octave(d_octave+1); }
  bool decrement_octave() { return (d_octave > 0 ) ? set_octave(d_octave-1) : false; }

  uint8_t note() const { return d_note; }
  uint8_t key() const { return d_key; }
  uint8_t octave() const { return d_octave; }

  const string key_human() const { return keys[d_key];  }
  int octave_human() const { return d_octave-2; }

  string get_status() const;
};

// - - - - - - - - - - - - - - - - - - - -
// implementation
// - - - - - - - - - - - - - - - - - - - -

const string hold_controller::keys[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "B#"
};

const string hold_controller::log_levels[] = {
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

string hold_controller::get_status() const
{
  stringstream ss;

  ss << "key: " << key_human()
     << " [" << (int)key() << "]"
     << " oct: " << octave_human()
     << " [" << (int)octave() << "]"
     << " note: [" << (int)note() << "]"
     << " is_on: [" << (int)is_on() << "]"
  ;
  
  return ss.str();
}

void hold_controller::play_midi(uint8_t note)
{
  if (logger_callback) {
    stringstream ss;
    ss << "play_midi: " << (int) note;
    logger_callback(DEBUG, ss.str().c_str());
  }

  std::vector<uint8_t> message(3);
  message[0] = 144;
  message[1] = note;
  message[2] = 90;
  if(midi_send_callback)
    midi_send_callback(&message);
}

void hold_controller::stop_midi(uint8_t note)
{
  if (logger_callback) {
    stringstream ss;
    ss << "stop_midi: " << (int) note;
    logger_callback(DEBUG, ss.str().c_str());
  }

  std::vector<uint8_t> message(3);
  message[0] = 128;
  message[1] = note;
  message[2] = 40;
  if(midi_send_callback)
    midi_send_callback(&message);
}

void hold_controller::reset_midi()
{
  if (logger_callback) {
    logger_callback(DEBUG, "reset_midi");
  }

  std::vector<uint8_t> message(3);
  message[0] = 128;
  message[2] = 40;
  for (uint8_t i=0; i<=127; i++){
    message[1] = i;
    if(midi_send_callback)
      midi_send_callback(&message);
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
    logger_callback(INFO, get_status().c_str());
}

void hold_controller::off()
{
  if(logger_callback)
    logger_callback(DEBUG, "off");

  stop_midi(d_active_note);
  d_on = false;

  if(logger_callback)
    logger_callback(INFO, get_status().c_str());
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
    logger_callback(INFO, get_status().c_str());
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
    logger_callback(DEBUG, get_status().c_str());
}

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

void ui_midi_send(vector<uint8_t> * message)
{
  g_midiout.sendMessage(message);
}

void ui_log(hold_controller::log_levels_t priority, const char * message)
{
  const string & p = hold_controller::log_levels[priority];
  printf("> [ %s ]%8s :: %s\n", gui_dtm().c_str(), p.c_str(), message);
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

  try {
    if(argc == 1) {
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
