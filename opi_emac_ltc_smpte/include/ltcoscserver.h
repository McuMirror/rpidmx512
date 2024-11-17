/**
 * @file ltcoscserver.h
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

#ifndef LTCOSCSERVER_H_
#define LTCOSCSERVER_H_

#include <cstdint>
#include <cstdio>

#if !(defined(CONFIG_LTC_DISABLE_RGB_PANEL) && defined (CONFIG_LTC_DISABLE_WS28XX))
# include "ltcdisplayrgb.h"
#else
# define LTC_NO_DISPLAY_RGB
#endif

#include "osc.h"

#include "network.h"

#include "debug.h"

namespace ltcoscserver {
static constexpr auto PATH_LENGTH_MAX =128;
}  // namespace ltcoscserver

class LtcOscServer {
public:
	LtcOscServer(): m_nPortIncoming(osc::port::DEFAULT_INCOMING) {
		DEBUG_ENTRY

		m_nPathLength = static_cast<uint32_t>(snprintf(m_aPath, sizeof(m_aPath) - 1, "/%s/tc/*", Network::Get()->GetHostName()) - 1);

		DEBUG_PRINTF("%d [%s]", m_nPathLength, m_aPath);
		DEBUG_EXIT
	}

	void Start() {
		assert(m_nHandle == -1);
		m_nHandle = Network::Get()->Begin(m_nPortIncoming);
		assert(m_nHandle != -1);
	}

	void Print() {
		puts("OSC Server");
		printf(" Port : %u\n", m_nPortIncoming);
		printf(" Path : [%s]\n", m_aPath);
	}

	void SetPortIncoming(const uint16_t nPortIncoming) {
		m_nPortIncoming = nPortIncoming;
	}

	uint16_t GetPortIncoming() const {
		return m_nPortIncoming;
	}

	void Run() {
		const auto nBytesReceived = Network::Get()->RecvFrom(m_nHandle, reinterpret_cast<const void **>(&m_pBuffer), &m_nRemoteIp, &m_nRemotePort);

		if (__builtin_expect((nBytesReceived <= 4), 1)) {
			return;
		}

		HandleOscRequest(nBytesReceived);
	}

private:
	void HandleOscRequest(const uint32_t nBytesReceived);
#if !defined(LTC_NO_DISPLAY_RGB)
	void SetWS28xxRGB(uint32_t nSize, ltcdisplayrgb::ColourIndex index);
#endif

private:
	uint16_t m_nPortIncoming;
	uint16_t m_nRemotePort { 0 };
	int32_t m_nHandle { -1 };
	uint32_t m_nRemoteIp { 0 };
	char m_aPath[ltcoscserver::PATH_LENGTH_MAX];
	uint32_t m_nPathLength { 0 };
	const uint8_t *m_pBuffer { nullptr };
};

#endif /* LTCOSCSERVER_H_ */
