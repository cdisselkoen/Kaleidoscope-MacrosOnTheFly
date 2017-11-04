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
  static bool modsAreSlots;
  static bool colorEffects;
  static cRGB recordColor;
  static cRGB slotColor;
  static cRGB successColor;
  static cRGB failColor;
  static cRGB playColor;
  static cRGB emptyColor;

 private:
  /* STORAGE_SIZE_IN_BYTES: Number of bytes of RAM to reserve for macro storage.
   * Each slot used requires one Slot object from this, and each keystroke that
   *   is part of a macro requires one Entry object.
   * Currently this means 6 bytes per slot used, plus 3 bytes per keystroke
   *   stored across all recorded macros.
   * Obviously STORAGE_SIZE_IN_BYTES could be adjusted higher, at the cost of
   *   this plugin using more space in the firmware image.
   * To adjust it higher than sizeof(Slot) + 3*255 (so currently 771 bytes),
   *   you'll need to increase numAllocatedKeystrokes to a uint16_t in the Slot
   *   object, and either also numUsedKeystrokes to uint16_t, or insert extra
   *   logic to limit numUsedKeystrokes to 255 even when numAllocatedKeystrokes
   *   is higher.  (Some local variables in MacrosOnTheFly.cpp may also need to
   *   be increased to uint16_t.)
   * STORAGE_SIZE_IN_BYTES could also be adjusted lower to save space, at the
   *   cost of more likely the user hits the limit.
   */
  static const uint16_t STORAGE_SIZE_IN_BYTES = 300;

  /* the actual storage for macros */
  static byte macroStorage[STORAGE_SIZE_IN_BYTES];

#define UP WAS_PRESSED
#define DOWN IS_PRESSED
#define TAP (UP | DOWN)

  /* one Entry is storage for one "keystroke" - either an up, down, or tap
   *   event for one key
   */
  typedef struct Entry_ {
    Key key;
    uint8_t state;  // UP, DOWN, or TAP
  } Entry;

  /* Metadata / header for each macro stored
   * A subtle assumption made by the code is that sizeof(Slot) >= sizeof(Entry)
   *   which should always be the case, but I thought I'd document it
   */
  typedef struct Slot_ {
    /* which key this Slot is associated with, or Key_NoKey if not associated
     *   with any key.  In that case, numUsedKeystrokes must be 0.
     * This is a "mapped" key, not a physical key - see issue #1.
     */
    Key key;

    /* Index in macroStorage of the previous Slot.
     * For the first Slot in macroStorage, this will be set to a value higher
     *   than STORAGE_SIZE_IN_BYTES.
     */
    uint16_t previousSlot;

    /* Allocated size of the keystrokes[] array.  Assume <= 255 */
    uint8_t numAllocatedKeystrokes;

    /* Number of entries in keystrokes[] that this is actually using (can be 0)
     * Must not exceed numAllocatedKeystrokes. If this is less than
     *   numAllocatedKeystrokes, that indicates there is extra unused space
     *   available between this Slot and the next
     */
    uint8_t numUsedKeystrokes;

    /* Array of stored keystrokes. Size of this array is equal at all times to
     *   numAllocatedKeystrokes, but only the first numUsedKeystrokes entries
     *   contain valid data.
     */
    Entry keystrokes[0];
  } Slot;

  typedef enum State_ {
    IDLE,
    PICKING_SLOT_FOR_REC,   // Key_MacroRec has been pressed, the next key chooses a slot
    PICKING_SLOT_FOR_PLAY,  // Key_MacroPlay has been pressed, the next key chooses a slot
  } State;
  static State currentState;

  /* are we currently recording a macro */
  static bool recording;

  /* are we currently playing a macro */
  static bool playing;

  /* if recording==TRUE, the index in macroStorage of the Slot we're recording
   *   into
   * if recording==TRUE, recordingSlot is guaranteed to be a valid Slot with
   *   at least one allocated keystroke
   */
  static uint16_t recordingSlot;

  /* get the index in macroStorage of the Slot currently associated with
   *   the given key; or if no such Slot, then -1
   */
  static int16_t findSlot(Key key);

  /* allocate a new Slot associated with the given key
   * One invariant maintained by the codebase is that any given key only ever
   *   has at most one Slot associated with it.  This function will not
   *   check/enforce that, so it is your responsibility to ensure there is not
   *   already a slot for the given key before calling this
   * The newly allocated Slot is guaranteed to have:
   *   -> key set to the key you pass in
   *   -> at least one allocated keystroke
   *   -> numUsedKeystrokes set to 0
   * Returns the index in macroStorage of the new slot; or if no room to create
   *   a new Slot, then -1
   */
  static int16_t newSlot(Key key);

  /* get the index in macroStorage of the Slot with the largest 'free' portion
   *   as determined by getFreeSpace()
   */
  static uint16_t getSlotWithMostFreeSpace();

  /* index: the index in macroStorage of any Slot
   * returns the amount of free space in that slot, in bytes
   *   (i.e. allocated space for keystrokes - used space for keystrokes)
   *   with the complication that for any Slot not associated with a key (i.e.
   *   key == Key_NoKey), the Slot struct itself also counts as 'free' space
   */
  static uint16_t getFreeSpace(uint16_t index);

  /* prepare for recording into the slot associated with the given key
   * returns FALSE if there is not enough free space, TRUE otherwise
   */
  static bool prepareForRecording(Key key);

  /* Record a keystroke into 'recordingSlot'.
   * returns FALSE if there was not enough room, TRUE otherwise
   */
  static bool recordKeystroke(Key key, uint8_t key_state);

  /* index: the index in macroStorage of the Slot to play
   * returns FALSE if the slot was empty, TRUE otherwise
   */
  static bool play(uint16_t index);

  /* the index in macroStorage of the Slot that was most recently played.
   * This is guaranteed to point to a valid Slot at all times
   */
  static uint16_t lastPlayedSlot;

  /* index: the index in macroStorage of the Slot to free */
  static void free(uint16_t index);

  static Key eventHandlerHook(Key mapped_key, byte row, byte col, uint8_t key_state);
  static void loopHook(bool postClear);

  static void LED_record_inprogress();
  static void LED_record_slotindicator(uint8_t row, uint8_t col);
  static void LED_record_fail(uint8_t row, uint8_t col);
  static void LED_record_success(uint8_t row, uint8_t col);
  static void LED_play_success(uint8_t row, uint8_t col);
  static void LED_play_fail(uint8_t row, uint8_t col);

  // keep track of where Key_MacroRec, Key_MacroPlay, and recordingSlot are
  //   for LED purposes
  static uint8_t play_row, play_col, rec_row, rec_col, slot_row, slot_col;

  // Maximum number of simultaneously held keys during a dynamic macro.
  // If MAX_SIMULTANEOUS_HELD_KEYS are held, you can still tap additional keys,
  //   you just can't hold any more (they will be instantly released)
  // This means that inside dynamic macros, we only support 16-key rollover
  //   (or whatever the value of MAX_SIMULTANEOUS_HELD_KEYS), not true NKRO.
  // Increasing this number by N increases local variables' RAM usage by
  //   2*N*(playback recursion depth)
  static const uint8_t MAX_SIMULTANEOUS_HELD_KEYS = 16;
  static void addToPressedKeys(Key key, Key* pressedKeys);
  static void removeFromPressedKeys(Key key, Key* pressedKeys);
  static void pressPressedKeys(Key* pressedKeys);
  static void clearPressedKeys(Key* pressedKeys);

  static FlashOverride flashOverride;
};

}

extern kaleidoscope::MacrosOnTheFly MacrosOnTheFly;
