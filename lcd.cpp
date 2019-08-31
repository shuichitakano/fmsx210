/*
 * author : Shuichi TAKANO
 * since  : Tue Aug 20 2019 1:16:40
 */

#include "lcd.h"
#include <gpio.h>
#include <sleep.h>
#include "spi_dma.h"

namespace
{
constexpr int LCD_X_MAX = 240;
constexpr int LCD_Y_MAX = 320;

alignas(8) uint16_t buffer_[LCD_X_MAX * LCD_Y_MAX];

enum
{
    NO_OPERATION = 0x00,
    SOFTWARE_RESET = 0x01,
    READ_ID = 0x04,
    READ_STATUS = 0x09,
    READ_POWER_MODE = 0x0A,
    READ_MADCTL = 0x0B,
    READ_PIXEL_FORMAT = 0x0C,
    READ_IMAGE_FORMAT = 0x0D,
    READ_SIGNAL_MODE = 0x0E,
    READ_SELT_DIAG_RESULT = 0x0F,
    SLEEP_ON = 0x10,
    SLEEP_OFF = 0x11,
    PARTIAL_DISPALY_ON = 0x12,
    NORMAL_DISPALY_ON = 0x13,
    INVERSION_DISPALY_OFF = 0x20,
    INVERSION_DISPALY_ON = 0x21,
    GAMMA_SET = 0x26,
    DISPALY_OFF = 0x28,
    DISPALY_ON = 0x29,
    HORIZONTAL_ADDRESS_SET = 0x2A,
    VERTICAL_ADDRESS_SET = 0x2B,
    MEMORY_WRITE = 0x2C,
    COLOR_SET = 0x2D,
    MEMORY_READ = 0x2E,
    PARTIAL_AREA = 0x30,
    VERTICAL_SCROL_DEFINE = 0x33,
    TEAR_EFFECT_LINE_OFF = 0x34,
    TEAR_EFFECT_LINE_ON = 0x35,
    MEMORY_ACCESS_CTL = 0x36,
    VERTICAL_SCROL_S_ADD = 0x37,
    IDLE_MODE_OFF = 0x38,
    IDLE_MODE_ON = 0x39,
    PIXEL_FORMAT_SET = 0x3A,
    WRITE_MEMORY_CONTINUE = 0x3C,
    READ_MEMORY_CONTINUE = 0x3E,
    SET_TEAR_SCANLINE = 0x44,
    GET_SCANLINE = 0x45,
    WRITE_BRIGHTNESS = 0x51,
    READ_BRIGHTNESS = 0x52,
    WRITE_CTRL_DISPALY = 0x53,
    READ_CTRL_DISPALY = 0x54,
    WRITE_BRIGHTNESS_CTL = 0x55,
    READ_BRIGHTNESS_CTL = 0x56,
    WRITE_MIN_BRIGHTNESS = 0x5E,
    READ_MIN_BRIGHTNESS = 0x5F,
    READ_ID1 = 0xDA,
    READ_ID2 = 0xDB,
    READ_ID3 = 0xDC,
    RGB_IF_SIGNAL_CTL = 0xB0,
    NORMAL_FRAME_CTL = 0xB1,
    IDLE_FRAME_CTL = 0xB2,
    PARTIAL_FRAME_CTL = 0xB3,
    INVERSION_CTL = 0xB4,
    BLANK_PORCH_CTL = 0xB5,
    DISPALY_FUNCTION_CTL = 0xB6,
    ENTRY_MODE_SET = 0xB7,
    BACKLIGHT_CTL1 = 0xB8,
    BACKLIGHT_CTL2 = 0xB9,
    BACKLIGHT_CTL3 = 0xBA,
    BACKLIGHT_CTL4 = 0xBB,
    BACKLIGHT_CTL5 = 0xBC,
    BACKLIGHT_CTL7 = 0xBE,
    BACKLIGHT_CTL8 = 0xBF,
    POWER_CTL1 = 0xC0,
    POWER_CTL2 = 0xC1,
    VCOM_CTL1 = 0xC5,
    VCOM_CTL2 = 0xC7,
    NV_MEMORY_WRITE = 0xD0,
    NV_MEMORY_PROTECT_KEY = 0xD1,
    NV_MEMORY_STATUS_READ = 0xD2,
    READ_ID4 = 0xD3,
    POSITIVE_GAMMA_CORRECT = 0xE0,
    NEGATIVE_GAMMA_CORRECT = 0xE1,
    DIGITAL_GAMMA_CTL1 = 0xE2,
    DIGITAL_GAMMA_CTL2 = 0xE3,
    INTERFACE_CTL = 0xF6,
};

} // namespace

