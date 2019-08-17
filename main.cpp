/*
 * author : Shuichi TAKANO
 * since  : Sat Aug 17 2019 12:49:29
 */

#include <stdio.h>
#include <assert.h>

#include <bsp.h>
#include <spi.h>
#include <fpioa.h>

#include <sd_card/ff.h>
#include <sd_card/sdcard.h>

// kflash -b 1500000 -p /dev/cu.wchusbserial1410 -t fmsx210.bin

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

int core1_function(void *ctx)
{
    uint64_t core = current_coreid();
    printf("Core %ld Hello world\n", core);
    while (1)
        ;
}

int main()
{
    uint64_t core = current_coreid();
    printf("Core %ld Hello world\n", core);
    register_core1(core1_function, NULL);

    sd_test();

    while (1)
        ;
}