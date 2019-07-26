












#ifndef WEBRTC_VIDEO_ENGINE_ENCODER_STATE_FEEDBACK_H_
#define WEBRTC_VIDEO_ENGINE_ENCODER_STATE_FEEDBACK_H_

#include <map>

#include "system_wrappers/interface/constructor_magic.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"  

namespace webrtc {

class CriticalSectionWrapper;
class EncoderStateFeedbackObserver;
class RtcpIntraFrameObserver;
class ViEEncoder;

class EncoderStateFeedback {
 public:
  friend class EncoderStateFeedbackObserver;

  EncoderStateFeedback();
  ~EncoderStateFeedback();

  
  bool AddEncoder(uint32_t ssrc, ViEEncoder* encoder);

  
  void RemoveEncoder(const ViEEncoder* encoder);

  
  
  RtcpIntraFrameObserver* GetRtcpIntraFrameObserver();

 protected:
  
  void OnReceivedIntraFrameRequest(uint32_t ssrc);
  void OnReceivedSLI(uint32_t ssrc, uint8_t picture_id);
  void OnReceivedRPSI(uint32_t ssrc, uint64_t picture_id);
  void OnLocalSsrcChanged(uint32_t old_ssrc, uint32_t new_ssrc);

 private:
  typedef std::map<uint32_t,  ViEEncoder*> SsrcEncoderMap;

  scoped_ptr<CriticalSectionWrapper> crit_;

  
  scoped_ptr<EncoderStateFeedbackObserver> observer_;

  
  SsrcEncoderMap encoders_;

  DISALLOW_COPY_AND_ASSIGN(EncoderStateFeedback);
};

}  

#endif  
