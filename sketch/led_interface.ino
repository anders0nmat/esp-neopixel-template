#include "led_interface.hpp"
#include <lz4.h>


//#define serial_dbg

#ifdef serial_dbg
#define _debugline(x) (x)
#else
#define _debugline(x)
#endif

NeoContainer::NeoContainer(std::vector<uint8_t>::iterator & buf, const std::vector<uint8_t>::iterator end) {
	status = status_ok;
	if (std::distance(buf, end) < 15) {
		status = status_error;
		return;
	}

	startIndex = (*buf++) | ((*buf++) << 8);
	indexLength = (*buf++) | ((*buf++) << 8); 
	decodeMask(*buf++);

	// Float read trick, direct cast only works if src address is a multiple of 4
	float f = 0.0f;
	uint8_t * fp = reinterpret_cast<uint8_t *>(&f);
	*fp++ = *buf++;
	*fp++ = *buf++;
	*fp++ = *buf++;
	*fp = *buf++;
	time = f;

	ledStart = (*buf++) | ((*buf++) << 8);
	ledLength = (*buf++) | ((*buf++) << 8);
	ledOffset = (*buf++) | ((*buf++) << 8);
}

void NeoContainer::decodeMask(uint8_t mask) {
	isCustom = mask & 1;
	isAnimated = (mask >> 1) & 1;
	animationMode = (mask >> 2) & 1;
	modeSpecifier = (mask >> 3) & 1;
}


template<typename T_COLOR_FEATURE>
uint16_t NeoHandler<T_COLOR_FEATURE>::ledInRange(uint16_t start, uint16_t offset, uint16_t length, uint16_t index) {
	return start + ((offset + index) % length);
}

template<typename T_COLOR_FEATURE> 
uint16_t NeoHandler<T_COLOR_FEATURE>::ledInRange(NeoContainer& e, uint16_t index) {
	return ledInRange(e.ledStart, e.ledOffset, e.ledLength, index);
}

template<typename T_COLOR_FEATURE> 
bool NeoHandler<T_COLOR_FEATURE>::inRange(uint16_t val, uint16_t start, uint16_t len) {
	return (start <= val) && (val <= start + len - 1);
}

template<typename T_COLOR_FEATURE> 
inline bool NeoHandler<T_COLOR_FEATURE>::inRange(uint16_t val, NeoContainer& e) {
	return inRange(val, e.startIndex, e.indexLength);
}

template<typename T_COLOR_FEATURE> 
void NeoHandler<T_COLOR_FEATURE>::addColor(typename T_COLOR_FEATURE::ColorObject color) {
	// Converts Rgbw to Grbw (or other target color scheme). Neccessary for direct writing calls
	T_Color dummy;
	T_COLOR_FEATURE::applyPixelColor(reinterpret_cast<uint8_t *>(&dummy), 0, color); 

	colorList.push_back(dummy);
}

template<typename T_COLOR_FEATURE> 
void NeoHandler<T_COLOR_FEATURE>::addColor(std::vector<typename T_COLOR_FEATURE::ColorObject> & c) {
	for (auto & e : c)
		addColor(e);
}

template<typename T_COLOR_FEATURE> 
void NeoHandler<T_COLOR_FEATURE>::clearColor() {
	colorList.clear();
}

template<typename T_COLOR_FEATURE> 
bool NeoHandler<T_COLOR_FEATURE>::isAnimate() {
	return animatedCount != 0;
}

template<typename T_COLOR_FEATURE>
void NeoHandler<T_COLOR_FEATURE>::registerAnimation(NeoPixelAnimator & animator, AnimUpdateCallback callback) {
	int animationIndex = 0;
	for (auto e : containerList) {
		if (!e.isAnimated) continue;

		uint16_t duration = static_cast<int>(e.time * (1000 / animator.getTimeScale())); // getting 100ms accuracy
		animator.StartAnimation(animationIndex++, duration, callback);
	}
}

