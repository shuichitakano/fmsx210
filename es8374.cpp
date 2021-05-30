/*
 * author : Shuichi TAKANO
 * since  : Sun Jan 24 2021 23:56:09
 */

#include "es8374.h"
#include <stdint.h>
#include <i2c.h>
#include <algorithm>
#include <stdio.h>

namespace
{

    struct Clock
    {
        enum LCLKDiv
        {
            LCLK_DIV_MIN = -1,
            LCLK_DIV_128 = 0,
            LCLK_DIV_192 = 1,
            LCLK_DIV_256 = 2,
            LCLK_DIV_384 = 3,
            LCLK_DIV_512 = 4,
            LCLK_DIV_576 = 5,
            LCLK_DIV_768 = 6,
            LCLK_DIV_1024 = 7,
            LCLK_DIV_1152 = 8,
            LCLK_DIV_1408 = 9,
            LCLK_DIV_1536 = 100,
            LCLK_DIV_2112 = 101,
            LCLK_DIV_2304 = 102,
            LCLK_DIV_125 = 16,
            LCLK_DIV_136 = 17,
            LCLK_DIV_250 = 18,
            LCLK_DIV_272 = 19,
            LCLK_DIV_375 = 20,
            LCLK_DIV_500 = 21,
            LCLK_DIV_544 = 22,
            LCLK_DIV_750 = 23,
            LCLK_DIV_1000 = 24,
            LCLK_DIV_1088 = 25,
            LCLK_DIV_1496 = 26,
            LCLK_DIV_1500 = 27,
        };

        enum MCLKDiv
        {
            MCLK_DIV_MIN = -1,
            MCLK_DIV_1 = 1,
            MCLK_DIV_2 = 2,
            MCLK_DIV_3 = 3,
            MCLK_DIV_4 = 4,
            MCLK_DIV_6 = 5,
            MCLK_DIV_8 = 6,
            MCLK_DIV_9 = 7,
            MCLK_DIV_11 = 8,
            MCLK_DIV_12 = 9,
            MCLK_DIV_16 = 10,
            MCLK_DIV_18 = 11,
            MCLK_DIV_22 = 12,
            MCLK_DIV_24 = 13,
            MCLK_DIV_33 = 14,
            MCLK_DIV_36 = 15,
            MCLK_DIV_44 = 16,
            MCLK_DIV_48 = 17,
            MCLK_DIV_66 = 18,
            MCLK_DIV_72 = 19,
            MCLK_DIV_5 = 20,
            MCLK_DIV_10 = 21,
            MCLK_DIV_15 = 22,
            MCLK_DIV_17 = 23,
            MCLK_DIV_20 = 24,
            MCLK_DIV_25 = 25,
            MCLK_DIV_30 = 26,
            MCLK_DIV_32 = 27,
            MCLK_DIV_34 = 28,
            MCLK_DIV_7 = 29,
            MCLK_DIV_13 = 30,
            MCLK_DIV_14 = 31,
        };

        LCLKDiv lclkDiv = LCLK_DIV_256;
        MCLKDiv mclkDiv = MCLK_DIV_4;
    };

    struct Config
    {
        enum ADCMode
        {
            ES8374_ADC_INPUT_LINE1 = 0,
            ES8374_ADC_INPUT_LINE2 = 1,
            ES8374_ADC_INPUT_ALL = 2,
            ES8374_ADC_INPUT_DIFFERENCE = 3,
        };

        enum DACMode
        {
            ES8374_DAC_OUTPUT_LINE1 = 0,
            ES8374_DAC_OUTPUT_LINE2 = 1,
            ES8374_DAC_OUTPUT_ALL = 2,
        };

        enum Mode
        {
            ES8374_MODE_ENCODE = 1,
            ES8374_MODE_DECODE = 2,
            ES8374_MODE_BOTH = 3,
            ES8374_MODE_LINE_IN = 4,
        };

        struct Interface
        {
            enum Mode
            {
                ES8374_MODE_SLAVE = 0,
                ES8374_MODE_MASTER = 1,
            };

            enum Format
            {
                ES8374_I2S_NORMAL = 0,
                ES8374_I2S_LEFT = 1,
                ES8374_I2S_RIGHT = 2,
                ES8374_I2S_DSP = 3,
            };

            enum Samples
            {
                ES8374_08K_SAMPLES = 0,
                ES8374_11K_SAMPLES = 1,
                ES8374_16K_SAMPLES = 2,
                ES8374_22K_SAMPLES = 3,
                ES8374_24K_SAMPLES = 4,
                ES8374_32K_SAMPLES = 5,
                ES8374_44K_SAMPLES = 6,
                ES8374_48K_SAMPLES = 7,
            };

