# Kaleidoscope-MacrosOnTheFly

![status][st:experimental] [![Build Status][travis:image]][travis:status]

 [travis:image]: https://travis-ci.org/keyboardio/Kaleidoscope-MacrosOnTheFly.svg?branch=master
 [travis:status]: https://travis-ci.org/keyboardio/Kaleidoscope-MacrosOnTheFly

 [st:stable]: https://img.shields.io/badge/stable-✔-black.svg?style=flat&colorA=44cc11&colorB=494e52
 [st:broken]: https://img.shields.io/badge/broken-X-black.svg?style=flat&colorA=e05d44&colorB=494e52
 [st:experimental]: https://img.shields.io/badge/experimental----black.svg?style=flat&colorA=dfb317&colorB=494e52

This plugin allows you to record and playback macros on-the-fly.  Do any
sequence of keystrokes once, then repeat it with the tap of a button.  Some
possible uses including typing some text over and over; invoking a complicated
shortcut several times; or (for programmers like me) making the same modification
to several different lines of code.  Furthermore, you can have up to 63
different macros stored at once, and play back any of them at any time.  The
possibilities are limited only by your imagination.

The organization and control scheme for MacrosOnTheFly is inspired by the
similar feature in Vim.  Vim users should feel right at home.  This plugin
simply takes that functionality and bakes it into your keyboard, so that it
works in any application and context wherever you go.  That said, using this
plugin does not require any Vim knowledge at all - it is fully explained below.

