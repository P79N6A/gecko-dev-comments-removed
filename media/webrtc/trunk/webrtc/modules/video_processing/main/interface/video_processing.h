
















#ifndef WEBRTC_MODULES_INTERFACE_VIDEO_PROCESSING_H
#define WEBRTC_MODULES_INTERFACE_VIDEO_PROCESSING_H

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_processing/main/interface/video_processing_defines.h"












namespace webrtc {

class VideoProcessingModule : public Module {
 public:
  


  struct FrameStats {
      FrameStats() :
          mean(0),
          sum(0),
          num_pixels(0),
          subSamplWidth(0),
          subSamplHeight(0) {
    memset(hist, 0, sizeof(hist));
  }

  uint32_t hist[256];       
  uint32_t mean;            
  uint32_t sum;             
  uint32_t num_pixels;       
  uint8_t  subSamplWidth;   
  uint8_t  subSamplHeight;  
};

  


  enum BrightnessWarning {
    kNoWarning,     
    kDarkWarning,   
    kBrightWarning  
  };

  







  static VideoProcessingModule* Create(int32_t id);

  





  static void Destroy(VideoProcessingModule* module);

  


  virtual int32_t TimeUntilNextProcess() { return -1; }

  


  virtual int32_t Process() { return -1; }

  



  virtual void Reset() = 0;

  











  static int32_t GetFrameStats(FrameStats* stats,
                               const I420VideoFrame& frame);

  








  static bool ValidFrameStats(const FrameStats& stats);

  





  static void ClearFrameStats(FrameStats* stats);

  






  static int32_t ColorEnhancement(I420VideoFrame* frame);

  











  static int32_t Brighten(I420VideoFrame* frame, int delta);

  














  virtual int32_t Deflickering(I420VideoFrame* frame, FrameStats* stats) = 0;

  








  virtual int32_t Denoising(I420VideoFrame* frame) = 0;

  












  virtual int32_t BrightnessDetection(const I420VideoFrame& frame,
                                      const FrameStats& stats) = 0;

  





  




  virtual void EnableTemporalDecimation(bool enable) = 0;

  














  virtual int32_t SetTargetResolution(uint32_t width,
                                      uint32_t height,
                                      uint32_t frame_rate) = 0;

  





  virtual int32_t SetMaxFramerate(uint32_t max_frame_rate) = 0;

  


  virtual uint32_t Decimatedframe_rate() = 0;

  


  virtual uint32_t DecimatedWidth() const = 0;

  


  virtual uint32_t DecimatedHeight() const = 0 ;

  







  virtual void SetInputFrameResampleMode(VideoFrameResampling
                                         resampling_mode) = 0;

  








  virtual int32_t PreprocessFrame(const I420VideoFrame& frame,
                                  I420VideoFrame** processed_frame) = 0;

  


  virtual VideoContentMetrics* ContentMetrics() const = 0 ;

  


  virtual void EnableContentAnalysis(bool enable) = 0;
};

}  

#endif  
