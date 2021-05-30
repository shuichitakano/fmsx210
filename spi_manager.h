/*
 * author : Shuichi TAKANO
 * since  : Sat May 08 2021 03:55:58
 */
#ifndef _88DB1EDE_B134_634A_363D_C41DCD29B46C
#define _88DB1EDE_B134_634A_363D_C41DCD29B46C

#include <stdint.h>
#include <spi.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct _SPIConfig
    {
        int numSS;
        int pinSS;
        int pinSCLK;
        int pinMISO;
        int pinMOSI;
        int freq;
        spi_work_mode_t workMode; // SPI_WORK_MODE_0;
        bool pullup;
    } SPIConfig;

    void spimSetConfig(const SPIConfig *config);
    void spimWriteData(const SPIConfig *config, const uint8_t *data, uint32_t size);
    void spimReadData(const SPIConfig *config, uint8_t *data, uint32_t size);
    void spimTransfer(const SPIConfig *config,
                      const uint8_t *tx, uint32_t txSize,
                      uint8_t *rx, uint32_t rxSize);
    void spimWriteDataDMA(const SPIConfig *config, const uint8_t *data, uint32_t size);
    void spimReadDataDMA(const SPIConfig *config, uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* _88DB1EDE_B134_634A_363D_C41DCD29B46C */
