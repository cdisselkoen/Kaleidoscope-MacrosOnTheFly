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
  for(Slot& slot : slots) {
    slot.macroStart = 0;
    slot.usedSize = 0;
    slot.allocatedSize = 0;
  }
  // Give the 0th slot all the free space to start with.
  // This is an arbitrary choice.  It's important that all the free space is
  //   in one big block, but completely unimportant who 'owns' the block.
  slots[0].allocatedSize = STORAGE_SIZE_IN_KEYSTROKES;
}

// all our (non-const) static member variables
bool MacrosOnTheFly::colorEffects = true;
cRGB MacrosOnTheFly::recordColor = CRGB(0,255,0);
cRGB MacrosOnTheFly::slotColor = CRGB(255,255,255);
cRGB MacrosOnTheFly::successColor = CRGB(0,200,0);
cRGB MacrosOnTheFly::failColor = CRGB(200,0,0);
cRGB MacrosOnTheFly::playColor = CRGB(0,255,0);
cRGB MacrosOnTheFly::emptyColor = CRGB(255,0,0);
MacrosOnTheFly::Entry MacrosOnTheFly::macroStorage[MacrosOnTheFly::STORAGE_SIZE_IN_KEYSTROKES];
MacrosOnTheFly::Slot MacrosOnTheFly::slots[MacrosOnTheFly::NUM_SLOTS];
MacrosOnTheFly::State MacrosOnTheFly::currentState = MacrosOnTheFly::IDLE;
bool MacrosOnTheFly::recording = false;
uint8_t MacrosOnTheFly::lastPlayedSlot = 0;
uint8_t MacrosOnTheFly::slotRow;
uint8_t MacrosOnTheFly::slotCol;
uint8_t MacrosOnTheFly::play_row;
uint8_t MacrosOnTheFly::play_col;
uint8_t MacrosOnTheFly::rec_row;
uint8_t MacrosOnTheFly::rec_col;
FlashOverride MacrosOnTheFly::flashOverride;

bool MacrosOnTheFly::prepareForRecording(uint8_t row, uint8_t col) {
  const uint8_t slotnum = slotNum(row, col);
  free(slotnum);  // remove any macro that previously existed in this slot
  Slot& slot = slots[slotnum];

  const uint8_t lfSlotnum = getLargestFreeSlot();  // could be the same as slotnum
  Slot& lfSlot = slots[lfSlotnum];
  const uint8_t freeSize = lfSlot.allocatedSize - lfSlot.usedSize;
  if(freeSize == 0) return false;

  // adjust slot's macroStart, usedSize, and allocatedSize
  slot.macroStart = lfSlot.macroStart + lfSlot.usedSize;
  slot.allocatedSize = freeSize;  // give it everything lfSlot wasn't using
  // slot.usedSize is already 0 from the free() earlier

  // make slot's previousSlot and nextSlot point to the right Slot
  if(lfSlot.usedSize == 0) slot.previousSlot = lfSlot.previousSlot;  // remove lfSlot from the linked list
  else if(slotnum == lfSlotnum) {}  // keep the same previousSlot
  else slot.previousSlot = lfSlotnum;
  slot.nextSlot = lfSlot.nextSlot;  // if lfSlot.nextSlot was invalid so is ours

  // make the nextSlot and previousSlot point to us as appropriate
  if(lfSlot.macroStart + lfSlot.allocatedSize < STORAGE_SIZE_IN_KEYSTROKES) {  // then lfSlot.nextSlot is valid
    slots[slot.nextSlot].previousSlot = slotnum;
  }
  if(slot.macroStart != 0) {  // then slot.previousSlot is valid
    slots[slot.previousSlot].nextSlot = slotnum;  // means lfSlot.nextSlot = slotnum, unless we removed lfSlot from the linked list earlier
  }

  // Take the free space we just acquired, away from lfSlot
  lfSlot.allocatedSize = lfSlot.usedSize;

  slotRow = row;
  slotCol = col;
  return true;
}

// Not only sets usedSize to 0, but also (tries to) set allocatedSize to 0
void MacrosOnTheFly::free(uint8_t slotnum) {
  Slot& slot = slots[slotnum];

  if(slot.allocatedSize == 0) return;  // nothing to do; usedSize is already 0, and previousSlot/nextSlot are invalid

  slot.usedSize = 0;

  bool nextSlotIsValid = (slot.macroStart + slot.allocatedSize < STORAGE_SIZE_IN_KEYSTROKES);
  bool prevSlotIsValid = (slot.macroStart != 0);

  if(prevSlotIsValid) {
    // Give our allocated space to the slot which holds the memory right before ours
    slots[slot.previousSlot].allocatedSize += slot.allocatedSize;
    slot.allocatedSize = 0;
    slots[slot.previousSlot].nextSlot = slot.nextSlot;  // (works whether nextSlot is valid or not)
  } else {  // slot.macroStart == 0
    // we don't have a previousSlot. Try to give away our allocated space to another slot.
    for(Slot& other : slots) {
      if(other.allocatedSize == 0) {
        other.macroStart = slot.macroStart;  // i.e. 0
        other.allocatedSize = slot.allocatedSize;
        other.nextSlot = slot.nextSlot;
        slot.allocatedSize = 0;
        break;
      }
    }
    // if no other slots had allocatedSize 0, and we don't have a previousSlot to give our space to,
    // we have no choice but to potentially leak our space.  Technically it won't be leaked if the
    // caller is capable of handling the case where free() doesn't set allocatedSize to 0.
  }
  if(nextSlotIsValid && slot.allocatedSize == 0) {
    // we succeeded in giving away our allocated space, and our nextSlot is valid and needs adjusting
    slots[slot.nextSlot].previousSlot = slot.previousSlot;
  }
}

