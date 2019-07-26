









#include "color_enhancement.h"
#include "color_enhancement_private.h"
#include "trace.h"
#include <cstdlib>  

namespace webrtc {

namespace VideoProcessing
{ 
    int32_t
    ColorEnhancement(I420VideoFrame* frame)
    {
        assert(frame);
        
        uint8_t* ptrU;
        uint8_t* ptrV;
        uint8_t tempChroma;

        if (frame->IsZeroSize())
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing,
                         -1, "Null frame pointer");
            return VPM_GENERAL_ERROR;
        }

        if (frame->width() == 0 || frame->height() == 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing,
                         -1, "Invalid frame size");
            return VPM_GENERAL_ERROR;
        }

        
        ptrU = frame->buffer(kUPlane);
        ptrV = frame->buffer(kVPlane);
        int size_uv = ((frame->width() + 1) / 2) * ((frame->height() + 1) / 2);

        
        for (int ix = 0; ix < size_uv; ix++)
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
