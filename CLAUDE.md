# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Arduino firmware for an LED hat: a 37×7 NeoPixel matrix (259 pixels) that shows a scrolling text ticker. You configure it at runtime through a text menu over Bluetooth (SoftwareSerial) or USB serial. All of the firmware is in the single sketch `LED_hat.ino`. There are no tests; `sim/` is a desktop simulator that runs the sketch without the hardware.

## Build / flash

The hat runs on an **Arduino Micro** (ATmega32U4, 16 MHz, native USB). The code writes Timer1 registers directly (`TCCR1B`, `OCR1A`, `TIFR1`), so it won't build for non-AVR boards. It needs the **Adafruit NeoPixel** library. On the development machine, `arduino-cli` is installed in `~/.local/bin`, with the `arduino:avr` core and Adafruit NeoPixel. The commands are:

```
arduino-cli core install arduino:avr
arduino-cli lib install "Adafruit NeoPixel"
arduino-cli compile --fqbn arduino:avr:micro .
arduino-cli upload  --fqbn arduino:avr:micro -p /dev/ttyACM0 .
```

Arduino tooling requires the sketch folder to have the same name as the `.ino` file. The repo directory is `led_hat` and the sketch is `LED_hat.ino`, so you may have to rename one of them or copy the sketch into a `LED_hat/` folder before you compile.

On Linux the Micro shows up as `/dev/ttyACM0`. Its stable name is `/dev/serial/by-id/usb-Arduino_LLC_Arduino_Micro-if00`. The port belongs to the `dialout` group, so your user must be in that group to open it. To upload, `arduino-cli` opens the port at 1200 baud, which resets the board into its Caterina bootloader. Because of that, never open the port at 1200 baud for anything else. This board's bootloader identifies as a **Leonardo** (`2341:0036`), not a Micro (`0037`). It stays for about 7 seconds, then the sketch comes back as `2341:8037`. The upload works either way, but scripts that look for the bootloader port by USB ID have to accept `0036`. If a broken sketch stops USB from working, press the board's reset button just as the upload starts.

Serial settings: USB is `Serial`, the 32U4's native USB CDC port. The sketch calls `Serial.begin(57600)`, but the baud rate has no effect on native USB, and opening the port does not reset the board, unlike on an Uno. Bluetooth is `SoftS` on pins 10 (RX) and 11 (TX) at 9600 baud. On the 32U4, SoftwareSerial can only receive on pins 8–11 and 14–16, so keep RX on one of those. The NeoPixel data line is on pin 5. Pin 13 is the status LED (the Micro's onboard `L` LED), which stays lit while the menu is open.

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
- The sketch runs in its own thread (`setup()`, then `loop()` forever). `sim/main.cpp` draws the latest shown frame on a CPU-projected cylinder with `SDL_RenderGeometry`, or flat with Tab. It also provides the ImGui console, with one tab per UART. It takes the matrix size from the sketch's `PixelWidth`/`PixelHeight` through `SketchCols`/`SketchRows`, which `sketch.cpp` exports.
- If you add an Arduino API to the sketch, add a stub for it under `sim/arduino/` (`random()` uses the host `rand()`). On the host `int` is 32-bit, not 16-bit as on AVR, and integer division by zero crashes instead of returning garbage.

## Architecture

**Frame timing.** Timer1 runs in CTC mode with a 1024 prescaler, but no interrupt is attached. `loop()` renders one frame and then busy-waits on the `OCF1A` flag before it calls `matrix.show()`. `fp4s` means "frames per 4 seconds": `OCR1A = 62500 / fp4s - 1`, and 62500 is 4 s × 15625 Hz. While it waits, the loop polls both serial ports. Any incoming byte switches to `mode = 0` (menu), and the sketch records which port the byte came from in `menue_input_interface`.

**Modes.** `mode` 0 is the menu, 1 is the test mode (one pixel at a time cycling R, G, B), 2 is the ticker, 3 is the dancing banana, 4 is Pac-Man chasing a ghost round the hat, 5 is Space Invaders (an AI cannon against a marching formation), and 6–9 are procedural effects: fire, plasma, Game of Life on the cylinder, and snow. Sprites for 3–5 are PROGMEM string arrays (`banana_frames`, `pac_frames`, `invader_frames`, ...); `drawWrappedSprite()` draws one wrapping around the 37 columns, with colors from `spriteColor()` palettes. `menu()` blocks until the user picks "0: Start". It returns a bitmask that `loop()` passes on as `init` to the renderers: bit 0 means the message or mode changed, so reset the scroll position; bit 1 means the rainbow palette must be regenerated. The renderers hold their state in function-local `static` variables.

