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
#include "cardkb.h"
#include "pspad.h"
#include <gpiohs.h>
#include <fpioa.h>
#include <sleep.h>
#include <unistd.h>

#include <sysctl.h>
#include <encoding.h> // read_cycle()

#define WIDTH 272  /* Buffer width    */
#define HEIGHT 228 /* Buffer height   */

static VideoOutMode videoOutMode = VIDEOOUTMODE_LCD;
void setFMSXVideoOutMode(VideoOutMode m)
{
  videoOutMode = m;
}

enum
{
  BUTTON_UP = 2,
  BUTTON_CENTER = 1,
  BUTTON_DOWN = 0,
};

static uint8_t buttonGPIO_[3];
static uint8_t buttonState_ = 0;

void initButton(int i, int pin, int gpio)
{
  buttonGPIO_[i] = gpio;
  fpioa_set_function(pin, (fpioa_function_t)(FUNC_GPIOHS0 + gpio));
  gpiohs_set_drive_mode(gpio, GPIO_DM_INPUT_PULL_UP);
}

int isButtonPushed(int i)
{
  return !gpiohs_get_pin(buttonGPIO_[i]);
}

int fetchButtons()
{
  int v = 0;
  for (int i = 0; i < 3; ++i)
  {
    v |= isButtonPushed(i) << i;
  }

  int edge = (buttonState_ ^ v) & v;
  buttonState_ = v;
  //  printf("s %d\n", buttonState_);
  return edge;
}

enum
{
  UIKEY_F6,
  UIKEY_F7,
  UIKEY_F10,
  UIKEY_ENTER,
  UIKEY_ESC,
  UIKEY_UP,
  UIKEY_DOWN,
};
static uint8_t uiKeyState_ = 0;

