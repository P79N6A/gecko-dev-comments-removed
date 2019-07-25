




































#ifndef nsNativeThemeColors_h_
#define nsNativeThemeColors_h_

#include "nsToolkit.h"
#import <Cocoa/Cocoa.h>

extern "C" {
  typedef CFTypeRef CUIRendererRef;
  void CUIDraw(CUIRendererRef r, CGRect rect, CGContextRef ctx, CFDictionaryRef options, CFDictionaryRef* result);
}

@interface NSWindow(CoreUIRendererPrivate)
+ (CUIRendererRef)coreUIRenderer;
@end

enum ColorName {
  toolbarTopBorderGrey,
  toolbarFillGrey,
  toolbarBottomBorderGrey,
};

static const int sLeopardThemeColors[][2] = {
  
  
  { 0xC0, 0xE2 }, 
  { 0x96, 0xCA }, 
  { 0x42, 0x89 }, 
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
  if (nsToolkit::OnLionOrLater())
    return sLionThemeColors[name][isMain ? 0 : 1];

  if (nsToolkit::OnSnowLeopardOrLater())
    return sSnowLeopardThemeColors[name][isMain ? 0 : 1];

  return sLeopardThemeColors[name][isMain ? 0 : 1];
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
