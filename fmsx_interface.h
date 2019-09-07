/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 18 2019 4:19:45
 */
#ifndef F9999F32_6134_1650_4001_F01C5FC932B5
#define F9999F32_6134_1650_4001_F01C5FC932B5

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        VIDEOOUTMODE_LCD,
        VIDEOOUTMODE_COMPOSITE_VIDEO,
    } VideoOutMode;

    void setFMSXVideoOutMode(VideoOutMode m);

    int start_fMSX();

    void initButton(int i, int pin, int gpio);

#ifdef __cplusplus
}
#endif

#endif /* F9999F32_6134_1650_4001_F01C5FC932B5 */
