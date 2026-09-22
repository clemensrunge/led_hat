// Compiles the unmodified sketch for the host, the way the Arduino IDE would:
// core header first, then generated prototypes, then the .ino itself.
#include "Arduino.h"
#include "sketch_prototypes.h"
#include SKETCH_FILE
