/*
 * author : Shuichi TAKANO
 * since  : Thu Aug 29 2019 1:18:1
 */
#ifndef _69B08F22_6134_1654_1202_ECAB715778AD
#define _69B08F22_6134_1654_1202_ECAB715778AD

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef uint64_t clock_t;

    void initChrono();

    clock_t getClockCounter();
    uint32_t clockToMicroSec(clock_t v);

    uint32_t getTickTimeInMicroSec();

#ifdef __cplusplus
}
#endif

#endif /* _69B08F22_6134_1654_1202_ECAB715778AD */
