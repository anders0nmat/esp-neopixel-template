# ESP NeoPixel Template

This s a template project for controlling adressable LEDs with ESP8266 (and theoretically every other microcontroller). With wrapper class for controlling LEDs and website template for web access.

## Used Libraries

[NeoPixelBus by Makuna](https://github.com/Makuna/NeoPixelBus) ([LGPL-3.0 License](https://github.com/Makuna/NeoPixelBus/blob/master/COPYING))

[LZ4](https://github.com/lz4/lz4)


## HowTo led_interface

There is a class `NeoPixels` of the following structure:  

`NeoPixels<color_feature, method>(led_count, led_pin, max_animations, animation_time)`

`color_feature` : See [NeoPixelBus wiki](https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object) for more information  
`method` : See [NeoPixelBus wiki](https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object) for more information  
`led_count` : The amount of LEDs  
`led_pin` : The pin the LED Data Line is connected to, see [NeoPixelBus wiki](https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object-API) for more information  
`max_animations` : The maximum amount of animations running in parallel, defaults to 32  
`animation_time` : How many ms a duration of one is, defaults to 100ms

In `loop()` call `NeoPixels.update()`

Turning the LEDs on/off with  
`toggle()` : Switches the current state  
`toggle(bool)` : turn on/off based on `bool`  
`toggleOn()`, `toggleOff()` : Toggles on/off

Clearing all LEDs with `clearTo(Color)`. See [NeoPixelBus color](https://github.com/Makuna/NeoPixelBus/wiki/Color-objects)

For processing commands (using [this](/COMMAND_FORMAT.md) structure) use  
`binaryCommand(std::vector<uint8_t>)` : process a command from binary data  
`b64Command(std::string)` : process a command from base-64-encoded data