/**
 * @file ili9341.h
 *
 */
/* Copyright (C) 2022-2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef SPI_ILI9341_H_
#define SPI_ILI9341_H_

#include <cstdint>
#include <cassert>

#include "spi/config.h"
#include "spi/spilcd.h"

namespace ili9341 {
namespace cmd {
static constexpr uint8_t NOP 		= 0x00;
static constexpr uint8_t SWRESET	= 0x01;
static constexpr uint8_t RDDID		= 0x04;
static constexpr uint8_t RDDST		= 0x09;
static constexpr uint8_t SLPIN		= 0x10;
static constexpr uint8_t SLPOUT		= 0x11;
static constexpr uint8_t PTLON		= 0x12;
static constexpr uint8_t NORON		= 0x13;
static constexpr uint8_t INVOFF		= 0x20;
static constexpr uint8_t INVON		= 0x21;
static constexpr uint8_t DISPOFF	= 0x28;
static constexpr uint8_t DISPON		= 0x29;
static constexpr uint8_t CASET		= 0x2A;
static constexpr uint8_t RASET		= 0x2B;
static constexpr uint8_t RAMWR		= 0x2C;
static constexpr uint8_t RAMRD		= 0x2E;
static constexpr uint8_t PTLAR		= 0x30;
static constexpr uint8_t MADCTL		= 0x36;
static constexpr uint8_t PIXFMT		= 0x3A;
}  // namespace cmd
namespace data {
/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */
static constexpr uint8_t MADCTL_MY	= 0x80;	///< Page Address Order ('0': Top to Bottom, '1': the opposite)
static constexpr uint8_t MADCTL_MX	= 0x40;	///< Column Address Order ('0': Left to Right, '1': the opposite)
static constexpr uint8_t MADCTL_MV	= 0x20;	///< Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode)
static constexpr uint8_t MADCTL_ML	= 0x10;	///< Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite)
static constexpr uint8_t MADCTL_RGB	= 0x00;	///< Red-Green-Blue pixel order
static constexpr uint8_t MADCTL_BGR	= 0x08;	///< Blue-Green-Red pixel order
}  // namespace data

namespace colour {
static constexpr uint16_t BLACK		= 0x0000;
static constexpr uint16_t BLUE		= 0x001F;
static constexpr uint16_t CYAN		= 0x07FF;
static constexpr uint16_t DARKBLUE	= 0X01CF;
static constexpr uint16_t GRAY		= 0X8430;
static constexpr uint16_t GREEN		= 0x07E0;
static constexpr uint16_t MAGENTA	= 0xF81F;
static constexpr uint16_t ORANGE 	= 0xFC00;
static constexpr uint16_t RED 		= 0xF800;
static constexpr uint16_t WHITE 	= 0xFFFF;
static constexpr uint16_t YELLOW 	= 0xFFE0;
}  // namespace colour
}  // namespace ili9341

#include "paint.h"

