










#include "webrtc_cng.h"
#include "signal_processing_library.h"
#include "typedefs.h"
#include "cng_helpfuns.h"


#ifdef __cplusplus
extern "C" {
#endif


void WebRtcCng_K2a16( 
    WebRtc_Word16 *k,           
    int            useOrder,
    WebRtc_Word16 *a            
)
{
    WebRtc_Word16 any[WEBRTC_SPL_MAX_LPC_ORDER+1];
    WebRtc_Word16 *aptr, *aptr2, *anyptr;
    G_CONST WebRtc_Word16 *kptr;
    int m, i;
    
    kptr = k;
    *a   = 4096;  
     *any = *a;
    a[1] = (*k+4) >> 3;
    for( m=1; m<useOrder; m++ )
    {
        kptr++;
        aptr = a;
        aptr++;
        aptr2 = &a[m];
        anyptr = any;
        anyptr++;

        any[m+1] = (*kptr+4) >> 3;
        for( i=0; i<m; i++ ) {
            *anyptr++ = (*aptr++) + (WebRtc_Word16)( (( (WebRtc_Word32)(*aptr2--) * (WebRtc_Word32)*kptr )+16384) >> 15);
        }

        aptr   = a;
        anyptr = any;
        for( i=0; i<(m+2); i++ ){
            *aptr++ = *anyptr++;
        }
    }
}


#ifdef __cplusplus
}
#endif

