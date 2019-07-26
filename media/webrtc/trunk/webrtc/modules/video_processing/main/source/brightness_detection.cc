









#include "video_processing.h"
#include "brightness_detection.h"
#include "trace.h"

#include <math.h>

namespace webrtc {

VPMBrightnessDetection::VPMBrightnessDetection() :
    _id(0)
{
    Reset();
}

VPMBrightnessDetection::~VPMBrightnessDetection()
{
}

WebRtc_Word32
VPMBrightnessDetection::ChangeUniqueId(const WebRtc_Word32 id)
{
    _id = id;
    return VPM_OK;
}

void
VPMBrightnessDetection::Reset()
{
    _frameCntBright = 0;
    _frameCntDark = 0;
}

WebRtc_Word32
VPMBrightnessDetection::ProcessFrame(const I420VideoFrame& frame,
                                     const VideoProcessingModule::FrameStats&
                                     stats)
{
    if (frame.IsZeroSize())
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id,
                     "Null frame pointer");
        return VPM_PARAMETER_ERROR;
    }
    int width = frame.width();
    int height = frame.height();

    if (!VideoProcessingModule::ValidFrameStats(stats))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id,
                     "Invalid frame stats");
        return VPM_PARAMETER_ERROR;
    }

    const WebRtc_UWord8 frameCntAlarm = 2;

    
    WebRtc_UWord8 lowTh = 20;
    float propLow = 0;
    for (WebRtc_UWord32 i = 0; i < lowTh; i++)
    {
        propLow += stats.hist[i];
    }
    propLow /= stats.numPixels;

    
    unsigned char highTh = 230;
    float propHigh = 0;
    for (WebRtc_UWord32 i = highTh; i < 256; i++)
    {
        propHigh += stats.hist[i];
    }
    propHigh /= stats.numPixels;

    if(propHigh < 0.4)
    {
        if (stats.mean < 90 || stats.mean > 170)
        {
            
            const uint8_t* buffer = frame.buffer(kYPlane);
            float stdY = 0;
            for (int h = 0; h < height; h += (1 << stats.subSamplHeight))
            {
                int row = h*width;
                for (int w = 0; w < width; w += (1 << stats.subSamplWidth))
                {
                    stdY += (buffer[w + row] - stats.mean) * (buffer[w + row] -
                        stats.mean);
                }
            }           
            stdY = sqrt(stdY / stats.numPixels);

            
            WebRtc_UWord32 sum = 0;
            WebRtc_UWord32 medianY = 140;
            WebRtc_UWord32 perc05 = 0;
            WebRtc_UWord32 perc95 = 255;
            float posPerc05 = stats.numPixels * 0.05f;
            float posMedian = stats.numPixels * 0.5f;
            float posPerc95 = stats.numPixels * 0.95f;
            for (WebRtc_UWord32 i = 0; i < 256; i++)
            {
                sum += stats.hist[i];

                if (sum < posPerc05)
                {
                    perc05 = i;     
                }
                if (sum < posMedian)
                {
                    medianY = i;    
                }
                if (sum < posPerc95)
                {
                    perc95 = i;     
                }
                else
                {
                    break;
                }
            }

            
            if ((stdY < 55) && (perc05 < 50))
            { 
                if (medianY < 60 || stats.mean < 80 ||  perc95 < 130 ||
                    propLow > 0.20)
                {
                    _frameCntDark++;
                }
                else
                {
                    _frameCntDark = 0;
                }
            } 
            else
            {
                _frameCntDark = 0;
            }

            
            if ((stdY < 52) && (perc95 > 200) && (medianY > 160))
            {
                if (medianY > 185 || stats.mean > 185 || perc05 > 140 ||
                    propHigh > 0.25)
                {
                    _frameCntBright++;  
                }
                else 
                {
                    _frameCntBright = 0;
                }
            } 
            else
            {
                _frameCntBright = 0;
            }

        } 
        else
        {
            _frameCntDark = 0;
            _frameCntBright = 0;
        }

    } 
    else
    {
        _frameCntBright++;
        _frameCntDark = 0;
    }
    
    if (_frameCntDark > frameCntAlarm)
    {
        return VideoProcessingModule::kDarkWarning;
    }
    else if (_frameCntBright > frameCntAlarm)
    {
        return VideoProcessingModule::kBrightWarning;
    }
    else
    {
        return VideoProcessingModule::kNoWarning;
    }
}

} 
