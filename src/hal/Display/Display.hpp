#pragma once

#include "pico/stdlib.h"
#include "hal/Display/TransientOverlayObserver.hpp"
#include <string.h>
#include "Font.hpp"

namespace display
{

#define OVERLAY_DISPLAY_TIME 3000  // 3 seconds, in milliseconds

class Display: public TransientOverlayObserver
{
public:
    //! Virtual destructor
    virtual inline ~Display() {}

    //! Copies the pixel data into the LCD frame buffer used by the DMA channel
    void setPixel(uint8_t x, uint8_t y, uint16_t color);

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

    void drawRect(int x0, int x1, int y0, int y1, uint16_t color);

    //! Refreshes the LCD panel by triggering a DMA transfer
    virtual void refresh(const uint32_t* screen, uint32_t len) = 0;

    //! Initializes the screen
    virtual bool initialize() = 0;

    //! Clears the screen by setting the LCD frame buffer to black
    virtual void clear() = 0;

    //! Display a splash image
    virtual void showSplash() = 0;

    void showOverlay();

    //! Returns true if the screen has been initialized, false otherwise
    inline bool isInitialized() { return mIsInitialized; }

    void putLetter(int ix, int iy, char text, uint16_t color);

    void putString(const char *text, int ix, int iy, uint16_t color);

    virtual void update() = 0;

    void drawCursor(int iy, uint16_t color);

    void notify(uint8_t& message);

    //! Function that reverses the byte order of a single uint32_t value
    inline uint32_t reverseByteOrder(uint32_t value) {
        return ((value >> 24) & 0xFF) | 
            ((value >> 8) & 0xFF00) | 
            ((value << 8) & 0xFF0000) | 
            ((value << 24) & 0xFF000000);
    }

public:
    //! Screen data block initialized to blank, 1536 bytes
    uint8_t oledFB[96 * 64 * 2] = {0x00};

    //we're pushing the boundaries of available memory space with this...
    //uint8_t overlayFB[96 * 10 * 2] = {0x00};

    uint8_t mCurrentPage;

protected:
    //! true iff "isInitialized"
    bool mIsInitialized = false;
};

}
