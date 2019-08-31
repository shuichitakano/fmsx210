/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 25 2019 17:33:29
 */
#ifndef AD19B26C_F134_1650_1089_D2BD915E1521
#define AD19B26C_F134_1650_1089_D2BD915E1521

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void initVideo(uint32_t pll0Clock, uint32_t dotClock, int scPerLinex2,
                   int w, int h);

    void startVideoTransfer();
    void setInterlaceMode(int f);

    void waitVBlank();
    void setVideoImagex2(int w, int h, int pitch, const uint16_t *img);
    void setVideoImagex4(int w, int h, int pitch, const uint16_t *img);

#ifdef __cplusplus
}
#endif
#endif /* AD19B26C_F134_1650_1089_D2BD915E1521 */
