








#include "webrtc/modules/video_processing/main/source/content_analysis.h"

#include <math.h>
#include <stdlib.h>

#include "webrtc/system_wrappers/interface/cpu_features_wrapper.h"
#include "webrtc/system_wrappers/interface/tick_util.h"

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
VPMContentAnalysis::ComputeContentMetrics(const I420VideoFrame& inputFrame)
{
    if (inputFrame.IsZeroSize())
    {
        return NULL;
    }

    
    if (_width != inputFrame.width() || _height != inputFrame.height())
    {
        if (VPM_OK != Initialize(inputFrame.width(), inputFrame.height()))
        {
            return NULL;
        }
    }
    
    _origFrame = inputFrame.buffer(kYPlane);

    
    (this->*ComputeSpatialMetrics)();

    
    if (_firstFrame == false)
        ComputeMotionMetrics();

    
    memcpy(_prevFrame, _origFrame, _width * _height);

    _firstFrame =  false;
    _CAInit = true;

    return ContentMetrics();
}

int32_t
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

int32_t
VPMContentAnalysis::Initialize(int width, int height)
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

    _prevFrame = new uint8_t[_width * _height] ; 
    if (_prevFrame == NULL)
    {
        return VPM_MEMORY;
    }

    return VPM_OK;
}




int32_t
VPMContentAnalysis::ComputeMotionMetrics()
{

    
    
    (this->*TemporalDiffMetric)();

    return VPM_OK;
}





int32_t
VPMContentAnalysis::TemporalDiffMetric_C()
{
    
    int sizei = _height;
    int sizej = _width;

    uint32_t tempDiffSum = 0;
    uint32_t pixelSum = 0;
    uint64_t pixelSqSum = 0;

    uint32_t numPixels = 0; 

    const int width_end = ((_width - 2*_border) & -16) + _border;

    for(int i = _border; i < sizei - _border; i += _skipNum)
    {
        for(int j = _border; j < width_end; j++)
        {
            numPixels += 1;
            int ssn =  i * sizej + j;

            uint8_t currPixel  = _origFrame[ssn];
            uint8_t prevPixel  = _prevFrame[ssn];

            tempDiffSum += (uint32_t)
                            abs((int16_t)(currPixel - prevPixel));
            pixelSum += (uint32_t) currPixel;
            pixelSqSum += (uint64_t) (currPixel * currPixel);
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








int32_t
VPMContentAnalysis::ComputeSpatialMetrics_C()
{
    
    const int sizei = _height;
    const int sizej = _width;

    
    uint32_t pixelMSA = 0;

    uint32_t spatialErrSum = 0;
    uint32_t spatialErrVSum = 0;
    uint32_t spatialErrHSum = 0;

    
    const int width_end = ((sizej - 2*_border) & -16) + _border;

    for(int i = _border; i < sizei - _border; i += _skipNum)
    {
        for(int j = _border; j < width_end; j++)
        {

            int ssn1=  i * sizej + j;
            int ssn2 = (i + 1) * sizej + j; 
            int ssn3 = (i - 1) * sizej + j; 
            int ssn4 = i * sizej + j + 1;   
            int ssn5 = i * sizej + j - 1;   

            uint16_t refPixel1  = _origFrame[ssn1] << 1;
            uint16_t refPixel2  = _origFrame[ssn1] << 2;

            uint8_t bottPixel = _origFrame[ssn2];
            uint8_t topPixel = _origFrame[ssn3];
            uint8_t rightPixel = _origFrame[ssn4];
            uint8_t leftPixel = _origFrame[ssn5];

            spatialErrSum  += (uint32_t) abs((int16_t)(refPixel2
                            - (uint16_t)(bottPixel + topPixel
                                             + leftPixel + rightPixel)));
            spatialErrVSum += (uint32_t) abs((int16_t)(refPixel1
                            - (uint16_t)(bottPixel + topPixel)));
            spatialErrHSum += (uint32_t) abs((int16_t)(refPixel1
                            - (uint16_t)(leftPixel + rightPixel)));

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
