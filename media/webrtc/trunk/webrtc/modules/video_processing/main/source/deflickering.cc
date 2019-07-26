










#include <math.h>
#include <stdlib.h>

#include "deflickering.h"
#include "trace.h"
#include "signal_processing_library.h"
#include "sort.h"

namespace webrtc {


enum { kFrequencyDeviation = 39 };      
enum { kMinFrequencyToDetect = 32 };    
enum { kNumFlickerBeforeDetect = 2 };   
enum { kMeanValueScaling = 4 };         
enum { kZeroCrossingDeadzone = 10 };    



enum { kDownsamplingFactor = 8 };
enum { kLog2OfDownsamplingFactor = 3 };






const WebRtc_UWord16 VPMDeflickering::_probUW16[kNumProbs] =
    {102, 205, 410, 614, 819, 1024, 1229, 1434, 1638, 1843, 1946, 1987}; 





const WebRtc_UWord16 VPMDeflickering::_weightUW16[kNumQuants - kMaxOnlyLength] =
    {16384, 18432, 20480, 22528, 24576, 26624, 28672, 30720, 32768}; 
 
VPMDeflickering::VPMDeflickering() :
    _id(0)
{
    Reset();
}

VPMDeflickering::~VPMDeflickering()
{
}

WebRtc_Word32
VPMDeflickering::ChangeUniqueId(const WebRtc_Word32 id)
{
    _id = id;
    return 0;
}

void
VPMDeflickering::Reset()
{
    _meanBufferLength = 0;
    _detectionState = 0;
    _frameRate = 0;

    memset(_meanBuffer, 0, sizeof(WebRtc_Word32) * kMeanBufferLength);
    memset(_timestampBuffer, 0, sizeof(WebRtc_Word32) * kMeanBufferLength);

    
    _quantHistUW8[0][0] = 0;
    _quantHistUW8[0][kNumQuants - 1] = 255;
    for (WebRtc_Word32 i = 0; i < kNumProbs; i++)
    {
        _quantHistUW8[0][i + 1] = static_cast<WebRtc_UWord8>((WEBRTC_SPL_UMUL_16_16(
            _probUW16[i], 255) + (1 << 10)) >> 11); 
    }
    
    for (WebRtc_Word32 i = 1; i < kFrameHistorySize; i++)
    {
        memcpy(_quantHistUW8[i], _quantHistUW8[0], sizeof(WebRtc_UWord8) * kNumQuants);
    }
}

WebRtc_Word32
VPMDeflickering::ProcessFrame(I420VideoFrame* frame,
                              VideoProcessingModule::FrameStats* stats)
{
    assert(frame);
    WebRtc_UWord32 frameMemory;
    WebRtc_UWord8 quantUW8[kNumQuants];
    WebRtc_UWord8 maxQuantUW8[kNumQuants];
    WebRtc_UWord8 minQuantUW8[kNumQuants];
    WebRtc_UWord16 targetQuantUW16[kNumQuants];
    WebRtc_UWord16 incrementUW16;
    WebRtc_UWord8 mapUW8[256];

    WebRtc_UWord16 tmpUW16;
    WebRtc_UWord32 tmpUW32;
    int width = frame->width();
    int height = frame->height();

    if (frame->IsZeroSize())
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id,
                     "Null frame pointer");
        return VPM_GENERAL_ERROR;
    }

    
    if (height < 2)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id,
                     "Invalid frame size");
        return VPM_GENERAL_ERROR;
    }

    if (!VideoProcessingModule::ValidFrameStats(*stats))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id,
                     "Invalid frame stats");
        return VPM_GENERAL_ERROR;
    }

    if (PreDetection(frame->timestamp(), *stats) == -1)
    {
        return VPM_GENERAL_ERROR;
    }

    
    WebRtc_Word32 detFlicker = DetectFlicker();
    if (detFlicker < 0)
    { 
        return VPM_GENERAL_ERROR;
    }
    else if (detFlicker != 1)
    {
        return 0;
    }

    
    const WebRtc_UWord32 ySize = height * width;

    const WebRtc_UWord32 ySubSize = width * (((height - 1) >>
        kLog2OfDownsamplingFactor) + 1);
    WebRtc_UWord8* ySorted = new WebRtc_UWord8[ySubSize];
    WebRtc_UWord32 sortRowIdx = 0;
    for (int i = 0; i < height; i += kDownsamplingFactor)
    {
        memcpy(ySorted + sortRowIdx * width,
               frame->buffer(kYPlane) + i * width, width);
        sortRowIdx++;
    }
    
    webrtc::Sort(ySorted, ySubSize, webrtc::TYPE_UWord8);

    WebRtc_UWord32 probIdxUW32 = 0;
    quantUW8[0] = 0;
    quantUW8[kNumQuants - 1] = 255;

    
    
    if (ySubSize > (1 << 21) - 1)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id, 
            "Subsampled number of pixels too large");
        return -1;
    }

    for (WebRtc_Word32 i = 0; i < kNumProbs; i++)
    {
        probIdxUW32 = WEBRTC_SPL_UMUL_32_16(ySubSize, _probUW16[i]) >> 11; 
        quantUW8[i + 1] = ySorted[probIdxUW32];
    }

    delete [] ySorted;
    ySorted = NULL;

    
    memmove(_quantHistUW8[1], _quantHistUW8[0], (kFrameHistorySize - 1) * kNumQuants *
        sizeof(WebRtc_UWord8));
    
    memcpy(_quantHistUW8[0], quantUW8, kNumQuants * sizeof(WebRtc_UWord8));

    
    
    frameMemory = (_frameRate + (1 << 5)) >> 5; 
                                                
    if (frameMemory > kFrameHistorySize)
    {
        frameMemory = kFrameHistorySize;
    }

    
    for (WebRtc_Word32 i = 0; i < kNumQuants; i++)
    {
        maxQuantUW8[i] = 0;
        minQuantUW8[i] = 255;
        for (WebRtc_UWord32 j = 0; j < frameMemory; j++)
        {
            if (_quantHistUW8[j][i] > maxQuantUW8[i])
            {
                maxQuantUW8[i] = _quantHistUW8[j][i];
            }

            if (_quantHistUW8[j][i] < minQuantUW8[i])
            {
                minQuantUW8[i] = _quantHistUW8[j][i];
            }
        }
    }
    
    
    for (WebRtc_Word32 i = 0; i < kNumQuants - kMaxOnlyLength; i++)
    {
        targetQuantUW16[i] = static_cast<WebRtc_UWord16>((WEBRTC_SPL_UMUL_16_16(
            _weightUW16[i], maxQuantUW8[i]) + WEBRTC_SPL_UMUL_16_16((1 << 15) -
            _weightUW16[i], minQuantUW8[i])) >> 8); 
    }

    for (WebRtc_Word32 i = kNumQuants - kMaxOnlyLength; i < kNumQuants; i++)
    {
        targetQuantUW16[i] = ((WebRtc_UWord16)maxQuantUW8[i]) << 7;
    }

    
    WebRtc_UWord16 mapUW16; 
    for (WebRtc_Word32 i = 1; i < kNumQuants; i++)
    {
        
        tmpUW32 = static_cast<WebRtc_UWord32>(targetQuantUW16[i] -
            targetQuantUW16[i - 1]); 
        tmpUW16 = static_cast<WebRtc_UWord16>(quantUW8[i] - quantUW8[i - 1]); 

        if (tmpUW16 > 0)
        {
            incrementUW16 = static_cast<WebRtc_UWord16>(WebRtcSpl_DivU32U16(tmpUW32,
                tmpUW16)); 
         }
        else
        {
            
            incrementUW16 = 0;
        }

        mapUW16 = targetQuantUW16[i - 1];
        for (WebRtc_UWord32 j = quantUW8[i - 1]; j < (WebRtc_UWord32)(quantUW8[i] + 1); j++)
        {
            mapUW8[j] = (WebRtc_UWord8)((mapUW16 + (1 << 6)) >> 7); 
            mapUW16 += incrementUW16;
        }
    }

    
    uint8_t* buffer = frame->buffer(kYPlane);
    for (WebRtc_UWord32 i = 0; i < ySize; i++)
    {
      buffer[i] = mapUW8[buffer[i]];
    }

    
    VideoProcessingModule::ClearFrameStats(stats);

    return 0;
}













