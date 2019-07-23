




































#ifndef nsNativeThemeColors_h_
#define nsNativeThemeColors_h_

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

static int NativeGreyColorAsInt(ColorName name, BOOL isMain)
{
  return sLeopardThemeColors[name][isMain ? 0 : 1];
}

static float NativeGreyColorAsFloat(ColorName name, BOOL isMain)
{
  return NativeGreyColorAsInt(name, isMain) / 255.0f;
}

static NSColor* NativeGreyColorAsNSColor(ColorName name, BOOL isMain)
{
  return [NSColor colorWithDeviceWhite:NativeGreyColorAsFloat(name, isMain) alpha:1.0f];
}

#endif 
