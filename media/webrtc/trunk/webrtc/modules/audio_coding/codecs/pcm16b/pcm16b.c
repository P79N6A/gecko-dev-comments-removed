










#include "pcm16b.h"

#include <stdlib.h>

#include "typedefs.h"

#ifdef WEBRTC_ARCH_BIG_ENDIAN
#include "signal_processing_library.h"
#endif

#define HIGHEND 0xFF00
#define LOWEND    0xFF




int16_t WebRtcPcm16b_EncodeW16(int16_t *speechIn16b,
                               int16_t len,
                               int16_t *speechOut16b)
{
#ifdef WEBRTC_ARCH_BIG_ENDIAN
    WEBRTC_SPL_MEMCPY_W16(speechOut16b, speechIn16b, len);
#else
    int i;
    for (i=0;i<len;i++) {
        speechOut16b[i]=(((uint16_t)speechIn16b[i])>>8)|((((uint16_t)speechIn16b[i])<<8)&0xFF00);
    }
#endif
    return(len<<1);
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



int16_t WebRtcPcm16b_DecodeW16(void *inst,
                               int16_t *speechIn16b,
                               int16_t len,
                               int16_t *speechOut16b,
                               int16_t* speechType)
{
#ifdef WEBRTC_ARCH_BIG_ENDIAN
    WEBRTC_SPL_MEMCPY_W8(speechOut16b, speechIn16b, ((len*sizeof(int16_t)+1)>>1));
#else
    int i;
    int samples=len>>1;

    for (i=0;i<samples;i++) {
        speechOut16b[i]=(((uint16_t)speechIn16b[i])>>8)|(((uint16_t)(speechIn16b[i]&0xFF))<<8);
    }
#endif

    *speechType=1;

    
    (void)(inst = NULL);

    return(len>>1);
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
