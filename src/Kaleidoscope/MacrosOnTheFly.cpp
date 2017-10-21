/* -*- mode: c++ -*-
 * Kaleidoscope-MacrosOnTheFly -- Record and play back macros on-the-fly.
 * Copyright (C) 2017  Craig Disselkoen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Kaleidoscope-MacrosOnTheFly.h>
#include <Kaleidoscope-LEDControl.h>

#ifdef ARDUINO_VIRTUAL
#define debug_print(...) printf(__VA_ARGS__)
#else
#define debug_print(...)
#endif

namespace kaleidoscope {

MacrosOnTheFly::MacrosOnTheFly(void) {
  // Initialize the 0th slot to indicate that the rest of the space is free
  Slot* slot = (Slot*)&macroStorage[0];
  slot->row = 255;
  slot->col = 255;
  slot->previousSlot = -1;  // previousSlot is unsigned, so this will give the max value the type can hold
  slot->numAllocatedKeystrokes = (STORAGE_SIZE_IN_BYTES - sizeof(Slot)) / sizeof(Entry);
  slot->numUsedKeystrokes = 0;
}

// all our (non-const) static member variables
bool MacrosOnTheFly::colorEffects = true;
cRGB MacrosOnTheFly::recordColor = CRGB(0,255,0);
cRGB MacrosOnTheFly::slotColor = CRGB(255,255,255);
cRGB MacrosOnTheFly::successColor = CRGB(0,200,0);
cRGB MacrosOnTheFly::failColor = CRGB(200,0,0);
cRGB MacrosOnTheFly::playColor = CRGB(0,255,0);
cRGB MacrosOnTheFly::emptyColor = CRGB(255,0,0);
byte MacrosOnTheFly::macroStorage[MacrosOnTheFly::STORAGE_SIZE_IN_BYTES];
MacrosOnTheFly::State MacrosOnTheFly::currentState = MacrosOnTheFly::IDLE;
bool MacrosOnTheFly::recording = false;
uint16_t MacrosOnTheFly::recordingSlot;
uint16_t MacrosOnTheFly::lastPlayedSlot = 0;
uint8_t MacrosOnTheFly::play_row;
uint8_t MacrosOnTheFly::play_col;
uint8_t MacrosOnTheFly::rec_row;
uint8_t MacrosOnTheFly::rec_col;
FlashOverride MacrosOnTheFly::flashOverride;

bool MacrosOnTheFly::prepareForRecording(const uint8_t row, const uint8_t col) {
  int16_t index = findSlot(row, col);
  if(index >= 0) {
    // this physical key already had a Slot associated with it
    // Clear out any macro that previously existed for this key so we can start anew
    free(index);
  }
  // At this point we know there is no Slot associated with this physical key
  index = newSlot(row, col);
  if(index < 0) return false;  // not enough room to create a new Slot

  recordingSlot = index;
  return true;
}

void MacrosOnTheFly::free(const uint16_t index) {
  Slot* slot = (Slot*)&macroStorage[index];
  if(index == 0) {
    // don't actually delete the Slot structure, just mark it all as extra space
    slot->row = 255;
    slot->col = 255;
    slot->numUsedKeystrokes = 0;
  } else {
    // give all this slot's space, plus the space taken up by its Slot structure itself, to previous Slot
    Slot* previousSlot = (Slot*)&macroStorage[slot->previousSlot];
    uint16_t bytesToGive = sizeof(Slot) + sizeof(Entry)*slot->numAllocatedKeystrokes;
    previousSlot->numAllocatedKeystrokes += bytesToGive / sizeof(Entry);
  }
}

int16_t MacrosOnTheFly::findSlot(const uint8_t row, const uint8_t col) {
  uint16_t index = 0;
  while(true) {
    Slot* slot = (Slot*)&macroStorage[index];
    if(slot->row == row && slot->col == col) return index;
    index += sizeof(Slot) + sizeof(Entry)*slot->numAllocatedKeystrokes;
    if(index > STORAGE_SIZE_IN_BYTES-sizeof(Slot)) return -1;
  }
}

int16_t MacrosOnTheFly::newSlot(const uint8_t row, const uint8_t col) {
  const uint16_t index = getSlotWithMostFreeSpace();
  const uint16_t freeSpace = getFreeSpace(index);  // technically getSlotWithMostFreeSpace() already computed this
  if(freeSpace < sizeof(Slot) + sizeof(Entry)) return false;  // not enough room for a 1-keystroke macro

  Slot* slot = (Slot*)&macroStorage[index];
  if(slot->row == 255 && slot->col == 255) {
    // take over this Slot entirely
    slot->row = row;
    slot->col = col;
    // numUsedKeystrokes is already 0 - this is a property of (255,255) Slots
    return index;
  } else {
    // allocate ourselves a Slot using all of this one's free space
    slot->numAllocatedKeystrokes = slot->numUsedKeystrokes;
    uint16_t newIndex = index + sizeof(Slot) + sizeof(Entry)*slot->numUsedKeystrokes;
    Slot* newSlot = (Slot*)&macroStorage[newIndex];
    newSlot->row = row;
    newSlot->col = col;
    newSlot->previousSlot = index;
    newSlot->numAllocatedKeystrokes = (freeSpace - sizeof(Slot)) / sizeof(Entry);
    newSlot->numUsedKeystrokes = 0;
    return newIndex;
  }
}

uint16_t MacrosOnTheFly::getSlotWithMostFreeSpace() {
  uint16_t winningSlot = 0;
  uint16_t winningFreeSpace = 0;  // how much free space in the winningSlot
  uint16_t index = 0;
  while(true) {
    uint16_t freeSpace = getFreeSpace(index);
    if(freeSpace > winningFreeSpace) {
      winningSlot = index;
      winningFreeSpace = freeSpace;
    }
    Slot* slot = (Slot*)&macroStorage[index];
    index += sizeof(Slot) + sizeof(Entry)*slot->numAllocatedKeystrokes;
    if(index > STORAGE_SIZE_IN_BYTES-sizeof(Slot)) return winningSlot;
  }
}

uint16_t MacrosOnTheFly::getFreeSpace(uint16_t index) {
  Slot* slot = (Slot*)&macroStorage[index];
  uint16_t freeSpace = sizeof(Entry) * (slot->numAllocatedKeystrokes - slot->numUsedKeystrokes);
  if(slot->row == 255 && slot->col == 255) freeSpace += sizeof(Slot);
  return freeSpace;
}

bool MacrosOnTheFly::recordKeystroke(const Key key, const uint8_t key_state) {
  if(!keyToggledOn(key_state) && !keyToggledOff(key_state)) {
    // we only care about toggle events. Carry on.
    return true;
  }

  Slot* slot = (Slot*)&macroStorage[recordingSlot];

  if(keyToggledOff(key_state)) {  // i.e. this is an UP
    if(slot->numUsedKeystrokes == 0) {
      // Don't record an UP as the first keystroke.
      // This applies in particular to not recording the UP event for the slot-selection key
      //   but also in general for any keys that might have been held while initiating recording
      return true;
    } else {  // at least one action already recorded
      // If this is an UP, and the last action was a DOWN for the same key, combine these into a TAP.
      // This can save a significant amount of storage for long macros that contain a lot of TAPs.
      Entry& prev_entry = slot->keystrokes[slot->numUsedKeystrokes-1];  // the most recent entry recorded
      if(prev_entry.key == key && prev_entry.state == DOWN) {
        prev_entry.state = TAP;
        return true;
      }
    }
  }

  if(slot->numUsedKeystrokes == slot->numAllocatedKeystrokes) {
    // no more room
    debug_print("MacrosOnTheFly: recordKeystroke: no room, used = allocated = %u\n", slot->numUsedKeystrokes);
    free(recordingSlot);
    return false;
  }

  Entry& entry = slot->keystrokes[slot->numUsedKeystrokes++];
  entry.key = key;
  entry.state = key_state & TAP;  // remove any other flags from the key state
  return true;
}

bool MacrosOnTheFly::play(const uint16_t index) {
  Slot* slot = (Slot*)&macroStorage[index];
  for(uint8_t i = 0; i < slot->numUsedKeystrokes; i++) {
    Entry& entry = slot->keystrokes[i];
    if(keyIsPressed(entry.state)) {
      handleKeyswitchEvent(entry.key, UNKNOWN_KEYSWITCH_LOCATION, IS_PRESSED | INJECTED);
      kaleidoscope::hid::sendKeyboardReport();
    }
    if(keyWasPressed(entry.state)) {
      handleKeyswitchEvent(entry.key, UNKNOWN_KEYSWITCH_LOCATION, WAS_PRESSED | INJECTED);
      kaleidoscope::hid::sendKeyboardReport();
    }
  }
  return (slot->numUsedKeystrokes > 0);
}

void MacrosOnTheFly::begin(void) {
  Kaleidoscope.useEventHandlerHook(eventHandlerHook);
  Kaleidoscope.useLoopHook(loopHook);
  flashOverride.begin();
}

Key MacrosOnTheFly::eventHandlerHook(Key mapped_key, byte row, byte col, uint8_t key_state) {
  /* NOTE: this function alone, and not any of its callees, is responsible for
   *   the upkeep of the variables 'currentState', 'recording', and 'lastPlayedSlot'.
   * No other function should modify them.
   */

  /* Injected keys:
   * While we're recording a macro, we don't want to record injected keys.  We'll record only
   *   the physical keys pressed, and then during playback those keys will inject the same
   *   keys (or not, as appropriate - e.g. if the macro has changed).
   *   Yes, this means that if you include "play macro C" as part of recording for macro D,
   *   later playback of D will use the *current* value of C and not its value at time of recording.
   * While we're not recording, though, we *do* want to handle injected keys, our own in
   *   particular.  This is to enable the very feature described above.  For example, if
   *   macro playback injects the MACROPLAY key, we definitely want to handle that normally.
   * One more exception is that we don't want to allow entering recording mode during
   *   playback.  It should be impossible to record a MACROREC keypress as part of a
   *   macro (if you tried, the MACROREC key would just end the macro), but just in case
   *   (or for the future, etc), we don't want to allow injected MACROREC to enter
   *   recording mode.
   * We don't want to blanket-ban injected MACROREC though - sometimes we do want to
   *   handle injected MACROREC, for instance if it is used for slot selection in a
   *   nested OnTheFly macro.
   * In summary, we want to *handle* injected keys, but not *record* them or allow them
   *   to initiate recording.
   */

  if(currentState == PICKING_SLOT_FOR_REC) {
    if(keyToggledOn(key_state)) {  // we only take action on ToggledOn events
      if(mapped_key.raw == MACROPLAY) {
        if(colorEffects) LED_record_fail(row, col);  // Trying to record into the PLAY slot is error
      } else {
        recording = prepareForRecording(row, col);
        if(!recording && colorEffects) LED_record_fail(row, col);

        // mask out this key until it is released, so that we don't accidentally include it in the
        //   recorded macro, or (if recording failed) it doesn't register as a keystroke
        KeyboardHardware.maskKey(row, col);
      }
      currentState = IDLE;
    }
    return Key_NoKey;
  }

  if(currentState == IDLE && mapped_key.raw == MACROREC) {
    if(keyToggledOn(key_state) && !(key_state & INJECTED)) {
      // we only take action on ToggledOn events; and we don't enter recording mode
      //   during playback (see notes on injected keys at the top of this function)
      rec_row = row;
      rec_col = col;
      if(recording) {
        recording = false;
        if(colorEffects) LED_record_success(row, col);
      } else {
        currentState = PICKING_SLOT_FOR_REC;
      }
    }
    return Key_NoKey;  // in any case, the key has been handled
  }

  if(recording && !(key_state & INJECTED)) {
    // Any key other than (idle) MACROREC during recording is recorded.
    // In particular, MACROPLAY is still recorded.  This means you can nest our macros,
    //   i.e. you can playback an on-the-fly macro as part of another on-the-fly macro.
    // This is a cool feature which we get for free with this ordering.
    // We also don't record injected keys - see comments at the top of this function
    recording = recordKeystroke(mapped_key, key_state);
    if(!recording && colorEffects) LED_record_fail(row, col);
    // Keys typed during recording should also be handled normally (including keys
    // controlling macro playback), so we fall through and continue handling the keys
    // as normal, through the macro-playback hooks below or else by the 'return
    // mapped_key' at the end.
  }

  if(currentState == PICKING_SLOT_FOR_PLAY) {
    if(keyToggledOn(key_state)) {  // we only take action on ToggledOn events
      currentState = IDLE;  // do this first, so keypresses injected by playing the macro get handled with currentState==IDLE
      bool success;
      if(mapped_key.raw == MACROPLAY) {
        success = play(lastPlayedSlot);
      } else {
        int16_t index = findSlot(row,col);
        success = index >= 0 && play(index);
        if(success) lastPlayedSlot = index;
        // we ensure that lastPlayedSlot always points to a valid Slot
        //   (and not, for instance, -1)
      }
      if(colorEffects) {
        if(success) LED_play_success();
        else LED_play_fail();
      }
      // mask out the key until release, so it doesn't register as a keystroke
      KeyboardHardware.maskKey(row, col);
    }
    return Key_NoKey;
  }

  if(mapped_key.raw == MACROPLAY) {
    if(keyToggledOn(key_state)) {  // we only take action on ToggledOn events
      play_row = row;
      play_col = col;
      currentState = PICKING_SLOT_FOR_PLAY;
    }
    return Key_NoKey;
  }

  return mapped_key;
}

