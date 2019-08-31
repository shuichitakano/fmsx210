/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 25 2019 15:39:25
 */
#ifndef E010AF3D_A134_1650_EAE0_1F6097E2920D
#define E010AF3D_A134_1650_EAE0_1F6097E2920D

#include <spi.h>
#include <functional>

class SPIDMA
{
    spi_device_num_t spiNum_{};
    volatile spi_t *spi_{};
    spi_chip_select_t cs_;
    dmac_channel_number_t dmaCh_;
    int priority_;

    std::function<void()> callback_;

public:
    void init(spi_device_num_t spiNum,
              dmac_channel_number_t dmaCh,
              int ssPin, int ss,
              int prio);

    void waitDone();
    void waitIdle();

    spi_device_num_t getSPINum() const
    {
        return spiNum_;
    }

    void setCallback(std::function<void()> &&f)
    {
        callback_ = f;
    }
    void resetCallback()
    {
        callback_ = std::function<void()>();
    }

    void _setupTransfer();
    void _transferBE(const uint32_t *data, uint32_t count);

    void transferBE(const uint32_t *data, uint32_t count);

    static SPIDMA &instance();

private:
    static int irqEntry(void *ctx);
    void irq();
};

#endif /* E010AF3D_A134_1650_EAE0_1F6097E2920D */
