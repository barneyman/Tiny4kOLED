/*
 * SSD1306xLED - Drivers for SSD1306 controlled dot matrix OLED/PLED 128x64 displays
 *
 * @created: 2014-08-12
 * @author: Neven Boyanov
 *
 * Source code available at: https://bitbucket.org/tinusaur/ssd1306xled
 *
 * Re-written and extended by Stephen Denne
 * from 2017-04-25 at https://github.com/datacute/Tiny4kOLED
 *
 */
#include <stdint.h>
#include <Arduino.h>
#if defined( ARDUINO_ARCH_ESP8266 ) || defined( ARDUINO_ARCH_ESP32 )
#include <Wire.h>
#define _WireClass Wire
#else
#define _WireClass TinyWire
#include <TinyWire.h>  // Version with buffer bugfix: https://github.com/adafruit/TinyWireM
#endif

#ifndef TINY4KOLED_H
#define TINY4KOLED_H

typedef struct {
	uint8_t *bitmap;      // character bitmaps data
	uint8_t width;        // character width in pixels
	uint8_t height;       // character height in pages (8 pixels)
	uint8_t first, last;  // ASCII extents
} DCfont;

// Two included fonts, The space isn't used unless it is needed
#include "font6x8.h"
#include "font8x16.h"

// ----------------------------------------------------------------------------

#ifndef SSD1306
#define SSD1306		0x3C	// Slave address
#endif

// ----------------------------------------------------------------------------

class SSD1306Device: public Print {

protected:
		void begin(uint8_t init_sequence_length, const uint8_t init_sequence []);


	public:
		// begin by calling the protected begin(...) with yuor init
		virtual void begin(void)=0;

		void setFont(const DCfont *font);
		virtual void setCursor(uint8_t x, uint8_t y);
		void newLine();
		void fill(uint8_t fill);
		void fillLine(uint8_t line, uint8_t fill);
		void fillToEOL(uint8_t fill);
		void fillLength(uint8_t fill, uint8_t length);
		void clear(void);
		void clearToEOL(void);
		void bitmap(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t bitmap[]);
		// the number of rows/8
		virtual uint8_t numberOfPages() = 0;
		// if the screen is smaller than 128, this is the offset from left, of screen memory
		virtual uint8_t oledXoffset() { return 0;  }
		// how wide the PHYSICAL screen is
		virtual uint8_t oledWidth() { return 128; }

		// 0. frame handling
		virtual void switchRenderFrame(void) {}
		virtual void switchDisplayFrame(void) {}
		virtual void switchFrame(void) {}
		virtual uint8_t currentRenderFrame(void) { return 1; }
		virtual uint8_t currentDisplayFrame(void) { return 1; }
		virtual bool offScreenRender() { return false; }

		// 1. Fundamental Command Table

		void setContrast(uint8_t contrast);
		void setEntireDisplayOn(bool enable);
		void setInverse(bool enable);
		void off(void);
		void on(void);

		// 2. Scrolling Command Table

		void scrollRight(uint8_t startPage, uint8_t interval, uint8_t endPage);
		void scrollLeft(uint8_t startPage, uint8_t interval, uint8_t endPage);
		void scrollRightOffset(uint8_t startPage, uint8_t interval, uint8_t endPage, uint8_t offset);
		void scrollLeftOffset(uint8_t startPage, uint8_t interval, uint8_t endPage, uint8_t offset);
		void deactivateScroll(void);
		void activateScroll(void);
		void setVerticalScrollArea(uint8_t top, uint8_t rows);

		// 3. Addressing Setting Command Table
		void setColumnStartAddress(uint8_t startAddress);
		void setMemoryAddressingMode(uint8_t mode);
		void setColumnAddress(uint8_t startAddress, uint8_t endAddress);
		void setPageAddress(uint8_t startPage, uint8_t endPage);
		void setPageStartAddress(uint8_t startPage);

		// 4. Hardware Configuration (Panel resolution and layout related) Command Table

		void setDisplayStartLine(uint8_t startLine);
		void setSegmentRemap(uint8_t remap);
		void setMultiplexRatio(uint8_t mux);
		void setComOutputDirection(uint8_t direction);
		void setDisplayOffset(uint8_t offset);
		void setComPinsHardwareConfiguration(uint8_t alternative, uint8_t enableLeftRightRemap);

