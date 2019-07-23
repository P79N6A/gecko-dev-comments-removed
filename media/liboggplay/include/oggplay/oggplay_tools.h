






































#ifndef __OGGPLAY_TOOLS_H__
#define __OGGPLAY_TOOLS_H__

#include <ogg/ogg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#include <time.h>
#endif


typedef struct _OggPlayYUVChannels {
    unsigned char * ptry;
    unsigned char * ptru;
    unsigned char * ptrv;
    int             y_width;
    int             y_height;
    int             uv_width;
    int             uv_height;
} OggPlayYUVChannels;


typedef struct _OggPlayRGBChannels {
    unsigned char * ptro;
    int             rgb_width;
    int             rgb_height;
} OggPlayRGBChannels;

void 
oggplay_yuv2rgba(const OggPlayYUVChannels *yuv, OggPlayRGBChannels * rgb);

void 
oggplay_yuv2bgra(const OggPlayYUVChannels* yuv, OggPlayRGBChannels * rgb);

void 
oggplay_yuv2argb(const OggPlayYUVChannels *yuv, OggPlayRGBChannels * rgb);

ogg_int64_t
oggplay_sys_time_in_ms(void);

void
oggplay_millisleep(long ms);

#ifdef __cplusplus
}
#endif

#endif

