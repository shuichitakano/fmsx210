/*
 * author : Shuichi TAKANO
 * since  : Tue Aug 20 2019 1:16:40
 */

#include "lcd.h"
#include <gpio.h>
#include <gpiohs.h>
#include <sleep.h>
#include "spi_dma.h"
#include "worker.h"

namespace
{
    constexpr int LCD_X_MAX = 320;
    constexpr int LCD_Y_MAX = 480;

    alignas(8) uint16_t buffer_[2][LCD_X_MAX * LCD_Y_MAX];

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
               int sclk_pin,
               int te_pin, int te_gpioHS,
               LCDType lcdType)
{
    type_ = lcdType;
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

    if (te_pin >= 0)
    {
        fpioa_set_function(te_pin, (fpioa_function_t)(FUNC_GPIOHS0 + te_gpioHS));
        gpiohs_set_drive_mode(te_gpioHS, GPIO_DM_INPUT);
        gpiohsTE_ = te_gpioHS;

        gpiohs_set_pin_edge(gpiohsTE_, GPIO_PE_RISING);
        gpiohs_set_irq(gpiohsTE_, 1, []() {
            instance().vsync_ = true;
        });
        //        sysctl_enable_irq();
    }

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

    switch (lcdType)
    {
    case LCDType::ILI9481:
        initILI9481();
        break;

    case LCDType::ILI9486:
        initILI9486();
        break;

    default:
        break;
    }

    /*exit sleep*/
    writeCommand(SLEEP_OFF);
    usleep(100000);

    /*pixel format*/
    writeCommand(PIXEL_FORMAT_SET);
    uint8_t data = 0x55;
    writeByte(&data, 1);

    setDirection(DIR_YX_RLDU);

    //    writeCommand(INVERSION_DISPALY_ON);
    //    usleep(10000);

    /*display on*/
    writeCommand(DISPALY_ON);

    // tearing effect
    switch (lcdType)
    {
    case LCDType::ILI9486:
    {
#if 0
        {
            writeCommand(SET_TEAR_SCANLINE);
            static constexpr int line = 470;
            static constexpr uint8_t data[] = {line >> 8, line & 255};
            writeByte(data, sizeof(data));
        }
#else
        {
            writeCommand(TEAR_EFFECT_LINE_ON);
            static constexpr uint8_t data[] = {0};
            writeByte(data, sizeof(data));
        }
#endif
        //
        {
            writeCommand(NORMAL_FRAME_CTL);
            // 62Hz
            static constexpr uint8_t data[] = {0b10100000, 0b10001};
            writeByte(data, sizeof(data));
        }
    }
    break;

    default:
        break;
    }
}

void LCD::initILI9481()
{
    {
        writeCommand(0XD1); // VCOM Control
        static constexpr uint8_t data[] = {
            0x00,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0XC8); // Gamma Setting
        static constexpr uint8_t data[] = {
#if 1
            0x00,
            0x30,
            0x56,
            0x00,
            0x0,
            8,

            0x34,
            0x75,
            0x77,
            0x00,
            8,
            0,
#else
            0,
            0x30,
            0x36,
            0x45,
            0x4,
            0x16,

            0x37,
            0x75,
            0x77,
            0x54,
            0xf,
            0,
#endif
        };
        writeByte(data, sizeof(data));
    }
    writeCommand(INVERSION_DISPALY_ON);
}

void LCD::initILI9486()
{
    {
        writeCommand(0XF1); /* Unk */
        static constexpr uint8_t data[] = {
            0x36,
            0x04,
            0x00,
            0x3C,
            0X0F,
            0x8F,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0XF2); /* Unk */
        static constexpr uint8_t data[] = {
            0x18,
            0xA3,
            0x12,
            0x02,
            0XB2,
            0x12,
            0xFF,
            0x10,
            0x00,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0XF8); /* Unk */
        static constexpr uint8_t data[] = {
            0x21,
            0x04,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0XF9); /* Unk */
        static constexpr uint8_t data[] = {
            0x00,
            0x08,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0x36); /* Memory Access Control */
        static constexpr uint8_t data[] = {
            0x28,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0xB4); /* Display Inversion Control */
        static constexpr uint8_t data[] = {
            0x00,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0xC1); /* Power Control 2 */
        static constexpr uint8_t data[] = {
            0x41,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0xC5); /* Vcom Control */
        static constexpr uint8_t data[] = {
            0x00,
            0x18,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0xE0); /* Positive Gamma Control */
        static constexpr uint8_t data[] = {
            0x0F,
            0x1F,
            0x1C,
            0x0C,
            0x0F,
            0x08,
            0x48,
            0x98,
            0x37,
            0x0A,
            0x13,
            0x04,
            0x11,
            0x0D,
            0x00,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0xE1); /* Negative Gamma Control */
        static constexpr uint8_t data[] = {
            0x0F,
            0x32,
            0x2E,
            0x0B,
            0x0D,
            0x05,
            0x47,
            0x75,
            0x37,
            0x06,
            0x10,
            0x03,
            0x24,
            0x20,
            0x00,
        };
        writeByte(data, sizeof(data));
    }
    {
        writeCommand(0x3A); /* Interface Pixel Format */
        static constexpr uint8_t data[] = {
            0x55,
        };
        writeByte(data, sizeof(data));
    }
}

