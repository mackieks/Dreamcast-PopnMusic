#pragma once

#include "pico/stdlib.h"

namespace display
{

class Display
{
public:
    //! Virtual destructor
    virtual inline ~Display() {}

    //! Copies the pixel data into the LCD frame buffer used by the DMA channel
    virtual void setPixel(uint8_t x, uint8_t y, uint16_t color) = 0;

    //! Refreshes the LCD panel by triggering a DMA transfer
    virtual void refresh() = 0;

    //! Initializes the screen
    virtual void initialize() = 0;

    //! Clears the screen by setting the LCD frame buffer to black
    virtual void clear() = 0;

    //! Display a splash image
    virtual void showSplash() = 0;

    //! Returns true if the screen has been initialized, false otherwise
    inline bool isInitialized() { return mIsInitialized; }

    // Function that reverses the byte order of a single uint32_t value
    inline uint32_t reverseByteOrder(uint32_t value) {
        return ((value >> 24) & 0xFF) | 
            ((value >> 8) & 0xFF00) | 
            ((value << 8) & 0xFF0000) | 
            ((value << 24) & 0xFF000000);
    }

protected:
    //! true iff "isInitialized"
    bool mIsInitialized;
};

}
