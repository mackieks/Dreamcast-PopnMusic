#ifndef ENABLE_UNIT_TEST

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/platform.h"

#include "configuration.h"
#include "pico_configurations.h"

#include "CriticalSectionMutex.hpp"
#include "Mutex.hpp"
#include "Clock.hpp"
#include "NonVolatilePicoSystemMemory.hpp"
#include "SSD1331.hpp"

#include "hal/System/LockGuard.hpp"
#include "hal/MapleBus/MapleBusInterface.hpp"

#include "DreamcastMainPeripheral.hpp"
#include "DreamcastController.hpp"
#include "DreamcastStorage.hpp"
#include "DreamcastVibration.hpp"
#include "DreamcastScreen.hpp"
#include "DreamcastTimer.hpp"

#include "led.hpp"

#include <memory>
#include <algorithm>
#include <cassert>

display::SSD1331 lcd;
static uint8_t LCDFramebuffer[192] = {0};
volatile uint16_t palette[] = {
    0xf800, // red
    0xfba0, // orange
    0xff80, // yellow
    0x7f80, // yellow-green
    0x0500, // green
    0x045f, // blue
    0x781f, // violet
    0x780d  // magenta
};

void screenCb(const uint32_t* screen, uint32_t len)
{
    //len is the number of words in the payload. For this it should be 48 total words, or 192 bytes.
    //The bytes in each word of screen need to be reversed.
    if(*screen != 0 && (len * sizeof(uint32_t)) == sizeof(LCDFramebuffer))
    {
        uint32_t reversedArr[len];

        // Reverse the byte order of each element and store it in reversedArr
        for (size_t i = 0; i < len; ++i) {
            reversedArr[i] = lcd.reverseByteOrder(screen[i]);
        }

        memcpy(LCDFramebuffer, reversedArr, len * sizeof(uint32_t));

        int x, y, pixel, bb;
        for (int fb = 0; fb < 192; fb++) {
            y = (fb / 6) * 2;
            int mod = (fb % 6) * 16;
            for (bb = 0; bb <= 7; bb++) {
                x = mod + (14 - bb * 2);
                pixel = ((LCDFramebuffer[fb] >> bb) & 0x01) * palette[0];
                lcd.setPixel(x, y, pixel);
                lcd.setPixel(x + 1, y, pixel);
                lcd.setPixel(x, y + 1, pixel);
                lcd.setPixel(x + 1, y + 1, pixel);
            }
        }

        lcd.refresh();
    }
}

void setTimeCb(const client::DreamcastTimer::SetTime& setTime)
{
    // TODO: Fill in
}

void setPwmFn(uint8_t width, uint8_t down)
{
    if (width == 0 || down == 0)
    {
        //buzzer.stop(2);
    }
    else
    {
        //buzzer.stop(2);
        //buzzer.buzzRaw({.priority=2, .wrapCount=width, .highCount=(width-down)});
    }
}

void display_select()
{
    // OLED Select GPIO (high/open = SSD1331, Low = SSD1306)
    gpio_init(OLED_SEL_PIN);
    gpio_set_dir(OLED_SEL_PIN, false);
    gpio_pull_up(OLED_SEL_PIN);

    uint8_t oledType = gpio_get(OLED_SEL_PIN);
    switch(oledType)
    {
        case SSD1331:
            break;
        case SSD1306:
            break;
        default:
            break;
    }
}

std::shared_ptr<NonVolatilePicoSystemMemory> mem =
    std::make_shared<NonVolatilePicoSystemMemory>(
        PICO_FLASH_SIZE_BYTES - client::DreamcastStorage::MEMORY_SIZE_BYTES,
        client::DreamcastStorage::MEMORY_SIZE_BYTES);

// Second Core Process
void core1()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    while (true)
    {
        // Writes vmu storage to pico flash
        mem->process();
    }
}

// First Core Process
void core0()
{
    set_sys_clock_khz(CPU_FREQ_KHZ, true);

    // Create the bus for client-mode operation
    std::shared_ptr<MapleBusInterface> bus = create_maple_bus(P1_BUS_START_PIN, P1_DIR_PIN, DIR_OUT_HIGH);

    // Main peripheral (address of 0x20) with 1 function: controller
    client::DreamcastMainPeripheral mainPeripheral(
        bus,
        0x20,
        0xFF,
        0x00,
        "Dreamcast Controller",
        "Version 1.010,1998/09/28,315-6211-AB   ,Analog Module : The 4th Edition.5/8  +DF",
        43.0,
        50.0);
    std::shared_ptr<client::DreamcastController> controller =
        std::make_shared<client::DreamcastController>();
    mainPeripheral.addFunction(controller);

    // First sub peripheral (address of 0x01) with 1 function: memory
    std::shared_ptr<client::DreamcastPeripheral> subPeripheral1 =
        std::make_shared<client::DreamcastPeripheral>(
            0x01,
            0xFF,
            0x00,
            "Visual Memory",
            "Version 1.005,1999/04/15,315-6208-03,SEGA Visual Memory System BIOS",
            12.4,
            13.0);
    std::shared_ptr<client::DreamcastStorage> dreamcastStorage =
        std::make_shared<client::DreamcastStorage>(mem, 0);
    subPeripheral1->addFunction(dreamcastStorage);

    //TODO add logic to check firmware version, format memory, and store it here
    //Format makes a call to store to flash so processing memory not necessary
    //dreamcastStorage->format();

    std::shared_ptr<client::DreamcastScreen> dreamcastScreen =
        std::make_shared<client::DreamcastScreen>(screenCb, 48, 32);
    subPeripheral1->addFunction(dreamcastScreen);
    
    lcd.initialize();
    lcd.showSplash();

    Clock clock;
    std::shared_ptr<client::DreamcastTimer> dreamcastTimer =
        std::make_shared<client::DreamcastTimer>(clock, setTimeCb, setPwmFn);
    subPeripheral1->addFunction(dreamcastTimer);

    mainPeripheral.addSubPeripheral(subPeripheral1);

    // Second sub peripheral (address of 0x02) with 1 function: vibration
    /*std::shared_ptr<client::DreamcastPeripheral> subPeripheral2 =
        std::make_shared<client::DreamcastPeripheral>(
            0x02,
            0xFF,
            0x00,
            "Puru Puru Pack",
            "Version 1.000,1998/11/10,315-6211-AH   ,Vibration Motor:1 , Fm:4 - 30Hz ,Pow:7",
            20.0,
            160.0);
    std::shared_ptr<client::DreamcastVibration> dreamcastVibration =
        std::make_shared<client::DreamcastVibration>();
    dreamcastVibration->setObserver(get_usb_vibration_observer());
    subPeripheral2->addFunction(dreamcastVibration);
    mainPeripheral.addSubPeripheral(subPeripheral2);*/

    multicore_launch_core1(core1);

    while(true)
    {
        mainPeripheral.task(time_us_64());
        led_task(mem->getLastActivityTime());
    }
}

int main()
{
    stdio_init_all();
    
    led_init();

    display_select();

    core0();

    return 0;
}

#endif
