/*
 * author : Shuichi TAKANO
 * since  : Fri Aug 23 2019 3:6:13
 */

#include "audio.h"
#include <i2s.h>
#include <fpioa.h>
#include <algorithm>
#include <assert.h>

namespace
{

constexpr int SIZE = 4096;
constexpr int UNIT_SIZE = SIZE / 8;

struct Buffer
{
    alignas(4) int16_t ring_[SIZE];

    volatile int wp_ = 0;
    volatile int rp_ = 0;

public:
    int getFree() const
    {
        if (wp_ >= rp_)
        {
            return SIZE - wp_ + rp_ - 1;
        }
        else
        {
            return rp_ - wp_ - 1;
        }
    }

    int getSize() const
    {
        if (wp_ >= rp_)
        {
            return wp_ - rp_;
        }
        else
        {
            return SIZE - rp_ + wp_;
        }
    }

    void write(const int16_t *p, int size)
    {
        auto s = std::min(size, SIZE - wp_);
        memcpy(&ring_[wp_], p, s * sizeof(int16_t));
        p += s;
        s = size - s;
        memcpy(&ring_[0], p, s * sizeof(int16_t));
        wp_ = (wp_ + size) & (SIZE - 1);
    }

    void writeMonoToStereo(const int16_t *p, int size)
    {
        assert((wp_ & 1) == 0);
        auto copy = [](uint32_t *dst, const int16_t *&src, int n) {
            while (n)
            {
                auto v = (uint16_t)*src++;
                *dst++ = v | (v << 16);
                --n;
            }
        };

        size <<= 1;
        auto s = std::min(size, SIZE - wp_);
        copy((uint32_t *)&ring_[wp_], p, s >> 1);
        s = size - s;
        copy((uint32_t *)&ring_[0], p, s >> 1);
        wp_ = (wp_ + size) & (SIZE - 1);
    }

    const int16_t *getReadPointer() const
    {
        return &ring_[rp_];
    }

    void advanceReadPointer(int size)
    {
        rp_ = (rp_ + size) & (SIZE - 1);
    }
};

Buffer buffer_;

volatile bool active_ = false;

void play()
{
    auto size = buffer_.getSize();
    if (size >= UNIT_SIZE)
    {
        i2s_data_t data;
        data.tx_channel = DMAC_CHANNEL0;
        data.tx_buf = (uint32_t *)buffer_.getReadPointer();
        data.tx_len = UNIT_SIZE / 2;
        data.transfer_mode = I2S_SEND;

        plic_interrupt_t irq;
        irq.callback = [](void *) -> int {
            buffer_.advanceReadPointer(UNIT_SIZE);
            play();
            return 0;
        };
        irq.priority = 2;
        active_ = true;
        i2s_handle_data_dma(I2S_DEVICE_0, data, &irq);
    }
    else
    {
        active_ = false;
    }
}

} // namespace

int getAudioBufferFree()
{
    return buffer_.getFree() >> 1;
}

void writeAudioData(const int16_t *p, int count)
{
    buffer_.writeMonoToStereo(p, count);
    if (!active_)
    {
        play();
    }
}

void initAudio(uint32_t freq)
{
    fpioa_set_function(34, FUNC_I2S0_OUT_D0);
    fpioa_set_function(35, FUNC_I2S0_SCLK);
    fpioa_set_function(33, FUNC_I2S0_WS);

    i2s_init(I2S_DEVICE_0, I2S_TRANSMITTER, 3);

    i2s_tx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_0,
                          RESOLUTION_16_BIT, SCLK_CYCLES_32,
                          TRIGGER_LEVEL_4,
                          RIGHT_JUSTIFYING_MODE);

    i2s_set_sample_rate(I2S_DEVICE_0, freq);
    i2s_set_dma_divide_16(I2S_DEVICE_0, true);
}
