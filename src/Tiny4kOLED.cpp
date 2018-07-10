/*
 * SSD1306xLED - Drivers for SSD1306 controlled dot matrix OLED/PLED 128x64 displays
 *
 * @created: 2014-08-12
 * @author: Neven Boyanov
 *
 * Source code available at: https://bitbucket.org/tinusaur/ssd1306xled
 *
 */

// ----------------------------------------------------------------------------

#include "Tiny4kOLED.h"

#define SSD1306_PAGES 4
#define SSD1306_MAX_PAGE 3

#define SSD1306_COMMAND 0x00
#define SSD1306_DATA 0x40

// ----------------------------------------------------------------------------

// Some code based on "IIC_without_ACK" by http://www.14blog.com/archives/1358

const uint8_t ssd1306_init_sequence [] PROGMEM = {	// Initialization Sequence
	0xAE,			// Display OFF (sleep mode)
	0x20, 0b00,		// Set Memory Addressing Mode
					// 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
					// 10=Page Addressing Mode (RESET); 11=Invalid
	0xB0,			// Set Page Start Address for Page Addressing Mode, 0-7
	0xC8,			// Set COM Output Scan Direction
	0x00,			// ---set low column address
	0x10,			// ---set high column address
	0x40,			// --set start line address
	0x81, 0x8F,		// Set contrast control register
	0xA1,			// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
	0xA6,			// Set display mode. A6=Normal; A7=Inverse
	0xA8, 0x1F,		// Set multiplex ratio(1 to 64)
	0xA4,			// Output RAM to Display
					// 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
	0xD3, 0x00,		// Set display offset. 00 = no offset
	0xD5, 0x80,		// --set display clock divide ratio/oscillator frequency
	0xD9, 0x22,		// Set pre-charge period
	0xDA, 0x02,		// Set com pins hardware configuration
	0xDB,			// --set vcomh
	0x20,			// 0x20,0.77xVcc
	0x8D, 0x14		// Set DC-DC enable
};

const DCfont *oledFont;
uint8_t oledX, oledY = 0;
uint8_t renderingFrame, drawingFrame = 0;

SSD1306Device::SSD1306Device(void){}

void SSD1306Device::begin(void) {
	//_WireClass.begin();

	ssd1306_send_start(SSD1306_COMMAND);
	for (uint8_t i = 0; i < sizeof(ssd1306_init_sequence); i++) {
		ssd1306_send_byte(SSD1306_COMMAND, pgm_read_byte(&ssd1306_init_sequence[i]));
	}
	ssd1306_send_stop();
}

void SSD1306Device::setFont(const DCfont *font) {
	oledFont = font;
}

void SSD1306Device::ssd1306_send_start(uint8_t transmission_type) {
	_WireClass.beginTransmission(SSD1306);
	_WireClass.write(transmission_type);
}

void SSD1306Device::ssd1306_send_stop(void) {
	_WireClass.endTransmission();
}

void SSD1306Device::ssd1306_send_byte(uint8_t transmission_type, uint8_t byte) {
	if (_WireClass.write(byte) == 0) {
		ssd1306_send_stop();
		ssd1306_send_start(transmission_type);
		_WireClass.write(byte);
	}
}

void SSD1306Device::ssd1306_send_command(uint8_t command) {
	ssd1306_send_start(SSD1306_COMMAND);
	_WireClass.write(command);
	ssd1306_send_stop();
}

void SSD1306Device::ssd1306_send_command2(uint8_t command1, uint8_t command2) {
	ssd1306_send_start(SSD1306_COMMAND);
	_WireClass.write(command1);
	_WireClass.write(command2);
	ssd1306_send_stop();
}

void SSD1306Device::setCursor(uint8_t x, uint8_t y) {
	ssd1306_send_start(SSD1306_COMMAND);
	if (renderingFrame == 1) {
		_WireClass.write(0xb0 | ((y + 4) & 0x07));
	}
	else {
		_WireClass.write(0xb0 | (y & 0x07));
	}
	_WireClass.write(((x & 0xf0) >> 4) | 0x10);
	_WireClass.write(x & 0x0f);
	ssd1306_send_stop();
	oledX = x;
	oledY = y;
}

