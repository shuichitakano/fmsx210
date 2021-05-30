
/*
 * author : Shuichi TAKANO
 * since  : Sat May 29 2021 16:06:36
 */
#ifndef _1FC51C99_5134_634A_F51B_A20C2EA86E06
#define _1FC51C99_5134_634A_F51B_A20C2EA86E06

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _SWSPIConfig
    {
        int pinSS;
        int gpioSS;

        int pinSCLK;
        int gpioSCLK;

        int pinMISO;
        int gpioMISO;
        uint8_t pullupMISO;

        int pinMOSI;
        int gpioMOSI;

    } SWSPIConfig;

    void initSWSPI(const SWSPIConfig *cfg);
    void enableSWSPI(const SWSPIConfig *cfg);
    void disableSWSPI(const SWSPIConfig *cfg);
    int sendByteReversedSWSPI(const SWSPIConfig *cfg, int data);

#ifdef __cplusplus
}
#endif

#endif /* _1FC51C99_5134_634A_F51B_A20C2EA86E06 */
