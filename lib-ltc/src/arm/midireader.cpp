/**
 * @file midireader.cpp
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

#if defined (DEBUG_ARM_MIDIREADER)
# undef NDEBUG
#endif

#pragma GCC push_options
#pragma GCC optimize ("O2")
#pragma GCC optimize ("no-tree-loop-distribute-patterns")

#include <cstdint>
#include <cstring>
#include <cassert>

#include "arm/midireader.h"
#include "ltc.h"
#include "timecodeconst.h"

#include "hardware.h"
// Input
#include "midi.h"
// Output
#include "ltcsender.h"
#include "artnetnode.h"
#include "rtpmidi.h"
#include "ltcetc.h"
#include "arm/ltcmidisystemrealtime.h"
#include "arm/ltcoutputs.h"

#include "arm/platform_ltc.h"

static uint8_t s_qf[8] __attribute__ ((aligned (4))) = { 0, 0, 0, 0, 0, 0, 0, 0 };

#if defined (H3)
static void irq_timer1_handler(void) {
	gv_ltc_bTimeCodeAvailable = true;
	gv_ltc_nTimeCodeCounter++;
}
#elif defined (GD32)
	// Defined in platform_ltc.cpp
#endif

void MidiReader::Start() {
	DEBUG_ENTRY

#if defined (H3)
	Midi::Get()->SetIrqTimer1(irq_timer1_handler);
#elif defined (GD32)
	platform::ltc::timer11_config();
	TIMER_CNT(TIMER11) = 0;
#endif
	Midi::Get()->Init(midi::Direction::INPUT);

	DEBUG_EXIT
}

void MidiReader::HandleMtc() {
	uint8_t nSystemExclusiveLength;
	const auto *pSystemExclusive = Midi::Get()->GetSystemExclusive(nSystemExclusiveLength);

	m_MidiTimeCode.nHours = pSystemExclusive[5] & 0x1F;
	m_MidiTimeCode.nMinutes = pSystemExclusive[6];
	m_MidiTimeCode.nSeconds = pSystemExclusive[7];
	m_MidiTimeCode.nFrames = pSystemExclusive[8];
	m_MidiTimeCode.nType = static_cast<uint8_t>(pSystemExclusive[5] >> 5);

	Update();

	gv_ltc_bTimeCodeAvailable = false;
}

void MidiReader::HandleMtcQf() {
	uint8_t nData1, nData2;
	Midi::Get()->GetMessageData(nData1, nData2);
	const auto nPart = static_cast<uint8_t>((nData1 & 0x70) >> 4);

	s_qf[nPart] = nData1 & 0x0F;

	if ((nPart == 7) || (m_nPartPrevious == 7)) {
	} else {
		m_bDirection = (m_nPartPrevious < nPart);
	}

	if ((m_bDirection && (nPart == 7)) || (!m_bDirection && (nPart == 0))) {
		m_MidiTimeCode.nHours = static_cast<uint8_t>(s_qf[6] | ((s_qf[7] & 0x1) << 4));
		m_MidiTimeCode.nMinutes = static_cast<uint8_t>(s_qf[4] | (s_qf[5] << 4));
		m_MidiTimeCode.nSeconds = static_cast<uint8_t>(s_qf[2] | (s_qf[3] << 4));
		m_MidiTimeCode.nFrames = static_cast<uint8_t>(s_qf[0] | (s_qf[1] << 4));
		m_MidiTimeCode.nType = static_cast<uint8_t>(s_qf[7] >> 1);

		if (m_MidiTimeCode.nFrames < m_nMtcQfFramePrevious) {
			m_nMtcQfFramesDelta = m_nMtcQfFramePrevious - m_MidiTimeCode.nFrames;
		} else {
			m_nMtcQfFramesDelta = m_MidiTimeCode.nFrames - m_nMtcQfFramePrevious;
		}

		m_nMtcQfFramePrevious = m_MidiTimeCode.nFrames;

		if (m_nMtcQfFramesDelta >= static_cast<uint32_t>(TimeCodeConst::FPS[m_MidiTimeCode.nType] - 2)) {
			m_nMtcQfFramesDelta = 2;
		}

		__DMB();

		if (gv_ltc_nTimeCodeCounter < m_nMtcQfFramesDelta) {
			Update();
		}

#if defined (H3)
		H3_TIMER->TMR1_CTRL |= TIMER_CTRL_SINGLE_MODE;
		H3_TIMER->TMR1_INTV = TimeCodeConst::TMR_INTV[m_MidiTimeCode.nType];
		H3_TIMER->TMR1_CTRL |= (TIMER_CTRL_EN_START | TIMER_CTRL_RELOAD);
#elif defined (GD32)
		TIMER_CAR(TIMER11) = TimeCodeConst::TMR_INTV[m_MidiTimeCode.nType];
		TIMER_CNT(TIMER11) = 0;
#endif
		gv_ltc_bTimeCodeAvailable = false;
		gv_ltc_nTimeCodeCounter = 0;
	}

	m_nPartPrevious = nPart;
}

void MidiReader::Update() {
	if (!ltc::g_DisabledOutputs.bLtc) {
		LtcSender::Get()->SetTimeCode(reinterpret_cast<const struct ltc::TimeCode*>(&m_MidiTimeCode));
	}

	if (!ltc::g_DisabledOutputs.bArtNet) {
		ArtNetNode::Get()->SendTimeCode(reinterpret_cast<const struct artnet::TimeCode*>(&m_MidiTimeCode));
	}

	if (!ltc::g_DisabledOutputs.bRtpMidi) {
		RtpMidi::Get()->SendTimeCode(&m_MidiTimeCode);
	}

	if (!ltc::g_DisabledOutputs.bEtc) {
		LtcEtc::Get()->Send(&m_MidiTimeCode);
	}

	LtcOutputs::Get()->Update(reinterpret_cast<const struct ltc::TimeCode*>(&m_MidiTimeCode));
}

void MidiReader::Run() {
	if (Midi::Get()->Read(static_cast<uint8_t>(midi::Channel::OMNI))) {
		if (Midi::Get()->GetChannel() == 0) {
			switch (Midi::Get()->GetMessageType()) {
			case midi::Types::TIME_CODE_QUARTER_FRAME:
				HandleMtcQf();
				break;
			case midi::Types::SYSTEM_EXCLUSIVE: {
				uint8_t nSystemExclusiveLength;
				const auto *pSystemExclusive = Midi::Get()->GetSystemExclusive(nSystemExclusiveLength);
				if ((pSystemExclusive[1] == 0x7F) && (pSystemExclusive[2] == 0x7F) && (pSystemExclusive[3] == 0x01)) {
					HandleMtc();
				}
			}
				break;
			case midi::Types::CLOCK:
				uint32_t nBPM;
				if (m_MidiBPM.Get(Midi::Get()->GetMessageTimeStamp() * 10, nBPM)) {
					LtcOutputs::Get()->ShowBPM(nBPM);
				}
				break;
			default:
				break;
			}
		}
	}

	__DMB();

	if (gv_ltc_bTimeCodeAvailable) {
		gv_ltc_bTimeCodeAvailable = false;
		const auto nFps = TimeCodeConst::FPS[m_MidiTimeCode.nType];

		if (m_bDirection) {
			m_MidiTimeCode.nFrames++;
			if (nFps == m_MidiTimeCode.nFrames) {
				m_MidiTimeCode.nFrames = 0;

				m_MidiTimeCode.nSeconds++;
				if (m_MidiTimeCode.nSeconds == 60) {
					m_MidiTimeCode.nSeconds = 0;

					m_MidiTimeCode.nMinutes++;
					if (m_MidiTimeCode.nMinutes == 60) {
						m_MidiTimeCode.nMinutes = 0;

						m_MidiTimeCode.nHours++;
						if (m_MidiTimeCode.nHours == 24) {
							m_MidiTimeCode.nHours = 0;
						}
					}
				}
			}
		} else {
			if (m_MidiTimeCode.nFrames == nFps - 1) {
				if (m_MidiTimeCode.nSeconds > 0) {
					m_MidiTimeCode.nSeconds--;
				} else {
					m_MidiTimeCode.nSeconds = 59;
				}

				if (m_MidiTimeCode.nSeconds == 59) {
					if (m_MidiTimeCode.nMinutes > 0) {
						m_MidiTimeCode.nMinutes--;
					} else {
						m_MidiTimeCode.nMinutes = 59;
					}

					if (m_MidiTimeCode.nMinutes == 59) {
						if (m_MidiTimeCode.nHours > 0) {
							m_MidiTimeCode.nHours--;
						} else {
							m_MidiTimeCode.nHours = 23;
						}
					}
				}
			}
		}

		Update();

 		if (m_nMtcQfFramesDelta == 2) {
 			m_nMtcQfFramesDelta = 0;
#if defined (H3)
 			H3_TIMER->TMR1_CTRL |= TIMER_CTRL_SINGLE_MODE;
 			H3_TIMER->TMR1_INTV = TimeCodeConst::TMR_INTV[m_MidiTimeCode.nType];
 			H3_TIMER->TMR1_CTRL |= (TIMER_CTRL_EN_START | TIMER_CTRL_RELOAD);
#elif defined (GD32)
 			TIMER_CAR(TIMER11) = TimeCodeConst::TMR_INTV[m_MidiTimeCode.nType];
 			TIMER_CNT(TIMER11) = 0;
#endif
 		}
	}

	if (Midi::Get()->GetUpdatesPerSecond() != 0)  {
		Hardware::Get()->SetMode(hardware::ledblink::Mode::DATA);
	} else {
		Hardware::Get()->SetMode(hardware::ledblink::Mode::NORMAL);
	}
}
