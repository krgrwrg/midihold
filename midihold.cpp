// midihold.cpp
// (cc) by krgrWrgkmn 2021
// all rights w dupie

#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <rtmidi/RtMidi.h>

using namespace std;

// - - - - - - - - - - - - - - - - - - - -
// interface
// - - - - - - - - - - - - - - - - - - - -

class hold_controller {
 public:
  static const string keys[];

  // limits to make all posible values as square
  // highest midi note is G8 ( 127 ) but higher tones missing
  // to avoid disfunction of controller, last octave is disabled
  static const unsigned int max_octave = 9;
  static const unsigned int max_note = 119;

 private:
  unsigned char d_note = 0;
  unsigned char d_key = 0;
  unsigned char d_octave = 0;
  bool d_on = false;

  bool set_key_octave(unsigned char k, unsigned char o)
  {
    if(k < 12 && o <= max_octave ) {
      d_key = k;
      d_octave = o;
      d_note = d_key + ( 12 * d_octave );
      return true;
    }
    return false;
  }

 public:
  bool set_note(unsigned char n)
  {
    if (n <= max_note ) {
      d_note = n;
      d_octave = d_note / 12;
      d_key =  d_note % 12;
      return true;
    }
    return false;
  }

  void on() { d_on = true; }
  void off() { d_on = true; }
  bool is_on() { return d_on; }

  bool set_key(unsigned char k) { return set_key_octave(k, d_octave); }
  bool set_octave(unsigned char o) { return set_key_octave(d_key, o); }

  bool increment_note() { return set_note(d_note+1); }
  bool decrement_note() { return (d_note > 0 ) ? set_note(d_note-1) : false; }
  bool increment_key() { return set_key(d_key+1); }
  bool decrement_key() { return (d_key > 0 ) ? set_key(d_key-1) : false; }
  bool increment_octave() { return set_octave(d_octave+1); }
  bool decrement_octave() { return (d_octave > 0 ) ? set_octave(d_octave-1) : false; }

  unsigned char note() const { return d_note; }
  unsigned char key() const { return d_key; }
  unsigned char octave() const { return d_octave; }

  const string key_human() const { return keys[d_key];  }
  int octave_human() const { return d_octave-2; }
};

const string hold_controller::keys[] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "B#"
};

typedef struct {
  hold_controller ctrl;
  unsigned char note_active = 0;

  void reset() {
    ctrl.set_note(0);
    ctrl.off();
    note_active = 0;
  }
} midi_mem_t;

// global variable
midi_mem_t midi_mem;

// - - - - - - - - - - - - - - - - - - - -
// midi-out implementation
// - - - - - - - - - - - - - - - - - - - -
void printControllerStatus()
{
  hold_controller *co = & midi_mem.ctrl;

  cout << "key: " << co->key_human()
       << " [" << (int)co->key() << "]"
       << " oct: " << co->octave_human()
       << " [" << (int)co->octave() << "]"
       << " note: [" << (int)co->note() << "]"
       << " is_on: [" << (int)co->is_on() << "]"
       << endl;
}

void playMidi(RtMidiOut * midiout, unsigned char n)
{
  std::cout << "playMidi : " << (int)n << endl;
  std::vector<unsigned char> message(3);
  message[0] = 144;
  message[1] = n;
  message[2] = 90;
  midiout->sendMessage( &message );
}

void stopMidi(RtMidiOut * midiout, unsigned char n)
{
  std::cout << "stopMidi : " << (int)n << endl;
  std::vector<unsigned char> message(3);
  message[0] = 128;
  message[1] = n;
  message[2] = 40;
  midiout->sendMessage( &message );
}

void resetMidi(RtMidiOut * midiout)
{
  std::cout << "resetMidi" << endl;
  midi_mem.reset();
  std::vector<unsigned char> message(3);
  message[0] = 128;
  message[2] = 40;
  for (unsigned char i=0; i<=127; i++){
    message[1] = i;
    midiout->sendMessage( &message );
  }
}

void doHoldOn(RtMidiOut * midiout)
{
  std::cout << "doHoldOn" << endl;
  midi_mem.ctrl.on();
  playMidi(midiout, midi_mem.ctrl.note());
  midi_mem.note_active = midi_mem.ctrl.note();
  printControllerStatus();
}

void doHoldOff(RtMidiOut * midiout)
{
  std::cout << "doHoldOff" << endl;
  if(midi_mem.ctrl.is_on()) {
    stopMidi(midiout, midi_mem.note_active);
    midi_mem.ctrl.off();
  }
  printControllerStatus();
}

void doMidiNote(RtMidiOut * midiout)
{

  if (midi_mem.ctrl.is_on())
  {
    playMidi(midiout, midi_mem.ctrl.note());
    stopMidi(midiout, midi_mem.note_active);

    midi_mem.note_active = midi_mem.ctrl.note();
  }

  printControllerStatus();
}

// - - - - - - - - - - - - - - - - - - - -
// user interface
// - - - - - - - - - - - - - - - - - - - -
bool parseMidiNote(unsigned char * out_note, const string & key, const string & oct)
{

  cout << "parseMidiNote: key:" << key << " oct: " << oct << endl;

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
  unsigned char n = 0;
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

  unsigned char n0 = n;

  if (o > 0)
    n += (o * 12);

  cout << "key: " << key 
       << " [" << (int)n0 << "]"
       << " oct: " << oct
       << " [" << o << "]"
       << " note: [" << (int)n << "]"
       << endl;
  
  
  *out_note = n;

  return true;
}

