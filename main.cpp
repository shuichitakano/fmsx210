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
#include <i2c.h>
#include <gpio.h>
#include "audio.h"
#include "video_out.h"
#include "chrono.h"
#include "ps2_keyboard.h"
#include "led.h"
#include "uarths.h"
#include "lcd.h"
#include "json.h"
#include "es8374.h"
#include "worker.h"
#include "fmsx_interface.h"
#include <string>
#include "pspad.h"
#include "cardkb.h"

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

//#define LCD_TE_PIN 19
#define LCD_TE_GPIOHSNUM 5

#define LED_R_PIN 14
#define LED_G_PIN 13
#define LED_B_PIN 12
#define LED_R_PIN_AMIGO 14
#define LED_G_PIN_AMIGO 15
#define LED_B_PIN_AMIGO 17
#define LED_R_GPIONUM 2
#define LED_G_GPIONUM 3
#define LED_B_GPIONUM 4

// #if AMIGO
// #define PS2_CLK_PIN 22
// #define PS2_DAT_PIN 25
// #else
// #define PS2_CLK_PIN 10
// #define PS2_DAT_PIN 11
// #endif
#define PS2_CLK_GPIOHSNUM 14
#define PS2_DAT_GPIOHSNUM 16

#define BUTTON0_PIN 16 // center
#define BUTTON1_PIN 15 // rec
#define BUTTON2_PIN 17
#define BUTTON0_PIN_AMIGO_TFT 31
#define BUTTON1_PIN_AMIGO_TFT 23
#define BUTTON2_PIN_AMIGO_TFT 20
#define BUTTON0_PIN_AMIGO_IPS 20
#define BUTTON1_PIN_AMIGO_IPS 16
#define BUTTON2_PIN_AMIGO_IPS 23
#define BUTTON0_GPIOHSNUM 1
#define BUTTON1_GPIOHSNUM 2
#define BUTTON2_GPIOHSNUM 4

#define AUDIO_ENABLE_GPIONUM 5

#define I2C_SCLK_PIN 24
#define I2C_SDA_PIN 27

// #define PSPAD_SS_PIN 29
// #define PSPAD_SCLK_PIN 28
// #define PSPAD_MISO_PIN 30
// #define PSPAD_MOSI_PIN 8

#define PSPAD_SS_GPIOHS 6
#define PSPAD_SCLK_GPIOHS 7
#define PSPAD_MISO_GPIOHS 8
#define PSPAD_MOSI_GPIOHS 9

namespace
{

    struct Settings
    {
        struct PS2
        {
            bool enable = false;
            int clkPin = 10;
            int datPin = 11;
        };

        struct PSPad
        {
            bool enable = false;
            int sckPin = 28;
            int selPin = 29;
            int datPin = 30;
            int cmdPin = 8;
        };

        struct CardKB
        {
            bool enable = false;
            int i2cNum = 0;
        };

        struct I2C
        {
            bool enable = false;
            int sclPin = 19;
            int sdaPin = 17;
        };

        std::string defaultROM[2]{};
        std::string defaultDisk[2]{};
        int ramPages = 2;
        int vramPages = 2;
        std::string video = "LCD";
        std::string board = "AMIGO_IPS";
        bool flipLCD = false;
        int tePin = -1;

        PS2 ps2;
        PSPad psPad;
        CardKB cardKB;
        I2C i2c1;

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

        auto setIfExistSub = [&](auto &obj, auto *dst, const char *name) {
            if (auto v = getValue<typename std::remove_reference<decltype(*dst)>::type>(obj.value(), name))
            {
                *dst = *v;
            }
        };

        auto setIfExist = [&](auto *dst, const char *name) {
            setIfExistSub(root, dst, name);
        };

        setIfExist(&dst->defaultROM[0], "default_rom0");
        setIfExist(&dst->defaultROM[1], "default_rom1");
        setIfExist(&dst->defaultDisk[0], "default_disk0");
        setIfExist(&dst->defaultDisk[1], "default_disk1");
        setIfExist(&dst->ramPages, "ram_pages");
        setIfExist(&dst->vramPages, "vram_pages");
        setIfExist(&dst->video, "video");
        setIfExist(&dst->board, "board");
        setIfExist(&dst->flipLCD, "flip_lcd");
        setIfExist(&dst->tePin, "te_pin");
        setIfExist(&dst->audioEnablePin, "audio_enable_pin");
        setIfExist(&dst->volume, "volume");