template<typename T_COLOR_FEATURE>
void NeoHandler<T_COLOR_FEATURE>::writeStatic(uint8_t * buffer, size_t size) {
	for (auto e : containerList) {
		if (e.status != status_ok) continue;

		if (e.isAnimated) continue;

		if ((e.startIndex + e.indexLength) * 4 > size) continue;

		uint8_t * pbuf = T_COLOR_FEATURE::getPixelAddress(buffer, e.startIndex); 

		if (e.ledStart + e.ledLength > colorList.size() || e.ledOffset >= e.ledLength) continue;

		if (e.isCustom) {
			if (e.ledLength != e.indexLength) continue;

			if (e.ledOffset > 0) {
				T_COLOR_FEATURE::movePixelsInc(pbuf, reinterpret_cast<uint8_t *>(&colorList[e.ledStart + e.ledOffset]), e.indexLength - e.ledOffset);
				T_COLOR_FEATURE::movePixelsInc(pbuf + (e.indexLength - e.ledOffset) * T_COLOR_FEATURE::PixelSize, reinterpret_cast<uint8_t *>(&colorList[e.ledStart + e.ledOffset]), e.ledOffset);
			}
			else {
				T_COLOR_FEATURE::movePixelsInc(pbuf, reinterpret_cast<uint8_t *>(&colorList[e.ledStart]), e.indexLength);
			}
		}
		else {
			T_COLOR_FEATURE::replicatePixel(pbuf, reinterpret_cast<uint8_t *>(&colorList[e.ledStart]), e.indexLength);
		}
	}
}

template<typename T_COLOR_FEATURE>
void NeoHandler<T_COLOR_FEATURE>::writeAnimate(int index, float progress, uint8_t * buffer, size_t size) {
	for (auto e : containerList) {
		if (e.isAnimated) --index;

		if (index > -1) continue;

		if (e.status != status_ok) continue;

		if ((e.startIndex + e.indexLength) * 4 > size) {
			e.status = status_error;
			continue;
		}

		uint8_t * pbuf = T_COLOR_FEATURE::getPixelAddress(buffer, e.startIndex);

		if (e.ledStart + e.ledLength > colorList.size() || e.ledOffset >= e.ledLength) {
			e.status = status_error;
			continue;
		}

		if (e.isCustom) {
			if (e.animationMode) { // Rotate LEDs
				int ledShift = static_cast<int>(e.ledLength * progress);
				int newOffset = e.ledOffset;
				if (e.modeSpecifier) { // Rotate Left
					newOffset += ledShift;
				}
				else { // Rotate Right
					newOffset -= ledShift;
					newOffset += e.ledLength;
				}
				newOffset %= e.ledLength;

				T_Color ledColors[e.indexLength];
				T_Color * pleft = &colorList[ledInRange(e, newOffset)];
				T_Color * pstart = &colorList[e.ledStart];
				T_Color * pend = &colorList[e.ledStart + e.ledLength];
				T_Color * lArr = ledColors;
				T_Color * lEnd = lArr + e.indexLength;
				while (lArr != lEnd) {
					if (pleft == pend) pleft = pstart;
					*lArr++ = *pleft++;
				}

				T_COLOR_FEATURE::movePixelsInc(pbuf, reinterpret_cast<uint8_t *>(ledColors), e.indexLength);
			}
			else { // Switch LEDs
				if (e.ledLength % e.indexLength != 0) {
					e.status = status_error;
					continue;
				}

				if (e.modeSpecifier) { // Fade
					float sequenceProgress = ((e.ledLength / e.indexLength) - 1) * progress;
					int sequenceIndex = static_cast<int>(sequenceProgress);

					T_Color ledColors[e.indexLength];
					T_Color * pleft = &colorList[ledInRange(e, e.indexLength * sequenceIndex)];
					T_Color * pright = &colorList[ledInRange(e, e.indexLength * (progress >= 1.0f ? sequenceIndex : sequenceIndex + 1))];
					T_Color * pstart = &colorList[e.ledStart];
					T_Color * pend = &colorList[e.ledStart + e.ledLength];

					if (pleft != pright) {
						float colorProgress = sequenceProgress - sequenceIndex;
						T_Color * lArr = ledColors;
						T_Color * lEnd = lArr + e.indexLength;
						while (lArr != lEnd) {
							if (pleft == pend) pleft = pstart;
							if (pright == pend) pright = pstart;

							*lArr++ = T_Color::LinearBlend(
								*pleft++, *pright++, colorProgress);
						}
					}
					else { // Hard Switch
						T_Color * lArr = ledColors;
						T_Color * lEnd = lArr + e.indexLength;
						while (lArr != lEnd) {
							if (pleft == pend) pleft = pstart;
							*lArr++ = *pleft++;
						}
					}

					T_COLOR_FEATURE::movePixelsInc(pbuf, reinterpret_cast<uint8_t *>(ledColors), e.indexLength);
				}
				else { // Hard Switch
					int sequenceIndex = static_cast<int>((e.ledLength / e.indexLength) * progress);
					if (progress >= 1.0f) --sequenceIndex;

					T_Color ledColors[e.indexLength];
					T_Color * pleft = &colorList[ledInRange(e, e.indexLength * sequenceIndex)];
					T_Color * pstart = &colorList[e.ledStart];
					T_Color * pend = &colorList[e.ledStart + e.ledLength];
					T_Color * lArr = ledColors;
					T_Color * lEnd = lArr + e.indexLength;
					while (lArr != lEnd) {
						if (pleft == pend) pleft = pstart;
						*lArr++ = *pleft++;
					}

					T_COLOR_FEATURE::movePixelsInc(pbuf, reinterpret_cast<uint8_t *>(ledColors), e.indexLength);
				}
			}
		}
		else {
			if (e.modeSpecifier) { // Fade
				float ledProgress = (e.ledLength - 1) * progress;
				int leftLed = ledInRange(e, (progress >= 1.0f ? e.ledLength - 1 : static_cast<int>(ledProgress))); 
				int rightLed = (progress >= 1.0f ? leftLed : leftLed + 1) % e.ledLength;

				T_Color currCol = T_Color::LinearBlend(
					colorList[e.ledStart + leftLed], colorList[e.ledStart + rightLed], ledProgress - static_cast<int>(ledProgress));
				T_COLOR_FEATURE::replicatePixel(pbuf, reinterpret_cast<uint8_t *>(&currCol), e.indexLength);
			}
			else { // Hard switch
				int ledIndex = ledInRange(e, (progress >= 1.0f ? e.ledLength - 1 : static_cast<int>(e.ledLength * progress)));
				T_COLOR_FEATURE::replicatePixel(pbuf, reinterpret_cast<uint8_t *>(&colorList[ledIndex]), e.indexLength);
			}
		}

		return;
	}
}

