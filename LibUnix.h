/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 18 2019 15:59:22
 */
#ifndef F07CE34F_E134_1650_34D8_858AB18CC88B
#define F07CE34F_E134_1650_34D8_858AB18CC88B

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
    } XImage;

#define SND_CHANNELS 16 /* Number of sound channels   */
#define SND_BITS 8
#define SND_BUFSIZE (1 << SND_BITS)

#define PIXEL(R, G, B) (pixel)(((32 * (R) >> 8) << 11) | ((64 * (G) >> 8) << 5) | (32 * (B) >> 8))
#define PIXEL2MONO(P) (522 * (((P)&31) + (((P) >> 5) & 63) + (((P) >> 11) & 31)) >> 8)
#define RMASK 0xF800
#define GMASK 0x07E0
#define BMASK 0x001F

    unsigned int InitAudio(unsigned int Rate, unsigned int Latency);
    void TrashAudio(void);
    //int PauseAudio(int Switch);

#ifdef __cplusplus
}
#endif

#endif /* F07CE34F_E134_1650_34D8_858AB18CC88B */
