








#include "content_analysis.h"
#include "tick_util.h"
#include "system_wrappers/interface/cpu_features_wrapper.h"

#include <math.h>
#include <stdlib.h>

namespace webrtc {

VPMContentAnalysis::VPMContentAnalysis(bool runtime_cpu_detection):
_origFrame(NULL),
_prevFrame(NULL),
_width(0),
_height(0),
_skipNum(1),
_border(8),
_motionMagnitude(0.0f),
_spatialPredErr(0.0f),
_spatialPredErrH(0.0f),
_spatialPredErrV(0.0f),
_firstFrame(true),
_CAInit(false),
_cMetrics(NULL)
{
    ComputeSpatialMetrics = &VPMContentAnalysis::ComputeSpatialMetrics_C;
    TemporalDiffMetric = &VPMContentAnalysis::TemporalDiffMetric_C;

    if (runtime_cpu_detection)
    {
#if defined(WEBRTC_ARCH_X86_FAMILY)
        if (WebRtc_GetCPUInfo(kSSE2))
        {
            ComputeSpatialMetrics =
                          &VPMContentAnalysis::ComputeSpatialMetrics_SSE2;
            TemporalDiffMetric = &VPMContentAnalysis::TemporalDiffMetric_SSE2;
        }
#endif
    }

    Release();
}

VPMContentAnalysis::~VPMContentAnalysis()
{
    Release();
}


VideoContentMetrics*
VPMContentAnalysis::ComputeContentMetrics(const VideoFrame* inputFrame)
{
    if (inputFrame == NULL)
    {
        return NULL;
    }

    
    if (_width != inputFrame->Width() || _height != inputFrame->Height())
    {
        if (VPM_OK != Initialize((WebRtc_UWord16)inputFrame->Width(),
                                 (WebRtc_UWord16)inputFrame->Height()))
        {
            return NULL;
        }
    }

    _origFrame = inputFrame->Buffer();

    
    (this->*ComputeSpatialMetrics)();

    
    if (_firstFrame == false)
        ComputeMotionMetrics();

    
    memcpy(_prevFrame, _origFrame, _width * _height);

    _firstFrame =  false;
    _CAInit = true;

    return ContentMetrics();
}

WebRtc_Word32
VPMContentAnalysis::Release()
{
    if (_cMetrics != NULL)
    {
        delete _cMetrics;
       _cMetrics = NULL;
    }

    if (_prevFrame != NULL)
    {
        delete [] _prevFrame;
        _prevFrame = NULL;
    }

    _width = 0;
    _height = 0;
    _firstFrame = true;

    return VPM_OK;
}

WebRtc_Word32
VPMContentAnalysis::Initialize(WebRtc_UWord16 width, WebRtc_UWord16 height)
{
   _width = width;
   _height = height;
   _firstFrame = true;

    
    
    _skipNum = 1;

    
    if ( (_height >=  576) && (_width >= 704) )
    {
        _skipNum = 2;
    }
    
    if ( (_height >=  1080) && (_width >= 1920) )
    {
        _skipNum = 4;
    }

    if (_cMetrics != NULL)
    {
        delete _cMetrics;
    }

    if (_prevFrame != NULL)
    {
        delete [] _prevFrame;
    }

    
    
    if (_width <= 32 || _height <= 32)
    {
        _CAInit = false;
        return VPM_PARAMETER_ERROR;
    }

    _cMetrics = new VideoContentMetrics();
    if (_cMetrics == NULL)
    {
        return VPM_MEMORY;
    }

    _prevFrame = new WebRtc_UWord8[_width * _height] ; 
    if (_prevFrame == NULL)
    {
        return VPM_MEMORY;
    }

    return VPM_OK;
}




WebRtc_Word32
VPMContentAnalysis::ComputeMotionMetrics()
{

    
    
    (this->*TemporalDiffMetric)();

    return VPM_OK;
}





WebRtc_Word32
VPMContentAnalysis::TemporalDiffMetric_C()
{
    
    WebRtc_UWord16 sizei = _height;
    WebRtc_UWord16 sizej = _width;

    WebRtc_UWord32 tempDiffSum = 0;
    WebRtc_UWord32 pixelSum = 0;
    WebRtc_UWord64 pixelSqSum = 0;

    WebRtc_UWord32 numPixels = 0; 
    WebRtc_UWord32 ssn;

    const WebRtc_Word32 width_end = ((_width - 2*_border) & -16) + _border;

    for(WebRtc_UWord16 i = _border; i < sizei - _border; i += _skipNum)
    {
        for(WebRtc_UWord16 j = _border; j < width_end; j++)
        {
            numPixels += 1;
            ssn =  i * sizej + j;

            WebRtc_UWord8 currPixel  = _origFrame[ssn];
            WebRtc_UWord8 prevPixel  = _prevFrame[ssn];

            tempDiffSum += (WebRtc_UWord32)
                            abs((WebRtc_Word16)(currPixel - prevPixel));
            pixelSum += (WebRtc_UWord32) currPixel;
            pixelSqSum += (WebRtc_UWord64) (currPixel * currPixel);
        }
    }

    
    _motionMagnitude = 0.0f;

    if (tempDiffSum == 0)
    {
        return VPM_OK;
    }

    
    float const tempDiffAvg = (float)tempDiffSum / (float)(numPixels);
    float const pixelSumAvg = (float)pixelSum / (float)(numPixels);
    float const pixelSqSumAvg = (float)pixelSqSum / (float)(numPixels);
    float contrast = pixelSqSumAvg - (pixelSumAvg * pixelSumAvg);

    if (contrast > 0.0)
    {
        contrast = sqrt(contrast);
       _motionMagnitude = tempDiffAvg/contrast;
    }

    return VPM_OK;

}








WebRtc_Word32
VPMContentAnalysis::ComputeSpatialMetrics_C()
{
    
    const WebRtc_UWord16 sizei = _height;
    const WebRtc_UWord16 sizej = _width;

    
    WebRtc_UWord32 pixelMSA = 0;

    WebRtc_UWord32 spatialErrSum = 0;
    WebRtc_UWord32 spatialErrVSum = 0;
    WebRtc_UWord32 spatialErrHSum = 0;

    
    const WebRtc_UWord32 width_end = ((sizej - 2*_border) & -16) + _border;

    for(WebRtc_UWord16 i = _border; i < sizei - _border; i += _skipNum)
    {
        for(WebRtc_UWord16 j = _border; j < width_end; j++)
        {
            WebRtc_UWord32 ssn1,ssn2,ssn3,ssn4,ssn5;

            ssn1=  i * sizej + j;
            ssn2 = (i + 1) * sizej + j; 
            ssn3 = (i - 1) * sizej + j; 
            ssn4 = i * sizej + j + 1;   
            ssn5 = i * sizej + j - 1;   

            WebRtc_UWord16 refPixel1  = _origFrame[ssn1] << 1;
            WebRtc_UWord16 refPixel2  = _origFrame[ssn1] << 2;

            WebRtc_UWord8 bottPixel = _origFrame[ssn2];
            WebRtc_UWord8 topPixel = _origFrame[ssn3];
            WebRtc_UWord8 rightPixel = _origFrame[ssn4];
            WebRtc_UWord8 leftPixel = _origFrame[ssn5];

            spatialErrSum  += (WebRtc_UWord32) abs((WebRtc_Word16)(refPixel2
                            - (WebRtc_UWord16)(bottPixel + topPixel
                                             + leftPixel + rightPixel)));
            spatialErrVSum += (WebRtc_UWord32) abs((WebRtc_Word16)(refPixel1
                            - (WebRtc_UWord16)(bottPixel + topPixel)));
            spatialErrHSum += (WebRtc_UWord32) abs((WebRtc_Word16)(refPixel1
                            - (WebRtc_UWord16)(leftPixel + rightPixel)));

            pixelMSA += _origFrame[ssn1];
        }
    }

    
    const float spatialErr  = (float)(spatialErrSum >> 2);
    const float spatialErrH = (float)(spatialErrHSum >> 1);
    const float spatialErrV = (float)(spatialErrVSum >> 1);
    const float norm = (float)pixelMSA;

    
    _spatialPredErr = spatialErr / norm;

    
    _spatialPredErrH = spatialErrH / norm;

    
    _spatialPredErrV = spatialErrV / norm;

    return VPM_OK;
}

VideoContentMetrics*
VPMContentAnalysis::ContentMetrics()
{
    if (_CAInit == false)
    {
        return NULL;
    }

    _cMetrics->spatial_pred_err = _spatialPredErr;
    _cMetrics->spatial_pred_err_h = _spatialPredErrH;
    _cMetrics->spatial_pred_err_v = _spatialPredErrV;
    
    _cMetrics->motion_magnitude = _motionMagnitude;

    return _cMetrics;

}

} 