uint8_t MacrosOnTheFly::getLargestFreeSlot() {
  uint8_t largestFreeSlot = 0;
  uint8_t largestFreeSize = 0;
  for(uint8_t candidate = 0; candidate < NUM_SLOTS; candidate++) {
    Slot& slot = slots[candidate];
    uint8_t freeSize = slot.allocatedSize - slot.usedSize;
    if(freeSize > largestFreeSize) {
      largestFreeSlot = candidate;
      largestFreeSize = freeSize;
    }
  }
  return largestFreeSlot;
}

bool MacrosOnTheFly::recordKeystroke(Key key, uint8_t key_state) {
  uint8_t slotnum = slotNum(slotRow, slotCol);
  Slot& slot = slots[slotnum];
  if(slot.allocatedSize == 0) {
    // no room
    debug_print("MacrosOnTheFly: recordKeystroke: no room, allocatedSize is 0\n");
    free(slotnum);
    return false;
  }

  if(!keyToggledOn(key_state) && !keyToggledOff(key_state)) {
    // we only care about toggle events. Carry on.
    return true;
  }

  if(slot.usedSize > 0  // at least one action already recorded
      && keyToggledOff(key_state)) {
    Entry& prev_entry = macroStorage[slot.macroStart + slot.usedSize - 1];  // the most recent entry recorded
    if(prev_entry.key == key && prev_entry.state == DOWN) {
      prev_entry.state = TAP;  // If this is an UP, and the last action was a DOWN for the same key,
                                // combine these into a TAP.  This can save a significant amount of storage
                                // for long macros that contain a lot of TAPs.
      return true;
    }
  }

  if(slot.usedSize == slot.allocatedSize) {
    // no more room
    debug_print("MacrosOnTheFly: recordKeystroke: no room, usedSize = allocatedSize = %d\n", slot.usedSize);
    free(slotnum);
    return false;
  }

  Entry& entry = macroStorage[slot.macroStart + slot.usedSize++];
  entry.key = key;
  entry.state = key_state & TAP;  // remove any other flags from the key state
  return true;
}

bool MacrosOnTheFly::play(uint8_t slotnum) {
  Slot& slot = slots[slotnum];
  for(uint8_t i = 0; i < slot.usedSize; i++) {
    Entry entry = macroStorage[slot.macroStart + i];
    if(keyIsPressed(entry.state)) {
      handleKeyswitchEvent(entry.key, UNKNOWN_KEYSWITCH_LOCATION, IS_PRESSED | INJECTED);
      kaleidoscope::hid::sendKeyboardReport();
    }
    if(keyWasPressed(entry.state)) {
      handleKeyswitchEvent(entry.key, UNKNOWN_KEYSWITCH_LOCATION, WAS_PRESSED | INJECTED);
      kaleidoscope::hid::sendKeyboardReport();
    }
  }
  return (slot.usedSize > 0);
}

void MacrosOnTheFly::begin(void) {
  Kaleidoscope.useEventHandlerHook(eventHandlerHook);
  Kaleidoscope.useLoopHook(loopHook);
  flashOverride.begin();
}

Key MacrosOnTheFly::eventHandlerHook(Key mapped_key, byte row, byte col, uint8_t key_state) {
  // NOTE: this function alone, and not any of its callees, is responsible for
  // the upkeep of the variables 'currentState', 'recording', and 'lastPlayedSlot'.
  // No other function should modify them.

  if(key_state & INJECTED)
    return mapped_key;  // We don't handle injected keys (either or own or from any other source).
                        // If we're recording a macro, we record only the physical keys pressed,
                        // and then during playback
                        // those keys will inject the same keys (or not, as appropriate).

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
    if(keyToggledOn(key_state)) {  // we only take action on ToggledOn events
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

  if(recording) {
    // Any key other than (idle) MACROREC during recording is recorded.
    // In particular, MACROPLAY is still recorded.  This means you can nest our macros,
    //   i.e. you can playback an on-the-fly macro as part of another on-the-fly macro.
    // This is a cool feature which we get for free with this ordering.
    recording = recordKeystroke(mapped_key, key_state);
    if(!recording && colorEffects) LED_record_fail(row, col);
    // Keys typed during recording should also be handled normally (including keys
    // controlling macro playback), so we fall through and continue handling the keys
    // as normal, through the macro-playback hooks below or else by the 'return
    // mapped_key' at the end.
  }

  if(currentState == PICKING_SLOT_FOR_PLAY) {
    if(keyToggledOn(key_state)) {  // we only take action on ToggledOn events
      bool success;
      if(mapped_key.raw == MACROPLAY) {
        success = play(lastPlayedSlot);
      } else {
        uint8_t slotnum = slotNum(row,col);
        success = play(slotnum);
        lastPlayedSlot = slotnum;
      }
      if(colorEffects) {
        if(success) LED_play_success();
        else LED_play_fail();
      }
      currentState = IDLE;
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

void MacrosOnTheFly::LED_record_fail(uint8_t row, uint8_t col) {
  flashOverride.flashAllLEDs(failColor);
}

void MacrosOnTheFly::LED_record_success(uint8_t row, uint8_t col) {
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
        ::LEDControl.setCrgbAt(slotRow, slotCol, slotColor);
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
