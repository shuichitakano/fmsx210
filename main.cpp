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
    //    printf("hello.\n");

    initChrono();

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

    if (1)
    {
        initVideo(390000000 * 2, 390000000 / 18, 228 * 2,
                  272 * 4, 228);
        //setInterlaceMode(true);
        startVideoTransfer();
        setFMSXVideoOutMode(VIDEOOUTMODE_COMPOSITE_VIDEO);
    }

    start_fMSX();

    while (1)
        ;
}