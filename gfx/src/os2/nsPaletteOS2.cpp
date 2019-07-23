



































#include "nsPaletteOS2.h"
#include "nsGfxDefs.h"
#ifdef DEBUG
#include <stdio.h>
#endif

HPAL nsPaletteOS2::hGlobalPalette = NULLHANDLE;
BOOL nsPaletteOS2::fPaletteInitialized = FALSE;
ULONG nsPaletteOS2::aulTable[256];

#define NUM_SYS_COLORS 24

typedef struct _MYRGB {
  BYTE red;
  BYTE green;
  BYTE blue;
} MYRGB;

MYRGB sysColors[NUM_SYS_COLORS] =
{
  0x00, 0x00, 0x00,   
  0x00, 0x00, 0x80,   
  0x00, 0x80, 0x00,   
  0x00, 0x80, 0x80,   
  0x80, 0x00, 0x00,   
  0x80, 0x00, 0x80,   
  0x80, 0x80, 0x00,   
  0x80, 0x80, 0x80,   
  0xCC, 0xCC, 0xCC,   
  0x00, 0x00, 0xFF,   
  0x00, 0xFF, 0x00,   
  0x00, 0xFF, 0xFF,   
  0xFF, 0x00, 0x00,   
  0xFF, 0x00, 0xFF,   
  0xFF, 0xFF, 0x00,   
  0xFE, 0xFE, 0xFE,   

  0xC0, 0xC0, 0xC0,   
  0xFF, 0xFB, 0xF0,   
  0xC0, 0xDC, 0xC0,   
  0xA4, 0xC8, 0xF0,   
  0xA4, 0xA0, 0xA4,   

  0xFF, 0xFF, 0xE4,   

  0x71, 0x71, 0x71,   
  0xEF, 0xEF, 0xEF    

};

void nsPaletteOS2::InitializeGlobalPalette()
{
  fPaletteInitialized = TRUE;
  LONG lCaps;
  HPS hps = ::WinGetScreenPS(HWND_DESKTOP);
  HDC hdc = ::GpiQueryDevice (hps);
  ::DevQueryCaps(hdc, CAPS_ADDITIONAL_GRAPHICS, 1, &lCaps);
  ::WinReleasePS(hps);

  if (lCaps & CAPS_PALETTE_MANAGER) {
    
    int i,j,k,l, ulCurTableEntry = 0;
  
    
    for (i = 0; i < NUM_SYS_COLORS; i++) {
      aulTable[ulCurTableEntry] = MK_RGB(sysColors[i].red, sysColors[i].green, sysColors[i].blue);
      ulCurTableEntry++;
    }
  
    
    
    for (i=0x00;i <= 0xff;i+=0x33) {
      for (j=0x00;j <= 0xff;j+=0x33) {
        for (k=0x00;k <= 0xff ;k+=0x33) {
          for (l=0;l<ulCurTableEntry;l++) {
            if (aulTable[l] == MK_RGB(i, j, k))
              break;
          }
          if (l == ulCurTableEntry) {
            aulTable[ulCurTableEntry] = MK_RGB(i, j, k);
            ulCurTableEntry++;
          }
        }
      }
    }
  
    
    
    ulCurTableEntry--;
  
    
    
    while (ulCurTableEntry < 255) {
      aulTable[ulCurTableEntry] = MK_RGB(254, 254, 254);
      ulCurTableEntry++;
    }
  
    
    aulTable[ulCurTableEntry] = MK_RGB(255, 255, 255);
  
#ifdef DEBUG_mikek
    for (i=0;i<256 ;i++ )
      printf("Entry[%d] in 256 color table is %x\n", i, aulTable[i]);
#endif

    
    hGlobalPalette = ::GpiCreatePalette ((HAB)0, 0,
                                         LCOLF_CONSECRGB, 256, aulTable);
  }
}

void nsPaletteOS2::FreeGlobalPalette()
{
  if (hGlobalPalette) {
    GpiDeletePalette(hGlobalPalette);
    hGlobalPalette = NULLHANDLE;
  }
}

void nsPaletteOS2::SelectGlobalPalette(HPS hps, HWND hwnd)
{
  if (!fPaletteInitialized)
    InitializeGlobalPalette();
  if (hGlobalPalette) {
    GpiSelectPalette(hps, hGlobalPalette);
    if (hwnd != NULLHANDLE) {
      ULONG cclr;
      WinRealizePalette(hwnd, hps, &cclr);
    }
  }
}

LONG nsPaletteOS2::QueryColorIndex(LONG lColor)
{
  for (int i=0;i<256;i++) {
     if (lColor == aulTable[i]) {
        return i;
     }
  }
  return 0;
}
