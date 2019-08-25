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

#include <sysctl.h>
#include <encoding.h> // read_cycle()

#define WIDTH 272  /* Buffer width    */
#define HEIGHT 228 /* Buffer height   */

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

  //ROMName[0] = "img/DS4_MSX2.ROM";
  DSKName[0] = "img/alst2_2.dsk";

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
  int SndVolume = 2;
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
    return 1;

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
}

unsigned int Mouse(byte N)
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