            enum Bits
            {
                ES8374_BIT_LENGTH_16BITS = 1,
                ES8374_BIT_LENGTH_24BITS = 2,
                ES8374_BIT_LENGTH_32BITS = 3,
            };

            Mode mode = ES8374_MODE_SLAVE;
            Format format = ES8374_I2S_NORMAL;
            Samples samples = ES8374_44K_SAMPLES;
            Bits bits = ES8374_BIT_LENGTH_16BITS;
        };

        ADCMode adcMode = ES8374_ADC_INPUT_LINE1;
        DACMode dacMode = ES8374_DAC_OUTPUT_LINE1;
        Mode mode = ES8374_MODE_BOTH;

        Interface interface;

    public:
        Config() {}
    };

    enum BitLength
    {
        BIT_LENGTH_MIN = -1,
        BIT_LENGTH_16BITS = 0x03,
        BIT_LENGTH_18BITS = 0x02,
        BIT_LENGTH_20BITS = 0x01,
        BIT_LENGTH_24BITS = 0x00,
        BIT_LENGTH_32BITS = 0x04,
    };

    enum
    {
        MIC_GAIN_MIN = -1,
        MIC_GAIN_0DB = 0,
        MIC_GAIN_3DB = 3,
        MIC_GAIN_6DB = 6,
        MIC_GAIN_9DB = 9,
        MIC_GAIN_12DB = 12,
        MIC_GAIN_15DB = 15,
        MIC_GAIN_18DB = 18,
        MIC_GAIN_21DB = 21,
        MIC_GAIN_24DB = 24,
    };

    enum
    {
        D2SE_PGA_GAIN_MIN = -1,
        D2SE_PGA_GAIN_DIS = 0,
        D2SE_PGA_GAIN_EN = 1,
        D2SE_PGA_GAIN_MAX = 2,
    };

    enum
    {
        ES8374_CTRL_STOP = 0x00,
        ES8374_CTRL_START = 0x01,
    };

    struct Manager
    {
        static constexpr int I2CAddr = 0x10;

        Clock clock_;
        Config config_;

    public:
        void init()
        {
            stop(config_.mode);
            init_reg(config_.interface.mode,
                     config_.interface.format,
                     BIT_LENGTH_16BITS,
                     clock_,
                     config_.dacMode,
                     config_.adcMode);

            setMICGain(MIC_GAIN_18DB);
            setD2sePga(D2SE_PGA_GAIN_EN);

            codecConfigI2S(config_.interface.format,
                           config_.interface.bits);
        }

        int readReg(uint8_t regAddr)
        {
            i2c_init(I2C_DEVICE_0, I2CAddr, 7, 100000);
            uint8_t buf[] = {regAddr};
            uint8_t rbuf[1] = {};
            i2c_recv_data(I2C_DEVICE_0, buf, sizeof(buf),
                          rbuf, 1);
            //            printf("readReg(%02x: %02x)\n", regAddr, rbuf[0]);
            return rbuf[0];
        }

        void writeReg(uint8_t regAddr, uint8_t data)
        {
            //            printf("writeReg(%02x: %02x)\n", regAddr, data);
            i2c_init(I2C_DEVICE_0, I2CAddr, 7, 100000);
            uint8_t buf[] = {regAddr, data};

            i2c_send_data(I2C_DEVICE_0, buf, sizeof(buf));
        }

        void start(Config::Mode mode)
        {
            if (mode == Config::Mode::ES8374_MODE_LINE_IN)
            {
                auto reg = readReg(0x1a); // set monomixer
                reg |= 0x60;
                reg |= 0x20;
                reg &= 0xf7;
                writeReg(0x1a, reg);
                reg = readReg(0x1c); // set spk mixer
                reg |= 0x40;
                writeReg(0x1c, reg);
                writeReg(0x1D, 0x02); // spk set
                writeReg(0x1F, 0x00); // spk set
                writeReg(0x1E, 0xA0); // spk on
            }

            if (mode == Config::Mode::ES8374_MODE_ENCODE || mode == Config::Mode::ES8374_MODE_BOTH || mode == Config::Mode::ES8374_MODE_LINE_IN)
            {
                auto reg = readReg(0x21); // power up adc and input
                reg &= 0x3f;
                writeReg(0x21, reg);
                reg = readReg(0x10); // power up adc and input
                reg &= 0x3f;
                writeReg(0x10, reg);
            }

            if (mode == Config::Mode::ES8374_MODE_DECODE || mode == Config::Mode::ES8374_MODE_BOTH || mode == Config::Mode::ES8374_MODE_LINE_IN)
            {
                auto reg = readReg(0x1a); // disable lout
                reg |= 0x08;
                writeReg(0x1a, reg);
                reg &= 0xdf;
                writeReg(0x1a, reg);
                writeReg(0x1D, 0x12); // mute speaker
                writeReg(0x1E, 0x20); // disable class d
                reg = readReg(0x15);  // power up dac
                reg &= 0xdf;
                writeReg(0x15, reg);
                reg = readReg(0x1a); // disable lout
                reg |= 0x20;
                writeReg(0x1a, reg);
                reg &= 0xf7;
                writeReg(0x1a, reg);
                writeReg(0x1D, 0x02); // mute speaker
                writeReg(0x1E, 0xa0); // disable class d

                setVoiceMute(false);
            }
        }

