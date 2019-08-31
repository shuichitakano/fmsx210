/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 25 2019 17:34:2
 */

#include "video_out.h"
#include "video.h"
#include "spi_dma.h"
#include <sysctl.h>

namespace
{

NTSCEncoder NTSC_;

enum class Stage
{
    FIELD1_VSYNC = 0,
    FIELD1_VIDEO,
    FIELD2_VSYNC,
    FIELD2_VIDEO,
    FIELD3_VSYNC,
    FIELD3_VIDEO,
    FIELD4_VSYNC,
    FIELD4_VIDEO,
};

Stage nextStage_{};
bool isInterlace_ = false;

volatile bool vblanking_ = false;
volatile int writeLineOffset_ = 0;

#if 1

const uint32_t *nextTransferData_ = nullptr;
uint32_t nextTransferSize_ = 0;

void transfer()
{
    auto &dma = SPIDMA::instance();
    if (nextTransferData_)
    {
        dma._transferBE(nextTransferData_, nextTransferSize_);
    }
    else
    {
        dma._transferBE(&nextTransferSize_, 1); // dummy
    }

    bool is4Phase = NTSC_.is4Phase();
    int field = (int)nextStage_ >> 1;
    bool vsync = (((int)nextStage_) & 1) == 0;
    bool oddField = field & 1;
    bool phase = is4Phase ? ((int)nextStage_ + 1) >> 1 : 0;

    nextStage_ = (Stage)(((int)nextStage_ + 1) & 7);
    vblanking_ = !vsync; // 次の転送単位なので反転
    writeLineOffset_ = NTSC_.getWriteLineOffset(phase);

    bool interlaced = isInterlace_ || is4Phase;

    if (vsync)
    {
        auto &v = NTSC_.getVSyncBuffer(oddField && interlaced);
        nextTransferData_ = v.data();
        nextTransferSize_ = v.size();
    }
    else
    {
        auto &v = NTSC_.getVideoBuffer();
        if (interlaced)
        {
            nextTransferData_ = v.data() + NTSC_.getVideoOffset(phase);
            nextTransferSize_ = NTSC_.getVideoDataSize(oddField);
        }
        else
        {
            nextTransferData_ = v.data();
            nextTransferSize_ = NTSC_.getVideoDataSizeNonInterlace();
        }
    }
}
#else
void transfer()
{
    bool is4Phase = NTSC_.is4Phase();
    auto &dma = SPIDMA::instance();
    int field = (int)nextStage_ >> 1;
    bool vsync = (((int)nextStage_) & 1) == 0;
    bool oddField = field & 1;
    bool phase = is4Phase ? ((int)nextStage_ + 1) >> 1 : 0;

    nextStage_ = (Stage)(((int)nextStage_ + 1) & 7);
    vblanking_ = vsync;
    writeLineOffset_ = NTSC_.getWriteLineOffset(phase);

    bool interlaced = isInterlace_ || is4Phase;

    if (vsync)
    {
        auto &v = NTSC_.getVSyncBuffer(oddField && interlaced);
        dma._transferBE(v.data(), v.size());
    }
    else
    {
        auto &v = NTSC_.getVideoBuffer();
        if (interlaced)
        {
            dma._transferBE(v.data() + NTSC_.getVideoOffset(phase),
                            NTSC_.getVideoDataSize(oddField));
        }
        else
        {
            dma._transferBE(v.data(),
                            NTSC_.getVideoDataSizeNonInterlace());
        }
    }
}
#endif

} // namespace

void initVideo(uint32_t pll0Clock, uint32_t dotClock, int scPerLinex2,
               int w, int h)
{
    sysctl_pll_set_freq(SYSCTL_PLL0, pll0Clock);
    auto realFreq = spi_set_clk_rate(SPIDMA::instance().getSPINum(),
                                     dotClock);

    NTSC_.init(realFreq, scPerLinex2);
    NTSC_.setSize(w, h);
    //    NTSC_.makeColorBar();

    NTSC_.makeColotLUT(5, 6, 5, 11, 5, 0);

    startWorker(); // かり
}

void startVideoTransfer()
{
    SPIDMA::instance().setCallback([&] {
        transfer();
    });

    SPIDMA::instance()._setupTransfer();

    nextStage_ = Stage::FIELD1_VSYNC;
    transfer();
}

void setInterlaceMode(int f)
{
    isInterlace_ = f;
}

void waitVBlank()
{
    while (!vblanking_)
        ;
}

void setVideoImagex2(int w, int h, int pitch, const uint16_t *img)
{
    NTSC_.setImagex2(w, h, pitch, img, writeLineOffset_);
}

void setVideoImagex4(int w, int h, int pitch, const uint16_t *img)
{
    NTSC_.setImagex4(w, h, pitch, img, writeLineOffset_);
}
