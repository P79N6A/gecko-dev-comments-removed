









#include "color_enhancement.h"
#include "color_enhancement_private.h"
#include "trace.h"
#include <cstdlib>  

namespace webrtc {

namespace VideoProcessing
{ 
    WebRtc_Word32
    ColorEnhancement(WebRtc_UWord8* frame,
                     const WebRtc_UWord32 width,
                     const WebRtc_UWord32 height)
    {
        
        WebRtc_UWord8* ptrU;
        WebRtc_UWord8* ptrV;
        WebRtc_UWord8 tempChroma;
        const WebRtc_UWord32 numPixels = width * height;


        if (frame == NULL)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, -1, "Null frame pointer");
            return VPM_GENERAL_ERROR;
        }

        if (width == 0 || height == 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, -1, "Invalid frame size");
            return VPM_GENERAL_ERROR;
        }
        
        
        
        
        
        
        ptrU = frame + numPixels;       
        ptrV = ptrU + (numPixels>>2);

        
        for (WebRtc_UWord32 ix = 0; ix < (numPixels>>2); ix++)
        {
            tempChroma = colorTable[*ptrU][*ptrV];
            *ptrV = colorTable[*ptrV][*ptrU];
            *ptrU = tempChroma;
            
            
            ptrU++;
            ptrV++;
        }
        return VPM_OK;
    }

} 

} 
