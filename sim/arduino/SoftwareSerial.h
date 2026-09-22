#pragma once
#include "Arduino.h"

// The sketch's Bluetooth port. The simulator connects the (single) instance
// to a pseudo-terminal.
class SoftwareSerial : public SimSerial
{
public:
  SoftwareSerial(uint8_t rxPin, uint8_t txPin) { instance = this; }
  static SoftwareSerial *instance;
};