void LCD::init(spi_device_num_t spi_num,
               dmac_channel_number_t dma_ch,
               uint32_t freq,
               int rst_pin, int rst,
               int dcx_pin, int dcx,
               int ss_pin, int ss,
               int sclk_pin)
{
    spiNum_ = spi_num;
    cs_ = (spi_chip_select_t)(SPI_CHIP_SELECT_0 + ss);
    dmaCh_ = dma_ch;

    gpioRST_ = rst;
    gpioDCX_ = dcx;

    fpioa_set_function(ss_pin, (fpioa_function_t)(FUNC_SPI0_SS0 + ss));
    fpioa_set_function(sclk_pin, FUNC_SPI0_SCLK);

    fpioa_set_function(dcx_pin, (fpioa_function_t)(FUNC_GPIO0 + dcx));
    gpio_set_drive_mode(dcx, GPIO_DM_OUTPUT);
    gpio_set_pin(gpioDCX_, GPIO_PV_HIGH);

    SPIDMA::instance().init(spiNum_, dmaCh_, ss_pin, ss, 1 /* prio */);

    if (rst >= 0)
    {
        fpioa_set_function(rst_pin, (fpioa_function_t)(FUNC_GPIO0 + rst));
        gpio_set_drive_mode(rst, GPIO_DM_OUTPUT);
        gpio_set_pin(gpioRST_, GPIO_PV_HIGH);
    }

    setRST(false);
    spi_init(spiNum_, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_set_clk_rate(spiNum_, freq);
    setRST(true);

    writeCommand(SOFTWARE_RESET);
    usleep(100000);

    /*exit sleep*/
    writeCommand(SLEEP_OFF);
    usleep(100000);

    /*pixel format*/
    writeCommand(PIXEL_FORMAT_SET);
    uint8_t data = 0x55;
    writeByte(&data, 1);

    setDirection(DIR_YX_RLDU);

    /*display on*/
    writeCommand(DISPALY_ON);
}

void LCD::setDCXData()
{
    gpio_set_pin(gpioDCX_, GPIO_PV_HIGH);
}

void LCD::setDCXControl()
{
    gpio_set_pin(gpioDCX_, GPIO_PV_LOW);
}

void LCD::setRST(bool f)
{
    if (gpioRST_ >= 0)
    {
        gpio_set_pin(gpioRST_, f ? GPIO_PV_HIGH : GPIO_PV_LOW);
    }
}

void LCD::setDirection(lcd_dir_t dir)
{
    if (dir & DIR_XY_MASK)
    {
        width_ = LCD_Y_MAX - 1;
        height_ = LCD_X_MAX - 1;
    }
    else
    {
        width_ = LCD_X_MAX - 1;
        height_ = LCD_Y_MAX - 1;
    }

    writeCommand(MEMORY_ACCESS_CTL);
    writeByte((uint8_t *)&dir, 1);
}

void LCD::setArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4] = {};

    data[0] = (uint8_t)(x1 >> 8);
    data[1] = (uint8_t)(x1);
    data[2] = (uint8_t)(x2 >> 8);
    data[3] = (uint8_t)(x2);
    writeCommand(HORIZONTAL_ADDRESS_SET);
    writeByte(data, 4);

    data[0] = (uint8_t)(y1 >> 8);
    data[1] = (uint8_t)(y1);
    data[2] = (uint8_t)(y2 >> 8);
    data[3] = (uint8_t)(y2);
    writeCommand(VERTICAL_ADDRESS_SET);
    writeByte(data, 4);

    writeCommand(MEMORY_WRITE);
}

void LCD::clear(uint16_t color)
{
    fill(0, 0, width_, height_, color);
}

void LCD::fill(uint16_t x1, uint16_t y1,
               uint16_t x2, uint16_t y2,
               uint16_t color)
{
    uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;
    setArea(x1, y1, x2, y2);
    fillData(&data, (y2 + 1 - y1) * (x2 + 1 - x1));
}

