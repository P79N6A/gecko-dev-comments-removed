




































#ifndef nsNativeThemeColors_h_
#define nsNativeThemeColors_h_

#import <Cocoa/Cocoa.h>

enum ColorName {
  headerStartGrey,
  headerEndGrey,
  headerBorderGrey
};

static const int sLeopardThemeColors[][2] = {
  
  
  { 0xC5, 0xE9 }, 
  { 0x96, 0xCA }, 
  { 0x42, 0x89 }  
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
