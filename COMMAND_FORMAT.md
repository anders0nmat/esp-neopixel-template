# Command Format of `NeoPixels`

This is a binary format for encoding LED status, including static and animated coloring, both with individual or same colors.

At the end there are some [examples](#examples)

## Message Structure

Numbers in brackets is byte size, if not specified it defaults to 1

```c
message = 0xF0, size(2), cmd

if command = C_RGBW:

message = 0xF0, size(2), cmd, modules, data
```

- `size` : Number of bytes in message (including `0xF0`) in little-endian format
- `cmd` : Any of [`enum Cmd`](#commands) 
- `modules` : The structure: `num_modules, module_data[]`
	- `num_modules` : Element count of `module_data`
	- `module_data` : List of [modules](#modules)
- `data` : The structure : <br>`0xDC, color_count(2), lz4_data[]` or <br>`0xDD, color_count(2), raw_data[]`
	- `color_count` : Amount of colors in `raw_data` or `lz4_data`
	- `lz4_data` : LZ4 compressed data, identical to `raw_data` if uncompressed
	- `raw_data` : 32-bit-color of RGBW type

## Commands

Enum|Value|Description
:---|:---:|:---
`Cmd::active`|`0xC0`|Does nothing
`Cmd::off`|`0xC1`|Toggles LEDs off
`Cmd::on`|`0xC2`|Toggles LEDs on
`Cmd::rgbw`|`0xC3`|Sets new color and animation data,<br>additionally, toggles LEDs on
`Cmd::status`|`0xC4`|Does nothing
`Cmd::rgbw_range`|`0xC5`|Does nothing
`Cmd::white`|`0xC6`|Does nothing
`Cmd::experimental`|`0xCF`|Does nothing

## Modules

All 2-byte numbers are little-endian
```c
start_index(2), index_length(2), type_bitmask, time(4), data_start(2), data_length(2), data_offset(2)
```
- `start_index` : Index of first LED affected by this module
- `index_length` : Amount of LEDs affected by this module
- `type_bitmask` : Bitmask of [type structure](#type-structure)
- `time` : Effect duration encoded as `float`
- `data_start` : Index of first LED data to use
- `data_length` : Amount of LED data to use
- `data_offset` : Offset from `data_start`. Shifts startpoint of LED data

## Type Structure

Bit|Name|Effect
:---|:---:|:---
0|IsCustom|0 : First entry in LED data sets color, more is ignored<br>1 : Every LED gets custom color
1|IsAnimated|0 : LEDs are not animated<br>1 : LEDs are animated
2|AnimationMode|0 : Switch between given color sequences<br>1 : Rotates through LED data (IsCustom = 1 only!)
3|ModeSpecifier|0 & AnimationMode = 0 : Hard Switch<br>0 & AnimationMode = 1 : Rotate Right<br>1 & AnimationMode = 0 : Fade<br>1 & AnimationMode = 1 : Rotate Left
4|-
5|-
6|-
7|-

## Examples

```
F0 04 C1
```
Toggles the LEDs off

---

```
F0 32 00 C3  02  10 00 40 00 00 00 00 00 00 00 00 01 00 00 00 
\  Header /      \  Range  /    \  Time   / \  Data Range   /
                 \             First Module                 /
				               

50 00 03 00 01 00 00 00 00 00 00 03 00 01 00  DC 03 00  
\  Range  /    \  Time   / \  Data Range   / 
\             Second Module                /

00 FF FF 00  FF 00 00 00  00 00 FF 00
\   Cyan  /  \   Red   /  \  Blue   /
```
Switches LEDs 16 to 79 to cyan (G = 255, B = 255) and LEDs 80 to 82 to red, blue, cyan

---

```
F0 27 00 C3  01  02 00 03 00 0A 40 40 00 00 00 00 04 00 00 00 
\  Header /      \  Range  /    \  Time   / \  Data Range   /
				               
DC 03 00  FF 00 00 00  00 FF 00 00  00 00 FF 00  00 00 00 FF
          \   Red   /  \  Green  /  \  Blue   /  \  White  /
```
Fades LEDs 2 to 4 through Red, Green, Blue, White. It takes 3 seconds in total
