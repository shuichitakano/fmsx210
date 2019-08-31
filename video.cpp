/*
 * author : Shuichi TAKANO
 * since  : Mon Aug 12 2019 12:22:58
 */

#include "video.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <bsp.h>

#include "chrono.h"

namespace
{

void setValue(std::vector<uint32_t> &v, int idx, int value)
{
    ((uint8_t *)&v[idx >> 2])[(idx & 3) ^ 3] = value;
}

} // namespace

void NTSCEncoder::makeSinTable(float signalScale)
{
    for (int i = 0; i < SIN_TABLE_SIZE; ++i)
    {
        float t = i * (3.14159265359f * 2 / SIN_TABLE_SIZE);
        sinTable_[i] = {
            int8_t(sinf(t) * 128 / 2.03f * signalScale),
            int8_t(cosf(t) * 128 / 1.14f * signalScale)};
    }
}

void NTSCEncoder::makeLineSinTable()
{
    lineSinTable_.clear();
    lineSinTable_.reserve(width_ * 2);
    for (int i = 0; i < 2; ++i)
    {
        auto ph = phaseStep24_ * (hStart_ - phaseOrigin_) + getSCPhase(i & 1);
        for (int x = 0; x < width_; ++x)
        {
            const auto &sc = sinTable_[(ph >> (24 - SIN_TABLE_BITS)) & (SIN_TABLE_SIZE - 1)];
            lineSinTable_.push_back(sc);
            ph += phaseStep24_;
        }
    }
}

void NTSCEncoder::setSize(int w, int h)
{
    width_ = w;
    height_ = h;
    vStart_ = ((242 - height_) >> 1) + 11;
    hStart_ = (((videoHSamples_ - width_) >> 1) + baseHOfs_ + 2) & ~3;
    printf("(%d x %d) ofs %d, %d\n", w, h, vStart_, hStart_);
    makeLineSinTable();
}

