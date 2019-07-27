




#ifndef nsNativeThemeColors_h_
#define nsNativeThemeColors_h_

#include "nsCocoaFeatures.h"
#import <Cocoa/Cocoa.h>

enum ColorName {
  toolbarTopBorderGrey,
  toolbarFillGrey,
  toolbarBottomBorderGrey,
};

static const int sSnowLeopardThemeColors[][2] = {
  
  
  { 0xD0, 0xF1 }, 
  { 0xA7, 0xD8 }, 
  { 0x51, 0x99 }, 
};

static const int sLionThemeColors[][2] = {
  
  
  { 0xD0, 0xF0 }, 
  { 0xB2, 0xE1 }, 
  { 0x59, 0x87 }, 
};

__attribute__((unused))
static int NativeGreyColorAsInt(ColorName name, BOOL isMain)
{
  if (nsCocoaFeatures::OnLionOrLater())
    return sLionThemeColors[name][isMain ? 0 : 1];

  return sSnowLeopardThemeColors[name][isMain ? 0 : 1];
}

__attribute__((unused))
static float NativeGreyColorAsFloat(ColorName name, BOOL isMain)
{
  return NativeGreyColorAsInt(name, isMain) / 255.0f;
}

__attribute__((unused))
static void DrawNativeGreyColorInRect(CGContextRef context, ColorName name,
                                      CGRect rect, BOOL isMain)
{
  float grey = NativeGreyColorAsFloat(name, isMain);
  CGContextSetRGBFillColor(context, grey, grey, grey, 1.0f);
  CGContextFillRect(context, rect);
}

#endif 
