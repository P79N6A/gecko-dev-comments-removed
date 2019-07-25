
















#ifndef WEBRTC_MODULES_INTERFACE_VIDEO_PROCESSING_H
#define WEBRTC_MODULES_INTERFACE_VIDEO_PROCESSING_H

#include "module.h"
#include "module_common_types.h"
#include "video_processing_defines.h"












namespace webrtc {

class VideoProcessingModule : public Module
{
public:
    


    struct FrameStats
    {
        FrameStats() :
            mean(0),
            sum(0),
            numPixels(0),
            subSamplWidth(0),
            subSamplHeight(0)
        {
            memset(hist, 0, sizeof(hist));
        }

        WebRtc_UWord32 hist[256];      
        WebRtc_UWord32 mean;           
        WebRtc_UWord32 sum;            
        WebRtc_UWord32 numPixels;      
        WebRtc_UWord8  subSamplWidth;  
        WebRtc_UWord8  subSamplHeight; 
    };

    


    enum BrightnessWarning 
    {
        kNoWarning,                
        kDarkWarning,              
        kBrightWarning            
    };

    







    static VideoProcessingModule* Create(WebRtc_Word32 id);

    





    static void Destroy(VideoProcessingModule* module);

    


    virtual WebRtc_Word32 TimeUntilNextProcess() { return -1; }

    


    virtual WebRtc_Word32 Process() { return -1; }

    



    virtual void Reset() = 0;

    

















    static WebRtc_Word32 GetFrameStats(FrameStats& stats,
                                     const WebRtc_UWord8* frame,
                                     WebRtc_UWord32 width,
                                     WebRtc_UWord32 height);

    


     static WebRtc_Word32 GetFrameStats(FrameStats& stats,
                                     const VideoFrame& frame);

    








    static bool ValidFrameStats(const FrameStats& stats);

    





    static void ClearFrameStats(FrameStats& stats);

    














    static WebRtc_Word32 ColorEnhancement(WebRtc_UWord8* frame,
                                        WebRtc_UWord32 width,
                                        WebRtc_UWord32 height);

    


    static WebRtc_Word32 ColorEnhancement(VideoFrame& frame);

    

















    static WebRtc_Word32 Brighten(WebRtc_UWord8* frame,
                                  int width, int height, int delta);
    


    static WebRtc_Word32 Brighten(VideoFrame& frame, int delta);

    























    virtual WebRtc_Word32 Deflickering(WebRtc_UWord8* frame,
                                     WebRtc_UWord32 width,
                                     WebRtc_UWord32 height,
                                     WebRtc_UWord32 timestamp,
                                     FrameStats& stats) = 0;
    
    


    virtual WebRtc_Word32 Deflickering(VideoFrame& frame,
                                     FrameStats& stats) = 0;

    














    virtual WebRtc_Word32 Denoising(WebRtc_UWord8* frame,
                                  WebRtc_UWord32 width,
                                  WebRtc_UWord32 height) = 0;
    
    


    virtual WebRtc_Word32 Denoising(VideoFrame& frame) = 0;

    


















    virtual WebRtc_Word32 BrightnessDetection(const WebRtc_UWord8* frame,
                                            WebRtc_UWord32 width,
                                            WebRtc_UWord32 height,
                                            const FrameStats& stats) = 0;

    


    virtual WebRtc_Word32 BrightnessDetection(const VideoFrame& frame,
                                            const FrameStats& stats) = 0;


    



	
    




    virtual void EnableTemporalDecimation(bool enable) = 0;
	
    














    virtual WebRtc_Word32 SetTargetResolution(WebRtc_UWord32 width, WebRtc_UWord32 height, WebRtc_UWord32 frameRate) = 0;
    
    





    virtual WebRtc_Word32 SetMaxFrameRate(WebRtc_UWord32 maxFrameRate) = 0;

    


    virtual WebRtc_UWord32 DecimatedFrameRate() = 0;
	
    


    virtual WebRtc_UWord32 DecimatedWidth() const = 0;

    


    virtual WebRtc_UWord32 DecimatedHeight() const = 0 ;

    






    virtual void SetInputFrameResampleMode(VideoFrameResampling resamplingMode) = 0;
  
    








    virtual WebRtc_Word32 PreprocessFrame(const VideoFrame* frame, VideoFrame** processedFrame) = 0;

    


    virtual VideoContentMetrics* ContentMetrics() const = 0 ;

    


    virtual void EnableContentAnalysis(bool enable) = 0;

};

} 

#endif