### Before we start: Alternatives to this plugin
MacrosOnTheFly is a powerful plugin.  However, if you already know what key
sequences you want to put on what buttons, consider using the
![Macros](https://github.com/keyboardio/Kaleidoscope-Macros)
plugin instead, which will bake them into your keymap and is more efficient than
recording and playing back on the fly.  Also, macros recorded on the fly will
be cleared whenever the keyboard loses power, whereas macros from the Macros
plugin are permanent (until you reflash your firmware, that is).

If the only reason you want to use this plugin over Macros is to because of its 63
different macro slots, consider adding a new layer to your keymap and marking
it up with macros from Macros (you can access this layer in any number of ways,
including with a ![OneShot](https://github.com/keyboardio/Kaleidoscope-OneShot)
key).

If you're excited by the ability to create custom key-sequences
on demand that are tailored to your particular situation, and work in any
application and context - this is the plugin for you!  Read on.

## Using the plugin: firmware configuration

To use the plugin, one needs to include the header, tell Kaleidoscope to `use`
the plugin, and place the `Key_MacroRec` and `Key_MacroPlay` keys on the keymap.

In general, this plugin desires to be as early as possible in the
`Kaleidoscope.use()` order, so that it can catch all keypresses before other
plugins try to handle them.  There may be exceptions.

### Setup operations in the firmware sketch (example):

```c++
#include <Kaleidoscope.h>
#include <Kaleidoscope-MacrosOnTheFly.h>

void setup (){
  Kaleidoscope.use(&MacrosOnTheFly);
  Kaleidoscope.setup();
}
```

### Keymap markup

Somewhere on the keymap, you should place the special keys `Key_MacroRec` and
`Key_MacroPlay`, which are used for macro recording and playback respectively.
Note these keys can be on any layer or on different layers - they could
even be the same key on different layers.

[Known issue: if you use a momentary layer key to access `Key_MacroRec`, the
momentary layer key may be 'stuck' after recording and/or playback, and
require another tap to 'reset' to its normal state.  To avoid this issue,
don't put `Key_MacroRec` on a layer you access with momentary layer keys.]

## Using the plugin: Plugin properties

The plugin provides the `MacrosOnTheFly` object, which has the following
public properties:

### `.colorEffects`

> If this is set to `true` (the default), the keyboard will use various
> colored LED effects to communicate what's happening.  Specifically:
> * While recording a macro, the `Key_MacroRec` key will be illuminated in
>   green, and the selected slot's key will be illuminated in white.
> * When done (successfully) recording a macro, the whole keyboard will
>   momentarily flash green.
> * If a macro-record option fails (for instance, if you exceed the
>   storage capacity), the whole keyboard will momentarily flash red.
> * When a macro playback completes, the `Key_MacroPlay` key will momentarily
>   flash green.
> * If you try to playback a macro slot which you haven't recorded
>   anything into, the `Key_MacroPlay` key will momentarily flash red.
>
> The specific colors mentioned above are the defaults, and can be
> customized using the properties below.
>
> The mentioned LED effects *should* play over top of all other LED effects,
> in particular any active LED modes.  If you find they don't, please
> let me know by opening an issue on this GitHub.
>

### `.recordColor`

> The color to use for the `Key_MacroRec` key during recording.  Default is
> `CRGB(0,255,0)`.


### `.slotColor`

> The color to use for the selected slot's key during recording. Default
> is `CRGB(255,255,255)`.

### `.successColor`

> The color to momentarily flash the keyboard after successful recording.
> Default is `CRGB(0,200,0)`.

### `.failColor`

> The color to momentarily flash the keyboard after recording failure
> (for instance, if the storage capacity is exceeded).  Default is
> `CRGB(200,0,0)`.

### `.playColor`

> The color to momentarily flash the `Key_MacroPlay` key when macro playback
> completes.  Default is `CRGB(0,255,0)`.

### `.emptyColor`

> The color to momentarily flash the `Key_MacroPlay` key if there is no
> macro recorded in the selected slot.  Default is `CRGB(255,0,0)`.

## Using the plugin: Actually using it!

After installing the firmware, you can use the `Key_MacroRec` and `Key_MacroPlay`
keys to record and play back macros on-the-fly.

MacrosOnTheFly provides 63 storage slots for macros; you can record and
play back any of them at any time.  Each of these storage slots is
represented by one of the physical keys on the keyboard, so you have a
Slot `A`, a Slot `X`, a Slot `4`, a Slot `Enter` - even a slot for each
thumb key and palm key.  The only exception is that whatever physical key
the `Key_MacroPlay` key is bound to is not a slot - but if `Key_MacroRec`
and `Key_MacroPlay` are on different physical keys, the `Key_MacroRec`
key is a slot.  Regardless, you have exactly 63 slots.

[Note: The above assumes you're using a Keyboardio Model 01 keyboard.
If you're using Kaleidoscope on a different keyboard, everything works
exactly the same, except that your number of storage slots is always
exactly one less than the number of physical keys on the keyboard.
Since the Model 01 has 64 physical keys, it has 63 storage slots.]

### Recording a macro

To start recording, tap the `Key_MacroRec` key followed by the key for the
particular slot you want to record into.  Then type whatever sequence of
keys you want to record.  Finally, to stop recording, tap the `Key_MacroRec`
key again.  For example, the sequence:

> `Key_MacroRec`, `Q`, `h`, `e`, `l`, `l`, `o`, `Key_MacroRec`

records the five-key sequence "hello" into the `Q` slot.

You can record pretty much any action into a macro: you can use modifiers,
layer switches, and even other macros - either static macros created with
![Macros](https://github.com/keyboardio/Kaleidoscope-Macros), or other OnTheFly
macros.  In any case, when you play back your recorded macro, it will repeat
exactly the same actions, as if you had tapped the same sequence of physical
keys again.

### Playing back a macro

To play back a macro, simply tap the `Key_MacroPlay` key followed by the key
for the slot you want to play.  Alternately, double-tapping `Key_MacroPlay`
will replay whichever macro you most recently played.  Continuing the
example, from above, the sequence:

> `Key_MacroPlay`, `Q`

will type the five-key sequence "hello"; and then the sequence:

> `Key_MacroPlay`, `Key_MacroPlay`

(or just `Key_MacroPlay`+`Q` again) will type the five-key sequence "hello"
again.

## Limitations

* There is a finite amount of storage available in your keyboard.  In the
event that you are recording a macro and exceed the total storage limit,
the "record" operation will stop, and (if `.colorEffects` is set to
`true`, its default), the keyboard will momentarily flash red (or the
color that `.failColor` is set to, if not the default).  This will
happen more quickly if you record very long macros and/or use a lot of
slots simultaneously.  You can free up storage space by deleting macros
you've already recorded (just record an empty macro over them, using
`Key_MacroRec`+key+`Key_MacroRec`), or reset everything by powering your
keyboard off and on, which will clear all your stored macros.

* Recorded macros remain in your keyboard until you record over them, or until
the keyboard loses power.  If you want your macros to stay in the keyboard
even after it loses power, use the
![Macros](https://github.com/keyboardio/Kaleidoscope-Macros) plugin instead.

* This plugin is a big hog of RAM, which means you can't use it simultaneously
with a complicated Kaleidoscope sketch using a lot of other plugins.
Reducing the RAM usage of this plugin is something I'm actively working on.

## Dependencies

* [Kaleidoscope-LEDControl](https://github.com/keyboardio/Kaleidoscope-LEDControl)

## Further reading

The [example][plugin:example] is a working sketch using MacrosOnTheFly.

 [plugin:example]: https://github.com/cdisselkoen/Kaleidoscope-MacrosOnTheFly/blob/master/examples/MacrosOnTheFly/MacrosOnTheFly.ino