        void stop(Config::Mode mode)
        {

            if (mode == Config::Mode::ES8374_MODE_LINE_IN)
            {
                auto reg = readReg(0x1a); // disable lout
                reg |= 0x08;
                writeReg(0x1a, reg);
                reg &= 0x9f;
                writeReg(0x1a, reg);
                writeReg(0x1D, 0x12); // mute speaker
                writeReg(0x1E, 0x20); // disable class d
                reg = readReg(0x1c);  // disable spkmixer
                reg &= 0xbf;
                writeReg(0x1c, reg);
                writeReg(0x1F, 0x00); // spk set
            }

            if (mode == Config::Mode::ES8374_MODE_DECODE ||
                mode == Config::Mode::ES8374_MODE_BOTH)
            {
                setVoiceMute(true);

                auto reg = readReg(0x1a); // disable lout
                reg |= 0x08;
                writeReg(0x1a, reg);
                reg &= 0xdf;
                writeReg(0x1a, reg);
                writeReg(0x1D, 0x12); // mute speaker
                writeReg(0x1E, 0x20); // disable class d
                reg = readReg(0x15);  // power up dac
                reg |= 0x20;
                writeReg(0x15, reg);
            }

            if (mode == Config::Mode::ES8374_MODE_ENCODE ||
                mode == Config::Mode::ES8374_MODE_BOTH)
            {
                auto reg = readReg(0x10); // power up adc and input
                reg |= 0xc0;
                writeReg(0x10, reg);
                reg = readReg(0x21); // power up adc and input
                reg |= 0xc0;
                writeReg(0x21, reg);
            }
        }

        void setVoiceMute(bool enable)
        {
            auto reg = readReg(0x36);
            reg &= 0xdf;
            reg = (reg | (enable << 5));
            writeReg(0x36, reg);
        }