void NTSCEncoder::init(uint32_t freq, int targetNumSCPerLine2)
{
    freq_ = freq;

    auto hCycle = (targetNumSCPerLine2 / (2.0f * 3579545)) * freq;
    signalStride_ = int(hCycle * 0.25 + 0.5) * 4;
    printf("signal freq %d, stride %d\n", freq, signalStride_);
    printf("line sc cycle %f\n", (double)signalStride_ / freq * 3579545);

    signalStrideInDW_ = signalStride_ >> 2;
    halfLineSignalSizeInDW_ = signalStride_ >> 3;

    // 同期信号 -40IRE
    // カラーバースト幅 40IRE
    // 白レベル 100IRE
    // ピーク 133IRE
    auto colorBurstAmp = 256 * 20 / (133 + 40);
    pedestalLevel_ = 256 * 40 / (133 + 40);
    setupLevel_ = pedestalLevel_;
    float signalScale = 100.0f / (133 + 40) * 0.75f; // 75%
    signalScale128_ = signalScale * 128.0f;

    makeSinTable(signalScale);

    auto iv = pedestalLevel_ * 0x01010101;

    vsync_[0].resize(9 * signalStrideInDW_, iv);
    vsync_[1].resize(10 * signalStrideInDW_ - halfLineSignalSizeInDW_, iv);

    const int signalBufferLines = 255;
    video_.resize(signalStrideInDW_ * signalBufferLines, iv);
    printf("vsync %zd+%zdbytes, video %zdbytes.\n", vsync_[0].size() * 4, vsync_[1].size() * 4, video_.size() * 4);

    // 信号全体をフロントポーチ(1.5us)分オフセットして考える
    // 水平同期4.7us
    // 水平同期からカラーバーストまで19sc
    // カラーバースト9sc
    // 水平同期からセットアップまで9.4us
    // 1ライン63.55556us=227.5sc
    // 映像期間63.6-10.9=52.7us
    // 等価パルス幅2.35us
    // 切り込みパルス幅4.7us

    baseHOfs_ = 9.4e-6 * freq_;
    videoHSamples_ = 52.7e-6 * freq_;

    int hsyncPulseWidth = 4.7e-6 * freq_;
    int equivPulseWidth = hsyncPulseWidth >> 1;
    int invSerratioPulseWidth = (signalStride_ >> 1) - hsyncPulseWidth;

    auto makePulse = [&](auto &v, int ofs, int w) {
        for (int i = 0; i < w; ++i)
        {
            setValue(v, i + ofs, 0);
        }
    };
    auto makeVSync = [&](auto &v, int ofs) {
        for (int i = 0; i < 6; ++i)
        {
            makePulse(v, ofs + (signalStride_ * i >> 1), equivPulseWidth);
            makePulse(v, ofs + (signalStride_ * (i + 12) >> 1), equivPulseWidth);
        }
        for (int i = 0; i < 6; ++i)
        {
            makePulse(v, ofs + (signalStride_ * (i + 6) >> 1), invSerratioPulseWidth);
        }
    };

    makeVSync(vsync_[0], 0);
    makeVSync(vsync_[1], (signalStride_ >> 1) - halfLineSignalSizeInDW_ * 4);

#if 0
    // ライン長からPLL想定
    freqSC_ = 227.5 * freq / signalStride_;
#else
    freqSC_ = 3579545;
#endif

    phaseStep24_ = 16777216.0 * freqSC_ / freq;
    phaseOffsetPerLine_ = targetNumSCPerLine2 & 1 ? 16777216 / 2 : 0;

    int colorBurstStart = (19.0f / freqSC_) * freq + 0.5f;
    int colorBurstWidth = (28.0f / freqSC_) * freq - colorBurstStart;
    //    auto colorBurstStartPhase24 = phaseStep24_ * colorBurstStart;

    phaseOrigin_ = colorBurstStart;
    // phaseOrigin_ = 0;

    printf("colorBurst %d, %d\n", colorBurstStart, colorBurstWidth);

    auto makeHSync = [&](int ofs, bool phase) {
        makePulse(video_, ofs, hsyncPulseWidth);

        ofs += colorBurstStart;
        //        auto ph = colorBurstStartPhase24 + getSCPhase(phase);
        auto ph = getSCPhase(phase); // カラーバーストを位相の起点にする
        //auto ph = phaseStep24_ * ofs;
        for (int i = 0; i < colorBurstWidth; ++i)
        {
            float phase = (ph & 16777215) * (3.14159265f * 2 / 16777216.0f);
            int value = -sinf(phase) * colorBurstAmp;
            setValue(video_, ofs, value + pedestalLevel_);
            ++ofs;
            ph += phaseStep24_;
        }
    };

    for (int i = 0; i < signalBufferLines; ++i)
    {
        makeHSync(i * signalStride_, i & 1);
    }
}

void NTSCEncoder::makeColorBar()
{
    // 75% white, yellow, cyan, green, megenta, red, blue, black

    auto makeLine = [&](int ofs, bool odd) {
        ofs += hStart_;
        auto ph = phaseStep24_ * (hStart_ - phaseOrigin_) + getSCPhase(odd);
        //auto ph = phaseStep24_ * ofs;
        auto unitWidth = width_ >> 3;

        uint8_t col[] = {7, 3, 6, 2, 5, 1, 4, 0};

        for (int i = 0; i < 8; ++i)
        {
            auto cm = col[i];
            float r = cm & 1 ? signalScale128_ * 2 : 0;
            float g = cm & 2 ? signalScale128_ * 2 : 0;
            float b = cm & 4 ? signalScale128_ * 2 : 0;

            for (int j = 0; j < unitWidth; ++j)
            {
                float phase = (ph & 16777215) * (3.14159265f * 2 / 16777216.0f);
                float sp = sinf(phase);
                float cp = cosf(phase);

                // Y = 0.587G+0.114B+0.299R
                // Cb ＝ B-Y = -0.587G+0.886B－0.299R
                // Cr ＝ R-Y = -0.587G－0.114B＋0.701R
                // Cb' = Cb/2.03
                // Cr' = Cr/1.14
                // NTSC = Y + Cb' sin(2 pi f t) +Cr' cos(2 pi f t)

                float y = 0.587f * g + 0.114f * b + 0.299 * r;
                float cb = (b - y) * (1 / 2.03f);
                float cr = (r - y) * (1 / 1.14f);
                int v = y + cb * sp + cr * cp;
                setValue(video_, ofs, v + pedestalLevel_);

                ofs += 1;
                ph += phaseStep24_;
            }
        }
    };

    int ofs = 0;
    for (int i = 0; i < 255; ++i)
    {
        makeLine(ofs, i & 1);
        ofs += signalStride_;
    }
}

