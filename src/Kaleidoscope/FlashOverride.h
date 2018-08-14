#include "Kaleidoscope-LEDControl.h"

namespace kaleidoscope {

// Made this a separate helper class because it's kind of conceptually separate from MacrosOnTheFly
// Could be its own plugin, I guess
class FlashOverride {
 public:
  // temporarily flash the given key a given color; overrides the current LEDMode for that key only
  static void flashLED(byte row, byte col, cRGB crgb);

  // use this if you want to flash two keys at once
  // (call flashLED() on the first, and this on the second)
  // This adds a second key to an ongoing flash effect; it will time out at exactly the same time
  //   as the flash that's already in progress, and won't refresh any ongoing flash counter.
  // Calling this while no flash is ongoing, or while an all-LED flash is ongoing, is invalid and
  //   has no effect.
  static void flashSecondLED(byte row, byte col, cRGB crgb);

  // temporarily flash all LEDs a given color, overriding the current LEDMode
  static void flashAllLEDs(cRGB crgb);

  kaleidoscope::EventHandlerResult afterEachCycle();

 protected:
  static const int16_t flashLengthInLoops = 200;  // length in loopHook() calls
  static int16_t flashCounter;  // number of loopHook() calls remaining in current flash
                                // or -1 if nothing currently being flashed
  static cRGB flashColor;
  static bool flashWholeKeyboard;  // whether we're doing an all-LED flash
  static bool secondLED;  // if flashWholeKeyboard is false, whether we're doing a two-LED flash

  // These two are only valid if flashWholeKeyboard is false
  static uint8_t flashedLEDRow;
  static uint8_t flashedLEDCol;

  // These three are only valid if flashWholeKeyboard is false and secondLED is true
  static uint8_t secondLEDRow;
  static uint8_t secondLEDCol;
  static cRGB secondColor;

 private:
  // return control of any flashed LEDs back to the active LEDMode
  static void unFlash();
};

}
