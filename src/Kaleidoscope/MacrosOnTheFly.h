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

#pragma once

#include <Kaleidoscope.h>
#include <Kaleidoscope-Ranges.h>
#include "FlashOverride.h"

#define MACROREC kaleidoscope::ranges::KALEIDOSCOPE_SAFE_START
#define MACROPLAY kaleidoscope::ranges::KALEIDOSCOPE_SAFE_START + 1
#define Key_MacroRec  (Key) {.raw = MACROREC}
#define Key_MacroPlay (Key) {.raw = MACROPLAY}

namespace kaleidoscope {

class MacrosOnTheFly : public KaleidoscopePlugin {
 public:
  MacrosOnTheFly(void);

  void begin(void) final;

  // basically everything else, public and private, has to be static so that
  // it can be called/accessed in our eventHandlerHook and/or loopHook
  static bool colorEffects;
  static cRGB recordColor;
  static cRGB slotColor;
  static cRGB successColor;
  static cRGB failColor;
  static cRGB playColor;
  static cRGB emptyColor;

 private:
  static const uint8_t STORAGE_SIZE_IN_KEYSTROKES = 150;
  // space for this many keystrokes total, across all recorded macros
  // Obviously this could be adjusted higher, at the cost of
  //   this plugin using more space in the firmware image.
  // To be adjusted over 255, you need to increase the
  //   'macroStart', 'usedSize', and 'allocatedSize' fields in
  //   the Slot struct below to 16 bits each, and likewise
  //   some local variables in the .cpp file that hold size-
  //   related values.  (And this constant, of course.)
  // (Or some extra code could manually limit macros to 255
  //   keystrokes each, and keep 'usedSize' at 8 bits, even if
  //   'macroStart' and 'allocatedSize' still need to be 16 bits each.)
  // This could also be adjusted lower to save space, at the cost
  //   of more likely the user hits the limit

  static const uint8_t MAX_SLOTS_SIMULTANEOUSLY_IN_USE = 16;
  // allow this many slots to be simultaneously in use.
  // Adjusting this higher, again, uses more space (RAM);
  //   adjusting it lower uses less space.
  // If you want this to be as high as possible (i.e. equal to
  //   the number of physical keys - 1), check out the all-slots
  //   branch, which may have a few (both space and time) advantages
  //   for that case vs. just changing the number here.
  // Also, this can't ever be higher than 127 (currently), as some
  //   code assumes that we can cover all in-use slot indexes with a
  //   int8_t

#define UP WAS_PRESSED
#define DOWN IS_PRESSED
#define TAP (UP | DOWN)

  // One entry is storage for one "keystroke" - either an up, down, or tap event for one key
  typedef struct Entry_ {
    Key key;
    uint8_t state;  // UP, DOWN, or TAP
  } Entry;

  static Entry macroStorage[STORAGE_SIZE_IN_KEYSTROKES];

  typedef struct Slot_ {
    uint8_t row, col;  // which physical key location this Slot is associated with
                       // These fields will be invalid if allocatedSize == 0
    uint8_t macroStart;  // index in macroStorage where this Slot's macro is stored
    uint8_t usedSize;  // number of entries in macroStorage that this Slot's macro uses
                        // (0 if the Slot is currently free / contains an empty macro)
                        // Must not exceed allocatedSize.
    uint8_t allocatedSize;  // number of entries in macroStorage that this Slot is currently allocated
    uint8_t previousSlot;  // the Slot which holds the memory immediately before this Slot's.
                           // This field will be invalid if macroStart == 0, or if allocatedSize == 0.
    uint8_t nextSlot;  // the Slot which holds the memory immediately after this Slot's.
                       // This field will be invalid if macroStart + allocatedSize == STORAGE_SIZE_IN_KEYSTROKES, or if allocatedSize == 0.
  } Slot;

  static Slot slots[MAX_SLOTS_SIMULTANEOUSLY_IN_USE];

  typedef enum State_ {
    IDLE,
    PICKING_SLOT_FOR_REC,   // Key_MacroRec has been pressed, the next key chooses a slot
    PICKING_SLOT_FOR_PLAY,  // Key_MacroPlay has been pressed, the next key chooses a slot
  } State;
  static State currentState;
  static bool recording;
  static uint8_t lastPlayedSlot;

  static uint8_t recordingSlot;  // if we are currently recording, which slot index
  static int8_t findSlot(uint8_t row, uint8_t col);  // gives the index of the Slot currently associated with (row, col); or if no such Slot, then a currently unassigned Slot; or if no such Slot and no unassigned Slots, then -1

  static bool prepareForRecording(uint8_t row, uint8_t col);  // returns FALSE if there is no free space
  static bool recordKeystroke(Key key, uint8_t key_state);  // returns FALSE if there was not enough room
  static bool play(uint8_t slotnum);  // returns FALSE if the slot was empty

  static void free(uint8_t slotnum);
  static uint8_t getLargestFreeSlot();  // slot with the largest 'free' portion (allocated minus used)

  static Key eventHandlerHook(Key mapped_key, byte row, byte col, uint8_t key_state);
  static void loopHook(bool postClear);

  static void LED_record_inprogress();
  static void LED_record_slotindicator(uint8_t row, uint8_t col);
  static void LED_record_fail(uint8_t row, uint8_t col);
  static void LED_record_success(uint8_t row, uint8_t col);
  static void LED_play_success();
  static void LED_play_fail();

  // keep track of where Key_MacroRec and Key_MacroPlay are for LED purposes
  static uint8_t play_row, play_col, rec_row, rec_col;
  // keep track of where the currently-recording-slot is for LED purposes
  static uint8_t slot_row, slot_col;

  static FlashOverride flashOverride;
};

}

extern kaleidoscope::MacrosOnTheFly MacrosOnTheFly;
