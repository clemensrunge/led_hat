// Interface between the emulated Arduino (hal.cpp + sketch) and the display (main.cpp).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sim
{
enum Port
{
  UsbSerial,  // Serial, 57600 baud
  Bluetooth,  // SoftS, 9600 baud
  PortCount
};

// Last frame passed to matrix.show(), as 0x00RRGGBB per pixel in strip order.
// Returns the total number of show() calls so far.
uint64_t latestFrame(std::vector<uint32_t> &out);

bool statusLed();  // pin 13

// Bytes typed into a UART, as received by the sketch.
void send(Port port, const std::string &data);
// Everything the sketch wrote to that UART since the last call.
std::string takeOutput(Port port);

// Starts the sketch thread.
void start();
}