void NTSCEncoder::makeColotLUT(int rBits, int gBits, int bBits,
                               int rShift, int gShift, int bShift)
{
    int bpp = rBits + gBits + bBits;
    int ct = 1 << bpp;
    colorLUT_.resize(ct);

    int rMax = 1 << rBits;
    int gMax = 1 << gBits;
    int bMax = 1 << bBits;
    int rMask = (rMax - 1) << rShift;
    int gMask = (gMax - 1) << gShift;
    int bMask = (bMax - 1) << bShift;
    float rNorm = 1.0f / rMask;
    float gNorm = 1.0f / gMask;
    float bNorm = 1.0f / bMask;

    float yscale = signalScale128_ * (1 << 7);
    float ybias = pedestalLevel_ << 6;

    for (int i = 0; i < ct; ++i)
    {
        float r = (i & rMask) * rNorm;
        float g = (i & gMask) * gNorm;
        float b = (i & bMask) * bNorm;

        float y = 0.299f * r + 0.587f * g + 0.114f * b;
        float cb = b - y;
        float cr = r - y;

        colorLUT_[i] = DiffColor{(uint16_t)(y * yscale + ybias),
                                 (int8_t)(cb * (1 << 7)),
                                 (int8_t)(cr * (1 << 7))};
    }
}

#if 1


void NTSCEncoder::setLinex2(int w, int line, const uint16_t *img)
{
    line += vStart_;
    int ofs = hStart_ + line * signalStride_;
    assert((ofs & 3) == 0);
    auto *p = (uint8_t *)&video_[ofs >> 2];
    auto *sct = getLineSinTable(line);

    auto ct = w;
    while (ct > 0)
    {
        auto c1 = colorLUT_[img[0]];
        auto c2 = colorLUT_[img[1]];

        int v0 = c1.compute(sct[0]);
        int v1 = c1.compute(sct[1]);
        int v2 = c2.compute(sct[2]);
        int v3 = c2.compute(sct[3]);

        p[0] = v3;
        p[1] = v2;
        p[2] = v1;
        p[3] = v0;
        p += 4;
        sct += 4;
        img += 2;
        ct -= 2;
    }
}

void NTSCEncoder::setLinex4(int w, int line, const uint16_t *img)
{
    line += vStart_;
    int ofs = hStart_ + line * signalStride_;
    assert((ofs & 3) == 0);
    auto *p = (uint8_t *)&video_[ofs >> 2];
    auto *sct = getLineSinTable(line);

    auto ct = w;
    while (ct > 0)
    {
        auto c = colorLUT_[img[0]];

        int v0 = c.compute(sct[0]);
        int v1 = c.compute(sct[1]);
        int v2 = c.compute(sct[2]);
        int v3 = c.compute(sct[3]);

        p[0] = v3;
        p[1] = v2;
        p[2] = v1;
        p[3] = v0;
        p += 4;
        sct += 4;
        img += 1;
        ct -= 1;
    }
}

#else

namespace
{

struct YCbCr
{
    int y;  // 8:20
    int cb; // 8:13
    int cr; // 8:13

    inline YCbCr(uint16_t src, int ss128)
    {
        static constexpr int rcoef = int(0.299f * 2048) << 5;
        static constexpr int gcoef = int(0.587f * 1024);
        static constexpr int bcoef = int(0.114f * 2048) << 5;

        int r = src >> 11;
        int g = src & (63 << 5);
        int b = src & 31;
        y = rcoef * r + gcoef * g + bcoef * b;
        cb = (b << 16) - y;
        cr = (r << 16) - y;
        y *= ss128;
    }

    inline int compute(const NTSCEncoder::SinCos &sc) const
    {
        return (y + cb * sc.sin_ + cr * sc.cos_) >> 20;
    }
};

} // namespace