bool LCD::waitVSync()
{
    return false;
    if (gpiohsTE_ < 0)
    {
        return false;
    }

    while (!vsync_)
        ;

    vsync_ = false;
    return true;
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

void LCD::drawGradation(int w, int h)
{
    auto draw = [&](const auto &f, int iy) {
        int y0 = h * iy / 4;
        int y1 = h * (iy + 1) / 4 - 1;
        for (int i = 0; i < 64; ++i)
        {
            int x0 = w * i / 64;
            int x1 = w * (i + 1) / 64 - 1;
            fill(y0, x0, y1, x1, f(i));
        }
    };

    draw([](int i) {
        return i >> 1;
    },
         0);

    draw([](int i) {
        return i << 5;
    },
         1);

    draw([](int i) {
        return (i >> 1) << 11;
    },
         2);

    draw([](int i) {
        return ((i >> 1) << 11) |
               (i << 5) |
               (i >> 1);
    },
         3);
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
    //    printf("dr %d %d %d %d %d %d %p\n", dx, dy, dw, sw, h, pitch, img);
    int wct = (h + 1) / 2;

    int sxd = (sw << 16) / dw;
    int sx = 0;

    dbid_ ^= 1;

    auto *p = buffer_[dbid_];
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

    SPIDMA::instance().waitDone();
    //    dmac_wait_done(dmaCh_);

    setArea(dy, dx, dy + dpitch - 1, dx + dw - 1);
    writeWord((uint32_t *)buffer_[dbid_], dpitch * dw / 2);
}

void LCD::drawScaleImage(int dx, int dy, int dw, int dh,
                         int sw, int sh,
                         int pitch, const uint16_t *img)
{
    int sxd = (sw << 8) / dw;
    int sx = 0;

    int syd = (sh << 8) / dh;

    dbid_ ^= 1;

    auto *p = buffer_[dbid_];
    for (int x = 0; x < dw; ++x)
    {
        auto sp = img + (sx >> 8);
        int t1 = sx & 255;
        int t0 = 256 - t1;

        int sy = 0;

        for (int y = 0; y < dh; y += 2)
        {
            auto compute = [&](const uint16_t *src0, const uint16_t *src1,
                               int s0, int s1) {
                auto v00 = src0[0];
                auto v01 = src0[1];
                auto v10 = src1[0];
                auto v11 = src1[1];

                int r00 = v00 & 0xf800;
                int g00 = v00 & 0x07e0;
                int b00 = v00 & 0x001f;

                int r01 = v01 & 0xf800;
                int g01 = v01 & 0x07e0;
                int b01 = v01 & 0x001f;

                int r10 = v10 & 0xf800;
                int g10 = v10 & 0x07e0;
                int b10 = v10 & 0x001f;

                int r11 = v11 & 0xf800;
                int g11 = v11 & 0x07e0;
                int b11 = v11 & 0x001f;

                int r = r00 * s0 * t0 +
                        r01 * s0 * t1 +
                        r10 * s1 * t0 +
                        r11 * s1 * t1;

                int g = g00 * s0 * t0 +
                        g01 * s0 * t1 +
                        g10 * s1 * t0 +
                        g11 * s1 * t1;

                int b = b00 * s0 * t0 +
                        b01 * s0 * t1 +
                        b10 * s1 * t0 +
                        b11 * s1 * t1;

                return ((r >> 16) & 0xf800) |
                       ((g >> 16) & 0x07e0) |
                       ((b >> 16) & 0x001f);
            };

            {
                auto spp = sp + pitch * (sy >> 8);
                int s1 = sy & 255;
                int s0 = 256 - s1;
                p[1] = compute(spp, spp + pitch, s0, s1);
                sy += syd;
            }
            {
                auto spp = sp + pitch * (sy >> 8);
                int s1 = sy & 255;
                int s0 = 256 - s1;
                p[0] = compute(spp, spp + pitch, s0, s1);
                sy += syd;
            }
            p += 2;
        }
        sx += sxd;
    }

    int dpitch = (dh + 1) & ~1;

    SPIDMA::instance().waitDone();

    setArea(dy, dx, dy + dpitch - 1, dx + dw - 1);
    writeWord((uint32_t *)buffer_[dbid_], dpitch * dw / 2);
}

// dst index -> src index (x left)
#define SI0_X(dx) SI0_X_(dx)
#define SI0_X_(dx) SI0_X##dx
// dst index -> src index (y up)
#define SI0_Y(dy) SI0_Y_(dy)
#define SI0_Y_(dy) SI0_Y##dy

// next index
#define NI(i) NI_(i)
#define NI_(i) NI##i
#define NI0 1
#define NI1 2
#define NI2 3

#define SI1_X(dx) NI(SI0_X(dx))
#define SI1_Y(dy) NI(SI0_Y(dy))

#define CAT3_(a, b, c) a##b##c
#define CAT3(a, b, c) CAT3_(a, b, c)
#define S00(prefix, x, y) CAT3(prefix, SI0_Y(y), SI0_X(x))
#define S01(prefix, x, y) CAT3(prefix, SI0_Y(y), SI1_X(x))
#define S10(prefix, x, y) CAT3(prefix, SI1_Y(y), SI0_X(x))
#define S11(prefix, x, y) CAT3(prefix, SI1_Y(y), SI1_X(x))

#define IP(prefix, x, y) (S00(prefix, x, y) * W00(x, y) + \
                          S01(prefix, x, y) * W01(x, y) + \
                          S10(prefix, x, y) * W10(x, y) + \
                          S11(prefix, x, y) * W11(x, y))

