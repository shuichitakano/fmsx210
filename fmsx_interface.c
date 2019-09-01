/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 18 2019 4:21:21
 */

#include "fmsx_interface.h"

#include <MSX.h>
#include <EMULib.h>
#include <stdio.h>
#include <string.h>
#include <sound.h>
#include "lcd.h"
#include "audio.h"
#include "video_out.h"
#include "led.h"
#include "ps2_keyboard.h"

#include <sysctl.h>
#include <encoding.h> // read_cycle()

#define WIDTH 272  /* Buffer width    */
#define HEIGHT 228 /* Buffer height   */

static VideoOutMode videoOutMode = VIDEOOUTMODE_LCD;
void setFMSXVideoOutMode(VideoOutMode m)
{
  videoOutMode = m;
}

static Image NormScreen;
static Image WideScreen;
static pixel *WBuf;
static pixel *XBuf;
static unsigned int XPal[80];
static unsigned int BPal[256];
static unsigned int XPal0;

static int OldScrMode;

static uint64_t frameCycles_ = 0;
static uint64_t prevCycle_ = 0;

void HandleKeys(unsigned int Key);
void PutImage(void);

#include "CommonMux.h"

int start_fMSX()
{
  if (!InitMachine())
  {
    return -1;
  }

  for (int i = 0; i < MAXCARTS; ++i)
  {
    ROMName[i] = NULL;
  }

  for (int i = 0; i < MAXDRIVES; ++i)
  {
    DSKName[i] = NULL;
  }

  ROMName[0] = "img/DS4_MSX2.ROM";
  //DSKName[0] = "img/alst2_2.dsk";

  Mode = (Mode & ~MSX_MODEL) | MSX_MSX2;
  Mode = (Mode & ~MSX_VIDEO) | MSX_NTSC;
  Mode = (Mode & ~MSX_JOYSTICKS) | MSX_JOY1 | MSX_NOJOY2;

  RAMPages = 4;
  VRAMPages = 2;

  UPeriod = 100;
  Verbose = 1;

  uint64_t clock = sysctl_clock_get_freq(SYSCTL_CLOCK_CPU);
  frameCycles_ = clock * 16688 / 1000000;
  prevCycle_ = read_cycle();
  printf("clock = %d, frame %d\n", (int)clock, (int)frameCycles_);

  StartMSX(Mode, RAMPages, VRAMPages);
  TrashMSX();
  TrashMachine();
  return 0;
}

unsigned int InitAudio(unsigned int Rate, unsigned int Latency)
{
  return Rate;
}

void TrashAudio(void)
{
}

int InitMachine(void)
{
  NewImage(&NormScreen, WIDTH, HEIGHT);
  XBuf = NormScreen.Data;

  NewImage(&WideScreen, WIDTH * 2, HEIGHT);
  WBuf = WideScreen.Data;

  SetScreenDepth(NormScreen.D);

  SetVideo(&NormScreen, 0, 0, WIDTH, HEIGHT);

  for (int i = 0; i < 80; ++i)
  {
    SetColor(i, 0, 0, 0);
  }

  for (int i = 0; i < 256; ++i)
  {
    int r = ((i >> 2) & 0x07) * 255 / 7;
    int g = ((i >> 5) & 0x07) * 255 / 7;
    int b = (i & 0x03) * 255 / 3;
    BPal[i] = r | (g << 8) | (b << 16);
  }
  OldScrMode = 0;

  InitSound(44100, 150);
  int SndSwitch = (1 << MAXCHANNELS) - 1;
  int SndVolume = 10;
  //int SndVolume = 100;
  SetChannels(SndVolume, SndSwitch);

  return 1;
}

void TrashMachine(void)
{
}

void PutImage(void)
{
#if 1
  if (ScrMode != OldScrMode)
  {
    OldScrMode = ScrMode;
    if ((ScrMode == 6) || ((ScrMode == 7) && !ModeYJK) ||
        (ScrMode == MAXSCREEN + 1))
    {
      SetVideo(&WideScreen, 0, 0, WIDTH * 2, HEIGHT);
    }
    else
    {
      SetVideo(&NormScreen, 0, 0, WIDTH, HEIGHT);
    }
  }
#endif
  ShowVideo();
}