        void init_reg(Config::Interface::Mode ms_mode,
                      Config::Interface::Format fmt,
                      BitLength bitLength,
                      const Clock &cfg,
                      Config::DACMode out_channel,
                      Config::ADCMode in_channel)
        {
            writeReg(0x00, 0x3F); // IC Rst start
            writeReg(0x00, 0x03); // IC Rst stop
            writeReg(0x01, 0x7F); // IC clk on // M ORG 7F

            auto reg = readReg(0x0F);
            reg &= 0x7f;
            reg |= (ms_mode << 7);
            writeReg(0x0f, reg); // CODEC IN I2S SLAVE MODE

            writeReg(0x6F, 0xA0); // pll set:mode enable
            writeReg(0x72, 0x41); // pll set:mode set
            //  writeReg(0x09,0x01)// pll set:reset on ,set start
            //  writeReg(0x0C,0x22)// pll set:k
            //  writeReg(0x0D,0x2E)// pll set:k
            //  writeReg(0x0E,0xC6)// pll set:k
            //  writeReg(0x0A,0x3A)// pll set:
            //  writeReg(0x0B,0x07)// pll set:n
            //  writeReg(0x09,0x41)// pll set:reset off ,set stop
            writeReg(0x09, 0x01); // pll set:reset on ,set start
            writeReg(0x0C, 0x08); // pll set:k
            writeReg(0x0D, 0x13); // pll set:k
            writeReg(0x0E, 0xE0); // pll set:k
            writeReg(0x0A, 0x8A); // pll set:
            writeReg(0x0B, 0x08); // pll set:n
            writeReg(0x09, 0x41); // pll set:reset off ,set stop

            i2sConfigClock(cfg);

            writeReg(0x24, 0x08); // adc set
            writeReg(0x36, 0x00); // dac set
            writeReg(0x12, 0x30); // timming set
            writeReg(0x13, 0x20); // timming set

            configI2SADCFormat(fmt);
            setADCBitsPerSample(bitLength);

            configI2SDACFormat(fmt);
            setDACBitsPerSample(bitLength);

            writeReg(0x21, 0x50); // adc set: SEL LIN1 CH+PGAGAIN=0DB
            writeReg(0x22, 0xFF); // adc set: PGA GAIN=0DB
            writeReg(0x21, 0x14); // adc set: SEL LIN1 CH+PGAGAIN=18DB
            writeReg(0x22, 0x55); // pga = +15db
            writeReg(0x08, 0x21); // set class d divider = 33, to avoid the high frequency tone on laudspeaker
            writeReg(0x00, 0x80); //  IC START

            setADCVolume(0, 0); // 0dB
            setDACVolume(0, 0); // 0dB

            writeReg(0x14, 0x8A); //  IC START
            writeReg(0x15, 0x40); //  IC START
            writeReg(0x1A, 0xA0); //  monoout set
            writeReg(0x1B, 0x19); //  monoout set
            writeReg(0x1C, 0x90); //  spk set
            writeReg(0x1D, 0x01); //  spk set
            writeReg(0x1F, 0x00); //  spk set
            writeReg(0x1E, 0x20); //  spk on
            writeReg(0x28, 0x70); //  alc set 0x70
            writeReg(0x26, 0x4E); //  alc set
            writeReg(0x27, 0x10); //  alc set
            writeReg(0x29, 0x00); //  alc set
            writeReg(0x2B, 0x00); //  alc set

            writeReg(0x25, 0x00); //  ADCVOLUME on
            writeReg(0x38, 0x00); //  DACVOLUME on
            writeReg(0x37, 0x30); //  dac set
            writeReg(0x6D, 0x60); // SEL:GPIO1=DMIC CLK OUT+SEL:GPIO2=PLL CLK OUT
            writeReg(0x71, 0x05); // for automute setting
            writeReg(0x73, 0x70);

            configDACOutput(out_channel); // 0x3c Enable DAC and Enable Lout/Rout/1/2
            configADCInput(in_channel);   // 0x00 LINSEL & RINSEL, LIN1/RIN1 as ADC Input DSSEL,use one DS Reg11 DSR, LINPUT1-RINPUT1
            setVoiceVolume(0);

            writeReg(0x37, 0x00); //  dac set
        }

        void i2sConfigClock(const Clock &cfg)
        {
            auto reg = readReg(0x0F); // power up adc and input
            reg &= 0xe0;
            reg |= cfg.mclkDiv;
            writeReg(0x0f, reg);

            auto div = [&] {
                switch (cfg.lclkDiv)
                {
                case Clock::LCLKDiv::LCLK_DIV_128:
                    return 128;
                case Clock::LCLKDiv::LCLK_DIV_192:
                    return 192;
                case Clock::LCLKDiv::LCLK_DIV_256:
                    return 256;
                case Clock::LCLKDiv::LCLK_DIV_384:
                    return 384;
                case Clock::LCLKDiv::LCLK_DIV_512:
                    return 512;
                case Clock::LCLKDiv::LCLK_DIV_576:
                    return 576;
                case Clock::LCLKDiv::LCLK_DIV_768:
                    return 768;
                case Clock::LCLKDiv::LCLK_DIV_1024:
                    return 1024;
                case Clock::LCLKDiv::LCLK_DIV_1152:
                    return 1152;
                case Clock::LCLKDiv::LCLK_DIV_1408:
                    return 1408;
                case Clock::LCLKDiv::LCLK_DIV_1536:
                    return 1536;
                case Clock::LCLKDiv::LCLK_DIV_2112:
                    return 2112;
                case Clock::LCLKDiv::LCLK_DIV_2304:
                    return 2304;
                case Clock::LCLKDiv::LCLK_DIV_125:
                    return 125;
                case Clock::LCLKDiv::LCLK_DIV_136:
                    return 136;
                case Clock::LCLKDiv::LCLK_DIV_250:
                    return 250;
                case Clock::LCLKDiv::LCLK_DIV_272:
                    return 272;
                case Clock::LCLKDiv::LCLK_DIV_375:
                    return 375;
                case Clock::LCLKDiv::LCLK_DIV_500:
                    return 500;
                case Clock::LCLKDiv::LCLK_DIV_544:
                    return 544;
                case Clock::LCLKDiv::LCLK_DIV_750:
                    return 750;
                case Clock::LCLKDiv::LCLK_DIV_1000:
                    return 1000;
                case Clock::LCLKDiv::LCLK_DIV_1088:
                    return 1088;
                case Clock::LCLKDiv::LCLK_DIV_1496:
                    return 1496;
                case Clock::LCLKDiv::LCLK_DIV_1500:
                    return 1500;

                default:
                    return 1;
                }
            }();

            printf("div = %d, %d\n", div, cfg.lclkDiv);

            auto dacratio_h = div >> 8;
            auto dacratio_l = div & 255;
            writeReg(0x06, dacratio_h); // ADCFsMode,singel SPEED,RATIO=256
            writeReg(0x07, dacratio_l); // ADCFsMode,singel SPEED,RATIO=256
        }

