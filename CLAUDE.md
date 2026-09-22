# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Arduino firmware for an LED hat: a 34×7 NeoPixel matrix (238 pixels) that shows a scrolling text ticker. You configure it at runtime through a text menu over Bluetooth (SoftwareSerial) or USB serial. All of the firmware is in the single sketch `LED_hat.ino`. There are no tests; `sim/` is a desktop simulator that runs the sketch without the hardware.

## Build / flash

It targets an AVR board with a 16 MHz clock (Uno/Nano class). The code writes Timer1 registers directly (`TCCR1B`, `OCR1A`, `TIFR1`), so it won't build for non-AVR boards. It needs the **Adafruit NeoPixel** library. This machine has no Arduino toolchain installed. With `arduino-cli` it would look like this:

```
arduino-cli lib install "Adafruit NeoPixel"
arduino-cli compile --fqbn arduino:avr:uno .
arduino-cli upload  --fqbn arduino:avr:uno -p /dev/ttyUSB0 .
```

Arduino tooling requires the sketch folder to have the same name as the `.ino` file. The repo directory is `led_hat` and the sketch is `LED_hat.ino`, so you may have to rename one of them or copy the sketch into a `LED_hat/` folder before you compile.

Serial settings: USB is `Serial` at 57600 baud. Bluetooth is `SoftS` on pins 10 (RX) and 11 (TX) at 9600 baud. The NeoPixel data line is on pin 5. Pin 13 is the status LED, which stays lit while the menu is open.

## Simulator (`sim/`)

A PC build of the unmodified sketch, for testing without hardware. It uses SDL3 and Dear ImGui. ImGui is fetched by CMake; SDL3 needs `libsdl3-dev`.

```
cmake -S sim -B sim/build -G Ninja && cmake --build sim/build
sim/build/led_hat_sim                 # --arc DEG, --screenshot out.bmp (saves after 3 s, then exits)
```

- `sim/sketch.cpp` `#include`s `LED_hat.ino` after `sim/arduino/` (stub `Arduino.h`, NeoPixel, SoftwareSerial and pgmspace headers) and after `sketch_prototypes.h`. `gen_prototypes.py` generates that header at build time, because the sketch relies on the Arduino IDE's automatic forward declarations.
- `sim/hal.cpp` emulates the runtime:
  - `TIFR1` is a class that reports `OCF1A` according to real elapsed time, derived from `TCCR1B`/`OCR1A`. Writing 1 to it clears the flag, as on the AVR.
  - `matrix.show()` publishes the frame to the display.
  - Both UARTs are in-memory queues.
- The sketch runs in its own thread (`setup()`, then `loop()` forever). `sim/main.cpp` draws the latest shown frame on a CPU-projected cylinder with `SDL_RenderGeometry`, or flat with Tab. It also provides the ImGui console, with one tab per UART.
- If you add an Arduino API to the sketch, add a stub for it under `sim/arduino/` (`random()` uses the host `rand()`). On the host `int` is 32-bit, not 16-bit as on AVR, and integer division by zero crashes instead of returning garbage.

## Architecture

**Frame timing.** Timer1 runs in CTC mode with a 1024 prescaler, but no interrupt is attached. `loop()` renders one frame and then busy-waits on the `OCF1A` flag before it calls `matrix.show()`. `fp4s` means "frames per 4 seconds": `OCR1A = 62500 / fp4s - 1`, and 62500 is 4 s × 15625 Hz. While it waits, the loop polls both serial ports. Any incoming byte switches to `mode = 0` (menu), and the sketch records which port the byte came from in `menue_input_interface`.

**Modes.** `mode` 0 is the menu, 1 is the test mode (one pixel at a time cycling R, G, B), 2 is the ticker, 3 is the dancing banana, 4 is Pac-Man chasing a ghost round the hat, 5 is Space Invaders (an AI cannon against a marching formation), and 6–9 are procedural effects: fire, plasma, Game of Life on the cylinder, and snow. Sprites for 3–5 are PROGMEM string arrays (`banana_frames`, `pac_frames`, `invader_frames`, ...); `drawWrappedSprite()` draws one wrapping around the 34 columns, with colors from `spriteColor()` palettes. `menu()` blocks until the user picks "0: Start". It returns a bitmask that `loop()` passes on as `init` to the renderers: bit 0 means the message or mode changed, so reset the scroll position; bit 1 means the rainbow palette must be regenerated. The renderers hold their state in function-local `static` variables.

**Pixel addressing.** The strip is wired as a serpentine. Even rows run left to right starting at `y*34`, and odd rows run right to left. `xyToNum()` does this mapping, and `drawverticalline()` computes the jump from row to row incrementally. If you change the matrix dimensions, update `xyToNum()`'s hardcoded `switch`, not only `PixelWidth`/`PixelHeight`.

**Font.** Each glyph is a `PROGMEM` byte array of drawing commands, terminated by `0`:
- `1, x, y` sets one pixel
- `2, y, x1, x2` draws a horizontal line
- `3, x, y1, y2` draws a vertical line

Glyphs are 5 columns wide and 7 rows tall, and they are laid out on a 6-column pitch (`letterpos += 6`). `letter_table[]` sits in RAM and holds pointers into PROGMEM. The glyph bytes are read with `pgm_read_byte_near`.

**Message encoding.** `message[0..39]` holds **indices into `letter_table`**, not ASCII. `-1` means a space, and `message[40]` holds the length. `SoftSmessageInput()` first writes raw ASCII into the buffer, and menu case 3 then converts it in place:
- A–Z → 0–25
- a–z → 26–51
- 0–9 → 52–61
- `!`–`/` → 62–76
- `:`–`@` → 77–83

The default message in `setup()` is already stored as indices. If you add glyphs, keep `letter_table` order and this conversion in sync.

**Color modes** (`colormode`, 1–7): 1 is a single `color`. The rest are rainbow/individual pairs that index into `multi_color[]` by column (2/3), by row (4/5), or by non-space character (6/7). The rainbow modes (2, 4, 6) fill `multi_color` through `init_tickerColor_rainbow()` and `Wheel()`. `tickerColor()` chooses the color for each pixel as it is drawn.

**Memory.** RAM on the ATmega328 is tight: about 900 bytes of globals, plus the 714-byte NeoPixel buffer that the library allocates at runtime, which leaves roughly 430 bytes for the stack. Menu strings use the `F()` macro, and the font, sprites and `sine_table` live in PROGMEM. The procedural effects share one scratch `union effect` (fire uses 4-bit heat per pixel), because only the active mode needs it. Put new per-mode buffers in that union rather than adding globals.
