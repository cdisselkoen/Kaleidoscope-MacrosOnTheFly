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
to several different lines of code.  Furthermore, you can have as many different
macros stored at once as you have keys on your keyboard, and play back any of them
at any time.  The possibilities are limited only by your imagination.

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

If the only reason you want to use this plugin over Macros is because of its
large number of available macro slots, consider adding a new layer to your
keymap and marking it up with macros from Macros (you can access this layer in
any number of ways, including with a
![OneShot](https://github.com/keyboardio/Kaleidoscope-OneShot) key).

If you're excited by the ability to create custom key-sequences
on demand that are tailored to your particular situation, and work in any
application and context - this is the plugin for you!  Read on.

## Installing this plugin

Install this plugin as you would any other 3rd-party plugin.  Specifically:

1. __Find your plugin directory.__  This is wherever you normally put plugins; it
should be `$SKETCHBOOK_DIR/hardware/keyboardio/avr/libraries`, or at least
accessible from there via symlink.  (If you don't know what a symlink is, don't
worry - just use the `libraries` directory directly.)  `$SKETCHBOOK_DIR` is your
Arduino sketchbook directory, perhaps `$HOME/Arduino` or `$HOME/Documents/Arduino`.
2. __Install this plugin into your plugin directory__ using one of the below options.
* __(Option 1 - using Git)__ Clone this Git repo into your plugin directory.  This can
be done from the command line - just navigate to your plugin directory and type
`git clone https://github.com/cdisselkoen/Kaleidoscope-MacrosOnTheFly`.
* __(Option 2 - no Git or command-line required)__ Click the green "Clone or download"
button at the top right of this page, and select "Download ZIP".  Then, unzip the
folder in your plugin directory.

You're done!  It was that easy.

## Adding the plugin to your sketch

To activate the plugin, one needs to include the header, tell Kaleidoscope to `use`
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

Starting from a layout reasonably close to the default Model 01 QWERTY layout,
some suggestions for places to put these keys are:
* the `pgup` and `pgdn` keys
* the `enter` key (if like me you use Fn-Space or some other key as 'enter')
* the `tab` key (if like me you've remapped it somewhere else)
* `Fn-q` (if you like similarity to Vim)
* `Fn-;` or `Fn-'` (both empty in the default firmware)
* the `any` key
* the `prog` key
* the butterfly key

## Using the plugin

After installing the firmware, you can use the `Key_MacroRec` and `Key_MacroPlay`
keys to record and play back macros on-the-fly.

MacrosOnTheFly provides a large number of storage slots for macros; you can
record and play back any of them at any time.  Each of the keys on your
keyboard is a storage slot, so you have a Slot `A`, a Slot `X`, a Slot `4`, a
Slot `Enter` - even a slot for each thumb key and palm key.  The only exception
is that the `Key_MacroPlay` key is not a slot - but `Key_MacroRec` is a slot.
You can use any combination of these storage slots you want, at any time.

### Recording a macro

To start recording, tap the `Key_MacroRec` key followed by the key for the
particular slot you want to record into.  Then type whatever sequence of
keys you want to record.  Finally, to stop recording, tap the `Key_MacroRec`
key again.  For example, the sequence:

> `Key_MacroRec`, `q`, `h`, `e`, `l`, `l`, `o`, `Key_MacroRec`

records the five-key sequence "hello" into the `q` slot.

You can record pretty much any action into a macro: you can use modifiers,
layer switches, and even other macros - either static macros created with
![Macros](https://github.com/keyboardio/Kaleidoscope-Macros), or other OnTheFly
macros.  In any case, when you play back your recorded macro, it will repeat
exactly the same actions as you made when you recorded it.

### Playing back a macro

To play back a macro, simply tap the `Key_MacroPlay` key followed by the key
for the slot you want to play.  Alternately, double-tapping `Key_MacroPlay`
will replay whichever macro you most recently played.  Continuing the
example, from above, the sequence:

> `Key_MacroPlay`, `q`

will type the five-key sequence "hello"; and then the sequence:

> `Key_MacroPlay`, `Key_MacroPlay`

(or just `Key_MacroPlay`+`q` again) will type the five-key sequence "hello"
again.

## Plugin options

The plugin provides the `MacrosOnTheFly` object, which has the following
public properties:

### `.modsAreSlots`

> If this is set to `true`, then modifier keys (like Shift, Control, etc) and
> layer keys (both `ShiftToLayer()` and `LockLayer()` keys) will be usable
> slots in their own right - i.e. you will have a Shift slot, a Control slot,
> etc.  If this is set to `false` (the default), then modifier and layer keys
> are **not** their own slots, but instead unlock entire additional "layers"
> of slots - i.e. `k`, `Shift+k`, `Ctrl+k`, `Alt+k`, and even `Shift+Ctrl+k` or
> `Ctrl+Alt+k` etc are all different macro slots.  Likewise, you will also have
> slots for any keys on other layers of your keyboard.

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
>   flash green and the played slot will momentarily flash white.
> * If you try to playback a macro slot which you haven't recorded
>   anything into, the `Key_MacroPlay` key and the selected slot will
>   momentarily flash red.
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

> The color to use for the selected slot's key during recording or after
> successful playback. Default is `CRGB(255,255,255)`.

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

> The color to momentarily flash the `Key_MacroPlay` key and the selected
> slot's key if there is no macro recorded in the selected slot.  Default is
> `CRGB(255,0,0)`.

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

## Dependencies

* [Kaleidoscope-LEDControl](https://github.com/keyboardio/Kaleidoscope-LEDControl)

## Further reading

The [example][plugin:example] is a working sketch using MacrosOnTheFly.

 [plugin:example]: https://github.com/cdisselkoen/Kaleidoscope-MacrosOnTheFly/blob/master/examples/MacrosOnTheFly/MacrosOnTheFly.ino
