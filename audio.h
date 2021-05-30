/*
 * author : Shuichi TAKANO
 * since  : Fri Aug 23 2019 3:6:52
 */
#ifndef AC813F38_5134_1655_2EC6_9866A27CE5EB
#define AC813F38_5134_1655_2EC6_9866A27CE5EB

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int getAudioBufferFree();
    void writeAudioData(const int16_t *p, int count);
    void initAudio(uint32_t freq, uint8_t amigo);

#ifdef __cplusplus
}
#endif

#endif /* AC813F38_5134_1655_2EC6_9866A27CE5EB */