void NTSCEncoder::setLinex2(int w, int line, const uint16_t *img)
{
    line += vStart_;
    int ofs = hStart_ + line * signalStride_;
    assert((ofs & 3) == 0);
    auto *p = (uint8_t *)&video_[ofs >> 2];
    auto *sct = getLineSinTable(line);

    auto ct = w;
    while (ct > 0)
    {
        YCbCr c1(img[0], signalScale128_);
        YCbCr c2(img[1], signalScale128_);

        int v0 = c1.compute(sct[0]) + pedestalLevel_;
        int v1 = c1.compute(sct[1]) + pedestalLevel_;
        int v2 = c2.compute(sct[2]) + pedestalLevel_;
        int v3 = c2.compute(sct[3]) + pedestalLevel_;

        p[0] = v3;
        p[1] = v2;
        p[2] = v1;
        p[3] = v0;
        p += 4;
        sct += 4;
        img += 2;
        ct -= 2;
    }
}

void NTSCEncoder::setLinex4(int w, int line, const uint16_t *img)
{
    line += vStart_;
    int ofs = hStart_ + line * signalStride_;
    assert((ofs & 3) == 0);
    auto *p = &video_[ofs >> 2];
    auto *sct = getLineSinTable(line);

    auto ct = w;
    while (ct)
    {
        YCbCr c(*img, signalScale128_);

        int v0 = c.compute(sct[0]) + pedestalLevel_;
        int v1 = c.compute(sct[1]) + pedestalLevel_;
        int v2 = c.compute(sct[2]) + pedestalLevel_;
        int v3 = c.compute(sct[3]) + pedestalLevel_;

        *p = (v0 << 24) | (v1 << 16) | (v2 << 8) | v3;

        ++p;
        sct += 4;
        ++img;
        --ct;
    }
}
#endif

namespace
{
using WorkerFunc = void (*)(void *);

void *volatile workerParam_ = nullptr;
volatile WorkerFunc workerFunc_ = nullptr;

} // namespace

int worker(void *)
{
    while (1)
    {
        if (workerFunc_)
        {
            workerFunc_(workerParam_);
            workerFunc_ = nullptr;
        }
    }
}

void setWorkload(WorkerFunc f, void *param)
{
    while (workerFunc_)
        ;
    workerParam_ = param;
    workerFunc_ = f;
}

void startWorker()
{
    register_core1(worker, nullptr);
}

namespace
{

template <class F>
void setImageImpl(int w, int h, int pitch, const uint16_t *img, int lineOffset, const F &lineFunc)
{
#if 0
    int line = lineOffset;
    for (int i = 0; i < h; ++i)
    {
        setLinex4(w, line, img);
        ++line;
        img += pitch;
    }
#else
    struct Param
    {
        const F &func_;
        int w, h, pitch, lineOffset;
        const uint16_t *img;
        volatile bool done = false;

        int proc()
        {
//            auto prevClk = getClockCounter();

            int line = lineOffset;
            for (int i = 0; i < h; i += 2)
            {
                func_(w, line, img);
                line += 2;
                img += pitch * 2;
            }
            done = true;

//            auto dt = clockToMicroSec(getClockCounter() - prevClk);
//            printf("core %d: %dus\n", (int)current_coreid(), dt);

            return 0;
        }
    };

    auto p0 = Param{lineFunc, w, h, pitch, lineOffset + 0, img + pitch * 0};
    auto p1 = Param{lineFunc, w, h, pitch, lineOffset + 1, img + pitch * 1};

    setWorkload([](void *p) { ((Param *)p)->proc(); }, &p1);
    p0.proc();
    while (!p1.done)
        ;
#endif
}

} // namespace

void NTSCEncoder::setImagex2(int w, int h, int pitch, const uint16_t *img,
                             int lineOffset)
{
    setImageImpl(w, h, pitch, img, lineOffset,
                 [&](int w, int line, const uint16_t *img) { setLinex2(w, line, img); });
}

void NTSCEncoder::setImagex4(int w, int h, int pitch, const uint16_t *img,
                             int lineOffset)
{
    setImageImpl(w, h, pitch, img, lineOffset,
                 [&](int w, int line, const uint16_t *img) { setLinex4(w, line, img); });
}
