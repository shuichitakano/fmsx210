/*
 * author : Shuichi TAKANO
 * since  : Thu Jan 28 2021 04:15:36
 */

#include "cardkb.h"
#include <i2c.h>
#include <stdio.h>

namespace
{
    uint8_t lastPressed_ = 0;
    int counter_ = 0;
    bool enabled_ = false;
    i2c_device_number_t device_ = I2C_DEVICE_0;
} // namespace

void initCardKB(uint8_t i2c1)
{
    printf("initCardKB: i2c %d\n", i2c1);
    device_ = i2c1 ? I2C_DEVICE_1 : I2C_DEVICE_0;
    enabled_ = true;
}

void fetchCardKB()
{
    if (!enabled_)
    {
        return;
    }

    static constexpr int CARDKB_ADDR = 0x5F;

    i2c_init(device_, CARDKB_ADDR, 7, 100000);

    uint8_t ch = 0;
    i2c_recv_data(device_, 0, 0, &ch, 1);
    if (ch)
    {
        //        printf("ch = %02x '%c'\n", ch, ch);
        lastPressed_ = ch;
        counter_ = 5;
    }
    else if (counter_)
    {
        --counter_;
    }
}

void getCardKBKeyState(uint8_t buf[16])
{
    if (!enabled_)
    {
        return;
    }

    if (counter_)
    {
        static char map[] = {
            '0', '1', '2', '3', '4', '5', '6', '7',  // 0
            '8', '9', '-', '^', '\\', '@', '[', ';', // 1
            ':', ']', ',', '.', '/', '_', 'a', 'b',  // 2
            'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',  // 3
            'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',  // 4
            's', 't', 'u', 'v', 'w', 'x', 'y', 'z',  // 5
            0x8c /* fn+tab */,                       // shift
            0x80 /* fn+esc */,                       // ctrl
            0x9e /* fn+g */,                         // graph
            0xa8 /* fn+c */,                         // caps
            0xa1 /* fn+k */,                         // kana
            0x81 /* fn+1 */,                         // F1
            0x82 /* fn+2 */,                         // F2
            0x83 /* fn+3 */,                         // F3
            0x84 /* fn+4 */,                         // F4
            0x85 /* fn+5 */,                         // F5
            0x1b,                                    // ESC
            0x09,                                    // TAB
            0x9b /* fn+s */,                         // STOP
            0x08,                                    // BS
            0x8F /* fn+e */,                         // SELECT
            0x0d,                                    // RET
            ' ',
            0x9f /* fn+h */,   // HOME
            0x94 /* fn+i */,   // INS
            0x8b /* fn+del */, // DEL
            0xb4,              // LEFT
            0xb5,              // UP
            0xb6,              // DOWN
            0xb7,              // RIGHT
        };

        int i = 0;
        for (auto ch : map)
        {
            if (ch == lastPressed_)
            {
                buf[i >> 3] &= ~(1 << (i & 7));
            }
            ++i;
        }
    }
}
