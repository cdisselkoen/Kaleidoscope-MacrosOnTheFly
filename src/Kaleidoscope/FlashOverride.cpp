#include "FlashOverride.h"

namespace kaleidoscope {

int16_t FlashOverride::flashCounter;
cRGB FlashOverride::flashColor;
bool FlashOverride::flashWholeKeyboard;
bool FlashOverride::secondLED;
uint8_t FlashOverride::flashedLEDRow;
uint8_t FlashOverride::flashedLEDCol;
uint8_t FlashOverride::secondLEDRow;
uint8_t FlashOverride::secondLEDCol;
cRGB FlashOverride::secondColor;

void FlashOverride::flashLED(byte row, byte col, cRGB crgb) {
  if(flashCounter >= 0) unFlash();
  flashCounter = flashLengthInLoops;
  flashWholeKeyboard = false;
  flashedLEDRow = row;
  flashedLEDCol = col;
  flashColor = crgb;
  secondLED = false;
}

void FlashOverride::flashSecondLED(byte row, byte col, cRGB crgb) {
  if(flashCounter < 0) return;  // invalid to call this if there's no flash taking place
  if(flashWholeKeyboard) return;  // no effect if we're flashing the whole keyboard
  secondLEDRow = row;
  secondLEDCol = col;
  secondColor = crgb;
  secondLED = true;
}

void FlashOverride::flashAllLEDs(cRGB crgb) {
  // don't need to unFlash(), we'll just override again anyway
  flashCounter = flashLengthInLoops;
  flashWholeKeyboard = true;
  flashColor = crgb;
}

void FlashOverride::begin(void) {
  Kaleidoscope.useLoopHook(loopHook);
}

void FlashOverride::loopHook(bool postClear) {
  if(!postClear) return;  // we want to go last, to override active LEDMode

  if(flashCounter == -1) return;

  if(flashCounter == 0) {
    // newly done flashing.  Restore previous LEDMode.
    unFlash();
  } else {
    // override active LEDMode with our desired color
    if(flashWholeKeyboard) ::LEDControl.set_all_leds_to(flashColor);
    else {
      ::LEDControl.setCrgbAt(flashedLEDRow, flashedLEDCol, flashColor);
      if(secondLED) ::LEDControl.setCrgbAt(secondLEDRow, secondLEDCol, secondColor);
    }
  }

  flashCounter--;
}

void FlashOverride::unFlash() {
  if(flashWholeKeyboard) ::LEDControl.refreshAll();
  else {
    ::LEDControl.refreshAt(flashedLEDRow, flashedLEDCol);
    if(secondLED) ::LEDControl.refreshAt(secondLEDRow, secondLEDCol);
  }
}

}
