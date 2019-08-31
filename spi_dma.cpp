/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 25 2019 15:33:59
 */

#include "spi_dma.h"

#include <dmac.h>
#include <utils.h>
#include <sysctl.h>
#include <spi.h>
#include <fpioa.h>

SPIDMA &SPIDMA::instance()
{
    static SPIDMA inst;
    return inst;
}

void SPIDMA::init(spi_device_num_t spiNum,
                  dmac_channel_number_t dmaCh,
                  int ssPin, int ss, int prio)
{
    spiNum_ = spiNum;

    volatile spi_t *const spi[4] =
        {
            (volatile spi_t *)SPI0_BASE_ADDR,
            (volatile spi_t *)SPI1_BASE_ADDR,
            (volatile spi_t *)SPI_SLAVE_BASE_ADDR,
            (volatile spi_t *)SPI3_BASE_ADDR,
        };
    spi_ = spi[spiNum];
    cs_ = (spi_chip_select_t)(SPI_CHIP_SELECT_0 + ss);
    dmaCh_ = dmaCh;
    priority_ = prio;

    fpioa_set_function(ssPin, (fpioa_function_t)(FUNC_SPI0_SS0 + ss));
}

void SPIDMA::waitDone()
{
    dmac_wait_done(dmaCh_);
}

void SPIDMA::waitIdle()
{
    dmac_wait_idle(dmaCh_);
}

int SPIDMA::irqEntry(void *ctx)
{
    ((SPIDMA *)ctx)->irq();
    return 0;
}

void SPIDMA::irq()
{
    dmac_irq_unregister(dmaCh_);

#if 0
    while ((spi_->sr & 0x05) != 0x04)
        ;
    spi_->ser = 0x00;
    spi_->ssienr = 0x00;
#endif
    if (callback_)
    {
        callback_();
    }
}

void SPIDMA::_setupTransfer()
{
    spi_init(spiNum_, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    spi_init_non_standard(spiNum_, 0 /*instrction length*/, 32 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    set_bit(&spi_->ctrlr0, 3 << 8, SPI_TMOD_TRANS << 8);
    spi_->dmacr = 0x2; /*enable dma transmit*/
    spi_->ssienr = 0x01;
    sysctl_dma_select((sysctl_dma_channel_t)dmaCh_, SYSCTL_DMA_SELECT_SSI0_TX_REQ);
}

void SPIDMA::_transferBE(const uint32_t *data, uint32_t count)
{
    dmac_irq_register(dmaCh_, irqEntry, this, priority_);
    dmac_set_single_mode(dmaCh_, data, (void *)(&spi_->dr[0]), DMAC_ADDR_INCREMENT, DMAC_ADDR_NOCHANGE,
                         DMAC_MSIZE_4, DMAC_TRANS_WIDTH_32, count);
    spi_->ser = 1U << cs_;
}

// Big Endianでデータを転送する
void SPIDMA::transferBE(const uint32_t *data, uint32_t count)
{
    _setupTransfer();
    _transferBE(data, count);
}