void doNote(RtMidiOut * midiout, const string & key, const string & oct)
{
  cout << "doNote: key:" << key << " oct: " << oct << endl;

  unsigned char n;
  if(parseMidiNote(&n, key, oct)){
    midi_mem.ctrl.set_note(n);
    doMidiNote(midiout);
  }
}

void doNoteUp(RtMidiOut * midiout)
{

  if (!midi_mem.ctrl.increment_note()) {
    cout << "doNoteUp = MAX" << endl;
    return;
  }
 
  doMidiNote(midiout);
}

void doNoteDown(RtMidiOut * midiout)
{

 if (!midi_mem.ctrl.decrement_note()) {
   cout << "doNoteDown = MIN" << endl;
   return;
 }
 
 doMidiNote(midiout);
}

void doKey(RtMidiOut * midiout, unsigned char key)
{
  if (!midi_mem.ctrl.set_key(key)) {
    cout << "doKey - invalid value: " << (int)key << endl;
    return;
  }

  doMidiNote(midiout);
}


void doKeyUp(RtMidiOut * midiout)
{

  if (!midi_mem.ctrl.increment_key()) {
     cout << "doKeyUp = MAX" << endl;
     return;
  }

  doMidiNote(midiout);
}

void doKeyDown(RtMidiOut * midiout)
{

  if (!midi_mem.ctrl.decrement_key()) {
    cout << "doKeyDown = MIN" << endl;
    return;
  }
 
  doMidiNote(midiout);
}

void doOct(RtMidiOut * midiout, unsigned char oct)
{
  if (!midi_mem.ctrl.set_octave(oct)) {
    cout << "doOct - invalid value: " << (int)oct << endl;
    return;
  }
 
  doMidiNote(midiout);
}

void doOctUp(RtMidiOut * midiout)
{

  if (!midi_mem.ctrl.increment_octave()) {
    cout << "doOctUp = MAX" << endl;
    return;
  }
 
  doMidiNote(midiout);
}

void doOctDown(RtMidiOut * midiout)
{

 if (!midi_mem.ctrl.decrement_octave()) {
   cout << "doOctDown = MIN" << endl;
   return;
 }
 
  doMidiNote(midiout);
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

  int ec = 0;
  RtMidiOut *midiout = new RtMidiOut();
  // Check available ports.
  unsigned int nPorts = midiout->getPortCount();
  if ( nPorts == 0 ) {
    cerr << "No ports available!" << endl;
    ec = 1;
    goto cleanup;
  }
  for(unsigned int i=0; i < nPorts; i++) {
    cout << "port[" << i << "]" << midiout->getPortName(i) << endl;
  }

  // Clean up
  cleanup:
    delete midiout;
    return ec;
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

bool doCmd(RtMidiOut * midiout, const string & cmd)
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
    printControllerStatus();
  }
  else if(cmd == "reset")
  {
    resetMidi(midiout);
  }
  else if(cmd == "on")
  {
    doHoldOn(midiout);
  }
  else if(cmd == "off")
  {
    doHoldOff(midiout);
  }
  else if(cmd == "note+")
  {
    doNoteUp(midiout);
  }
  else if(cmd == "note-")
  {
    doNoteDown(midiout);
  }
  else if(cmd == "key+")
  {
    doKeyUp(midiout);
  }
  else if(cmd == "key-")
  {
    doKeyDown(midiout);
  }
  else if(cmd == "oct+")
  {
    doOctUp(midiout);
  }
  else if(cmd == "oct-")
  {
    doOctDown(midiout);
  }
  else if(args.size() > 0 && args[0] == "note")
  {
    const string note = (args.size() > 1) ? args[1] : "";
    const string oct = (args.size() > 2) ? args[2] : "";
    doNote(midiout, note, oct);
  }
  else if(args.size() > 1 && args[0] == "key")
  {
    try {
      unsigned int key = stoul(args[1]);
      if (key <= 0xFF) {
        doKey(midiout, key);
      } else {
        cerr << "Argument  '" << args[1] << "' is > 0xFF" << endl;
      }
    } catch (invalid_argument const& ex) {
      cerr << "Argument  '" << args[1] << "' is not unsigned integer" << endl;
    }
  }
  else if(args.size() > 1 && args[0] == "oct")
  {
    try {
      unsigned int oct = stoul(args[1]);
      if (oct <= 0xFF) {
        doOct(midiout, oct);
      } else {
        cerr << "Argument  '" << args[1] << "' is > 0xFF" << endl;
      }
    } catch (invalid_argument const& ex) {
      cerr << "Argument  '" << args[1] << "' is not unsigned integer" << endl;
    }
  }
  else
  {
    cout << "Unknown command '" << cmd << "' Type 'help' for command list" << endl;
  }
  return false;
}

int readStdIn(unsigned int port) {
  int ec=0;
  RtMidiOut *midiout = new RtMidiOut();
  // Check available ports.
  unsigned int nPorts = midiout->getPortCount();
  if ( port >= nPorts ) {
    cerr << "Selected port [" << port << "] nPorts[ " << nPorts << "]" << endl;
    ec = 1;
    goto cleanup;
  }
  cout << " Open port [" << port << "] :"
       << midiout->getPortName( port )
       << endl;
  midiout->openPort( port );

  for (string line; getline(cin, line);) {
    if (doCmd(midiout, line))
      break;
  }


  // Clean up
  cleanup:
    delete midiout;
    return ec;

}

int main(int argc, char **argv) {
  try {
    if(argc == 1) {
      return listPorts();
    } else {
      try {
        unsigned int port = stoul(argv[1]);
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
