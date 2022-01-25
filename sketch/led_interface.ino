#include "led_interface.hpp"
#include <lz4.h>


//#define serial_dbg

#ifdef serial_dbg
#define _debugline(x) (x)
#else
#define _debugline(x)
#endif

template<typename T_COLOR_FEATURE, typename T_METHOD>
void NeoPixels<T_COLOR_FEATURE, T_METHOD>::begin() {
	strip.Begin();
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
CmdResult NeoPixels<T_COLOR_FEATURE, T_METHOD>::binaryCommand(std::vector<uint8_t> & msg) {
	if (msg.size() < 4) return CmdResult::error;
	if (msg[0] != static_cast<uint8_t>(CmdHeader::message_start)) return CmdResult::error;
	uint16_t msg_size = msg[1] | (msg[2] << 8);
	if (msg.size() != msg_size) return CmdResult::error;
	
	switch (static_cast<Cmd>(msg[3])) {
	case Cmd::off: {
		if (isOn) {
			isOn = false;
			updateLight();
		}
		return CmdResult::ok;
	}
	case Cmd::on: {
		if (!isOn) {
			isOn = true;
			updateLight();
		}
		return CmdResult::ok;
	}
	case Cmd::rgbw: {
		auto it = msg.begin() + 4; // 4 Bytes for Header (1), Size (2) and Command (1)
		uint8_t moduleCount = *it;
		_debugline(Serial.println(String(moduleCount) + " modules in message"));
		_debugline(Serial.println(String(msg_size) + " bytes in message"));

		std::vector<NeoContainer> containers;
		for (int i = 0; i < moduleCount; i++) {
			NeoContainer c(it, msg.end());
			if (c.status != status_ok) return CmdResult::error;
			containers.push_back(c);
		}

		std::vector<RgbwColor> colors;
		if (*it == static_cast<uint8_t>(CmdHeader::led_data)) {
			it++;
			uint16_t colCount = (*it++) | ((*it++) << 8);

			_debugline(Serial.println("Color count: " + String(colCount)));
			_debugline(Serial.println("Uncompressed Colors"));

			for (int i = 0; i < colCount; ++i) {
				colors.push_back(RgbwColor(*it++, *it++, *it++, *it++));
			}
		}
		else if (*it == static_cast<uint8_t>(CmdHeader::led_data_compressed)) {
			it++;
			uint16_t colCount = (*it++) | ((*it++) << 8);
			_debugline(Serial.println("Compressed Colors"));
			_debugline(Serial.println("Color count: " + String(colCount)));

			std::vector<uint8_t> color_buf(colCount * 4);
			const char* cbuf = reinterpret_cast<char*>(&(*it));

			int succ = LZ4_decompress_safe(cbuf, reinterpret_cast<char*>(color_buf.data()), std::distance(it, msg.end()), color_buf.size() * sizeof(char));

			_debugline(Serial.println("decompression result: " + String(succ)));
			_debugline(Serial.println("compressed stream length: " + String(std::distance(it, msg.end()))));
			_debugline(Serial.println("decompressed stream length: " + String(color_buf.size() * sizeof(char))));

			if (succ < 1) return CmdResult::error; // Any error

			for (int i = 0; i < colCount; i++)
				colors.push_back(RgbwColor(color_buf[i * 4], color_buf[i * 4 + 1], color_buf[i * 4 + 2], color_buf[i * 4 + 3]));
		}
		else {
			_debugline(Serial.println("LED data header not found"));
			return CmdResult::error;
		}
		
		handler.clearContainer();
		handler.clearColor();

		handler.addContainer(containers);
		handler.addColor(colors);

		_debugline(Serial.println("everything went fine."));
		strip.ClearTo(RgbwColor(0, 0, 0, 0));
		_debugline(Serial.println("Writing static pixels"));
		handler.writeStatic(strip.Pixels(), strip.PixelsSize());
		_debugline(Serial.println("Registering Animations. Count: " + String(handler.animatedCount)));

		handler.registerAnimation(animator, std::bind(&NeoPixels::animationUpdate, this, std::placeholders::_1));
		strip.Dirty();
		return CmdResult::ok;
	}
	default: return CmdResult::error;
	}
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
void NeoPixels<T_COLOR_FEATURE, T_METHOD>::updateLight() {
	if (isOn) {
		handler.writeStatic(strip.Pixels(), strip.PixelsSize());
		animator.Resume();
		strip.Dirty();
	}
	else {
		animator.Pause();
		strip.ClearTo(RgbwColor(0, 0, 0, 0));
	}
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
void NeoPixels<T_COLOR_FEATURE, T_METHOD>::animationUpdate(const AnimationParam& param) {
	if (param.state == AnimationState_Progress || param.state == AnimationState_Started) {
#ifdef serial_dbg
		if (param.state == AnimationState_Progress) {
			Serial.println("\nnew animation color values are written for animation " + String(param.index));
		}
		else {
			Serial.println("\nrestarted animation " + String(param.index));
		}
#endif
		handler.writeAnimate(param.index, (param.state == AnimationState_Progress ? param.progress : 0.0f), strip.Pixels(), strip.PixelsSize());
		strip.Dirty();
	}
	else if (param.state == AnimationState_Completed) {
		animator.RestartAnimation(param.index);
	}
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
void NeoPixels<T_COLOR_FEATURE, T_METHOD>::clearTo(T_COLOR color) {
	strip.ClearTo(color);
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
void NeoPixels<T_COLOR_FEATURE, T_METHOD>::update() {
	if (handler.isAnimate())
		animator.UpdateAnimations();
	strip.Show();
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
inline void NeoPixels<T_COLOR_FEATURE, T_METHOD>::toggleOn() {
	toggle(true);
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
inline void NeoPixels<T_COLOR_FEATURE, T_METHOD>::toggleOff() {
	toggle(false);
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
void NeoPixels<T_COLOR_FEATURE, T_METHOD>::toggle(bool status) {
	if (isOn != status) {
		isOn = status;
		updateLight();
	}
}

template<typename T_COLOR_FEATURE, typename T_METHOD>
inline void NeoPixels<T_COLOR_FEATURE, T_METHOD>::toggle() {
	toggle(!isOn);
}
