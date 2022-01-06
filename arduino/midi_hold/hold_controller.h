// hold_controller.h
// (cc) by krgrWrgkmn 2021
// all rights w dupie

#include <stdint.h>

#define HOLD_CONTROLLER_VERSION "1.0.1"

class hold_controller {
 public:
  static const char* keys[];

  typedef enum { DEBUG, INFO, WARNING, ERROR } log_levels_t;
  static const char* log_levels[];

  // limits to make all posible values as square
  // highest midi note is G8 ( 127 ) but higher tones missing
  // to avoid disfunction of controller, last octave is disabled
  static const uint8_t max_octave = 9;
  static const uint8_t max_note = 119;

  typedef void (*f_midi_send_t) (const uint8_t message[], uint8_t size);
  typedef void (*f_logger_t) (log_levels_t priority, const char * message);

 private:
  uint8_t d_active_note = 0;   // midi note which is playing just now
  uint8_t d_note = 0;          // midi note which is selected
  uint8_t d_key = 0;           // key which is selected ( C=0 )
  uint8_t d_octave = 0;        // ocatave which is selected  ( -2 octave = 0 )
  bool d_on = false;           // on-off switch

  char d_status[256];          // buffer for status

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
  bool key_sharp() const { return keys[d_key][1] != '\0'; }

  const char* key_human() const { return keys[d_key];  }
  int octave_human() const { return d_octave-2; }

  const char* get_status();
};
