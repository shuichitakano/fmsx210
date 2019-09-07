cmake_minimum_required(VERSION 3.0)

add_definitions(
#    -DBPP16 -DUNIX
#    -DCONDEBUG
    -Werror=unused-const-variable=0
    -Werror=format-overflow=0
    -Wno-unused-but-set-variable
    -Wno-error=stringop-truncation
)

add_library(fmsxlib
    Z80/ConDebug.c
    Z80/Z80.c
    fMSX/MSX.c
    fMSX/V9938.c
    fMSX/I8251.c
    fMSX/Patch.c
    fMSX/Menu.c
    EMULib/SHA1.c
    EMULib/Floppy.c
    EMULib/FDIDisk.c
    EMULib/MCF.c
    EMULib/Hunt.c
    EMULib/I8255.c
    EMULib/YM2413.c
    EMULib/AY8910.c
    EMULib/SCC.c
    EMULib/WD1793.c
    EMULib/EMULib.c
    EMULib/Sound.c
    EMULib/Image.c
    EMULib/Console.c
    EMULib/Record.c
#    EMULib/NetPlay.c
    EMULib/Touch.c
)