WebRtc_Word32
VPMDeflickering::PreDetection(const WebRtc_UWord32 timestamp,
                              const VideoProcessingModule::FrameStats& stats)
{
    WebRtc_Word32 meanVal; 
    WebRtc_UWord32 frameRate = 0;
    WebRtc_Word32 meanBufferLength; 

    meanVal = ((stats.sum << kMeanValueScaling) / stats.numPixels);
    


    memmove(_meanBuffer + 1, _meanBuffer, (kMeanBufferLength - 1) * sizeof(WebRtc_Word32));
    _meanBuffer[0] = meanVal;

    


    memmove(_timestampBuffer + 1, _timestampBuffer, (kMeanBufferLength - 1) *
        sizeof(WebRtc_UWord32));
    _timestampBuffer[0] = timestamp;

    
    if (_timestampBuffer[kMeanBufferLength - 1] != 0)
    {
        frameRate = ((90000 << 4) * (kMeanBufferLength - 1));
        frameRate /= (_timestampBuffer[0] - _timestampBuffer[kMeanBufferLength - 1]);
    }else if (_timestampBuffer[1] != 0)
    {
        frameRate = (90000 << 4) / (_timestampBuffer[0] - _timestampBuffer[1]);
    }

    
    if (frameRate == 0) {
        meanBufferLength = 1;
    }
    else {
        meanBufferLength = (kNumFlickerBeforeDetect * frameRate) / kMinFrequencyToDetect;
    }
    
    if (meanBufferLength >= kMeanBufferLength)
    {
        


        _meanBufferLength = 0;
        return 2;
    }
    _meanBufferLength = meanBufferLength;

    if ((_timestampBuffer[_meanBufferLength - 1] != 0) && (_meanBufferLength != 1))
    {
        frameRate = ((90000 << 4) * (_meanBufferLength - 1));
        frameRate /= (_timestampBuffer[0] - _timestampBuffer[_meanBufferLength - 1]);
    }else if (_timestampBuffer[1] != 0)
    {
        frameRate = (90000 << 4) / (_timestampBuffer[0] - _timestampBuffer[1]);
    }
    _frameRate = frameRate;

    return 0;
}










