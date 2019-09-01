/*
 * author : Shuichi TAKANO
 * since  : Sat Aug 31 2019 13:46:18
 */

#include "ps2_keyboard.h"

#include <fpioa.h>
#include <bsp.h>

PS2Keyboard &
PS2Keyboard::instance()
{
    static PS2Keyboard inst;
    return inst;
}

void PS2Keyboard::init(int clkPin, int clkGPIOHS,
                       int datPin, int datGPIOHS,
                       int prio)
{
    clkGPIO_ = clkGPIOHS;
    datGPIO_ = datGPIOHS;
    prio_ = prio;

    fpioa_set_function(clkPin, (fpioa_function_t)(FUNC_GPIOHS0 + clkGPIOHS));
    fpioa_set_function(datPin, (fpioa_function_t)(FUNC_GPIOHS0 + datGPIOHS));

    gpiohs_set_drive_mode(clkGPIO_, GPIO_DM_INPUT);
    gpiohs_set_drive_mode(datGPIO_, GPIO_DM_INPUT);
    gpiohs_set_pin(clkGPIO_, GPIO_PV_LOW);
    gpiohs_set_pin(datGPIO_, GPIO_PV_LOW);
    // Open drainにできないので L しか出してはいけない

    buffer_ = 0;
    counter_ = 0;
    parity_ = 0;
    f0_ = false;
    e0_ = false;

    gpiohs_set_pin_edge(clkGPIO_, GPIO_PE_FALLING);
}

void PS2Keyboard::start()
{
    printf("start kb\n");
    gpiohs_irq_register(clkGPIO_, prio_, [](void *p) -> int {
        ((PS2Keyboard *)p)->callback();
        return 0;
    },
                        this);
}

void PS2Keyboard::callback()
{
    auto v = gpiohs_get_pin(datGPIO_);
    //    printf("%d", v);

    parity_ ^= buffer_ & 1;
    parity_ ^= v;
    buffer_ = (buffer_ >> 1) | (v << 10);
    ++counter_;

    if (counter_ >= 11)
    {
        if ((buffer_ & 0b10000000001) == 0b10000000000 && !parity_)
        {
            int data = (buffer_ >> 1) & 0xff;
            //            printf("%02x\n", data);

            if (data == 0xf0)
            {
                f0_ = true;
            }
            else if (data == 0xe0)
            {
                e0_ = true;
            }
            else
            {
                int code = data + (e0_ ? 256 : 0);
                if (f0_)
                {
                    // release
                    pushed_[code >> 5] &= ~(1 << (code & 31));
                }
                else
                {
                    // push
                    pushed_[code >> 5] |= 1 << (code & 31);
                }
                f0_ = false;
                e0_ = false;
            }
            counter_ = 0;
        }
        else
        {
            counter_ = 11;
        }
    }
}

int isPS2KeyboardPushed(int code, int e0)
{
    return PS2Keyboard::instance().isPushed(code, e0);
}