void MacrosOnTheFly::LED_record_fail(const uint8_t row, const uint8_t col) {
  flashOverride.flashAllLEDs(failColor);
}

void MacrosOnTheFly::LED_record_success(const uint8_t row, const uint8_t col) {
  flashOverride.flashAllLEDs(successColor);
}

void MacrosOnTheFly::LED_play_success() {
  flashOverride.flashLED(play_row, play_col, playColor);
}

void MacrosOnTheFly::LED_play_fail() {
  flashOverride.flashLED(play_row, play_col, emptyColor);
}

void MacrosOnTheFly::loopHook(bool postClear) {
  if(!colorEffects) return;
  if(!postClear) return;
  debug_print("MacrosOnTheFly: currentState ");
  switch(currentState) {
    case IDLE:
      if(recording) {
        debug_print("IDLE, recording\n");
        ::LEDControl.setCrgbAt(rec_row, rec_col, recordColor);
        Slot* slot = (Slot*)&macroStorage[recordingSlot];
        ::LEDControl.setCrgbAt(slot->row, slot->col, slotColor);
      } else {
        debug_print("IDLE, not recording\n");
      }
      break;
    case PICKING_SLOT_FOR_REC:
      debug_print("PICKING_SLOT_FOR_REC\n");
      ::LEDControl.setCrgbAt(rec_row, rec_col, recordColor);
      break;
    case PICKING_SLOT_FOR_PLAY:
      debug_print("PICKING_SLOT_FOR_PLAY\n");
      break;
    default:
      debug_print("bad\n");
      break;
  }
}

}

kaleidoscope::MacrosOnTheFly MacrosOnTheFly;
