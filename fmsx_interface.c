/*
 * author : Shuichi TAKANO
 * since  : Sun Aug 18 2019 4:21:21
 */

#include "fmsx_interface.h"

#include <MSX.h>
#include <EMULib.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 272  /* Buffer width    */
#define HEIGHT 228 /* Buffer height   */

static Image NormScreen;
static Image WideScreen;
static pixel *WBuf;
static pixel *XBuf;
static unsigned int XPal[80];
static unsigned int BPal[256];
static unsigned int XPal0;

void HandleKeys(unsigned int Key);
void PutImage(void);

#include "CommonMux.h"

int start_fMSX()
{
  if (!InitMachine())
  {
    return -1;
  }

  Mode = (Mode & ~MSX_VIDEO) | MSX_PAL;
  Mode = (Mode & ~MSX_MODEL) | MSX_MSX2;
  Mode = (Mode & ~MSX_JOYSTICKS) | MSX_JOY1 | MSX_JOY2;

  RAMPages = 2;
  VRAMPages = 2;

  Verbose = 1;

  StartMSX(Mode, RAMPages, VRAMPages);
  TrashMSX();
  TrashMachine();
  return 0;
}

unsigned int InitAudio(unsigned int Rate, unsigned int Latency)
{
  return 1;
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

  printf("screen depth = %d\n", NormScreen.D);
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

  return 1;
}

void TrashMachine(void)
{
}

void PutImage(void)
{
  printf("put image.\n");
}

void PlayAllSound(int uSec)
{
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