**Pixel addressing.** The strip is wired as a serpentine that starts at the top right. Even rows run right to left, from `(y+1)*37-1` down to `y*37`, and odd rows run left to right, starting at `y*37`. `xyToNum()` does this mapping. `drawhorizontalline()` steps through a row according to its direction, and `drawverticalline()` computes the jump from row to row incrementally, so a change to the wiring has to update all three. The simulator's `pixelIndex()` in `sim/main.cpp` is a separate model of the physical wiring and has to match. The matrix was mirrored left to right when it was upgraded to 37 columns. Before that, row 0 started at the top left. `PixelCount` is derived as `PixelWidth * PixelHeight`. If you change the height, also update `xyToNum()`'s hardcoded `switch`, which has one case per row. There are 259 pixels, more than a `uint8_t` can count, so pixel indices (`xyToNum()`'s result, loop counters over `PixelCount`) must be `uint16_t`. A `uint8_t` loop to `PixelCount` never ends and freezes the hat. To clear the matrix, use `clearMatrix()`. Column and row coordinates still fit in `uint8_t`. Per-column colors live in `multi_color[40]`, so the width can't exceed 40.

The hardware really is 37 columns. The firmware that was on the board before 2026-11-21 drove 259 LEDs (checked in its disassembly), but the repo said 34×7 until then. Don't change the dimensions to match old docs or comments.

**Font.** Each glyph is a `PROGMEM` byte array of drawing commands, terminated by `0`:
- `1, x, y` sets one pixel
- `2, y, x1, x2` draws a horizontal line
- `3, x, y1, y2` draws a vertical line

Glyphs are 5 columns wide and 7 rows tall, and they are laid out on a 6-column pitch (`letterpos += 6`). `letter_table[]` sits in RAM and holds pointers into PROGMEM. The glyph bytes are read with `pgm_read_byte_near`.

**Message encoding.** `message[0..39]` holds **indices into `letter_table`**, not ASCII. `-1` means a space, and `message[40]` holds the length. Index 0 is 'A', so test for a space with `< 0`. Testing with `< 1` treats 'A' as a space and throws off the per-character colors. Each character takes 6 columns. A message that fits around the hat (`message[40] * 6 <= PixelWidth`, so up to 6 characters at 37 columns) loops continuously over the seam between the last and first column. `ticker()` does this by calling `tickerDraw()` twice, one turn apart. A longer message scrolls in at the seam and back out of it, then starts over. `SoftSmessageInput()` first writes raw ASCII into the buffer, and menu case 3 then converts it in place:
- A–Z → 0–25
- a–z → 26–51
- 0–9 → 52–61
- `!`–`/` → 62–76
- `:`–`@` → 77–83

The default message in `setup()` is already stored as indices. If you add glyphs, keep `letter_table` order and this conversion in sync.

**Color modes** (`colormode`, 1–7): 1 is a single `color`. The rest are rainbow/individual pairs that index into `multi_color[]` by column (2/3), by row (4/5), or by non-space character (6/7). The rainbow modes (2, 4, 6) fill `multi_color` through `init_tickerColor_rainbow()` and `Wheel()`. `tickerColor()` chooses the color for each pixel as it is drawn.

**Memory.** The Micro's ATmega32U4 has 2.5 KB of RAM and 28 KB of usable flash, because the Caterina bootloader takes 4 KB of the 32 KB. An Uno has 31.5 KB of usable flash, so a sketch that fits an Uno can be too big for the Micro. The sketch uses about 23 KB of flash. RAM is tight: the Micro build has about 880 bytes of globals, including the USB core. With the 777-byte NeoPixel buffer that the library allocates at runtime, that leaves about 900 bytes for the stack. Menu strings use the `F()` macro, and the font, sprites and `sine_table` live in PROGMEM. The procedural effects share one scratch `union effect` (fire uses 4-bit heat per pixel), because only the active mode needs it. Put new per-mode buffers in that union rather than adding globals.
