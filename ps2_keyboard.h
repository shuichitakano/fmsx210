/*
 * author : Shuichi TAKANO
 * since  : Sat Aug 31 2019 13:44:26
 */
#ifndef _5CAA7E2C_3134_1656_CD1F_159BFFE0628E
#define _5CAA7E2C_3134_1656_CD1F_159BFFE0628E

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int isPS2KeyboardPushed(int code, int e0);

#ifdef __cplusplus
}

#include <gpiohs.h>

class PS2Keyboard
{
public:
    void init(int clkPin, int clkGPIOHS,
              int datPin, int datGPIOHS,
              int prio);
    void start();

    inline bool isPushed(int code, bool e0) const
    {
        return pushed_[(code + (e0 ? 256 : 0)) >> 5] & (1 << (code & 31));
    }

    static PS2Keyboard &instance();

protected:
    void callback();

private:
    uint8_t clkGPIO_;
    uint8_t datGPIO_;
    int prio_;

    uint32_t buffer_;
    int counter_;
    int parity_;

    bool f0_;
    bool e0_;

    uint32_t pushed_[512 / 32]{}; // normal + nullock

    enum class State
    {
        RECEIVING,
        SENDING,
    };
    State state_;
};

#endif

#endif /* _5CAA7E2C_3134_1656_CD1F_159BFFE0628E */
