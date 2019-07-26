










#include "pcm16b.h"

#include <stdlib.h>

#include "typedefs.h"

#define HIGHEND 0xFF00
#define LOWEND    0xFF




WebRtc_Word16 WebRtcPcm16b_EncodeW16(WebRtc_Word16 *speechIn16b,
                                     WebRtc_Word16 len,
                                     WebRtc_Word16 *speechOut16b)
{
#ifdef WEBRTC_BIG_ENDIAN
    memcpy(speechOut16b, speechIn16b, len * sizeof(WebRtc_Word16));
#else
    int i;
    for (i=0;i<len;i++) {
        speechOut16b[i]=(((WebRtc_UWord16)speechIn16b[i])>>8)|((((WebRtc_UWord16)speechIn16b[i])<<8)&0xFF00);
    }
#endif
    return(len<<1);
}



WebRtc_Word16 WebRtcPcm16b_Encode(WebRtc_Word16 *speech16b,
                                  WebRtc_Word16 len,
                                  unsigned char *speech8b)
{
    WebRtc_Word16 samples=len*2;
    WebRtc_Word16 pos;
    WebRtc_Word16 short1;
    WebRtc_Word16 short2;
    for (pos=0;pos<len;pos++) {
        short1=HIGHEND & speech16b[pos];
        short2=LOWEND & speech16b[pos];
        short1=short1>>8;
        speech8b[pos*2]=(unsigned char) short1;
        speech8b[pos*2+1]=(unsigned char) short2;
    }
    return(samples);
}



WebRtc_Word16 WebRtcPcm16b_DecodeW16(void *inst,
                                     WebRtc_Word16 *speechIn16b,
                                     WebRtc_Word16 len,
                                     WebRtc_Word16 *speechOut16b,
                                     WebRtc_Word16* speechType)
{
#ifdef WEBRTC_BIG_ENDIAN
    memcpy(speechOut16b, speechIn16b, ((len*sizeof(WebRtc_Word16)+1)>>1));
#else
    int i;
    int samples=len>>1;

    for (i=0;i<samples;i++) {
        speechOut16b[i]=(((WebRtc_UWord16)speechIn16b[i])>>8)|(((WebRtc_UWord16)(speechIn16b[i]&0xFF))<<8);
    }
#endif

    *speechType=1;

    
    (void)(inst = NULL);

    return(len>>1);
}


WebRtc_Word16 WebRtcPcm16b_Decode(unsigned char *speech8b,
                                  WebRtc_Word16 len,
                                  WebRtc_Word16 *speech16b)
{
    WebRtc_Word16 samples=len>>1;
    WebRtc_Word16 pos;
    WebRtc_Word16 shortval;
    for (pos=0;pos<samples;pos++) {
        shortval=((unsigned short) speech8b[pos*2]);
        shortval=(shortval<<8)&HIGHEND;
        shortval=shortval|(((unsigned short) speech8b[pos*2+1])&LOWEND);
        speech16b[pos]=shortval;
    }
    return(samples);
}
