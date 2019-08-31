/*
 * author : Shuichi TAKANO
 * since  : Mon Aug 12 2019 12:15:45
 */
#ifndef _59D6FE19_2134_1651_B97A_A42DFABF74CA
#define _59D6FE19_2134_1651_B97A_A42DFABF74CA

#include <stdint.h>
#include <utility>
#include <vector>

class NTSCEncoder
{
public:
    struct SinCos
    {
        int8_t sin_;
        int8_t cos_;
    };

public:
    void init(uint32_t freq, int targetNumSCPerLine2 = 455);

    void setSize(int w, int h);
    void makeColotLUT(int rBits, int gBits, int bBits,
                      int rShift, int gShift, int bShift);

    void makeColorBar();

    const std::vector<uint32_t> &getVSyncBuffer(bool odd) const
    {
        return vsync_[odd];
    }

    std::vector<uint32_t> &getVideoBuffer()
    {
        return video_;
    }

    int getVideoDataSize(bool odd) const
    {
        return signalStrideInDW_ * 253 + (odd ? 0 : halfLineSignalSizeInDW_);
    }

    int getVideoOffset(bool phase) const
    {
        return phase ? signalStrideInDW_ : 0;
    }

    int getWriteLineOffset(bool phase) const
    {
        return phase ? 1 : 0;
    }

    int getVideoDataSizeNonInterlace(int lines = 262) const
    {
        return signalStrideInDW_ * (lines - 9);
    }

    uint32_t
    getSCPhase(bool odd) const
    {
        return odd ? phaseOffsetPerLine_ : 0;
    }

    bool is4Phase() const
    {
        return phaseOffsetPerLine_;
    }

    void setLinex2(int w, int line, const uint16_t *img);
    void setLinex4(int w, int line, const uint16_t *img);

    void setImagex2(int w, int h, int pitch, const uint16_t *img,
                    int lineOffset);
    void setImagex4(int w, int h, int pitch, const uint16_t *img,
                    int lineOffset);

protected:
    void makeSinTable(float signalScale);
    void makeLineSinTable();

    const SinCos *getLineSinTable(int line)
    {
        return &lineSinTable_[width_ * (line & 1)];
    }

private:
    std::vector<uint32_t> vsync_[2];
    std::vector<uint32_t> video_;

    uint32_t freq_ = 0;
    float freqSC_ = 0;
    int signalStride_ = 0;
    int signalStrideInDW_ = 0;
    int halfLineSignalSizeInDW_ = 0;
    uint32_t phaseStep24_ = 0;
    uint32_t phaseOffsetPerLine_ = 0;
    int phaseOrigin_ = 0;
    int baseHOfs_ = 0;
    int videoHSamples_ = 0;

    int pedestalLevel_ = 0;
    int setupLevel_ = 0;
    int colorBurstAmp_ = 0;
    int signalScale128_ = 0;

    int width_ = 0;
    int height_ = 0;
    int vStart_ = 0;
    int hStart_ = 0;

    static constexpr int SIN_TABLE_BITS = 8;
    static constexpr int SIN_TABLE_SIZE = 1 << SIN_TABLE_BITS;
    SinCos sinTable_[SIN_TABLE_SIZE];

    std::vector<SinCos> lineSinTable_; // x2 line

    struct DiffColor
    {
        uint16_t y;
        int8_t cb, cr;

        inline int compute(const SinCos& sc) const
        {
            return (y + cb * sc.sin_ + cr * sc.cos_) >> 6;
        }
    };
    std::vector<DiffColor> colorLUT_;
};

void startWorker();

#endif /* _59D6FE19_2134_1651_B97A_A42DFABF74CA */