/*
 * author : Shuichi TAKANO
 * since  : Mon May 10 2021 02:08:32
 */
#ifndef FB510269_E134_6345_1FC8_59DD08368B8D
#define FB510269_E134_6345_1FC8_59DD08368B8D

#include <stdint.h>
#include "sw_spi.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void initPSPad(const SWSPIConfig *config);
    void updatePSPad();

    uint8_t isPSPADDetected();
    int makeJoyStateFromPSPad();
    void getPSPADKeyState(uint8_t buf[16]);

#ifdef __cplusplus
}
#endif

#endif /* FB510269_E134_6345_1FC8_59DD08368B8D */
