/*
 * author : Shuichi TAKANO
 * since  : Mon May 10 2021 02:09:26
 */

#include "pspad.h"
#include <stdio.h>
#include <sleep.h>

namespace
{
    SWSPIConfig spi_;
    bool enabled_ = false;
    bool detected_ = false;

    enum PSPAD_BIT
    {
        SELECT = 0,
        START = 3,
        UP = 4,
        RIGHT = 5,
        DOWN = 6,
        LEFT = 7,
        L2 = 8,
        R2 = 9,
        L1 = 10,
        R1 = 11,
        TRIANGE = 12,
        CIRCLE = 13,
        CROSS = 14,
        AQUARE = 15,
    };

    uint16_t prevButton_ = 0;
    uint16_t currentButton_ = 0;
}

void initPSPad(const SWSPIConfig *config)
{
    if (config)
    {
        spi_ = *config;

        initSWSPI(&spi_);
        enabled_ = true;
    }
    else
    {
        enabled_ = false;
    }
}

void updatePSPad()
{
    prevButton_ = currentButton_;
    if (!enabled_)
    {
        detected_ = false;
        return;
    }

    static constexpr uint8_t cmd[] = {1, 0x42, 0, 0, 0};
    int rbuf[sizeof(cmd)];
    enableSWSPI(&spi_);
    usleep(10);
    {
        for (int i = 0; i < sizeof(cmd); ++i)
        {
            rbuf[i] = sendByteReversedSWSPI(&spi_, cmd[i]);
        }
    }
    disableSWSPI(&spi_);
    // printf("rx: (%02x %02x %02x %02x %02x)\n",
    //        rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4]);

    if (rbuf[2] != 0x5a)
    {
        currentButton_ = 0;
        detected_ = false;
    }
    else
    {
        currentButton_ = ~(rbuf[3] | (rbuf[4] << 8));
        detected_ = true;
    }
}

uint8_t isPSPADDetected()
{
    return detected_;
}

int makeJoyStateFromPSPad()
{
    if (!enabled_ || !detected_)
    {
        return 0;
    }

    static constexpr PSPAD_BIT bits[] = {
        PSPAD_BIT::CROSS,
        PSPAD_BIT::CIRCLE,
        PSPAD_BIT::RIGHT,
        PSPAD_BIT::LEFT,
        PSPAD_BIT::DOWN,
        PSPAD_BIT::UP,
    };

    int r = 0;
    for (auto b : bits)
    {
        r <<= 1;
        if (currentButton_ & (1 << static_cast<int>(b)))
        {
            r |= 1;
        }
    }
    // printf("%02x : %02x\n", currentButton_, r);
    return r;
}

void getPSPADKeyState(uint8_t buf[16])
{
    if (!enabled_ || !detected_)
    {
        return;
    }

    struct Entry
    {
        PSPAD_BIT padBit;
        int row;
        int bit;
    };
    static constexpr Entry entries[] = {
        {PSPAD_BIT::START, 7, 4},
        {PSPAD_BIT::SELECT, 7, 6},
        {PSPAD_BIT::L1, 6, 5},
        {PSPAD_BIT::R1, 7, 1},
    };

    for (auto &e : entries)
    {
        if (currentButton_ & (1 << static_cast<int>(e.padBit)))
        {
            buf[e.row] &= ~(1 << e.bit);
        }
    }
}