        if (auto sub = get(root.value(), "ps2"))
        {
            setIfExistSub(sub, &dst->ps2.enable, "enable");
            setIfExistSub(sub, &dst->ps2.clkPin, "clk_pin");
            setIfExistSub(sub, &dst->ps2.datPin, "dat_pin");
        }
        if (auto sub = get(root.value(), "pspad"))
        {
            setIfExistSub(sub, &dst->psPad.enable, "enable");
            setIfExistSub(sub, &dst->psPad.sckPin, "sck_pin");
            setIfExistSub(sub, &dst->psPad.selPin, "sel_pin");
            setIfExistSub(sub, &dst->psPad.datPin, "dat_pin");
            setIfExistSub(sub, &dst->psPad.cmdPin, "cms_pin");
        }
        if (auto sub = get(root.value(), "cardkb"))
        {
            setIfExistSub(sub, &dst->cardKB.enable, "enable");
            setIfExistSub(sub, &dst->cardKB.i2cNum, "i2c_num");
        }
        if (auto sub = get(root.value(), "i2c1"))
        {
            setIfExistSub(sub, &dst->i2c1.enable, "enable");
            setIfExistSub(sub, &dst->i2c1.sclPin, "scl_pin");
            setIfExistSub(sub, &dst->i2c1.sdaPin, "sda_pin");
        }

        printf("default rom 0: %s\n", dst->defaultROM[0].c_str());
        printf("default rom 1: %s\n", dst->defaultROM[1].c_str());
        printf("default disk 0: %s\n", dst->defaultDisk[0].c_str());
        printf("default disk 1: %s\n", dst->defaultDisk[1].c_str());
        printf("video: %s\n", dst->video.c_str());
        printf("board: %s\n", dst->board.c_str());
        printf("flip: %d\n", dst->flipLCD);
        printf("te pin: %d\n", dst->tePin);
        printf("ram: %d vram: %d\n", dst->ramPages, dst->vramPages);
        printf("audio enable pin: %d\n", dst->audioEnablePin);
        printf("volume: %d\n", dst->volume);

        return true;
    }

    SPIConfig spiConfigSD_{};

    void initSD(bool amigo)
    {
        if (amigo)
        {
            spiConfigSD_.pinSCLK = 11;
            spiConfigSD_.pinMOSI = 10;
            spiConfigSD_.pinMISO = 6;
            spiConfigSD_.pinSS = 26;
        }
        else
        {
            spiConfigSD_.pinSCLK = 27;
            spiConfigSD_.pinMOSI = 28;
            spiConfigSD_.pinMISO = 26;
            spiConfigSD_.pinSS = 29;
        }
        spiConfigSD_.numSS = 3;
        spiConfigSD_.freq = 200000;
        spiConfigSD_.workMode = SPI_WORK_MODE_0;

        spimSetConfig(&spiConfigSD_);

        {
            auto r = sd_init(&spiConfigSD_);
            printf("sd_init %d\n", r);
            assert(r == FR_OK);
        }

        {
            static FATFS sdcard_fs;
            auto r = f_mount(&sdcard_fs, _T("0:"), 1);
            printf("mount sdcard:%d\n", r);
            assert(r == FR_OK);
        }

#if 1
        // 全然わからないけど必要(えー
        DIR dj;
        FILINFO fno;
        auto status = f_findfirst(&dj, &fno, _T("0:"), _T("*"));
#if 1
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
#endif
    }

    void initPad(const Settings::PSPad &settings)
    {
        SWSPIConfig config{};
        config.pinSCLK = settings.sckPin;
        config.pinMOSI = settings.cmdPin;
        config.pinMISO = settings.datPin;
        config.pinSS = settings.selPin;
        config.gpioSCLK = PSPAD_SCLK_GPIOHS;
        config.gpioMOSI = PSPAD_MOSI_GPIOHS;
        config.gpioMISO = PSPAD_MISO_GPIOHS;
        config.gpioSS = PSPAD_SS_GPIOHS;
        config.pullupMISO = false;
        initPSPad(&config);
    }

    void initI2S(int enablePin, bool amigo)
    {
        if (amigo || 1)
        {
            sysctl_clock_enable(SYSCTL_CLOCK_I2S0);
            sysctl_reset(SYSCTL_RESET_I2S0);
            sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2S0, 7);
            sysctl_pll_set_freq(SYSCTL_PLL2, 262144000UL);
            sysctl_clock_set_threshold(SYSCTL_THRESHOLD_I2S0_M, 31);
        }
        else
        {
            sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
        }

