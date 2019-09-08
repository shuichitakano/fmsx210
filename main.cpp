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
#include "lcd.h"
#include "json.h"
#include "fmsx_interface.h"
#include <string>

#include <sd_card/ff.h>
#include <sd_card/sdcard.h>

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

#define BUTTON0_PIN 16
#define BUTTON1_PIN 15
#define BUTTON2_PIN 17
#define BUTTON0_GPIOHSNUM 0
#define BUTTON1_GPIOHSNUM 2
#define BUTTON2_GPIOHSNUM 3

#define AUDIO_ENABLE_GPIONUM 5

namespace
{

struct Settings
{
    std::string defaultROM[2]{};
    std::string defaultDisk[2]{};
    int ramPages = 2;
    int vramPages = 2;
    std::string video = "LCD";
    bool flipLCD = false;
    int audioEnablePin = -1;
    int volume = 2;
};

bool loadSettings(Settings *dst, const char *filename)
{
    auto root = parseJSONFile(filename);
    if (!root)
    {
        return false;
    }

    auto setIfExist = [&](auto *dst, const char *name) {
        if (auto v = getValue<typename std::remove_reference<decltype(*dst)>::type>(root.value(), name))
        {
            *dst = *v;
        }
    };

    setIfExist(&dst->defaultROM[0], "default_rom0");
    setIfExist(&dst->defaultROM[1], "default_rom1");
    setIfExist(&dst->defaultDisk[0], "default_disk0");
    setIfExist(&dst->defaultDisk[1], "default_disk1");
    setIfExist(&dst->ramPages, "ram_pages");
    setIfExist(&dst->vramPages, "vram_pages");
    setIfExist(&dst->video, "video");
    setIfExist(&dst->flipLCD, "flip_lcd");
    setIfExist(&dst->audioEnablePin, "audio_enable_pin");
    setIfExist(&dst->volume, "volume");

    printf("default rom 0: %s\n", dst->defaultROM[0].c_str());
    printf("default rom 1: %s\n", dst->defaultROM[1].c_str());
    printf("default disk 0: %s\n", dst->defaultDisk[0].c_str());
    printf("default disk 1: %s\n", dst->defaultDisk[1].c_str());
    printf("video: %s\n", dst->video.c_str());
    printf("flip: %d\n", dst->flipLCD);
    printf("ram: %d vram: %d\n", dst->ramPages, dst->vramPages);
    printf("audio enable pin: %d\n", dst->audioEnablePin);
    printf("volume: %d\n", dst->volume);

    return true;
}

void initSD()
{
    fpioa_set_function(27, FUNC_SPI1_SCLK);
    fpioa_set_function(28, FUNC_SPI1_D0);
    fpioa_set_function(26, FUNC_SPI1_D1);
    fpioa_set_function(29, FUNC_SPI1_SS3);

    {
        auto r = sd_init();
        printf("sd_init %d\n", r);
        assert(r == FR_OK);
    }

    {
        static FATFS sdcard_fs;
        auto r = f_mount(&sdcard_fs, _T("0:"), 1);
        printf("mount sdcard:%d\n", r);
        assert(r == FR_OK);
    }

    // 全然わからないけど必要(えー
    DIR dj;
    FILINFO fno;
    auto status = f_findfirst(&dj, &fno, _T("0:"), _T("*"));
#if 0
    while (status == FR_OK && fno.fname[0])
    {
        if (!(fno.fattrib & AM_HID))
        {
            if (fno.fattrib & AM_DIR)
                printf("dir:%s\r\n", fno.fname);
            else
                printf("file:%s\r\n", fno.fname);
        }
        status = f_findnext(&dj, &fno);
    }
#endif
    f_closedir(&dj);
}

void initI2S(int enablePin)
{
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);

    if (enablePin >= 0)
    {
        fpioa_set_function(enablePin,
                           (fpioa_function_t)(FUNC_GPIO0 + AUDIO_ENABLE_GPIONUM));
        gpio_set_drive_mode(AUDIO_ENABLE_GPIONUM, GPIO_DM_OUTPUT);
        gpio_set_pin(AUDIO_ENABLE_GPIONUM, GPIO_PV_HIGH);
    }

    initAudio(44100);
}

} // namespace

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

    initLED(LED_R_PIN, LED_R_GPIONUM,
            LED_G_PIN, LED_G_GPIONUM,
            LED_B_PIN, LED_B_GPIONUM);
    setRGBLED(0);

    initButton(0, BUTTON0_PIN, BUTTON0_GPIOHSNUM);
    initButton(1, BUTTON1_PIN, BUTTON1_GPIOHSNUM);
    initButton(2, BUTTON2_PIN, BUTTON2_GPIOHSNUM);

    initSD();

    Settings settings;
    loadSettings(&settings, "settings.json");

    sysctl_set_spi0_dvp_data(1);

    LCD &lcd = LCD::instance();
    lcd.init(LCD_SPI_CH, LCD_DMA_CH, 20000000,
             LCD_RST_PIN, LCD_RST_GPIONUM,
             LCD_DCX_PIN, LCD_DCX_GPIONUM,
             LCD_SS_PIN, LCD_SS,
             LCD_SCLK_PIN);

    lcd.setDirection(settings.flipLCD ? LCD::DIR_XY_RLDU
                                      : LCD::DIR_XY_LRUD);
    lcd.clear(0);

    initI2S(settings.audioEnablePin);

    PS2Keyboard::instance().init(PS2_CLK_PIN, PS2_CLK_GPIOHSNUM,
                                 PS2_DAT_PIN, PS2_DAT_GPIOHSNUM,
                                 4);

    if (settings.video == "NTSC")
    {
        initVideo(390000000 * 2, 390000000 / 18, 228 * 2,
                  272 * 4, 228);
        //setInterlaceMode(true);
        startVideoTransfer();
        setFMSXVideoOutMode(VIDEOOUTMODE_COMPOSITE_VIDEO);
    }

    PS2Keyboard::instance().start();

    start_fMSX(settings.defaultROM[0].c_str(),
               settings.defaultROM[1].c_str(),
               settings.defaultDisk[0].c_str(),
               settings.defaultDisk[1].c_str(),
               settings.ramPages, settings.vramPages,
               settings.volume);

    while (1)
        ;
}