class ILI9341 : public Paint {
public:
	ILI9341(uint32_t nCS) : Paint(nCS) {
		DEBUG_ENTRY

#if defined(SPI_LCD_RST_GPIO)
		HardwareReset();
#endif

		WriteCommand(0xC0);    //Power control
		WriteDataByte(0x23);   //VRH[5:0]

		WriteCommand(0xC1);    //Power control
		WriteDataByte(0x10);   //SAP[2:0];BT[3:0]

		WriteCommand(0xC5);    //VCM control
		WriteDataByte(0x3e); //
		WriteDataByte(0x28);

		WriteCommand(0xC7);    //VCM control2
		WriteDataByte(0x86);  //--

		WriteCommand(0x3A);
		WriteDataByte(0x55);

		WriteCommand(0xB1);
		WriteDataByte(0x00);
		WriteDataByte(0x18);

		WriteCommand(0xB6);    // Display Function Control
		WriteDataByte(0x08);
		WriteDataByte(0xA2);
		WriteDataByte(0x27);

		WriteCommand(0xF2);    // 3Gamma Function Disable
		WriteDataByte(0x00);

		WriteCommand(0x26);    //Gamma curve selected
		WriteDataByte(0x01);

		WriteCommand(0xE0);    //Set Gamma
		WriteDataByte(0x0F);
		WriteDataByte(0x31);
		WriteDataByte(0x2B);
		WriteDataByte(0x0C);
		WriteDataByte(0x0E);
		WriteDataByte(0x08);
		WriteDataByte(0x4E);
		WriteDataByte(0xF1);
		WriteDataByte(0x37);
		WriteDataByte(0x07);
		WriteDataByte(0x10);
		WriteDataByte(0x03);
		WriteDataByte(0x0E);
		WriteDataByte(0x09);
		WriteDataByte(0x00);

		WriteCommand(0XE1);    //Set Gamma
		WriteDataByte(0x00);
		WriteDataByte(0x0E);
		WriteDataByte(0x14);
		WriteDataByte(0x03);
		WriteDataByte(0x11);
		WriteDataByte(0x07);
		WriteDataByte(0x31);
		WriteDataByte(0xC1);
		WriteDataByte(0x48);
		WriteDataByte(0x08);
		WriteDataByte(0x0F);
		WriteDataByte(0x0C);
		WriteDataByte(0x31);
		WriteDataByte(0x36);
		WriteDataByte(0x0F);

		SetRotation(0);

		WriteCommand(0x11);    //Exit Sleep
		WriteCommand(0x29);    //Display on

		DEBUG_EXIT
	}

	~ILI9341() override {
		DEBUG_ENTRY
		DEBUG_EXIT
	}

	void SetRotation(const uint32_t nRotation) {
		WriteCommand(ili9341::cmd::MADCTL);

		switch (nRotation) {
		case 0:
			WriteDataByte(ili9341::data::MADCTL_BGR);
			m_nWidth = config::WIDTH;
			m_nHeight = config::HEIGHT;
			break;
		case 1:
			WriteDataByte(ili9341::data::MADCTL_MX | ili9341::data::MADCTL_MV | ili9341::data::MADCTL_BGR);
			m_nWidth = config::HEIGHT;
			m_nHeight = config::WIDTH;
			break;
		case 2:
			WriteDataByte(ili9341::data::MADCTL_MX | ili9341::data::MADCTL_MY | ili9341::data::MADCTL_BGR);
			m_nWidth = config::WIDTH;
			m_nHeight = config::HEIGHT;
			break;
		case 3:
			WriteDataByte(ili9341::data::MADCTL_MY | ili9341::data::MADCTL_MV | ili9341::data::MADCTL_BGR);
			m_nWidth = config::HEIGHT;
			m_nHeight = config::WIDTH;
			break;
		default:
			assert(0);
			__builtin_unreachable();
			break;
		}

		m_nRotate = nRotation;
	}

	void SetBackLight(uint32_t nValue) {
		FUNC_PREFIX(gpio_write(SPI_LCD_BL_GPIO, nValue == 0 ? LOW : HIGH));
	}

	void EnableDisplay(bool bEnable) {
		WriteCommand(bEnable ? ili9341::cmd::DISPON : ili9341::cmd::DISPOFF);
	}

	void EnableSleep(bool bEnable) {
		WriteCommand(bEnable ? ili9341::cmd::SLPIN : ili9341::cmd::SLPOUT);
	}

	void EnableColourInversion(bool bEnable) {
		WriteCommand(bEnable ? ili9341::cmd::INVON : ili9341::cmd::INVOFF);
	}

private:
	void SetAddressWindow(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) override {
		WriteCommand(ili9341::cmd::CASET);
		{
			uint8_t data[] = { static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0), static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1) };
			write_data(data, sizeof(data));
		}

		WriteCommand(ili9341::cmd::RASET);
		{
			uint8_t data[] = { static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0), static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1) };
			write_data(data, sizeof(data));
		}

		WriteCommand(ili9341::cmd::RAMWR);
	}

protected:
	uint32_t m_nShiftX { 0 };
	uint32_t m_nShiftY { 0 };
};

#endif /* SPI_ILI9341_H_ */
