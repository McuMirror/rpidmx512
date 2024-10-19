/**
 * @file ltc.h
 *
 */
/* Copyright (C) 2019-2024 by Arjan van Vught mailto:info@gd32-dmx.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef LTC_H_
#define LTC_H_

#include <cstdint>
#include <time.h>

namespace ltc {
enum class Source : uint8_t {
	LTC, ARTNET, MIDI, TCNET, INTERNAL, APPLEMIDI, SYSTIME, ETC, UNDEFINED
};
enum class Type : uint8_t {
	FILM, EBU, DF, SMPTE, UNKNOWN, INVALID = 255
};

struct TimeCode {
	uint8_t nFrames;		///< Frames time. 0 – 29 depending on mode.
	uint8_t nSeconds;		///< Seconds. 0 - 59.
	uint8_t nMinutes;		///< Minutes. 0 - 59.
	uint8_t nHours;			///< Hours. 0 - 59.
	uint8_t nType;			///< 0 = Film (24fps) , 1 = EBU (25fps), 2 = DF (29.97fps), 3 = SMPTE (30fps)
}__attribute__((packed));

struct DisabledOutputs {
	bool bOled;
	bool bMax7219;
	bool bMidi;
	bool bArtNet;
	bool bLtc;
	bool bEtc;
	bool bNtp;
	bool bRtpMidi;
	bool bWS28xx;
	bool bRgbPanel;
};

namespace systemtime {
namespace index {
static constexpr auto HOURS = 0;
static constexpr auto HOURS_TENS = 0;
static constexpr auto HOURS_UNITS = 1;
static constexpr auto COLON_1 = 2;
static constexpr auto MINUTES = 3;
static constexpr auto MINUTES_TENS = 3;
static constexpr auto MINUTES_UNITS = 4;
static constexpr auto COLON_2 = 5;
static constexpr auto SECONDS = 6;
static constexpr auto SECONDS_TENS = 6;
static constexpr auto SECONDS_UNITS = 7;
}  // namespace index
}  // namespace systemtime

namespace timecode {
static constexpr auto CODE_MAX_LENGTH = 11;
static constexpr auto TYPE_MAX_LENGTH = 11;
static constexpr auto RATE_MAX_LENGTH = 2;
static constexpr auto SYSTIME_MAX_LENGTH = CODE_MAX_LENGTH;

namespace index {
static constexpr auto HOURS = 0;
static constexpr auto HOURS_TENS = 0;
static constexpr auto HOURS_UNITS = 1;
static constexpr auto COLON_1 = 2;
static constexpr auto MINUTES = 3;
static constexpr auto MINUTES_TENS = 3;
static constexpr auto MINUTES_UNITS = 4;
static constexpr auto COLON_2 = 5;
static constexpr auto SECONDS = 6;
static constexpr auto SECONDS_TENS = 6;
static constexpr auto SECONDS_UNITS = 7;
static constexpr auto COLON_3 = 8;
static constexpr auto FRAMES = 9;
static constexpr auto FRAMES_TENS = 9;
static constexpr auto FRAMES_UNITS = 10;
}  // namespace index
}  // namespace timecode

const char *get_type();
const char *get_type(const ltc::Type type);
ltc::Type get_type(const uint8_t nFps);
void set_type(const uint8_t nFps);

void init_timecode(char *pTimeCode);
void init_systemtime(char *pSystemTime);

void itoa_base10(const struct ltc::TimeCode *ptLtcTimeCode, char *pTimeCode);
void itoa_base10(const struct tm *ptLocalTime, char *pSystemTime);

bool parse_timecode(const char *pTimeCode, uint8_t nFps, struct ltc::TimeCode *ptLtcTimeCode);
bool parse_timecode_rate(const char *pTimeCodeRate, uint8_t &nFPS);
}  // namespace ltc


#include "arm/platform_ltc.h"

namespace ltc {
extern struct DisabledOutputs g_DisabledOutputs;
extern ltc::Type g_Type;
}  // namespace ltc

#endif /* LTC_H_ */
