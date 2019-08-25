/*
 * author : Shuichi TAKANO
 * since  : Tue Aug 20 2019 1:15:52
 */
#ifndef DEEA86CB_F134_1652_119F_7CF5DEAA3AA8
#define DEEA86CB_F134_1652_119F_7CF5DEAA3AA8

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void lcdDrawImage(const uint16_t *data, int w, int h, int pitch);
    void lcdDrawHScaleImage(int dx, int dy, int dw, int sw, int h,
                            int pitch, const uint16_t *img);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <spi.h>
#include <fpioa.h>

class LCD
{
public:
    enum lcd_dir_t
    {
        DIR_XY_RLUD = 0x00,
        DIR_YX_RLUD = 0x20,
        DIR_XY_LRUD = 0x40,
        DIR_YX_LRUD = 0x60,
        DIR_XY_RLDU = 0x80,
        DIR_YX_RLDU = 0xA0,
        DIR_XY_LRDU = 0xC0,
        DIR_YX_LRDU = 0xE0,
        DIR_XY_MASK = 0x20,
        DIR_RL_MASK = 0x40,
        DIR_UD_MASK = 0x80
    };

public:
    void init(spi_device_num_t spi_num,
              dmac_channel_number_t dma_ch,
              uint32_t freq,
              int rst_pin, int rst,
              int dcx_pin, int dcx,
              int ss_pin, int ss,
              int sclk_pin);

    void setDirection(lcd_dir_t dir);
    void setArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    void clear(uint16_t color);
    void fill(uint16_t x1, uint16_t y1,
              uint16_t x2, uint16_t y2,
              uint16_t color);
    void drawImage(uint16_t x1, uint16_t y1,
                   uint16_t width, uint16_t height,
                   const uint16_t *ptr);

    void drawHScaleImage(int dx, int dy, int dw, int sw, int h,
                         int pitch, const uint16_t *img);

    static LCD &instance();

protected:
    void setDCXData();
    void setDCXControl();
    void setRST(bool f);

    void writeCommand(uint8_t cmd);
    void writeByte(const uint8_t *data_buf, uint32_t length);
    void writeHalf(const uint16_t *data_buf, uint32_t length);
    void writeWord(const uint32_t *data_buf, uint32_t length);
    void fillData(const uint32_t *data_buf, uint32_t length);

private:
    spi_device_num_t spiNum_;
    spi_chip_select_t cs_;
    dmac_channel_number_t dmaCh_;

    int gpioRST_;
    int gpioDCX_;

    int width_;
    int height_;
};
#endif

#endif /* DEEA86CB_F134_1652_119F_7CF5DEAA3AA8 */
