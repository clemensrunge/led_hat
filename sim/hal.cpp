// Emulated Arduino runtime: serial ports, Timer1, NeoPixel output, pins.
#include "Arduino.h"
#include "Adafruit_NeoPixel.h"
#include "sim.h"

#include <atomic>
#include <chrono>
#include <thread>

// Provided by the sketch (sketch.cpp).
void setup();
void loop();

using Clock = std::chrono::steady_clock;

static const Clock::time_point bootTime = Clock::now();
static std::atomic<bool> pin13{false};

static std::mutex frameMutex;
static std::vector<uint32_t> frame;
static uint64_t showCount = 0;

/* ---- pins / time ---- */
void pinMode(uint8_t, uint8_t) {}

void digitalWrite(uint8_t pin, uint8_t val)
{
  if (pin == 13) pin13 = val != LOW;
}

unsigned long millis()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - bootTime).count();
}

void delay(unsigned long ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

long random(long howbig) { return howbig <= 0 ? 0 : std::rand() % howbig; }
long random(long howsmall, long howbig) { return howsmall >= howbig ? howsmall : howsmall + random(howbig - howsmall); }
void randomSeed(unsigned long seed) { std::srand(seed); }

/* ---- String ---- */
String::String(long n, int base)
{
  char buf[40];
  std::snprintf(buf, sizeof buf, base == HEX ? "%lx" : "%ld", n);
  s_ = buf;
}

/* ---- Serial ---- */
HardwareSerial Serial;
HardwareSerial Serial1;

int SimSerial::available()
{
  size_t n;
  {
    std::lock_guard<std::mutex> lock(m_);
    n = rx_.size();
  }
  // The sketch polls this in tight loops; don't burn a whole core doing so.
  if (n == 0) std::this_thread::sleep_for(std::chrono::microseconds(100));
  return (int)n;
}

int SimSerial::read()
{
  std::lock_guard<std::mutex> lock(m_);
  if (rx_.empty()) return -1;
  uint8_t b = rx_.front();
  rx_.pop_front();
  return b;
}

void SimSerial::feed(const char *data, size_t len)
{
  std::lock_guard<std::mutex> lock(m_);
  rx_.insert(rx_.end(), data, data + len);
}

size_t SimSerial::write(uint8_t b)
{
  if (sink) sink(reinterpret_cast<const char *>(&b), 1);
  return 1;
}

size_t SimSerial::print(const char *s)
{
  size_t n = std::strlen(s);
  if (sink) sink(s, n);
  return n;
}

size_t SimSerial::print(const String &s) { return print(s.c_str()); }

size_t SimSerial::print(long n, int base)
{
  char buf[40];
  std::snprintf(buf, sizeof buf, base == HEX ? "%lX" : "%ld", n);
  return print(buf);
}

size_t SimSerial::print(unsigned long n, int base)
{
  char buf[40];
  std::snprintf(buf, sizeof buf, base == HEX ? "%lX" : "%lu", n);
  return print(buf);
}

/* ---- Timer1 ---- */
uint8_t TCCR1A, TCCR1B;
uint16_t TCNT1, OCR1A;
Tifr1Reg TIFR1;

static bool timerRunning = false;
static bool ocf1a = false;
static Clock::time_point nextMatch;

static Clock::duration timerPeriod()
{
  static const unsigned prescalers[8] = {0, 1, 8, 64, 256, 1024, 0, 0};
  unsigned presc = prescalers[TCCR1B & 7];
  double seconds = (OCR1A + 1.0) * presc / 16e6;
  return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
}

Tifr1Reg::operator uint8_t()
{
  auto now = Clock::now();
  unsigned presc = TCCR1B & 7;
  if (presc == 0 || presc > 5)  // stopped (external clock sources not emulated)
  {
    timerRunning = false;
  }
  else
  {
    auto period = timerPeriod();
    if (!timerRunning)
    {
      timerRunning = true;
      nextMatch = now + period;
    }
    if (now >= nextMatch)
    {
      ocf1a = true;
      while (nextMatch <= now) nextMatch += period;
    }
  }

  if (!ocf1a)  // the sketch busy-waits on this flag; nap instead of spinning
  {
    auto nap = std::chrono::duration_cast<Clock::duration>(std::chrono::milliseconds(1));
    if (timerRunning && nextMatch - now < nap) nap = nextMatch - now;
    std::this_thread::sleep_for(nap);
  }
  return ocf1a ? (1 << OCF1A) : 0;
}

Tifr1Reg &Tifr1Reg::operator=(uint8_t v)
{
  if (v & (1 << OCF1A)) ocf1a = false;  // write one to clear
  return *this;
}

/* ---- NeoPixel ---- */
void Adafruit_NeoPixel::show()
{
  std::lock_guard<std::mutex> lock(frameMutex);
  frame = pixels_;
  showCount++;
}

/* ---- simulator interface ---- */
namespace sim
{
uint64_t latestFrame(std::vector<uint32_t> &out)
{
  std::lock_guard<std::mutex> lock(frameMutex);
  out = frame;
  return showCount;
}

bool statusLed() { return pin13; }

static std::mutex outMutex;
static std::string output[PortCount];

static SimSerial &uart(Port port)
{
  return port == Bluetooth ? Serial1 : Serial;
}

void send(Port port, const std::string &data) { uart(port).feed(data.data(), data.size()); }

std::string takeOutput(Port port)
{
  std::lock_guard<std::mutex> lock(outMutex);
  std::string out;
  out.swap(output[port]);
  return out;
}

void start()
{
  for (int p = 0; p < PortCount; p++)
  {
    uart((Port)p).sink = [p](const char *d, size_t n) {
      std::lock_guard<std::mutex> lock(outMutex);
      output[p].append(d, n);
    };
  }

  std::thread([] {
    setup();
    for (;;) loop();
  }).detach();
}
}
