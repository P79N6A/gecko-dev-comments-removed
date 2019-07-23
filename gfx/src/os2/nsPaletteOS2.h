



































#ifndef _nspaletteos2_h
#define _nspaletteos2_h

#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include "gfxCore.h"

class NS_GFX nsPaletteOS2 {
public:
  static void FreeGlobalPalette();
  static void InitializeGlobalPalette();
  static void SelectGlobalPalette(HPS hps, HWND hwnd = NULLHANDLE);
  static LONG QueryColorIndex(LONG lColor);
private:
  static HPAL hGlobalPalette;
  static BOOL fPaletteInitialized;
  static ULONG aulTable[256];
};

#endif
