/**
 * @file main.cpp
 *
 */
/* Copyright (C) 2021-2024 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#pragma GCC push_options
#pragma GCC optimize ("O2")
#pragma GCC optimize ("no-tree-loop-distribute-patterns")

#include <cstdint>

#include "hardware.h"
#include "network.h"

#include "net/apps/mdns.h"

#include "displayudf.h"
#include "displayudfparams.h"
#include "displayhandler.h"

#include "artnetnode.h"
#include "artnetparams.h"
#include "artnetmsgconst.h"
#include "artnettriggerhandler.h"

#include "pixeldmxconfiguration.h"
#include "pixeltype.h"
#include "pixeltestpattern.h"
#include "pixeldmxparams.h"
#include "ws28xxdmx.h"

#include "dmxparams.h"
#include "dmxsend.h"
#include "dmxconfigudp.h"

#include "lightsetwith4.h"

#if defined (NODE_RDMNET_LLRP_ONLY)
# include "rdmdeviceparams.h"
# include "rdmnetdevice.h"
# include "rdmnetconst.h"
# include "rdmpersonality.h"
# include "rdm_e120.h"
# include "factorydefaults.h"
#endif

#if defined (NODE_SHOWFILE)
# include "showfile.h"
# include "showfileparams.h"
#endif

#include "remoteconfig.h"
#include "remoteconfigparams.h"

#include "flashcodeinstall.h"
#include "configstore.h"

#include "firmwareversion.h"
#include "software_version.h"

namespace hal {
void reboot_handler() {
	WS28xx::Get()->Blackout();
	Dmx::Get()->Blackout();
	ArtNetNode::Get()->Stop();
}
}  // namespace hal

int main() {
	Hardware hw;
	DisplayUdf display;
	ConfigStore configStore;
	Network nw;
	FirmwareVersion fw(SOFTWARE_VERSION, __DATE__, __TIME__);
	FlashCodeInstall spiFlashInstall;

	fw.Print("Art-Net 4 Pixel controller {1x 4 Universes} / DMX");
	
	ArtNetNode node;

	ArtNetParams artnetParams;
	artnetParams.Load();
	artnetParams.Set();

	// LightSet A - Pixel - 4 Universes

	PixelDmxConfiguration pixelDmxConfiguration;

	PixelDmxParams pixelDmxParams;
	pixelDmxParams.Load();
	pixelDmxParams.Set();

	WS28xxDmx pixelDmx;

	auto isPixelUniverseSet = false;
	const auto nStartPixelUniverse = pixelDmxParams.GetStartUniversePort(0, isPixelUniverseSet);

	if (isPixelUniverseSet) {
		const auto nUniverses = pixelDmxConfiguration.GetUniverses();

		for (uint32_t nPortIndex = 0; nPortIndex < nUniverses; nPortIndex++) {
			node.SetUniverse(nPortIndex, lightset::PortDir::OUTPUT, static_cast<uint16_t>(nStartPixelUniverse + nPortIndex));
			char label[artnet::SHORT_NAME_LENGTH];
			snprintf(label, artnet::SHORT_NAME_LENGTH - 1, "Pixel Port U:%u", nStartPixelUniverse + nPortIndex);
			node.SetShortName(nPortIndex, label);
		}
	}

	const auto nTestPattern = static_cast<pixelpatterns::Pattern>(pixelDmxParams.GetTestPattern());
	PixelTestPattern pixelTestPattern(nTestPattern, 1);

	// LightSet B - DMX - 1 Universe

	uint32_t nDmxUniverses = 0;

	if (artnetParams.GetDirection(0) == lightset::PortDir::OUTPUT) {
		node.SetUniverse(dmxsend::DMXPORT_OFFSET, lightset::PortDir::OUTPUT, artnetParams.GetUniverse(0));
		nDmxUniverses = 1;
	}

	Dmx dmx;

	DmxParams dmxparams;
	dmxparams.Load();
	dmxparams.Set(&dmx);

	if (node.GetPortDirection(dmxsend::DMXPORT_OFFSET) == lightset::PortDir::OUTPUT) {
		dmx.SetPortDirection(0, dmx::PortDirection::OUTP, false);
	}

	DmxSend dmxSend;

	display.SetDmxInfo(displayudf::dmx::PortDir::OUTPUT, nDmxUniverses);

	// LightSet 4with4

	LightSetWith4<4> lightSet((PixelTestPattern::Get()->GetPattern() != pixelpatterns::Pattern::NONE) ? nullptr : &pixelDmx, (nDmxUniverses != 0) ? &dmxSend : nullptr);
	lightSet.Print();

	ArtNetTriggerHandler artnetTriggerHandler(&lightSet, &pixelDmx);

	node.SetOutput(&lightSet);
	node.Print();

#if defined (NODE_RDMNET_LLRP_ONLY)
	display.TextStatus(RDMNetConst::MSG_CONFIG, CONSOLE_YELLOW);

	char aDescription[rdm::personality::DESCRIPTION_MAX_LENGTH + 1];
	snprintf(aDescription, sizeof(aDescription) - 1, "Art-Net Pixel 1-%s:%d", pixel::pixel_get_type(pixelDmxConfiguration.GetType()), pixelDmxConfiguration.GetCount());

	char aLabel[RDM_DEVICE_LABEL_MAX_LENGTH + 1];
	const auto nLength = snprintf(aLabel, sizeof(aLabel) - 1, "Orange Pi Zero Pixel-DMX");

	RDMPersonality *pPersonalities[1] = { new RDMPersonality(aDescription, nullptr) };
	RDMNetDevice llrpOnlyDevice(pPersonalities, 1);

	llrpOnlyDevice.SetLabel(RDM_ROOT_DEVICE, aLabel, static_cast<uint8_t>(nLength));
	llrpOnlyDevice.SetProductCategory(E120_PRODUCT_CATEGORY_FIXTURE);
	llrpOnlyDevice.SetProductDetail(E120_PRODUCT_DETAIL_LED);

	node.SetRdmUID(llrpOnlyDevice.GetUID(), true);

	llrpOnlyDevice.Init();

	RDMDeviceParams rdmDeviceParams;
	rdmDeviceParams.Load();
	rdmDeviceParams.Set(&llrpOnlyDevice);

	llrpOnlyDevice.Print();
#endif

#if defined (NODE_SHOWFILE)
	ShowFile showFile;

	ShowFileParams showFileParams;
	showFileParams.Load();
	showFileParams.Set();

	if (showFile.IsAutoStart()) {
		showFile.Play();
	}

	showFile.Print();
#endif

	display.SetTitle("Art-Net 4 Pixel 1 - DMX");
	display.Set(2, displayudf::Labels::VERSION);
	display.Set(3, displayudf::Labels::IP);
	display.Set(4, displayudf::Labels::DEFAULT_GATEWAY);
	display.Set(5, displayudf::Labels::DMX_DIRECTION);

	DisplayUdfParams displayUdfParams;
	displayUdfParams.Load();
	displayUdfParams.Set(&display);

	display.Show();
	display.Printf(7, "%s:%d G%d %s",
		pixel::pixel_get_type(pixelDmxConfiguration.GetType()),
		pixelDmxConfiguration.GetCount(),
		pixelDmxConfiguration.GetGroupingCount(),
		pixel::pixel_get_map(pixelDmxConfiguration.GetMap()));

	if (nTestPattern != pixelpatterns::Pattern::NONE) {
		display.ClearLine(6);
		display.Printf(6, "%s:%u", PixelPatterns::GetName(nTestPattern), static_cast<uint32_t>(nTestPattern));
	}

	RemoteConfig remoteConfig(remoteconfig::Node::ARTNET, remoteconfig::Output::PIXEL, node.GetActiveOutputPorts());

	RemoteConfigParams remoteConfigParams;
	remoteConfigParams.Load();
	remoteConfigParams.Set(&remoteConfig);

	display.TextStatus(ArtNetMsgConst::START, CONSOLE_YELLOW);

	node.Start();

	display.TextStatus(ArtNetMsgConst::STARTED, CONSOLE_GREEN);

	hw.WatchdogInit();

	for (;;) {
		hw.WatchdogFeed();
		nw.Run();
		node.Run();
#if defined (NODE_SHOWFILE)
		showFile.Run();
#endif
		remoteConfig.Run();
		configStore.Flash();
		pixelTestPattern.Run();
		display.Run();
		hw.Run();
	}
}
