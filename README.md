Runs a WS2801 LED Strip with an ESP32 processor. The actual logic is
implemented in Lua and can be changed by connecting to the open WiFi
network and interacting with http://192.168.4.1/ there.

You need to adjust the strip_len constant to match the number of LED
tuples (red, green, blue) your strip has. You may also need to adjust
some of the PIN constants.

The MOSI is on pin 13 by default, while the CLOCK is on PIN 14.

Note that this requires a patched fastled library that supports the
ESP32's hardware SPI bus.


# Writing Lua scripts

Two functions are exported to Lua:

## delay(ms)

This calls the arduino delay() function.

## set_leds(pixels)

This takes a list of integers, one integer per LED, and forwards this
information to the LED strip. You need to write your own function to convert
RGB values to a single value. See the buntes_programm.lua file for an example.

# TODOs

* Export strip len to Lua
