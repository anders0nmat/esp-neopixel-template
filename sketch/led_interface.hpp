#pragma once

#include <vector>
#include <string>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

enum containerStatus {
	status_ok,
	status_error
};

struct NeoContainer {
	uint16_t startIndex;
	uint16_t indexLength;

	bool isCustom;
	bool isAnimated;
	bool animationMode;
	bool modeSpecifier;

	float time;
	uint16_t ledStart;
	uint16_t ledLength;
	uint16_t ledOffset;

	containerStatus status = status_ok;

	NeoContainer() : status(status_ok) {}

	NeoContainer(std::vector<uint8_t>::iterator & buf, const std::vector<uint8_t>::iterator end);

	void decodeMask(uint8_t mask); 
};

template<typename T_COLOR_FEATURE> 
class NeoHandler {
private:
	typedef typename T_COLOR_FEATURE::ColorObject T_Color;
	std::vector<NeoContainer> containerList;
	std::vector<T_Color> colorList;
	
	uint16_t ledInRange(uint16_t start, uint16_t offset, uint16_t length, uint16_t index);

	uint16_t ledInRange(NeoContainer& e, uint16_t index);

	bool inRange(uint16_t val, uint16_t start, uint16_t len);

	bool inRange(uint16_t val, NeoContainer& e);

public:
	uint16_t animatedCount = 0;

	NeoHandler() : animatedCount(0) {}

	void addColor(T_Color color);

	void addColor(std::vector<T_Color> & c);
	
	void clearColor();

	bool isAnimate();
	
	void registerAnimation(NeoPixelAnimator & animator, AnimUpdateCallback callback);
	
	void writeStatic(uint8_t * buffer, size_t size);

	void writeAnimate(int index, float progress, uint8_t * buffer, size_t size);

	void addContainer(NeoContainer container, bool resolveOverlap = true);

	void addContainer(std::vector<NeoContainer> c, bool resolveOverlap = true);

	void clearContainer();
};

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

	CmdResult b64Command(std::string b64);

	void clearTo(T_COLOR color);

	void update();

	void toggleOn();

	void toggleOff();

	void toggle(bool status);

	void toggle();
};

std::vector<uint8_t> b64_to_byte(std::string b64, std::string lastChars = "+/");
std::string byte_to_b64(std::vector<uint8_t> bytes, std::string lastChars = "+/");
