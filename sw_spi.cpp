/*
 * author : Shuichi TAKANO
 * since  : Sat May 29 2021 16:08:45
 */

#include "sw_spi.h"
#include <fpioa.h>
#include <gpiohs.h>
#include <sleep.h>

void initSWSPI(const SWSPIConfig *cfg)
{
    auto makeFPIOAFunc = [](int i) {
        return static_cast<fpioa_function_t>(FUNC_GPIOHS0 + i);
    };

    fpioa_set_function(cfg->pinSCLK, makeFPIOAFunc(cfg->gpioSCLK));
    fpioa_set_function(cfg->pinSS, makeFPIOAFunc(cfg->gpioSS));
    fpioa_set_function(cfg->pinMISO, makeFPIOAFunc(cfg->gpioMISO));
    fpioa_set_function(cfg->pinMOSI, makeFPIOAFunc(cfg->gpioMOSI));

    gpiohs_set_drive_mode(cfg->gpioSCLK, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(cfg->gpioSS, GPIO_DM_OUTPUT);
    gpiohs_set_drive_mode(cfg->gpioMISO,
                          cfg->pullupMISO ? GPIO_DM_INPUT_PULL_UP : GPIO_DM_INPUT);
    gpiohs_set_drive_mode(cfg->gpioMOSI, GPIO_DM_OUTPUT);

    disableSWSPI(cfg);
    gpiohs_set_pin(cfg->gpioSCLK, GPIO_PV_HIGH); // mode3

    //    fpioa_set_io_driving(cfg->pinMISO, FPIOA_DRIVING_2);
}

void enableSWSPI(const SWSPIConfig *cfg)
{
    gpiohs_set_pin(cfg->gpioSS, GPIO_PV_LOW);
}

void disableSWSPI(const SWSPIConfig *cfg)
{
    gpiohs_set_pin(cfg->gpioSS, GPIO_PV_HIGH);
}

int sendByteReversedSWSPI(const SWSPIConfig *cfg, int data)
{
    int r = 0;
    for (int i = 0; i < 8; ++i)
    {
        gpiohs_set_pin(cfg->gpioMOSI,
                       static_cast<gpio_pin_value_t>(data & 1));
        data >>= 1;

        gpiohs_set_pin(cfg->gpioSCLK, GPIO_PV_LOW);
        usleep(5);

        r |= gpiohs_get_pin(cfg->gpioMISO) << i;
        gpiohs_set_pin(cfg->gpioSCLK, GPIO_PV_HIGH);
        usleep(5);
    }
    //    gpiohs_set_pin(cfg->gpioMOSI, GPIO_PV_HIGH);
    usleep(5);
    return r;
}