WebRtc_Word32 VPMDeflickering::DetectFlicker()
{
    
    WebRtc_UWord32  i;
    WebRtc_Word32  freqEst;       
    WebRtc_Word32  retVal = -1;

    
    if (_meanBufferLength < 2)
    {
        
        return(2);
    }
    


    WebRtc_Word32 deadzone = (kZeroCrossingDeadzone << kMeanValueScaling); 
    WebRtc_Word32 meanOfBuffer = 0; 
    WebRtc_Word32 numZeros     = 0; 
    WebRtc_Word32 cntState     = 0; 
    WebRtc_Word32 cntStateOld  = 0; 

    for (i = 0; i < _meanBufferLength; i++)
    {
        meanOfBuffer += _meanBuffer[i];
    }
    meanOfBuffer += (_meanBufferLength >> 1); 
    meanOfBuffer /= _meanBufferLength;

    
    cntStateOld = (_meanBuffer[0] >= (meanOfBuffer + deadzone));
    cntStateOld -= (_meanBuffer[0] <= (meanOfBuffer - deadzone));
    for (i = 1; i < _meanBufferLength; i++)
    {
        cntState = (_meanBuffer[i] >= (meanOfBuffer + deadzone));
        cntState -= (_meanBuffer[i] <= (meanOfBuffer - deadzone));
        if (cntStateOld == 0)
        {
            cntStateOld = -cntState;
        }
        if (((cntState + cntStateOld) == 0) && (cntState != 0))
        {
            numZeros++;
            cntStateOld = cntState;
        }
    }
    

    




    freqEst = ((numZeros * 90000) << 3);
    freqEst /= (_timestampBuffer[0] - _timestampBuffer[_meanBufferLength - 1]);

    
    WebRtc_UWord8 freqState = 0; 
                               
                               
                               
    WebRtc_Word32 freqAlias = freqEst;
    if (freqEst > kMinFrequencyToDetect)
    {
        WebRtc_UWord8 aliasState = 1;
        while(freqState == 0)
        {
            
            freqAlias += (aliasState * _frameRate);
            freqAlias += ((freqEst << 1) * (1 - (aliasState << 1)));
            
            freqState = (abs(freqAlias - (100 << 4)) <= kFrequencyDeviation);
            freqState += (abs(freqAlias - (120 << 4)) <= kFrequencyDeviation);
            freqState += 2 * (freqAlias > ((120 << 4) + kFrequencyDeviation));
            
            aliasState++;
            aliasState &= 0x01;
        }
    }
    
    if (freqState == 1)
    {
        retVal = 1;
    }else if (freqState == 0)
    {
        retVal = 2;
    }else
    {
        retVal = 0;
    }
    return retVal;
}

} 