#define C(x, y) (((IP(rb, x, y) >> 5) & 0xf81f) | ((IP(g, x, y) >> 5) & 0x07e0))

void LCD::drawFixScaleImage320(int dx, int dy, int sw, int sh, int pitch,
                               const uint16_t *img)
{
    // x: 3/2倍 272pix->408pix
    // y: 4/3倍 228pix->304pix
    // (7/5, 11/7)倍で (427, 319) でかなり良いがちょっと大きすぎるし、y周期が奇数なので面倒

    int xblks = sw / 2;
    int yblks = sh / 3;
    // 1 pixel 余分にアクセスするが気にしない…
    int dw = xblks * 3;
    int dh = yblks * 4;

    dbid_ ^= 1;
    auto *px = buffer_[dbid_];
    for (int ix = 0; ix < xblks; ++ix)
    {
        auto *p = px;
        auto *s = img;
        for (int iy = 0; iy < yblks; ++iy)
        {
            const auto *s0 = s;
            const auto *s1 = s0 + pitch;
            const auto *s2 = s1 + pitch;
            const auto *s3 = s2 + pitch;

            auto *p0 = p;
            auto *p1 = p0 + dh;
            auto *p2 = p1 + dh;

            const uint32_t rgb00 = s0[0];
            const uint32_t rgb01 = s0[1];
            const uint32_t rgb02 = s0[2];
            const uint32_t rgb10 = s1[0];
            const uint32_t rgb11 = s1[1];
            const uint32_t rgb12 = s1[2];
            const uint32_t rgb20 = s2[0];
            const uint32_t rgb21 = s2[1];
            const uint32_t rgb22 = s2[2];
            const uint32_t rgb30 = s3[0];
            const uint32_t rgb31 = s3[1];
            const uint32_t rgb32 = s3[2];

            const uint32_t rb00 = rgb00 & 0xf81f;
            const uint32_t rb01 = rgb01 & 0xf81f;
            const uint32_t rb02 = rgb02 & 0xf81f;
            const uint32_t rb10 = rgb10 & 0xf81f;
            const uint32_t rb11 = rgb11 & 0xf81f;
            const uint32_t rb12 = rgb12 & 0xf81f;
            const uint32_t rb20 = rgb20 & 0xf81f;
            const uint32_t rb21 = rgb21 & 0xf81f;
            const uint32_t rb22 = rgb22 & 0xf81f;
            const uint32_t rb30 = rgb30 & 0xf81f;
            const uint32_t rb31 = rgb31 & 0xf81f;
            const uint32_t rb32 = rgb32 & 0xf81f;

            const uint32_t g00 = rgb00 & 0x07e0;
            const uint32_t g01 = rgb01 & 0x07e0;
            const uint32_t g02 = rgb02 & 0x07e0;
            const uint32_t g10 = rgb10 & 0x07e0;
            const uint32_t g11 = rgb11 & 0x07e0;
            const uint32_t g12 = rgb12 & 0x07e0;
            const uint32_t g20 = rgb20 & 0x07e0;
            const uint32_t g21 = rgb21 & 0x07e0;
            const uint32_t g22 = rgb22 & 0x07e0;
            const uint32_t g30 = rgb30 & 0x07e0;
            const uint32_t g31 = rgb31 & 0x07e0;
            const uint32_t g32 = rgb32 & 0x07e0;

            // #define W00(x, y) ((3 - (x * 2 % 3)) * (4 - (y * 3 % 4)) * 32 / 12)
            // #define W01(x, y) ((x * 2 % 3) * (4 - (y * 3 % 4)) * 32 / 12)
            // #define W10(x, y) ((3 - (x * 2 % 3)) * (y * 3 % 4) * 32 / 12)
            // #define W11(x, y) ((x * 2 % 3) * (y * 3 % 4) * 32 / 12)

#define WX1(x) (x * 2 % 3)
#define WX0(x) (3 - WX1(x))
#define WY1(y) (y * 3 % 4)
#define WY0(y) (4 - WY1(y))
#define WN00(x, y) (WX0(x) * WY0(y) * 32)
#define WN01(x, y) (WX1(x) * WY0(y) * 32 + (WN00(x, y) % 12))
#define WN10(x, y) (WX0(x) * WY1(y) * 32 + (WN01(x, y) % 12))
#define WN11(x, y) (WX1(x) * WY1(y) * 32 + (WN10(x, y) % 12))
#define W00(x, y) (WN00(x, y) / 12)
#define W01(x, y) (WN01(x, y) / 12)
#define W10(x, y) (WN10(x, y) / 12)
#define W11(x, y) (WN11(x, y) / 12)

#define SI0_X0 0 // 0
#define SI0_X1 0 // 2/3
#define SI0_X2 1 // 4/3

#define SI0_Y0 0 // 0
#define SI0_Y1 0 // 3/4
#define SI0_Y2 1 // 6/4
#define SI0_Y3 2 // 9/4

            p0[1] = rgb00;
            p0[0] = C(0, 1);
            p0[3] = C(0, 2);
            p0[2] = C(0, 3);

            p1[1] = C(1, 0);
            p1[0] = C(1, 1);
            p1[3] = C(1, 2);
            p1[2] = C(1, 3);

            p2[1] = C(2, 0);
            p2[0] = C(2, 1);
            p2[3] = C(2, 2);
            p2[2] = C(2, 3);

            p = p + 4;
            s = s3;
        }
        px += dh * 3;
        img += 2;
    }

    //    printf("te %d\n", gpiohs_get_pin(gpiohsTE_));
    SPIDMA::instance().waitDone();

    waitVSync();
    setArea(dy, dx, dy + dh - 1, dx + dw - 1);
    writeWord((uint32_t *)buffer_[dbid_], dh * dw / 2);
}

