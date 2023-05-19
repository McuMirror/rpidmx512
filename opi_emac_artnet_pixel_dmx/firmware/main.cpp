/**
 * @file main.cpp
 *
 */
/* Copyright (C) 2021-2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
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
#include <cassert>

#include "hardware.h"
#include "network.h"
#include "networkconst.h"

#include "mdns.h"

#if defined (ENABLE_HTTPD)
# include "httpd/httpd.h"
#endif

#include "displayudf.h"
#include "displayudfparams.h"
#include "displayhandler.h"
#include "display_timeout.h"

#include "artnet4node.h"
#include "artnetparams.h"
#include "artnetmsgconst.h"
#include "artnettriggerhandler.h"

#include "pixeldmxconfiguration.h"
#include "pixeltype.h"
#include "pixeltestpattern.h"
#include "pixeldmxparams.h"
#include "ws28xxdmx.h"
#include "ws28xxdmxstartstop.h"

#include "dmxparams.h"
#include "dmxsend.h"
#include "dmxconfigudp.h"

#include "lightset4with4.h"

#if defined (NODE_RDMNET_LLRP_ONLY)
# include "rdmdeviceparams.h"
# include "rdmnetdevice.h"
# include "rdmnetconst.h"
# include "rdmpersonality.h"
# include "rdm_e120.h"
# include "factorydefaults.h"
#endif

#include "remoteconfig.h"
#include "remoteconfigparams.h"

#include "flashcodeinstall.h"
#include "configstore.h"
#include "storeartnet.h"
#include "storedisplayudf.h"
#include "storedmxsend.h"
#include "storenetwork.h"
#if defined (NODE_RDMNET_LLRP_ONLY)
# include "storerdmdevice.h"
#endif
#include "storeremoteconfig.h"
#include "storepixeldmx.h"

#include "firmwareversion.h"
#include "software_version.h"

static constexpr auto DMXPORT_OFFSET = 4U;

void Hardware::RebootHandler() {
	WS28xx::Get()->Blackout();
	Dmx::Get()->Blackout();
	ArtNet4Node::Get()->Stop();
}

void main() {
	Hardware hw;
	DisplayUdf display;
	ConfigStore configStore;
	display.TextStatus(NetworkConst::MSG_NETWORK_INIT, Display7SegmentMessage::INFO_NETWORK_INIT, CONSOLE_YELLOW);
	StoreNetwork storeNetwork;
	Network nw(&storeNetwork);
	display.TextStatus(NetworkConst::MSG_NETWORK_STARTED, Display7SegmentMessage::INFO_NONE, CONSOLE_GREEN);
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);
	FlashCodeInstall spiFlashInstall;

	fw.Print("Art-Net 4 Pixel controller {1x 4 Universes} / DMX");
	nw.Print();

	display.TextStatus(NetworkConst::MSG_MDNS_CONFIG, Display7SegmentMessage::INFO_MDNS_CONFIG, CONSOLE_YELLOW);

	MDNS mDns;
	mDns.AddServiceRecord(nullptr, mdns::Services::CONFIG);
	mDns.AddServiceRecord(nullptr, mdns::Services::TFTP);
#if defined (ENABLE_HTTPD)
	mDns.AddServiceRecord(nullptr, mdns::Services::HTTP, "node=Art-Net Pixel DMX");
#endif
	mDns.Print();

#if defined (ENABLE_HTTPD)
	HttpDaemon httpDaemon;
#endif

	display.TextStatus(ArtNetMsgConst::PARAMS, Display7SegmentMessage::INFO_NODE_PARMAMS, CONSOLE_YELLOW);

	ArtNet4Node node;

	StoreArtNet storeArtNet;
	ArtNetParams artnetParams(&storeArtNet);

	node.SetArtNetStore(&storeArtNet);

	if (artnetParams.Load()) {
		artnetParams.Dump();
		artnetParams.Set(DMXPORT_OFFSET);
	}

	// LightSet A - Pixel - 4 Universes

	PixelDmxConfiguration pixelDmxConfiguration;

	StorePixelDmx storePixelDmx;
	PixelDmxParams pixelDmxParams(&storePixelDmx);

	if (pixelDmxParams.Load()) {
		pixelDmxParams.Set(&pixelDmxConfiguration);
		pixelDmxParams.Dump();
	}

	WS28xxDmx pixelDmx(pixelDmxConfiguration);
	pixelDmx.SetPixelDmxHandler(new PixelDmxStartStop);

	auto isPixelUniverseSet = false;
	const auto nStartPixelUniverse = pixelDmxParams.GetStartUniversePort(0, isPixelUniverseSet);

	if (isPixelUniverseSet) {
		const auto nUniverses = pixelDmx.GetUniverses();

		for (uint32_t nPortIndex = 0; nPortIndex < nUniverses; nPortIndex++) {
			node.SetUniverse(nPortIndex, lightset::PortDir::OUTPUT, static_cast<uint16_t>(nStartPixelUniverse + nPortIndex));
		}
	}

	const auto nTestPattern = static_cast<pixelpatterns::Pattern>(pixelDmxParams.GetTestPattern());
	PixelTestPattern pixelTestPattern(nTestPattern, 1);

	// LightSet B - DMX - 1 Universe

	auto isDmxUniverseSet = false;
	const auto nDmxUniverse = artnetParams.GetUniverse(0, isDmxUniverseSet);
	const auto portDirection = artnetParams.GetDirection(0);

	if (portDirection == lightset::PortDir::OUTPUT) {
		const auto nAddress = static_cast<uint16_t>((artnetParams.GetNet() & 0x7F) << 8) | static_cast<uint16_t>((artnetParams.GetSubnet() & 0x0F) << 4);
		node.SetUniverse(DMXPORT_OFFSET, lightset::PortDir::OUTPUT, static_cast<uint16_t>(nAddress | nDmxUniverse));
	}

	StoreDmxSend storeDmxSend;
	DmxParams dmxparams(&storeDmxSend);

	Dmx dmx;

	if (dmxparams.Load()) {
		dmxparams.Dump();
		dmxparams.Set(&dmx);
	}

	DmxSend dmxSend;

	dmxSend.Print();

	DmxConfigUdp *pDmxConfigUdp = nullptr;

	if (isDmxUniverseSet) {
		pDmxConfigUdp = new DmxConfigUdp;
		assert(pDmxConfigUdp != nullptr);
	}

	display.SetDmxInfo(displayudf::dmx::PortDir::OUTPUT, isDmxUniverseSet ? 1 : 0);

	// LightSet 4with4

	LightSet4with4 lightSet((PixelTestPattern::GetPattern() != pixelpatterns::Pattern::NONE) ? nullptr : &pixelDmx, isDmxUniverseSet ? &dmxSend : nullptr);
	lightSet.Print();

	ArtNetTriggerHandler triggerHandler(&lightSet, &pixelDmx);

	node.SetOutput(&lightSet);
	node.Print();

#if defined (NODE_RDMNET_LLRP_ONLY)
	display.TextStatus(RDMNetConst::MSG_CONFIG, Display7SegmentMessage::INFO_RDMNET_CONFIG, CONSOLE_YELLOW);

	char aDescription[rdm::personality::DESCRIPTION_MAX_LENGTH + 1];
	snprintf(aDescription, sizeof(aDescription) - 1, "Art-Net Pixel %d-%s:%d DMX %d", isPixelUniverseSet, PixelType::GetType(WS28xx::Get()->GetType()), WS28xx::Get()->GetCount(), isDmxUniverseSet);

	char aLabel[RDM_DEVICE_LABEL_MAX_LENGTH + 1];
	const auto nLength = snprintf(aLabel, sizeof(aLabel) - 1, "Orange Pi Zero Pixel-DMX");

	RDMPersonality *pPersonalities[1] = { new RDMPersonality(aDescription, nullptr) };
	RDMNetDevice llrpOnlyDevice(pPersonalities, 1);

	llrpOnlyDevice.SetLabel(RDM_ROOT_DEVICE, aLabel, static_cast<uint8_t>(nLength));
	llrpOnlyDevice.SetProductCategory(E120_PRODUCT_CATEGORY_FIXTURE);
	llrpOnlyDevice.SetProductDetail(E120_PRODUCT_DETAIL_ETHERNET_NODE);

	node.SetRdmUID(llrpOnlyDevice.GetUID(), true);

	llrpOnlyDevice.Init();

	StoreRDMDevice storeRdmDevice;
	RDMDeviceParams rdmDeviceParams(&storeRdmDevice);

	if (rdmDeviceParams.Load()) {
		rdmDeviceParams.Dump();
		rdmDeviceParams.Set(&llrpOnlyDevice);
	}

	llrpOnlyDevice.SetRDMDeviceStore(&storeRdmDevice);
	llrpOnlyDevice.Print();
#endif

	display.SetTitle("Art-Net 4 Pixel 1 - DMX");
	display.Set(2, displayudf::Labels::VERSION);
	display.Set(3, displayudf::Labels::NODE_NAME);
	display.Set(4, displayudf::Labels::IP);
	display.Set(5, displayudf::Labels::DEFAULT_GATEWAY);
	display.Set(6, displayudf::Labels::DMX_DIRECTION);
	display.Printf(7, "%s:%d G%d %s",
		PixelType::GetType(pixelDmxConfiguration.GetType()),
		pixelDmxConfiguration.GetCount(),
		pixelDmxConfiguration.GetGroupingCount(),
		PixelType::GetMap(pixelDmxConfiguration.GetMap()));

	StoreDisplayUdf storeDisplayUdf;
	DisplayUdfParams displayUdfParams(&storeDisplayUdf);

	if (displayUdfParams.Load()) {
		displayUdfParams.Dump();
		displayUdfParams.Set(&display);
	}

	display.Show(&node, DMXPORT_OFFSET);

	if (nTestPattern != pixelpatterns::Pattern::NONE) {
		display.ClearLine(6);
		display.Printf(6, "%s:%u", PixelPatterns::GetName(nTestPattern), static_cast<uint32_t>(nTestPattern));
	}

	RemoteConfig remoteConfig(remoteconfig::Node::ARTNET, remoteconfig::Output::PIXEL, node.GetActiveOutputPorts());

	StoreRemoteConfig storeRemoteConfig;
	RemoteConfigParams remoteConfigParams(&storeRemoteConfig);

	if (remoteConfigParams.Load()) {
		remoteConfigParams.Dump();
		remoteConfigParams.Set(&remoteConfig);
	}

	while (configStore.Flash())
		;

	display.TextStatus(ArtNetMsgConst::START, Display7SegmentMessage::INFO_NODE_START, CONSOLE_YELLOW);

	node.Start();

	display.TextStatus(ArtNetMsgConst::STARTED, Display7SegmentMessage::INFO_NODE_STARTED, CONSOLE_GREEN);

	hw.WatchdogInit();

	for (;;) {
		hw.WatchdogFeed();
		nw.Run();
		node.Run();
		remoteConfig.Run();
#if defined (NODE_RDMNET_LLRP_ONLY)
		llrpOnlyDevice.Run();
#endif
		configStore.Flash();
		if (__builtin_expect((PixelTestPattern::GetPattern() != pixelpatterns::Pattern::NONE), 0)) {
			pixelTestPattern.Run();
		}
		if (pDmxConfigUdp != nullptr) {
			pDmxConfigUdp->Run();
		}
		mDns.Run();
#if defined (ENABLE_HTTPD)
		httpDaemon.Run();
#endif
		display.Run();
		hw.Run();
	}
}