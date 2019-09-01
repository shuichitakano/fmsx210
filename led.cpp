/*
 * author : Shuichi TAKANO
 * since  : Sat Aug 31 2019 14:34:10
 */

#include "led.h"
#include <fpioa.h>
#include <gpio.h>

namespace
{

enum
{
    R,
    G,
    B,
};

uint8_t gpio_[3];

} // namespace

void initLED(int rPin, int rGPIO,
             int gPin, int gGPIO,
             int bPin, int bGPIO)
{
    gpio_[R] = rGPIO;
    gpio_[G] = gGPIO;
    gpio_[B] = bGPIO;

    fpioa_set_function(rPin, (fpioa_function_t)(FUNC_GPIO0 + rGPIO));
    fpioa_set_function(gPin, (fpioa_function_t)(FUNC_GPIO0 + gGPIO));
    fpioa_set_function(bPin, (fpioa_function_t)(FUNC_GPIO0 + bGPIO));

    for (int i = 0; i < 3; ++i)
    {
        gpio_set_drive_mode(gpio_[i], GPIO_DM_OUTPUT);
        gpio_set_pin(gpio_[i], GPIO_PV_HIGH);
    }
}

void setLED(int i, int on)
{
    gpio_set_pin(gpio_[i], on ? GPIO_PV_LOW : GPIO_PV_HIGH);
}

void setRedLED(int on)
{
    setLED(R, on);
}

void setGreenLED(int on)
{
    setLED(G, on);
}

void setBlueLED(int on)
{
    setLED(B, on);
}

void setRGBLED(int rgbMask)
{
    setLED(R, rgbMask & 1);
    setLED(G, rgbMask & 2);
    setLED(B, rgbMask & 4);
}
