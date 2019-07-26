















#ifndef ANDROID_UI_DISPLAY_INFO_H
#define ANDROID_UI_DISPLAY_INFO_H

#include <stdint.h>
#include <sys/types.h>

#include <ui/PixelFormat.h>

namespace android {

struct DisplayInfo {
    uint32_t w;
    uint32_t h;
    float xdpi;
    float ydpi;
    float fps;
    float density;
    uint8_t orientation;
    bool secure;
    uint8_t reserved[2];
    
    PixelFormatInfo pixelFormatInfo;
};


enum {
    DISPLAY_ORIENTATION_0 = 0,
    DISPLAY_ORIENTATION_90 = 1,
    DISPLAY_ORIENTATION_180 = 2,
    DISPLAY_ORIENTATION_270 = 3
};

}; 

#endif 
