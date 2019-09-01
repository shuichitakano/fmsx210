/*
 * author : Shuichi TAKANO
 * since  : Sat Aug 31 2019 14:33:44
 */
#ifndef _08FDF64F_5134_1656_DABA_EFBCFA515716
#define _08FDF64F_5134_1656_DABA_EFBCFA515716

#ifdef __cplusplus
extern "C"
{
#endif

    void initLED(int rPin, int rGPIO,
                 int gPin, int gGPIO,
                 int bPin, int bGPIO);

    void setLED(int i, int on);

    void setRedLED(int on);
    void setGreenLED(int on);
    void setBlueLED(int on);

    void setRGBLED(int rgbMask);

#define LEDCOLOR_OFF 0
#define LEDCOLOR_RED 1
#define LEDCOLOR_GREEN 2
#define LEDCOLOR_YELLOW 3
#define LEDCOLOR_BLUE 4
#define LEDCOLOR_MAGENTA 5
#define LEDCOLOR_CYAN 6
#define LEDCOLOR_WHITE 7

#ifdef __cplusplus
}
#endif

#endif /* _08FDF64F_5134_1656_DABA_EFBCFA515716 */
