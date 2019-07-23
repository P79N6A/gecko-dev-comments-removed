




































#ifndef nsNativeThemeColors_h_
#define nsNativeThemeColors_h_

#include "nsToolkit.h"
#import <Cocoa/Cocoa.h>

enum ColorName {
  headerStartGrey,
  headerEndGrey,
  headerBorderGrey,
  toolbarTopBorderGrey,
  statusbarFirstTopBorderGrey,
  statusbarSecondTopBorderGrey,
  statusbarGradientStartGrey,
  statusbarGradientEndGrey
};

static const int sLeopardThemeColors[][2] = {
  
  
  { 0xC5, 0xE9 }, 
  { 0x96, 0xCA }, 
  { 0x42, 0x89 }, 
  { 0xC0, 0xE2 }, 
  
  { 0x42, 0x86 }, 
  { 0xD8, 0xEE }, 
  { 0xBD, 0xE4 }, 
  { 0x96, 0xCF }  
};

static const int sSnowLeopardThemeColors[][2] = {
  
  
  { 0xD1, 0xEE }, 
  { 0xA7, 0xD8 }, 
  { 0x51, 0x99 }, 
  { 0xD0, 0xF1 }, 
  
  { 0x51, 0x99 }, 
  { 0xE8, 0xF6 }, 
  { 0xCB, 0xEA }, 
  { 0xA7, 0xDE }  
};

static int NativeGreyColorAsInt(ColorName name, BOOL isMain)
{
  if (nsToolkit::OnSnowLeopardOrLater())
    return sSnowLeopardThemeColors[name][isMain ? 0 : 1];

  return sLeopardThemeColors[name][isMain ? 0 : 1];
}

static float NativeGreyColorAsFloat(ColorName name, BOOL isMain)
{
  return NativeGreyColorAsInt(name, isMain) / 255.0f;
}

static void DrawNativeGreyColorInRect(CGContextRef context, ColorName name,
                                      CGRect rect, BOOL isMain)
{
  float grey = NativeGreyColorAsFloat(name, isMain);
  CGContextSetRGBFillColor(context, grey, grey, grey, 1.0f);
  CGContextFillRect(context, rect);
}

#endif 