int fetchUIKeys()
{
  int v = (isPS2KeyboardPushed(0x0b, 0) ? 1 << UIKEY_F6 : 0) |
          (isPS2KeyboardPushed(0x83, 0) ? 1 << UIKEY_F7 : 0) |
          (isPS2KeyboardPushed(0x09, 0) ? 1 << UIKEY_F10 : 0) |
          (isPS2KeyboardPushed(0x5a, 0) ? 1 << UIKEY_ENTER : 0) |
          (isPS2KeyboardPushed(0x76, 0) ? 1 << UIKEY_ESC : 0) |
          (isPS2KeyboardPushed(0x75, 1) ? 1 << UIKEY_UP : 0) |
          (isPS2KeyboardPushed(0x72, 1) ? 1 << UIKEY_DOWN : 0) |
          0;

  int edge = (uiKeyState_ ^ v) & v;
  uiKeyState_ = v;
  return edge;
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

static int soundVolume = 2;

void HandleKeys(unsigned int Key);
void PutImage(void);

#include "CommonMux.h"

int start_fMSX(const char *rom0, const char *rom1,
               const char *disk0, const char *disk1,
               int ramPages, int vramPages, int volume)
{
  soundVolume = volume;

  if (!InitMachine())
  {
    return -1;
  }

  for (int i = 0; i < MAXCARTS; ++i)
  {
    ROMName[i] = NULL;
  }
  ROMName[0] = rom0;
  ROMName[1] = rom1;

  for (int i = 0; i < MAXDRIVES; ++i)
  {
    DSKName[i] = NULL;
  }
  DSKName[0] = disk0;
  DSKName[1] = disk1;

  chdir("\\");

  //ROMName[0] = "img/DS4_MSX2.ROM";
  //DSKName[0] = "img/ys2_1.dsk";
  //DSKName[0] = "img/alst2_2.dsk";

  Mode = (Mode & ~MSX_MODEL) | MSX_MSX2;
  Mode = (Mode & ~MSX_VIDEO) | MSX_NTSC;
  Mode = (Mode & ~MSX_JOYSTICKS) | MSX_NOJOY1 | MSX_NOJOY2;
  Mode &= ~MSX_MSXDOS2; // DOS2あるといろいろ動かない
                        //  Mode |= MSX_PATCHBDOS;

  RAMPages = ramPages;
  VRAMPages = vramPages;

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

void setVolume(int v)
{
  printf("vol %d\n", v);
  int SndSwitch = (1 << MAXCHANNELS) - 1;
  SetChannels(v, SndSwitch);
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
  setVolume(soundVolume);

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

  int loadPercent = 0;

  if (videoOutMode == VIDEOOUTMODE_LCD)
  {
#if 1
    while (1)
    {
      uint64_t cy = read_cycle();
      uint64_t delta = cy - prevCycle_;
      if (delta >= frameCycles_)
      {
        prevCycle_ = cy;
        loadPercent = (int)(delta * 100 / frameCycles_);
        break;
      }
    }
#endif
    if (lcdIsQVGA())
    {
      lcdDrawHScaleImage(0, (240 - img->H) >> 1, 320,
                         img->W, img->H, img->L, img->Data);
    }
    else
    {
      if (img->W <= 272)
      {
        int x0 = (480 - img->W * 3 / 2) >> 1;
        int y0 = (320 - img->H * 4 / 3) >> 1;
        lcdDrawFixScaleImage320(x0, y0, img->W, img->H, img->L, img->Data);
      }
      else
      {
        int x0 = (480 - img->W * 3 / 4) >> 1;
        int y0 = (320 - img->H * 4 / 3) >> 1;
        lcdDrawFixScaleImage320W(x0, y0, img->W, img->H, img->L, img->Data);
        //      lcdDrawScaleImage(0, 0, 480, 320, img->W, img->H, img->L, img->Data);
      }
    }
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
    loadPercent = (int)(delta * 100 / frameCycles_);
  }
  setRGBLED(loadPercent <= 101 ? LEDCOLOR_GREEN : LEDCOLOR_RED);
  printf("%d%%\n", loadPercent);
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
  updatePSPad();

  SETJOYTYPE(0, isPSPADDetected() ? JOY_STICK : JOY_NONE);
  return makeJoyStateFromPSPad();
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

  fetchCardKB();
  getCardKBKeyState((uint8_t *)KeyState);
  getPSPADKeyState((uint8_t *)KeyState);

  int uiKeys = fetchUIKeys();
  int buttons = fetchButtons();
  if ((uiKeys & (1 << UIKEY_F10)) || (buttons & (1 << BUTTON_CENTER)))
  {
    MenuMSX();
  }
#if 0
  if (uiKeys & (1 << UIKEY_F6))
  {
    LoadSTA(STAName ? STAName : "DEFAULT.STA");
    //    RPLPlay(RPL_OFF);
  }
  if (uiKeys & (1 << UIKEY_F7))
  {
    LoadSTA(STAName ? STAName : "DEFAULT.STA");
    //    RPLPlay(RPL_OFF);
  }
#endif
  if (buttons & (1 << BUTTON_UP))
  {
    soundVolume += 1;
    if (soundVolume >= 255)
    {
      soundVolume = 255;
    }
    setVolume(soundVolume);
  }
  if (buttons & (1 << BUTTON_DOWN))
  {
    soundVolume -= 1;
    if (soundVolume <= 0)
    {
      soundVolume = 0;
    }
    setVolume(soundVolume);
  }
}

unsigned int Mouse(byte N)
{
  return 0;
}

unsigned int GetKey(void)
{
  int uikey = fetchUIKeys();
  int button = fetchButtons();
  if (uikey & (1 << UIKEY_UP) || (button & (1 << BUTTON_UP)))
  {
    return CON_UP;
  }
  if (uikey & (1 << UIKEY_DOWN) || (button & (1 << BUTTON_DOWN)))
  {
    return CON_DOWN;
  }
  if (uikey & (1 << UIKEY_ENTER) || (button & (1 << BUTTON_CENTER)))
  {
    return CON_OK;
  }
  if (uikey & (1 << UIKEY_ESC))
  {
    return CON_EXIT;
  }
  return 0;
}

unsigned int WaitKey(void)
{
  while (1)
  {
    usleep(1000);
    int key = GetKey();
    if (key)
    {
      return key;
    }
  }
  return 0;
}

unsigned int WaitKeyOrMouse(void)
{
  return WaitKey();
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