int ShowVideo(void)
{
  const Image *img = VideoImg;
  if (!img)
  {
    return 1;
  }

  if (videoOutMode == VIDEOOUTMODE_LCD)
  {
    while (1)
    {
      uint64_t cy = read_cycle();
      uint64_t delta = cy - prevCycle_;
      if (delta >= frameCycles_)
      {
        prevCycle_ = cy;
        printf("%d%%\n", (int)(delta * 100 / frameCycles_));
        break;
      }
    }
    lcdDrawHScaleImage(0, (240 - img->H) >> 1, 320,
                       img->W, img->H, img->L, img->Data);
  }
  else
  {
    waitVBlank();

    uint64_t cy = read_cycle();
    uint64_t delta = cy - prevCycle_;
    prevCycle_ = cy;

    if (img->W == WIDTH)
    {
      setVideoImagex4(img->W, img->H, img->L, img->Data);
    }
    else
    {
      setVideoImagex2(img->W, img->H, img->L, img->Data);
    }
    int loadPercent = (int)(delta * 100 / frameCycles_);
    setRGBLED(loadPercent <= 101 ? LEDCOLOR_GREEN : LEDCOLOR_RED);
    //    printf("%d%%\n", loadPercent);
  }
  return 1;
}

void PlayAllSound(int uSec)
{
  int ct = uSec * (1.1f * 44100 / 1000000);
  //int ct = uSec * (2.0f * 44100 / 1000000);
  RenderAndPlayAudio(ct);
}

unsigned int GetFreeAudio(void)
{
  return getAudioBufferFree();
}

unsigned int WriteAudio(sample *Data, unsigned int Length)
{
  writeAudioData(Data, Length);
  return Length;
}

unsigned int Joystick(void)
{
  return 0;
}

void Keyboard(void)
{
  // 0: 76543210
  // 1: ;[@\^-98
  // 2: ba_/.,]:
  // 3: jihgfedc
  // 4: rqponmlk
  // 5: zyxwvuts
  // 6: F3_F2_F1_かな_CAPS_GRAPH_CTRL_SHIFT
  // 7: RET_SELECT_BS_STOP_TAB_ESC_F5_F4
  // 8: →↓↑←_DEL_INS_HOME_SPACE
  // 9: NUM4_NUM3_NUM2_NUM1_NUM0_NUM/_NUM+_NUM*
  // a: NUM._NUM,_NUM-_NUM9_NUM8_NUM7_NUM6_NUM5
  // b: *_*_*_*_No_*_Yes_*

  /* clang-format off */
  // MSXのキーコードに対するPS2のスキャンコードマップ
  // e0系は+256
  static uint16_t codeMap[] = {
    0x45, 0x16, 0x1e, 0x26, 0x25, 0x2e, 0x36, 0x3d,
    0x3e, 0x46, 0x4e, 0x55, 0x6a, 0x54, 0x5b, 0x4c,
    0x52, 0x5d, 0x41, 0x49, 0x4a, 0x51, 0x1c, 0x32,
    0x21, 0x23, 0x24, 0x2b, 0x34, 0x33, 0x43, 0x3b,
    0x42, 0x4b, 0x3a, 0x31, 0x44, 0x4d, 0x15, 0x2d,
    0x1b, 0x2c, 0x3c, 0x2a, 0x1d, 0x22, 0x35, 0x1a,
    0x12, 0x14, 0x11 /*alt*/, 0x58, 0x0e/*全*/, 0x05, 0x06, 0x04, 
    0x0c, 0x03, 0x76, 0x0d, 0x169 /*end*/, 0x66, 0x17a /*PgDn*/, 0x5a,
    0x29, 0x16c, 0x170, 0x171, 0x16b, 0x175, 0x172, 0x174,
    0x7c, 0x79, 0x14a, 0x70, 0x69, 0x72, 0x7a, 0x6b,
    0x73, 0x74, 0x6c, 0x75, 0x7d, 0x7b, 0, 0x71,
    };
  /* clang-format on */

  const uint16_t *p = codeMap;
  for (int i = 0; i < 0xb; ++i)
  {
    int v = 0;
    for (int j = 0; j < 8; ++j)
    {
      int m = *p++;
      v |= isPS2KeyboardPushed(m & 0xff, m & 256) ? 0 : 1 << j;
    }
    KeyState[i] = v;
  }

  static int prevF10 = 0;
  int f10 = isPS2KeyboardPushed(0x09, 0);
  if (!prevF10 && f10)
  {
    MenuMSX();
  }
  prevF10 = f10;
}

unsigned int Mouse(byte N)
{
  return 0;
}

unsigned int GetKey(void)
{
  return 0;
}

unsigned int WaitKey(void)
{
  return 0;
}

unsigned int WaitKeyOrMouse(void)
{
  return 0;
}

void SetColor(byte N, byte R, byte G, byte B)
{
  XPal[N] = (unsigned int)(PIXEL(R, G, B));
}

pixel *GenericNewImage(Image *Img, int Width, int Height);
pixel *NewImage(Image *Img, int Width, int Height)
{
  return GenericNewImage(Img, Width, Height);
}

void GenericSetVideo(Image *Img, int X, int Y, int W, int H);
void SetVideo(Image *Img, int X, int Y, int W, int H)
{
  GenericSetVideo(Img, X, Y, W, H);
}