		// 5. Timing and Driving Scheme Setting Command table

		void setDisplayClock(uint8_t divideRatio, uint8_t oscillatorFrequency);
		void setPrechargePeriod(uint8_t phaseOnePeriod, uint8_t phaseTwoPeriod);
		void setVcomhDeselectLevel(uint8_t level);
		void nop(void);

		// 6. Advance Graphic Command table

		void fadeOut(uint8_t interval);
		void blink(uint8_t interval);
		void disableFadeOutAndBlinking(void);
		void enableZoomIn(void);
		void disableZoomIn(void);

		// Charge Pump Settings

		void enableChargePump(void);
		void disableChargePump(void);

		virtual size_t write(byte c);
		using Print::write;

protected:

	uint8_t renderingFrame = 0xB0, drawingFrame = 0x40;

	private:
		void newLine(uint8_t fontHeight);

};

// implementation of a 128x32 panel - i don't have one of these, cannot confirm it works
const uint8_t SSD1306_128x32_init_sequence[] PROGMEM = {
	// Initialization Sequence
	//	0xAE,			// Display OFF (sleep mode)
	//	0x20, 0b10,		// Set Memory Addressing Mode
	// 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
	// 10=Page Addressing Mode (RESET); 11=Invalid
	//	0xB0,			// Set Page Start Address for Page Addressing Mode, 0-7
	0xC8,			// Set COM Output Scan Direction
	//	0x00,			// ---set low column address
	//	0x10,			// ---set high column address
	//	0x40,			// --set start line address
	//	0x81, 0x7F,		// Set contrast control register
	0xA1,			// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
	//	0xA6,			// Set display mode. A6=Normal; A7=Inverse
	0xA8, 0x1F,		// Set multiplex ratio(1 to 64)
	//	0xA4,			// Output RAM to Display
	// 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
	//	0xD3, 0x00,		// Set display offset. 00 = no offset
	//	0xD5, 0x80,		// --set display clock divide ratio/oscillator frequency
	//	0xD9, 0x22,		// Set pre-charge period
	0xDA, 0x02,		// Set com pins hardware configuration
	//	0xDB, 0x20,		// --set vcomh 0x20 = 0.77xVcc
	0x8D, 0x14		// Set DC-DC enable
};

class SSD1306_128x32 : public SSD1306Device
{
public:

	virtual void begin();
	virtual uint8_t numberOfPages() { return 4; }

	// 0. frame handling
	void switchRenderFrame(void);
	void switchDisplayFrame(void);
	void switchFrame(void);
	uint8_t currentRenderFrame(void);
	uint8_t currentDisplayFrame(void);
	virtual bool offScreenRender() { return true; }



};


// implementation of a 128x64 panel 

const uint8_t SSD1306_128x64_init_sequence[] PROGMEM = {
	// Initialization Sequence
	0xB0,			// Set Page Start Address for Page Addressing Mode, 0-7
	0xC8,			// Set COM Output Scan Direction
	0x40,			// --set start line address
	0xA1,			// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
	0xA8, 0x3f,		// Set multiplex ratio(1 to 63)
	0xDA, 0x12,		// Set com pins hardware configuration
	0x8D, 0x14		// Set DC-DC enable
};

class SSD1306_128x64 : public SSD1306Device
{
public:

	virtual void begin();
	virtual uint8_t numberOfPages() { return 8; }


protected:


};


const uint8_t SSD1306_64x48_init_sequence[] PROGMEM = {
	// Initialization Sequence
	0xB0,			// Set Page Start Address for Page Addressing Mode, 0-7
	0xC8,			// Set COM Output Scan Direction
	0x50,				// --set start line address
	0xA1,			// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
	0xA8, 0x2f,		// Set multiplex ratio(1 to 63)

	0xDA, 0x12,		// Set com pins hardware configuration
	0x8D, 0x14		// Set DC-DC enable

};

// implementation of a 64x48 panel (wemos OLED shield)
class SSD1306_64x48 : public SSD1306Device
{
public:

	virtual void begin();
	virtual uint8_t numberOfPages() { return 6; }
	// if the screen is smaller than 128, this is the offset from left, of screen memory
	virtual uint8_t oledXoffset() { return 32; }
	// how wide the PHYSICAL screen is
	virtual uint8_t oledWidth() { return 64; }

protected:


};




// ----------------------------------------------------------------------------

#endif
