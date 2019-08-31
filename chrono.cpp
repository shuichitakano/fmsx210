/*
 * author : Shuichi TAKANO
 * since  : Thu Aug 29 2019 1:19:5
 */

#include "chrono.h"
#include <encoding.h> // read_cycle()
#include <sysctl.h>
#include <bsp.h>

namespace
{

constexpr int CORE_COUNT = 2;

uint64_t cpuClock_ = 0;
clock_t prevClock_[CORE_COUNT];

} // namespace

void initChrono()
{
    cpuClock_ = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
    prevClock_[0] = prevClock_[1] = read_cycle();

    printf("cpu clock: %d\n", (int)cpuClock_);
}

clock_t getClockCounter()
{
    return read_cycle();
}

uint32_t clockToMicroSec(clock_t v)
{
    return v * 1000000 / cpuClock_;
}

uint32_t getTickTimeInMicroSec()
{
    uint64_t core = current_coreid();
    auto prev = prevClock_[core];
    auto clk = read_cycle();
    auto r = clockToMicroSec(clk - prev);
    prevClock_[core] = clk;
    return r;
}
