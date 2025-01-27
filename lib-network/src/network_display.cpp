/**
 * @file network_display.cpp
 *
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstdint>

#include "display.h"
#include "network.h"
#include "net/protocol/dhcp.h"

#if !defined (CONFIG_DISPLAY_LINE_IP)
static constexpr uint32_t LINE_IP = 2;
#else
static constexpr uint32_t LINE_IP = CONFIG_DISPLAY_LINE_IP;
#endif

__attribute__((weak)) void network_display_emac_config() {
#if !defined (NO_EMAC)
	Display::Get()->ClearLine(LINE_IP);
	Display::Get()->PutString("Ethernet config");
#endif
}

__attribute__((weak)) void network_display_emac_start() {
#if !defined (NO_EMAC)
	Display::Get()->ClearLine(LINE_IP);
	Display::Get()->PutString("Ethernet start");
#endif
}

__attribute__((weak)) void network_display_emac_status([[maybe_unused]] const bool isLinkUp) {
#if !defined (NO_EMAC)
	Display::Get()->ClearLine(LINE_IP);
	Display::Get()->PutString("Ethernet Link ");
	if (isLinkUp) {
		Display::Get()->PutString("UP");
	} else {
		Display::Get()->PutString("DOWN");
	}
#endif
}

__attribute__((weak)) void network_display_ip() {
#if !defined (NO_EMAC)
	Display::Get()->ClearLine(LINE_IP);
	Display::Get()->Printf(LINE_IP, "" IPSTR "/%d %c", IP2STR(Network::Get()->GetIp()), Network::Get()->GetNetmaskCIDR(), Network::Get()->GetAddressingMode());
#endif
}

__attribute__((weak)) void network_display_netmask() {
#if !defined (NO_EMAC)
	network_display_ip();
#endif
}

__attribute__((weak)) void network_display_gateway() {}

__attribute__((weak)) void network_display_hostname() {}

__attribute__((weak)) void network_display_emac_shutdown() {
#if !defined (NO_EMAC)
	Display::Get()->ClearLine(LINE_IP);
	Display::Get()->PutString("Ethernet shutdown");
#endif
}

__attribute__((weak)) void network_display_dhcp_status([[maybe_unused]] net::dhcp::State state) {
#if !defined (NO_EMAC)
	Display::Get()->ClearLine(LINE_IP);

	switch (state) {
	case net::dhcp::State::STATE_OFF:
		break;
	case net::dhcp::State::STATE_RENEWING:
		Display::Get()->PutString("DHCP renewing");
		break;
	case net::dhcp::State::STATE_BOUND:
		Display::Get()->PutString("Got IP");
		break;
	case net::dhcp::State::STATE_REQUESTING:
		Display::Get()->PutString("DHCP requesting");
		break;
	case net::dhcp::State::STATE_BACKING_OFF:
		Display::Get()->PutString("DHCP Error");
		break;
	default:
		break;
	}
#endif
}
