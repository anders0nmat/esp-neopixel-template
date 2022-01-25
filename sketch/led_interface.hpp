#pragma once

#include <vector>
#include <NeoHandler.hpp>

enum class Cmd: uint8_t {active = 0xC0, off, on, rgbw, status, rgbw_range, white, experimental = 0xCF};
enum class CmdResult: uint8_t {ok = 0xA0, status, error = 0xAF};
enum class CmdHeader: uint8_t {message_start = 0xF0, led_data_compressed = 0xDC, led_data = 0xDD};

template<typename T_COLOR_FEATURE, typename T_METHOD>
class NeoPixels {
private:
	typedef typename T_COLOR_FEATURE::ColorObject T_COLOR;
	NeoPixelBus<T_COLOR_FEATURE, T_METHOD> strip;
	NeoHandler<T_COLOR_FEATURE> handler;
	NeoPixelAnimator animator;
	bool isOn = false;

	void animationUpdate(const AnimationParam& param);
	void updateLight();
public:
	NeoPixels(uint16_t pixel_count, uint8_t pin, uint16_t max_animations = 32, uint16_t animation_timescale = NEO_DECISECONDS)
		: strip(pixel_count, pin), animator(max_animations, animation_timescale) {}

	void begin();

	CmdResult binaryCommand(std::vector<uint8_t> & msg);

	void clearTo(T_COLOR color);

	void update();

	void toggleOn();

	void toggleOff();

	void toggle(bool status);

	void toggle();
};
