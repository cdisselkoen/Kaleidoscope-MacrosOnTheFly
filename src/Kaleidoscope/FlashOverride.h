#include "Kaleidoscope-LEDControl.h"

namespace kaleidoscope {

// Made this a separate helper class because it's kind of conceptually separate from MacrosOnTheFly
// Could be its own plugin, I guess
class FlashOverride {
 public:
  // temporarily flash the given key a given color; overrides the current LEDMode for that key only
  static void flashLED(byte row, byte col, cRGB crgb);

  // temporarily flash all LEDs a given color, overriding the current LEDMode
  static void flashAllLEDs(cRGB crgb);

  void begin(void);

 protected:
  static const int16_t flashLengthInLoops = 200;  // length in loopHook() calls
  static int16_t flashCounter;  // number of loopHook() calls remaining in current flash
                                // or -1 if nothing currently being flashed
  static cRGB flashColor;
  static bool flashWholeKeyboard;
  static uint8_t flashedLEDRow;  // only valid if flashWholeKeyboard is false
  static uint8_t flashedLEDCol;  // only valid if flashWholeKeyboard is false

  static void loopHook(bool postClear);

 private:
  // return control of any flashed LEDs back to the active LEDMode
  static void unFlash();
};

}