void LCD::drawImage(uint16_t x1, uint16_t y1,
                    uint16_t width, uint16_t height,
                    const uint16_t *ptr)
{
    setArea(x1, y1, x1 + width - 1, y1 + height - 1);
    writeWord((uint32_t *)ptr, width * height / 2);
}

void LCD::drawHScaleImage(int dx, int dy, int dw, int sw, int h,
                          int pitch, const uint16_t *img)
{
    int wct = (h + 1) / 2;

    int sxd = (sw << 16) / dw;
    int sx = 0;

    auto *p = buffer_;
    for (int x = 0; x < dw; ++x)
    {
        auto sp = img + (sx >> 16);
        int t1 = sx & 65535;
        int t0 = 65536 - t1;

        for (int ct = wct; ct; --ct)
        {
            auto compute = [&](const uint16_t *src) {
                auto v0 = src[0];
                auto v1 = src[1];

                int r0 = v0 & 0xf800;
                int g0 = v0 & 0x07e0;
                int b0 = v0 & 0x001f;

                int r1 = v1 & 0xf800;
                int g1 = v1 & 0x07e0;
                int b1 = v1 & 0x001f;

                int r = ((r0 * t0 + r1 * t1) >> 16) & 0xf800;
                int g = ((g0 * t0 + g1 * t1) >> 16) & 0x07e0;
                int b = ((b0 * t0 + b1 * t1) >> 16) & 0x001f;

                return r | g | b;
            };

            p[0] = compute(sp + pitch);
            p[1] = compute(sp);

            p += 2;
            sp += pitch * 2;
        }

        sx += sxd;
    }

    int dpitch = wct << 1;

    dmac_wait_done(dmaCh_);

    setArea(dy, dx, dy + dpitch - 1, dx + dw - 1);
    writeWord((uint32_t *)buffer_, dpitch * dw / 2);
}

void LCD::writeCommand(uint8_t cmd)
{
    SPIDMA::instance().waitDone();
    setDCXControl();
    spi_init(spiNum_, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(spiNum_, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal_dma(dmaCh_, spiNum_, cs_, (uint8_t *)(&cmd), 1, SPI_TRANS_CHAR);
}

void LCD::writeByte(const uint8_t *data_buf, uint32_t length)
{
    SPIDMA::instance().waitDone();
    setDCXData();
    spi_init(spiNum_, SPI_WORK_MODE_0, SPI_FF_OCTAL, 8, 0);
    spi_init_non_standard(spiNum_, 8 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal_dma(dmaCh_, spiNum_, cs_, data_buf, length, SPI_TRANS_CHAR);
}

void LCD::writeHalf(const uint16_t *data_buf, uint32_t length)
{
    SPIDMA::instance().waitDone();
    setDCXData();
    spi_init(spiNum_, SPI_WORK_MODE_0, SPI_FF_OCTAL, 16, 0);
    spi_init_non_standard(spiNum_, 16 /*instrction length*/, 0 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal_dma(dmaCh_, spiNum_, cs_, data_buf, length, SPI_TRANS_SHORT);
}

void LCD::writeWord(const uint32_t *data_buf, uint32_t length)
{
    auto &dma = SPIDMA::instance();
    dma.waitDone();
    setDCXData();
    dma.resetCallback();
    dma.transferBE(data_buf, length);
}

void LCD::fillData(const uint32_t *data_buf, uint32_t length)
{
    setDCXData();
    spi_init(spiNum_, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    spi_init_non_standard(spiNum_, 0 /*instrction length*/, 32 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_fill_data_dma(dmaCh_, spiNum_, cs_, data_buf, length);
}

LCD &LCD::instance()
{
    static LCD inst_;
    return inst_;
}

void lcdDrawImage(const uint16_t *data, int w, int h, int pitch)
{
    LCD::instance().drawImage(0, 0, w, h, data);
}

void lcdDrawHScaleImage(int dx, int dy, int dw, int sw, int h,
                        int pitch, const uint16_t *img)
{
    LCD::instance().drawHScaleImage(dx, dy, dw, sw, h,
                                    pitch, img);
}