void SSD1306Device::clear(void) {
	fill(0x00);
}

void SSD1306Device::fill(uint8_t fill) {
	for (uint8_t m = 0; m < SSD1306_PAGES; m++) {
		setCursor(0, m);
		ssd1306_send_start(SSD1306_DATA);
		for (uint8_t n = 0; n < 128; n++) {
			ssd1306_send_byte(SSD1306_DATA, fill);
		}
		ssd1306_send_stop();
	}
	setCursor(0, 0);
}

size_t SSD1306Device::write(byte c) {
	if (!oledFont)
		return 1;

	uint8_t h = oledFont->height;
	uint8_t w = oledFont->width;

	if (c == '\r')
		return 1;
	
	if (c == '\n') {
		oledY+=h;
		if (oledY > SSD1306_PAGES - h) {
			oledY = SSD1306_PAGES - h;
		}
		setCursor(0, oledY);
		return 1;
	}

	if (oledX > (128 - w)) {
		oledY+=h;
		if (oledY > SSD1306_PAGES - h) {
			oledY = SSD1306_PAGES - h;
		}
		setCursor(0, oledY);
	}

	int offet = ((uint8_t)c - oledFont->first) * w * h;
	for (uint8_t line = 0; line < h; line++) {
		ssd1306_send_start(SSD1306_DATA);
		for (uint8_t i = 0; i < w; i++) {
			ssd1306_send_byte(SSD1306_DATA, pgm_read_byte(&(oledFont->bitmap[offet++])));
		}
		ssd1306_send_stop();
		if (h == 1) {
			oledX+=w;
		}
		else {
			if (line < h - 1) {
				setCursor(oledX, oledY + 1);
			}
			else {
				setCursor(oledX + w, oledY - (h - 1));
			}
		}
	}
	return 1;
}

void SSD1306Device::bitmap(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t bitmap[]) {
	uint16_t j = 0;
 	for (uint8_t y = y0; y < y1; y++) {
		setCursor(x0,y);
		ssd1306_send_start(SSD1306_DATA);
		for (uint8_t x = x0; x < x1; x++) {
			ssd1306_send_byte(SSD1306_DATA, pgm_read_byte(&bitmap[j++]));
		}
		ssd1306_send_stop();
	}
	setCursor(0, 0);
}

void SSD1306Device::clearToEOL(void) {
	fillToEOL(0x00);
}

void SSD1306Device::fillToEOL(uint8_t fill) {
	fillLength(fill, 128 - oledX);
}

void SSD1306Device::fillLength(uint8_t fill, uint8_t length) {
	ssd1306_send_start(SSD1306_DATA);
	for (uint8_t n = 0; n < length; n++) {
		ssd1306_send_byte(SSD1306_DATA, fill);
	}
	ssd1306_send_stop();
	oledX += length;
}

void SSD1306Device::switchRenderFrame(void) {
	if (renderingFrame == 1) {
		renderingFrame = 0;
	}
	else {
		renderingFrame = 1;
	}
}

void SSD1306Device::switchDisplayFrame(void) {
	if (drawingFrame == 1) {
		drawingFrame = 0;
		ssd1306_send_command(0x40);
	}
	else {
		drawingFrame = 1;
		ssd1306_send_command(0x60);
	}
}

void SSD1306Device::switchFrame(void) {
	switchDisplayFrame();
	switchRenderFrame();
}

// 1. Fundamental Command Table

void SSD1306Device::setContrast(uint8_t contrast) {
	ssd1306_send_command2(0x81,contrast);
}

void SSD1306Device::setEntireDisplayOn(bool enable) {
	if (enable)
		ssd1306_send_command(0xA5);
	else
		ssd1306_send_command(0xA4);
}

void SSD1306Device::setInverse(bool enable) {
	if (enable)
		ssd1306_send_command(0xA7);
	else
		ssd1306_send_command(0xA6);
}

void SSD1306Device::off(void) {
	ssd1306_send_command(0xAE);
}

void SSD1306Device::on(void) {
	ssd1306_send_command(0xAF);
}

// 2. Scrolling Command Table