template<typename T_COLOR_FEATURE>
void NeoHandler<T_COLOR_FEATURE>::addContainer(NeoContainer container, bool resolveOverlap) {
	if (resolveOverlap) {
		for (size_t i = containerList.size(); i >= 0; i--) {
			auto & e = containerList[i];
			int32_t start_diff = container.startIndex - e.startIndex;
			int32_t len_diff = container.indexLength - e.indexLength;
			bool startInNew = inRange(e.startIndex, container), endInNew = inRange(e.startIndex + e.indexLength - 1, container);
			bool startInOld = inRange(container.startIndex, e), endInOld = inRange(container.startIndex + container.indexLength - 1, e);
			if (startInNew && endInNew) {
				containerList.erase(containerList.begin() + i);
			}
			else if (endInNew && startInOld) {
				e.indexLength = container.startIndex - e.startIndex;
			}
			else if (startInNew && endInOld) {
				uint16_t oldStart = e.startIndex;
				e.indexLength = (e.startIndex + e.indexLength - 1) - (container.startIndex + container.indexLength - 1);
				e.startIndex = container.startIndex + container.indexLength;
				e.ledOffset += e.startIndex - oldStart;
			}
			else if (startInOld && endInOld) {
				NeoContainer e_split(e);
				e.indexLength = container.startIndex - e.startIndex;

				e_split.indexLength = (e_split.startIndex + e_split.indexLength - 1) - (container.startIndex + container.indexLength - 1);
				e_split.startIndex = container.startIndex + container.indexLength;
				e_split.ledOffset += e_split.startIndex - e.startIndex;
				containerList.push_back(e_split);
			}
		}
	}
	containerList.push_back(container);
	if (container.isAnimated) ++animatedCount;
}

template<typename T_COLOR_FEATURE>
void NeoHandler<T_COLOR_FEATURE>::addContainer(std::vector<NeoContainer> c, bool resolveOverlap) {
	for (auto &e : c)
		addContainer(e, resolveOverlap);
}

template<typename T_COLOR_FEATURE>
void NeoHandler<T_COLOR_FEATURE>::clearContainer() {
	containerList.clear();
}


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
