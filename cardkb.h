/*
 * author : Shuichi TAKANO
 * since  : Thu Jan 28 2021 04:16:13
 */
#ifndef _512D4F1D_B134_61B8_3F6A_45FBF8AEFF53
#define _512D4F1D_B134_61B8_3F6A_45FBF8AEFF53

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void initCardKB(uint8_t i2c1);

    void fetchCardKB();
    void getCardKBKeyState(uint8_t buf[16]);

#ifdef __cplusplus
}
#endif

#endif /* _512D4F1D_B134_61B8_3F6A_45FBF8AEFF53 */