void SSD1306Device::scrollRight(uint8_t startPage, uint8_t interval, uint8_t endPage) {
	ssd1306_send_start(SSD1306_COMMAND);
	_WireClass.write(0x26);
	_WireClass.write(0x00);
	_WireClass.write(startPage);
	_WireClass.write(interval);
	_WireClass.write(endPage);
	_WireClass.write(0x00);
	_WireClass.write(0xFF);
	ssd1306_send_stop();
}

void SSD1306Device::scrollLeft(uint8_t startPage, uint8_t interval, uint8_t endPage) {
	ssd1306_send_start(SSD1306_COMMAND);
	_WireClass.write(0x27);
	_WireClass.write(0x00);
	_WireClass.write(startPage);
	_WireClass.write(interval);
	_WireClass.write(endPage);
	_WireClass.write(0x00);
	_WireClass.write(0xFF);
	ssd1306_send_stop();
}

void SSD1306Device::scrollRightOffset(uint8_t startPage, uint8_t interval, uint8_t endPage, uint8_t offset) {
	ssd1306_send_start(SSD1306_COMMAND);
	_WireClass.write(0x29);
	_WireClass.write(0x00);
	_WireClass.write(startPage);
	_WireClass.write(interval);
	_WireClass.write(endPage);
	_WireClass.write(offset);
	ssd1306_send_stop();
}

void SSD1306Device::scrollLeftOffset(uint8_t startPage, uint8_t interval, uint8_t endPage, uint8_t offset) {
	ssd1306_send_start(SSD1306_COMMAND);
	_WireClass.write(0x2A);
	_WireClass.write(0x00);
	_WireClass.write(startPage);
	_WireClass.write(interval);
	_WireClass.write(endPage);
	_WireClass.write(offset);
	ssd1306_send_stop();
}

void SSD1306Device::deactivateScroll(void) {
	ssd1306_send_command(0x2E);
}

void SSD1306Device::activateScroll(void) {
	ssd1306_send_command(0x2F);
}

void SSD1306Device::setVerticalScrollArea(uint8_t top, uint8_t rows) {
	ssd1306_send_start(SSD1306_COMMAND);
	_WireClass.write(0xA3);
	_WireClass.write(top);
	_WireClass.write(rows);
	ssd1306_send_stop();
}

// 3. Addressing Setting Command Table

// 4. Hardware Configuration (Panel resolution and layout related) Command Table

void SSD1306Device::setDisplayStartLine(uint8_t startLine) {
	ssd1306_send_command(0x40 | (startLine & 0x3F));
}

void SSD1306Device::setSegmentRemap(uint8_t remap) {
	ssd1306_send_command(0xA0 | (remap & 0x01));
}

void SSD1306Device::setMultiplexRatio(uint8_t mux) {
	ssd1306_send_command2(0xA8, mux);
}

void SSD1306Device::setComOutputDirection(uint8_t direction) {
	ssd1306_send_command(0xC0 | ((direction & 0x01)<<3));
}

void SSD1306Device::setDisplayOffset(uint8_t offset) {
	ssd1306_send_command2(0xD3, offset);
}

void SSD1306Device::setComPinsHardwareConfiguration(uint8_t alternative, uint8_t enableLeftRightRemap) {
	ssd1306_send_command2(0xDA, ((enableLeftRightRemap & 0x01) << 5) | ((alternative & 0x01) << 4) | 0x02 );
}

void SSD1306Device::setDisplayClock(uint8_t divideRatio, uint8_t oscillatorFrequency) {
	ssd1306_send_command2(0xD5, ((oscillatorFrequency & 0x0F) << 4) | (divideRatio & 0x0F));
}

void SSD1306Device::setPrechargePeriod(uint8_t phaseOnePeriod, uint8_t phaseTwoPeriod) {
	ssd1306_send_command2(0xD9, ((phaseTwoPeriod & 0x0F) << 4) | (phaseOnePeriod & 0x0F));
}

void SSD1306Device::setVcomhDeselectLevel(uint8_t level) {
	ssd1306_send_command2(0xDB, (level & 0x07) << 4);
}

void SSD1306Device::nop(void) {
	ssd1306_send_command(0xE3);
}



// ----------------------------------------------------------------------------