void LCD::drawFixScaleImage320W(int dx, int dy, int sw, int sh, int pitch,
                                const uint16_t *img)
{
    // x: 3/4倍 544pix->408pix
    // y: 4/3倍 228pix->304pix

    int xblks = sw / 4;
    int yblks = sh / 3;
    // 1 pixel 余分にアクセスするが気にしない…
    int dw = xblks * 3;
    int dh = yblks * 4;

    dbid_ ^= 1;

    auto task = [=](int ix0, int ixEnd) {
        auto *px = buffer_[dbid_] + dh * ix0 * 3;
        auto *sx = img + ix0 * 4;
        for (int ix = ix0; ix < ixEnd; ++ix)
        {
            auto *p = px;
            auto *s = sx;
            for (int iy = 0; iy < yblks; ++iy)
            {
                const auto *s0 = s;
                const auto *s1 = s0 + pitch;
                const auto *s2 = s1 + pitch;
                const auto *s3 = s2 + pitch;

                auto *p0 = p;
                auto *p1 = p0 + dh;
                auto *p2 = p1 + dh;

                const uint32_t rgb00 = s0[0];
                const uint32_t rgb01 = s0[1];
                const uint32_t rgb02 = s0[2];
                const uint32_t rgb03 = s0[3];
                //            const uint32_t rgb04 = s0[4];
                const uint32_t rgb10 = s1[0];
                const uint32_t rgb11 = s1[1];
                const uint32_t rgb12 = s1[2];
                const uint32_t rgb13 = s1[3];
                //            const uint32_t rgb14 = s1[4];
                const uint32_t rgb20 = s2[0];
                const uint32_t rgb21 = s2[1];
                const uint32_t rgb22 = s2[2];
                const uint32_t rgb23 = s2[3];
                //            const uint32_t rgb24 = s2[4];
                const uint32_t rgb30 = s3[0];
                const uint32_t rgb31 = s3[1];
                const uint32_t rgb32 = s3[2];
                const uint32_t rgb33 = s3[3];
                //            const uint32_t rgb34 = s3[4];

                const uint32_t rb00 = rgb00 & 0xf81f;
                const uint32_t rb01 = rgb01 & 0xf81f;
                const uint32_t rb02 = rgb02 & 0xf81f;
                const uint32_t rb03 = rgb03 & 0xf81f;
                //            const uint32_t rb04 = rgb04 & 0xf81f;
                const uint32_t rb10 = rgb10 & 0xf81f;
                const uint32_t rb11 = rgb11 & 0xf81f;
                const uint32_t rb12 = rgb12 & 0xf81f;
                const uint32_t rb13 = rgb13 & 0xf81f;
                //            const uint32_t rb14 = rgb14 & 0xf81f;
                const uint32_t rb20 = rgb20 & 0xf81f;
                const uint32_t rb21 = rgb21 & 0xf81f;
                const uint32_t rb22 = rgb22 & 0xf81f;
                const uint32_t rb23 = rgb23 & 0xf81f;
                //            const uint32_t rb24 = rgb24 & 0xf81f;
                const uint32_t rb30 = rgb30 & 0xf81f;
                const uint32_t rb31 = rgb31 & 0xf81f;
                const uint32_t rb32 = rgb32 & 0xf81f;
                const uint32_t rb33 = rgb33 & 0xf81f;
                //            const uint32_t rb34 = rgb34 & 0xf81f;

                const uint32_t g00 = rgb00 & 0x07e0;
                const uint32_t g01 = rgb01 & 0x07e0;
                const uint32_t g02 = rgb02 & 0x07e0;
                const uint32_t g03 = rgb03 & 0x07e0;
                //            const uint32_t g04 = rgb04 & 0x07e0;
                const uint32_t g10 = rgb10 & 0x07e0;
                const uint32_t g11 = rgb11 & 0x07e0;
                const uint32_t g12 = rgb12 & 0x07e0;
                const uint32_t g13 = rgb13 & 0x07e0;
                //            const uint32_t g14 = rgb14 & 0x07e0;
                const uint32_t g20 = rgb20 & 0x07e0;
                const uint32_t g21 = rgb21 & 0x07e0;
                const uint32_t g22 = rgb22 & 0x07e0;
                const uint32_t g23 = rgb23 & 0x07e0;
                //            const uint32_t g24 = rgb24 & 0x07e0;
                const uint32_t g30 = rgb30 & 0x07e0;
                const uint32_t g31 = rgb31 & 0x07e0;
                const uint32_t g32 = rgb32 & 0x07e0;
                const uint32_t g33 = rgb33 & 0x07e0;
                //            const uint32_t g34 = rgb34 & 0x07e0;

#undef WX0
#undef WX1
#undef WY0
#undef WY1
#undef WN00
#undef WN01
#undef WN10
#undef WN11
#undef W00
#undef W01
#undef W10
#undef W11
#undef SI0_X0
#undef SI0_X1
#undef SI0_X2
#undef SI0_Y0
#undef SI0_Y1
#undef SI0_Y2
#undef SI0_Y3

                // #define W00(x, y) ((3 - (x * 4 % 3)) * (4 - (y * 3 % 4)) * 32 / 12)
                // #define W01(x, y) ((x * 4 % 3) * (4 - (y * 3 % 4)) * 32 / 12)
                // #define W10(x, y) ((3 - (x * 4 % 3)) * (y * 3 % 4) * 32 / 12)
                // #define W11(x, y) ((x * 4 % 3) * (y * 3 % 4) * 32 / 12)

#define WX1(x) (x * 4 % 3)
#define WX0(x) (3 - WX1(x))
#define WY1(y) (y * 3 % 4)
#define WY0(y) (4 - WY1(y))
#define WN00(x, y) (WX0(x) * WY0(y) * 32)
#define WN01(x, y) (WX1(x) * WY0(y) * 32 + (WN00(x, y) % 12))
#define WN10(x, y) (WX0(x) * WY1(y) * 32 + (WN01(x, y) % 12))
#define WN11(x, y) (WX1(x) * WY1(y) * 32 + (WN10(x, y) % 12))
#define W00(x, y) (WN00(x, y) / 12)
#define W01(x, y) (WN01(x, y) / 12)
#define W10(x, y) (WN10(x, y) / 12)
#define W11(x, y) (WN11(x, y) / 12)

// dst index -> src index (x left)
#define SI0_X0 0 // 0
#define SI0_X1 1 // 4/3
#define SI0_X2 2 // 8/3

// dst index -> src index (y up)
#define SI0_Y0 0 // 0
#define SI0_Y1 0 // 3/4
#define SI0_Y2 1 // 6/4
#define SI0_Y3 2 // 9/4

                p0[1] = rgb00;
                p0[0] = C(0, 1);
                p0[3] = C(0, 2);
                p0[2] = C(0, 3);

                p1[1] = C(1, 0);
                p1[0] = C(1, 1);
                p1[3] = C(1, 2);
                p1[2] = C(1, 3);

                p2[1] = C(2, 0);
                p2[0] = C(2, 1);
                p2[3] = C(2, 2);
                p2[2] = C(2, 3);

                p = p + 4;
                s = s3;
            }
            px += dh * 3;
            sx += 4;
        }
    };

    volatile bool done = false;
    addWorkload([&] {
        task(0, xblks / 2);
        done = true;
    });
    task(xblks / 2, xblks);

    while (!done)
        ;

    SPIDMA::instance().waitDone();

    waitVSync();
    setArea(dy, dx, dy + dh - 1, dx + dw - 1);
    writeWord((uint32_t *)buffer_[dbid_], dh * dw / 2);
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
#if 0
    SPIDMA::instance().waitDone();
    setDCXData();
    spi_init(spiNum_, SPI_WORK_MODE_0, SPI_FF_OCTAL, 32, 0);
    spi_init_non_standard(spiNum_, 0 /*instrction length*/, 32 /*address length*/, 0 /*wait cycles*/,
                          SPI_AITM_AS_FRAME_FORMAT /*spi address trans mode*/);
    spi_send_data_normal_dma(dmaCh_, spiNum_, cs_, data_buf, length, SPI_TRANS_INT);
#else
    auto &dma = SPIDMA::instance();
    dma.waitDone();
    setDCXData();
    dma.resetCallback();
    dma.transferBE(data_buf, length);
#endif
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

void lcdDrawScaleImage(int dx, int dy, int dw, int dh,
                       int sw, int sh,
                       int pitch, const uint16_t *img)
{
    LCD::instance().drawScaleImage(dx, dy, dw, dh, sw, sh, pitch, img);
}

void lcdDrawFixScaleImage320(int dx, int dy, int sw, int sh, int pitch,
                             const uint16_t *img)
{
    LCD::instance().drawFixScaleImage320(dx, dy, sw, sh, pitch, img);
}

void lcdDrawFixScaleImage320W(int dx, int dy, int sw, int sh, int pitch,
                              const uint16_t *img)
{
    LCD::instance().drawFixScaleImage320W(dx, dy, sw, sh, pitch, img);
}

int lcdWaitVSync()
{
    return LCD::instance().waitVSync();
}

uint8_t lcdIsQVGA()
{
    return LCD::instance().getLCDType() == LCD::LCDType::DEFAULT;
}