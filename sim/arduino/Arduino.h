// Host-side stand-in for the Arduino core, just enough to compile LED_hat.ino
// on a PC. Implementations live in hal.cpp.
#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <deque>
#include <mutex>
#include <functional>

typedef bool boolean;
typedef uint8_t byte;

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define DEC 10
#define HEX 16

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
unsigned long millis();
void delay(unsigned long ms);
long random(long howbig);
long random(long howsmall, long howbig);
void randomSeed(unsigned long seed);

// Arduino's min/max are macros that accept mixed types.
#include <type_traits>
template <typename A, typename B> inline typename std::common_type<A, B>::type min(A a, B b) { return b < a ? b : a; }
template <typename A, typename B> inline typename std::common_type<A, B>::type max(A a, B b) { return a < b ? b : a; }
using std::abs;

inline bool isDigit(int c) { return c >= '0' && c <= '9'; }

/* ---- F() / String ---- */
class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper *>(s))

class String
{
public:
  String() {}
  String(const char *s) : s_(s ? s : "") {}
  String(const __FlashStringHelper *s) : s_(reinterpret_cast<const char *>(s)) {}
  explicit String(char c) : s_(1, c) {}
  explicit String(long n, int base = DEC);

  String &operator+=(char c) { s_ += c; return *this; }
  String &operator+=(const String &o) { s_ += o.s_; return *this; }
  unsigned int length() const { return s_.size(); }
  long toInt() const { return std::atol(s_.c_str()); }
  void remove(unsigned int index, unsigned int count)
  {
    if (index < s_.size()) s_.erase(index, count);
  }
  const char *c_str() const { return s_.c_str(); }

private:
  std::string s_;
};

/* ---- Serial ports ---- */
// Common base of the serial ports: an RX queue fed by the simulator and a TX
// sink that the simulator provides.
class SimSerial
{
public:
  void begin(unsigned long baud) { baud_ = baud; }
  int available();
  int read();

  size_t write(uint8_t b);
  size_t print(const String &s);
  size_t print(const char *s);
  size_t print(const __FlashStringHelper *s) { return print(reinterpret_cast<const char *>(s)); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(unsigned char n, int base = DEC) { return print((unsigned long)n, base); }
  size_t print(int n, int base = DEC) { return print((long)n, base); }
  size_t print(unsigned int n, int base = DEC) { return print((unsigned long)n, base); }
  size_t print(long n, int base = DEC);
  size_t print(unsigned long n, int base = DEC);
  size_t println() { return print("\r\n"); }
  template <typename T> size_t println(const T &v) { size_t n = print(v); return n + println(); }
  template <typename T> size_t println(const T &v, int base) { size_t n = print(v, base); return n + println(); }

  // Simulator side
  void feed(const char *data, size_t len);
  std::function<void(const char *, size_t)> sink;

private:
  std::mutex m_;
  std::deque<uint8_t> rx_;
  unsigned long baud_ = 0;
};

class HardwareSerial : public SimSerial {};
extern HardwareSerial Serial;
extern HardwareSerial Serial1;  // the HM-10 Bluetooth module

/* ---- ATmega328 Timer1 registers ---- */
#define WGM12 3
#define CS12 2
#define CS11 1
#define CS10 0
#define OCF1A 1

extern uint8_t TCCR1A, TCCR1B;
extern uint16_t TCNT1, OCR1A;

// TIFR1 is emulated in real time: reading it reports OCF1A once a compare
// match (CTC mode, 16 MHz / prescaler) has elapsed; writing 1 clears the flag.
class Tifr1Reg
{
public:
  operator uint8_t();
  Tifr1Reg &operator=(uint8_t v);
};
extern Tifr1Reg TIFR1;