        if (enablePin >= 0)
        {
            fpioa_set_function(enablePin,
                               (fpioa_function_t)(FUNC_GPIO0 + AUDIO_ENABLE_GPIONUM));
            gpio_set_drive_mode(AUDIO_ENABLE_GPIONUM, GPIO_DM_OUTPUT);
            gpio_set_pin(AUDIO_ENABLE_GPIONUM, GPIO_PV_HIGH);
        }
        initAudio(44100, amigo);
    }

    void initI2C0()
    {
        sysctl_clock_enable(SYSCTL_CLOCK_I2C0);
        fpioa_set_function(I2C_SCLK_PIN, FUNC_I2C0_SCLK);
        fpioa_set_function(I2C_SDA_PIN, FUNC_I2C0_SDA);
        fpioa_set_io_pull(I2C_SCLK_PIN, FPIOA_PULL_UP);
        fpioa_set_io_pull(I2C_SDA_PIN, FPIOA_PULL_UP);
    }

    bool detectI2C0(int addr)
    {
        i2c_init(I2C_DEVICE_0, addr, 7, 100000);
        uint8_t src[] = {0};
        return i2c_send_data(I2C_DEVICE_0, src, 1) == 0;
    }

    void uninitI2C0()
    {
        fpioa_set_function(I2C_SCLK_PIN, FUNC_RESV0);
        fpioa_set_function(I2C_SDA_PIN, FUNC_RESV0);
    }

    void initI2C1(int sclPin, int sdaPin)
    {
        printf("initI2C1: scl %d, sda %d\n", sclPin, sdaPin);
        sysctl_clock_enable(SYSCTL_CLOCK_I2C1);
        fpioa_set_function(sclPin, FUNC_I2C1_SCLK);
        fpioa_set_function(sdaPin, FUNC_I2C1_SDA);
        fpioa_set_io_pull(sclPin, FPIOA_PULL_UP);
        fpioa_set_io_pull(sdaPin, FPIOA_PULL_UP);
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

    sysctl_clock_enable(SYSCTL_CLOCK_APB0);
    sysctl_clock_enable(SYSCTL_CLOCK_APB1);
    sysctl_clock_enable(SYSCTL_CLOCK_APB2);

    initChrono();
    startWorker();

    initI2C0();

    bool amigo = detectI2C0(0x10);
    if (amigo)
    {
        printf("The Current System is prpbably MAIX AMIGO.\n");
    }
    else
    {
        printf("The Current System is prpbably not MAIX AMIGO.\n");
        uninitI2C0();
    }

    initSD(amigo);

    Settings settings;
    loadSettings(&settings, "settings.json");

    if (amigo)
    {
        settings.audioEnablePin = -1;
    }

    bool ips = settings.board == "AMIGO_IPS";

    if (amigo)
    {
        initLED(LED_R_PIN_AMIGO, LED_R_GPIONUM,
                LED_G_PIN_AMIGO, LED_G_GPIONUM,
                LED_B_PIN_AMIGO, LED_B_GPIONUM);

        if (ips)
        {
            initButton(0, BUTTON0_PIN_AMIGO_IPS, BUTTON0_GPIOHSNUM);
            initButton(1, BUTTON1_PIN_AMIGO_IPS, BUTTON1_GPIOHSNUM);
            initButton(2, BUTTON2_PIN_AMIGO_IPS, BUTTON2_GPIOHSNUM);
        }
        else
        {
            initButton(0, BUTTON0_PIN_AMIGO_TFT, BUTTON0_GPIOHSNUM);
            initButton(1, BUTTON1_PIN_AMIGO_TFT, BUTTON1_GPIOHSNUM);
            initButton(2, BUTTON2_PIN_AMIGO_TFT, BUTTON2_GPIOHSNUM);
        }
    }
    else
    {
        initLED(LED_R_PIN, LED_R_GPIONUM,
                LED_G_PIN, LED_G_GPIONUM,
                LED_B_PIN, LED_B_GPIONUM);

        initButton(0, BUTTON0_PIN, BUTTON0_GPIOHSNUM);
        initButton(1, BUTTON1_PIN, BUTTON1_GPIOHSNUM);
        initButton(2, BUTTON2_PIN, BUTTON2_GPIOHSNUM);
    }
    setRGBLED(0);

    if (settings.psPad.enable)
    {
        initPad(settings.psPad);
    }

    if (settings.i2c1.enable)
    {
        initI2C1(settings.i2c1.sclPin, settings.i2c1.sdaPin);
    }
    if (settings.cardKB.enable)
    {
        initCardKB(settings.cardKB.i2cNum);
    }

    sysctl_set_spi0_dvp_data(1);

    LCD &lcd = LCD::instance();
    lcd.init(LCD_SPI_CH, LCD_DMA_CH, 20000000,
             LCD_RST_PIN, LCD_RST_GPIONUM,
             LCD_DCX_PIN, LCD_DCX_GPIONUM,
             LCD_SS_PIN, LCD_SS,
             LCD_SCLK_PIN,
             settings.tePin, LCD_TE_GPIOHSNUM,
             amigo ? (ips ? LCD::LCDType::ILI9481 : LCD::LCDType::ILI9486)
                   : LCD::LCDType::DEFAULT);

    if (amigo)
    {
        lcd.setDirection((LCD::lcd_dir_t)((settings.flipLCD ? LCD::DIR_XY_LRDU
                                                            : LCD::DIR_XY_RLUD) |
                                          LCD::DIR_RGB2BRG));
    }
    else
    {
        lcd.setDirection(settings.flipLCD ? LCD::DIR_XY_RLDU
                                          : LCD::DIR_XY_LRUD);
    }
    lcd.clear(0);

#if 0
    lcd.drawGradation(480, 320);
    while (1)
        ;
#endif

    if (amigo)
    {
        initES8374();
    }
    initI2S(settings.audioEnablePin, amigo);

    if (settings.ps2.enable)
    {
        PS2Keyboard::instance().init(settings.ps2.clkPin, PS2_CLK_GPIOHSNUM,
                                     settings.ps2.datPin, PS2_DAT_GPIOHSNUM,
                                     4);
    }

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