/*
 * author : Shuichi TAKANO
 * since  : Sat May 08 2021 04:03:25
 */

#include "spi_manager.h"
#include <fpioa.h>
#include <stdio.h>
#include <sleep.h>
#include <algorithm>

#include "utils.h"

namespace
{
    class SPIManager
    {
    public:
        void setConfig(const SPIConfig &config)
        {
            if (currentConfig_ != &config)
            {
                if (currentConfig_)
                {
                    fpioa_set_function_raw(currentConfig_->pinSCLK, FUNC_RESV0);
                    fpioa_set_function_raw(currentConfig_->pinMOSI, FUNC_RESV0);
                    fpioa_set_function_raw(currentConfig_->pinMISO, FUNC_RESV0);
                }
                currentConfig_ = &config;
                printf("setConfig %p\n", currentConfig_);

                fpioa_set_function(config.pinSCLK, FUNC_SPI1_SCLK);
                fpioa_set_function(config.pinMOSI, FUNC_SPI1_D0);
                fpioa_set_function(config.pinMISO, FUNC_SPI1_D1);
                fpioa_set_function(config.pinSS,
                                   static_cast<fpioa_function_t>(FUNC_SPI1_SS0 + config.numSS));

                if (config.pullup)
                {
                    printf("pu\n");
                    //                    fpioa_set_io_pull(config.pinMISO, FPIOA_PULL_UP);
                    fpioa_set_io_pull(config.pinMOSI, FPIOA_PULL_UP);
                }

                printf("%d, %d, %d, %d, %d\n", config.pinSCLK, config.pinMOSI, config.pinMISO, config.pinSS, config.numSS);

                usleep(1000);
            }

            if (currentFreq_ != config.freq)
            {
                currentFreq_ = config.freq;
                printf("clk %d\n", config.freq);
                spi_set_clk_rate(SPI_DEVICE_1, config.freq);
            }
        }

    private:
        const SPIConfig *currentConfig_ = nullptr;
        int currentFreq_ = 0;
    };

    SPIManager &getSPIManager()
    {
        static SPIManager inst;
        return inst;
    }
}

void spimSetConfig(const SPIConfig *config)
{
    getSPIManager().setConfig(*config);
}

void spimWriteData(const SPIConfig *config, const uint8_t *data, uint32_t size)
{
    spimSetConfig(config);
    spi_init(SPI_DEVICE_1, config->workMode, SPI_FF_STANDARD, 8, 0);
    spi_send_data_standard(SPI_DEVICE_1,
                           static_cast<spi_chip_select_t>(SPI_CHIP_SELECT_0 + config->numSS),
                           NULL, 0, data, size);
}

void spimReadData(const SPIConfig *config, uint8_t *data, uint32_t size)
{
    spimSetConfig(config);
    spi_init(SPI_DEVICE_1, config->workMode, SPI_FF_STANDARD, 8, 0);
    spi_receive_data_standard(SPI_DEVICE_1,
                              static_cast<spi_chip_select_t>(SPI_CHIP_SELECT_0 + config->numSS),
                              NULL, 0, data, size);
}

void spimTransfer(const SPIConfig *config,
                  const uint8_t *tx, uint32_t txSize,
                  uint8_t *rx, uint32_t rxSize)
{
    spimSetConfig(config);

    spi_init(SPI_DEVICE_1, config->workMode, SPI_FF_STANDARD, 32, 1);
    spi_dup_send_receive_data_dma(DMAC_CHANNEL0, DMAC_CHANNEL1,
                                  SPI_DEVICE_1,
                                  static_cast<spi_chip_select_t>(SPI_CHIP_SELECT_0 + config->numSS),
                                  tx, txSize, rx, rxSize);
}

void spimWriteDataDMA(const SPIConfig *config, const uint8_t *data, uint32_t size)
{
    spimSetConfig(config);
    spi_init(SPI_DEVICE_1, config->workMode, SPI_FF_STANDARD, 32, 1);
    spi_send_data_standard_dma(DMAC_CHANNEL0,
                               SPI_DEVICE_1,
                               static_cast<spi_chip_select_t>(SPI_CHIP_SELECT_0 + config->numSS),
                               NULL, 0,
                               const_cast<uint8_t *>(data), size);
}

void spimReadDataDMA(const SPIConfig *config, uint8_t *data, uint32_t size)
{
    spimSetConfig(config);
    spi_init(SPI_DEVICE_1, config->workMode, SPI_FF_STANDARD, 32, 1);
    spi_receive_data_standard_dma(static_cast<dmac_channel_number_t>(-1),
                                  DMAC_CHANNEL0,
                                  SPI_DEVICE_1,
                                  static_cast<spi_chip_select_t>(SPI_CHIP_SELECT_0 + config->numSS),
                                  NULL, 0, data, size);
}
