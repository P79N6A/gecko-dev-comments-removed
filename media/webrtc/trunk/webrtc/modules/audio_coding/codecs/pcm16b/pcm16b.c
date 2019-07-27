










#include "pcm16b.h"

#include <stdlib.h>
#ifdef WEBRTC_ARCH_BIG_ENDIAN
#include <string.h>
#endif

#include "webrtc/typedefs.h"

#define HIGHEND 0xFF00
#define LOWEND    0xFF




int16_t WebRtcPcm16b_EncodeW16(const int16_t* speechIn16b,
                               int16_t length_samples,
                               int16_t* speechOut16b)
{
#ifdef WEBRTC_ARCH_BIG_ENDIAN
    memcpy(speechOut16b, speechIn16b, length_samples * sizeof(int16_t));
#else
    int i;
    for (i = 0; i < length_samples; i++) {
        speechOut16b[i]=(((uint16_t)speechIn16b[i])>>8)|((((uint16_t)speechIn16b[i])<<8)&0xFF00);
    }
#endif
    return length_samples << 1;
}



int16_t WebRtcPcm16b_Encode(int16_t *speech16b,
                            int16_t len,
                            unsigned char *speech8b)
{
    int16_t samples=len*2;
    int16_t pos;
    int16_t short1;
    int16_t short2;
    for (pos=0;pos<len;pos++) {
        short1=HIGHEND & speech16b[pos];
        short2=LOWEND & speech16b[pos];
        short1=short1>>8;
        speech8b[pos*2]=(unsigned char) short1;
        speech8b[pos*2+1]=(unsigned char) short2;
    }
    return(samples);
}



int16_t WebRtcPcm16b_DecodeW16(int16_t *speechIn16b,
                               int16_t length_bytes,
                               int16_t *speechOut16b,
                               int16_t* speechType)
{
#ifdef WEBRTC_ARCH_BIG_ENDIAN
    memcpy(speechOut16b, speechIn16b, length_bytes);
#else
    int i;
    int samples = length_bytes >> 1;

    for (i=0;i<samples;i++) {
        speechOut16b[i]=(((uint16_t)speechIn16b[i])>>8)|(((uint16_t)(speechIn16b[i]&0xFF))<<8);
    }
#endif

    *speechType=1;

    return length_bytes >> 1;
}


int16_t WebRtcPcm16b_Decode(unsigned char *speech8b,
                            int16_t len,
                            int16_t *speech16b)
{
    int16_t samples=len>>1;
    int16_t pos;
    int16_t shortval;
    for (pos=0;pos<samples;pos++) {
        shortval=((unsigned short) speech8b[pos*2]);
        shortval=(shortval<<8)&HIGHEND;
        shortval=shortval|(((unsigned short) speech8b[pos*2+1])&LOWEND);
        speech16b[pos]=shortval;
    }
    return(samples);
}
