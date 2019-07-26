









#include "denoising.h"
#include "trace.h"

#include <cstring>

namespace webrtc {

enum { kSubsamplingTime = 0 };       
enum { kSubsamplingWidth = 0 };      
enum { kSubsamplingHeight = 0 };     
enum { kDenoiseFiltParam = 179 };    
enum { kDenoiseFiltParamRec = 77 };  
enum { kDenoiseThreshold = 19200 };  

VPMDenoising::VPMDenoising() :
    _id(0),
    _moment1(NULL),
    _moment2(NULL)
{
    Reset();
}

VPMDenoising::~VPMDenoising()
{
    if (_moment1)
    {
        delete [] _moment1;
        _moment1 = NULL;
    }
    
    if (_moment2)
    {
        delete [] _moment2;
        _moment2 = NULL;
    }
}

WebRtc_Word32
VPMDenoising::ChangeUniqueId(const WebRtc_Word32 id)
{
    _id = id;
    return VPM_OK;
}

void
VPMDenoising::Reset()
{
    _frameSize = 0;
    _denoiseFrameCnt = 0;

    if (_moment1)
    {
        delete [] _moment1;
        _moment1 = NULL;
    }
    
    if (_moment2)
    {
        delete [] _moment2;
        _moment2 = NULL;
    }
}

WebRtc_Word32
VPMDenoising::ProcessFrame(I420VideoFrame* frame)
{
    assert(frame);
    WebRtc_Word32     thevar;
    int               k;
    int               jsub, ksub;
    WebRtc_Word32     diff0;
    WebRtc_UWord32    tmpMoment1;
    WebRtc_UWord32    tmpMoment2;
    WebRtc_UWord32    tmp;
    WebRtc_Word32     numPixelsChanged = 0;

    if (frame->IsZeroSize())
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id,
                     "zero size frame");
        return VPM_GENERAL_ERROR;
    }

    int width = frame->width();
    int height = frame->height();

    
    const WebRtc_UWord32 ysize  = height * width;

    
    if (ysize != _frameSize)
    {
        delete [] _moment1;
        _moment1 = NULL;

        delete [] _moment2;
        _moment2 = NULL;
    }
    _frameSize = ysize;

    if (!_moment1)
    {
        _moment1 = new WebRtc_UWord32[ysize];
        memset(_moment1, 0, sizeof(WebRtc_UWord32)*ysize);
    }
    
    if (!_moment2)
    {
        _moment2 = new WebRtc_UWord32[ysize];
        memset(_moment2, 0, sizeof(WebRtc_UWord32)*ysize);
    }

    
    uint8_t* buffer = frame->buffer(kYPlane);
    for (int i = 0; i < height; i++)
    { 
        k = i * width;
        ksub = ((i >> kSubsamplingHeight) << kSubsamplingHeight) * width;
        for (int j = 0; j < width; j++)
        { 
            jsub = ((j >> kSubsamplingWidth) << kSubsamplingWidth);
            
            tmpMoment1 = _moment1[k + j];
            tmpMoment1 *= kDenoiseFiltParam; 
            tmpMoment1 += ((kDenoiseFiltParamRec *
                          ((WebRtc_UWord32)buffer[k + j])) << 8);
            tmpMoment1 >>= 8; 
            _moment1[k + j] = tmpMoment1;

            tmpMoment2 = _moment2[ksub + jsub];
            if ((ksub == k) && (jsub == j) && (_denoiseFrameCnt == 0))
            {
                tmp = ((WebRtc_UWord32)buffer[k + j] *
                      (WebRtc_UWord32)buffer[k + j]);
                tmpMoment2 *= kDenoiseFiltParam; 
                tmpMoment2 += ((kDenoiseFiltParamRec * tmp)<<8);
                tmpMoment2 >>= 8; 
            }
            _moment2[k + j] = tmpMoment2;
            
            diff0 = ((WebRtc_Word32)buffer[k + j] << 8) - _moment1[k + j];
            
            thevar = _moment2[k + j];
            thevar -= ((_moment1[k + j] * _moment1[k + j]) >> 8);
            





            if ((thevar < kDenoiseThreshold)
                && ((diff0 * diff0 >> 8) < kDenoiseThreshold))
            { 
                buffer[k + j] = (WebRtc_UWord8)(_moment1[k + j] >> 8);
                numPixelsChanged++;
            }
        }
    }

    
    _denoiseFrameCnt++;
    if (_denoiseFrameCnt > kSubsamplingTime)
    {
        _denoiseFrameCnt = 0;
    }

    return numPixelsChanged;
}

} 
