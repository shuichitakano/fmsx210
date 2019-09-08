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

    int start_fMSX(const char *rom0, const char *rom1,
                   const char *disk0, const char *disk1,
                   int ramPages, int vramPages, int volume);

    void initButton(int i, int pin, int gpio);

#ifdef __cplusplus
}
#endif

#endif /* F9999F32_6134_1650_4001_F01C5FC932B5 */
