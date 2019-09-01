/*
 * author : Shuichi TAKANO
 * since  : Sat Aug 17 2019 12:49:29
 */

#include <stdio.h>
#include <assert.h>

#include <bsp.h>
#include <spi.h>
#include <fpioa.h>
#include <sysctl.h>
#include <i2s.h>
#include <gpio.h>
#include "audio.h"
#include "video_out.h"
#include "chrono.h"
#include "ps2_keyboard.h"
#include "led.h"
#include "uarths.h"

#include <sd_card/ff.h>
#include <sd_card/sdcard.h>

#include "lcd.h"

#include "fmsx_interface.h"

// kflash -b 1500000 -p /dev/cu.wchusbserial1410 -t fmsx210.bin

#define LCD_SPI_CH SPI_DEVICE_0
#define LCD_DMA_CH DMAC_CHANNEL3
#define LCD_RST_PIN 37
#define LCD_DCX_PIN 38
#define LCD_SS_PIN 36
#define LCD_SCLK_PIN 39
#define LCD_RST_GPIONUM 6
#define LCD_DCX_GPIONUM 7
#define LCD_SS 3

#define LED_R_PIN 14
#define LED_G_PIN 13
#define LED_B_PIN 12
#define LED_R_GPIONUM 2
#define LED_G_GPIONUM 3
#define LED_B_GPIONUM 4

#define PS2_CLK_PIN 10
#define PS2_DAT_PIN 11
#define PS2_CLK_GPIOHSNUM 14
#define PS2_DAT_GPIOHSNUM 16

#define AUDIO_ENABLE_PIN 32 // MAIX GO
//#define AUDIO_ENABLE_PIN 2  // MAIXDUINO
#define AUDIO_ENABLE_GPIONUM 5

void sd_test()
{
    fpioa_set_function(27, FUNC_SPI1_SCLK);
    fpioa_set_function(28, FUNC_SPI1_D0);
    fpioa_set_function(26, FUNC_SPI1_D1);
    fpioa_set_function(29, FUNC_SPI1_SS3);

    auto r = sd_init();
    printf("sd_init %d\n", r);
    assert(r == 0);

    printf("CardCapacity:%ld\n", cardinfo.CardCapacity);
    printf("CardBlockSize:%d\n", cardinfo.CardBlockSize);

    static FATFS sdcard_fs;
    FRESULT status;
    DIR dj;
    FILINFO fno;

    status = f_mount(&sdcard_fs, _T("0:"), 1);
    printf("mount sdcard:%d\r\n", status);
    if (status != FR_OK)
        return;

    printf("printf filename\r\n");
    status = f_findfirst(&dj, &fno, _T("0:"), _T("*"));
    while (status == FR_OK && fno.fname[0])
    {
        if (fno.fattrib & AM_DIR)
            printf("dir:%s\r\n", fno.fname);
        else
            printf("file:%s\r\n", fno.fname);
        status = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
}

void initI2S()
{
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);

    fpioa_set_function(AUDIO_ENABLE_PIN,
                       (fpioa_function_t)(FUNC_GPIO0 + AUDIO_ENABLE_GPIONUM));
    gpio_set_drive_mode(AUDIO_ENABLE_GPIONUM, GPIO_DM_OUTPUT);
    gpio_set_pin(AUDIO_ENABLE_GPIONUM, GPIO_PV_HIGH);

    initAudio(44100);
}

int main()
{
    //    auto cpuFreq = sysctl_cpu_set_freq(600000000);
    sysctl_pll_set_freq(SYSCTL_PLL0, 800000000);
    fpioa_init();
    gpio_init();
    //    uarths_init();
    plic_init();
    sysctl_enable_irq();

    sysctl_set_power_mode(SYSCTL_POWER_BANK1, SYSCTL_POWER_V33);

    initChrono();

#if 0
    // Maix Dock: 0, 14, 16, 18
    int num = 14;
    fpioa_set_tie_enable((fpioa_function_t)(FUNC_GPIOHS0 + num), false);
    // fpioa_set_function(16, (fpioa_function_t)(FUNC_GPIOHS0 + num));
    fpioa_set_function(PS2_CLK_PIN, (fpioa_function_t)(FUNC_GPIOHS0 + num));

    //    gpiohs_set_drive_mode(num, GPIO_DM_INPUT_PULL_UP);
    gpiohs_set_drive_mode(num, GPIO_DM_INPUT);
    printf("out %08x, in %08x\n", gpiohs->output_en.u32[0], gpiohs->input_en.u32[0]);

    fpioa_io_config_t c;
    fpioa_get_io(16, &c);
    printf("ch_sel %d, ds %d, oe_en %d, oe_inv %d, do_sel %d, do_inv %d, pu %d, pd %d, resv0 %d, sl %d, ie_en %d, ie_inv %d, di_inv %d, st %d, resv1 %d pad_di %d\n",
           c.ch_sel, c.ds, c.oe_en, c.oe_inv, c.do_sel, c.do_inv, c.pu, c.pd, c.resv0, c.sl, c.ie_en, c.ie_inv, c.di_inv, c.st, c.resv1, c.pad_di);

    volatile auto *const g = (volatile gpiohs_raw_t *)GPIOHS_BASE_ADDR;
    printf("iv %08x, ie %08x, oe %08x, ov %08x, pe %08x, dr %08x, rie %08x, rip %08x, fie %08x, fip %08x, hie %08x, hip %08x, liw %08x, lip %08x, iofe %08x, iofs %08x, ox %08x\n",
           g->input_val, g->input_en, g->output_en, g->output_val, g->pullup_en, g->drive,
           g->rise_ie, g->rise_ip, g->fall_ie, g->fall_ip, g->high_ie, g->high_ip,
           g->low_ie, g->low_ip, g->iof_en, g->iof_sel, g->output_xor);

    //    printf("size %zd\n", sizeof(gpiohs_u32_t));
    while (1)
    {
        // fpioa_get_io(16, &c);
        // if (!c.pad_di)
        // {
        //     printf("w");
        // }

        int v = gpiohs_get_pin(num);
        if (!v)
            printf("%d", v);
        //printf("%04x\n", gpiohs->input_val.u32[0]);
        // printf("%04x ", g->input_val);
    }
#endif

    initLED(LED_R_PIN, LED_R_GPIONUM,
            LED_G_PIN, LED_G_GPIONUM,
            LED_B_PIN, LED_B_GPIONUM);
    setRGBLED(0);

    sd_test();

    sysctl_set_spi0_dvp_data(1);

    LCD &lcd = LCD::instance();
    lcd.init(LCD_SPI_CH, LCD_DMA_CH, 20000000,
             LCD_RST_PIN, LCD_RST_GPIONUM,
             LCD_DCX_PIN, LCD_DCX_GPIONUM,
             LCD_SS_PIN, LCD_SS,
             LCD_SCLK_PIN);

    lcd.setDirection(LCD::DIR_XY_RLDU);
    lcd.clear(0);

    initI2S();

    PS2Keyboard::instance().init(PS2_CLK_PIN, PS2_CLK_GPIOHSNUM,
                                 PS2_DAT_PIN, PS2_DAT_GPIOHSNUM,
                                 4);

    if (0)
    {
        initVideo(390000000 * 2, 390000000 / 18, 228 * 2,
                  272 * 4, 228);
        //setInterlaceMode(true);
        startVideoTransfer();
        setFMSXVideoOutMode(VIDEOOUTMODE_COMPOSITE_VIDEO);
    }

    PS2Keyboard::instance().start();

    start_fMSX();

    while (1)
        ;
}