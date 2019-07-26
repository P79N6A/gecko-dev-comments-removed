









#ifndef WEBRTC_VIDEO_ENGINE_VIE_FRAME_PROVIDER_BASE_H_
#define WEBRTC_VIDEO_ENGINE_VIE_FRAME_PROVIDER_BASE_H_

#include <vector>

#include "common_types.h"  
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"  

namespace webrtc {

class CriticalSectionWrapper;
class VideoEncoder;
class I420VideoFrame;



class ViEFrameCallback {
 public:
  virtual void DeliverFrame(int id,
                            I420VideoFrame* video_frame,
                            int num_csrcs = 0,
                            const WebRtc_UWord32 CSRC[kRtpCsrcSize] = NULL) = 0;

  
  
  virtual void DelayChanged(int id, int frame_delay) = 0;

  
  virtual int GetPreferedFrameSettings(int* width,
                                       int* height,
                                       int* frame_rate) = 0;

  
  
  virtual void ProviderDestroyed(int id) = 0;

  virtual ~ViEFrameCallback() {}
};



class ViEFrameProviderBase {
 public:
  ViEFrameProviderBase(int Id, int engine_id);
  virtual ~ViEFrameProviderBase();

  
  int Id();

  
  virtual int RegisterFrameCallback(int observer_id,
                                    ViEFrameCallback* callback_object);

  virtual int DeregisterFrameCallback(const ViEFrameCallback* callback_object);

  virtual bool IsFrameCallbackRegistered(
      const ViEFrameCallback* callback_object);

  int NumberOfRegisteredFrameCallbacks();

  
  
  
  virtual int FrameCallbackChanged() = 0;

 protected:
  void DeliverFrame(I420VideoFrame* video_frame,
                    int num_csrcs = 0,
                    const WebRtc_UWord32 CSRC[kRtpCsrcSize] = NULL);
  void SetFrameDelay(int frame_delay);
  int FrameDelay();
  int GetBestFormat(int* best_width,
                    int* best_height,
                    int* best_frame_rate);

  int id_;
  int engine_id_;

  
  typedef std::vector<ViEFrameCallback*> FrameCallbacks;
  FrameCallbacks frame_callbacks_;
  scoped_ptr<CriticalSectionWrapper> provider_cs_;

 private:
  scoped_ptr<I420VideoFrame> extra_frame_;
  int frame_delay_;
};

}  

#endif  