        void configI2SADCFormat(Config::Interface::Format fmt)
        {
            auto reg = readReg(0x10);
            reg &= 0xfc;
            writeReg(0x10, (reg | fmt));
        }

        void configI2SDACFormat(Config::Interface::Format fmt)
        {
            auto reg = readReg(0x11);
            reg &= 0xfc;
            writeReg(0x11, (reg | fmt));
        }

        void setADCBitsPerSample(BitLength bit_per_sample)
        {
            auto bits = bit_per_sample & 0x0f;
            auto reg = readReg(0x10);
            reg &= 0xe3;
            writeReg(0x10, (reg | (bits << 2)));
        }

        void setDACBitsPerSample(BitLength bit_per_sample)
        {
            auto bits = bit_per_sample & 0x0f;
            auto reg = readReg(0x11);
            reg &= 0xe3;
            writeReg(0x11, (reg | (bits << 2)));
        }

        int makeVolumeVal(int volume, int dot)
        {
            return (std::min(96, std::max(0, -volume)) << 1) + (dot >= 5 ? 1 : 0);
        }

        void setADCVolume(int volume, int dot)
        {
            writeReg(0x25, makeVolumeVal(volume, dot));
        }

        void setDACVolume(int volume, int dot)
        {
            writeReg(0x38, makeVolumeVal(volume, dot));
        }

        void configDACOutput(Config::DACMode output)
        {
            writeReg(0x1d, 0x02);

            auto reg = readReg(0x1c); // set spk mixer
            reg |= 0x80;
            writeReg(0x1c, reg);
            writeReg(0x1D, 0x02); // spk set
            writeReg(0x1F, 0x00); // spk set
            writeReg(0x1E, 0xA0); // spk on
        }

        void configADCInput(Config::ADCMode input)
        {
            auto reg = readReg(0x21);
            reg = (reg & 0xcf) | 0x14;
            writeReg(0x21, reg);
            writeReg(0x21, 0x24);
        }

        void codecConfigI2S(Config::Interface::Format interface_fmt,
                            Config::Interface::Bits interface_bits)
        {
            configI2SADCFormat(interface_fmt);

            auto bl = [&] {
                switch (interface_bits)
                {
                default:
                case Config::Interface::ES8374_BIT_LENGTH_16BITS:
                    return BIT_LENGTH_16BITS;
                case Config::Interface::ES8374_BIT_LENGTH_24BITS:
                    return BIT_LENGTH_24BITS;
                case Config::Interface::ES8374_BIT_LENGTH_32BITS:
                    return BIT_LENGTH_32BITS;
                }
            }();
            setADCBitsPerSample(bl);
            setDACBitsPerSample(bl);
        }

        void setMICGain(int gain)
        {
            if ((gain > MIC_GAIN_MIN) && (gain < MIC_GAIN_24DB))
            {
                auto gain_n = int(gain / 3);
                auto reg = (gain_n | (gain_n << 4));
                writeReg(0x22, reg); // MIC PGA
            }
        }

        void setD2sePga(int gain)
        {
            if ((gain > D2SE_PGA_GAIN_MIN) && (gain < D2SE_PGA_GAIN_MAX))
            {
                auto reg = readReg(0x21);
                reg &= 0xfb;
                reg |= gain << 2;
                writeReg(0x21, reg); // MIC PGA
            }
        }

        void setVoiceVolume(int volume)
        {
            if (volume < 0)
            {
                volume = 192;
            }
            else if (volume > 96)
            {
                volume = 0;
            }
            else
            {
                volume = 192 - volume * 2;
            }

            writeReg(0x38, volume);
        }

        static Manager &instance()
        {
            static Manager inst;
            return inst;
        }
    };

} // namespace

void initES8374()
{
    auto &m = Manager::instance();
    printf("initES8374\n");
    m.init();
    m.start(Config::Mode::ES8374_MODE_DECODE);
    //m.start(Config::Mode::ES8374_MODE_BOTH);

    //    m.setDACVolume(0, 0);
    m.setVoiceVolume(96);

    //    m.writeReg(0x1e, 0xa4);
}
