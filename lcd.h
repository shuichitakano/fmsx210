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

    int lcdWaitVSync();

    void lcdDrawImage(const uint16_t *data, int w, int h, int pitch);
    void lcdDrawHScaleImage(int dx, int dy, int dw, int sw, int h,
                            int pitch, const uint16_t *img);

    void lcdDrawScaleImage(int dx, int dy, int dw, int dh,
                           int sw, int sh,
                           int pitch, const uint16_t *img);

    void lcdDrawFixScaleImage320(int dx, int dy, int sw, int sh, int pitch,
                                 const uint16_t *img);
    void lcdDrawFixScaleImage320W(int dx, int dy, int sw, int sh, int pitch,
                                  const uint16_t *img);

    uint8_t lcdIsQVGA();

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
        DIR_UD_MASK = 0x80,
        DIR_RGB2BRG = 0x08,
    };

    enum class LCDType
    {
        DEFAULT,
        ILI9486, // for AMIGO tft
        ILI9481, // for AMIGO ips
    };

    struct ILI9481GammaSettings
    {
        // fine adjustment for positive polarity. (3bit)
        int kp[6] = {
            0,
            0,
            0,
            3,
            6,
            3,
        };

        // gradient adjustment for positive polarity. (3bit)
        int rp[2] = {
            5,
            4,
        };

        // amplitude adjustment for positive polarity. (4, 5bit)
        int vrp[2] = {
            4,
            22,
        };

        // fine adjustment for negative polarity. (3bit)
        int kn[6] = {
            7,
            3,
            5,
            7,
            7,
            7,
        };

        // gradient adjustment for negative polarity. (3bit)
        int rn[2] = {
            4,
            5,
        };

        // amplitude adjustment for negative polarity. (4, 5bit)
        int vrn[2] = {
            15,
            0,
        };
    };

public:
    void init(spi_device_num_t spi_num,
              dmac_channel_number_t dma_ch,
              uint32_t freq,
              int rst_pin, int rst,
              int dcx_pin, int dcx,
              int ss_pin, int ss,
              int sclk_pin,
              int te_pin, int te_gpioHS,
              LCDType lcdType);

    bool waitVSync();

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
    void drawScaleImage(int dx, int dy, int dw, int dh,
                        int sw, int sh,
                        int pitch, const uint16_t *img);

    void drawFixScaleImage320(int dx, int dy, int sw, int sh, int pitch,
                              const uint16_t *img);
    void drawFixScaleImage320W(int dx, int dy, int sw, int sh, int pitch,
                               const uint16_t *img);

    void drawGradation(int w, int h);
    LCDType getLCDType() const { return type_; }

    static LCD &instance();

protected:
    void initILI9481();
    void initILI9486();

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
    LCDType type_;

    int gpioRST_;
    int gpioDCX_;
    int gpiohsTE_ = -1;

    int width_;
    int height_;

    int dbid_ = 0;
    volatile bool vsync_ = true;
};
#endif

#endif /* DEEA86CB_F134_1652_119F_7CF5DEAA3AA8 */
