
















#ifndef ANDROID_UI_DISPLAY_INFO_H
#define ANDROID_UI_DISPLAY_INFO_H

#include <stdint.h>
#include <sys/types.h>

#include "PixelFormat.h"

namespace android {

struct DisplayInfo {
    uint32_t            w;
    uint32_t            h;
    PixelFormatInfo     pixelFormatInfo;
    uint8_t             orientation;
    uint8_t             reserved[3];
    float               fps;
    float               density;
    float               xdpi;
    float               ydpi;
};


enum {
    DISPLAY_ORIENTATION_0 = 0,
    DISPLAY_ORIENTATION_90 = 1,
    DISPLAY_ORIENTATION_180 = 2,
    DISPLAY_ORIENTATION_270 = 3
};


}; 

#endif 

