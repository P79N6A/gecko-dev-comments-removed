











#ifndef WEBRTC_VIDEO_ENGINE_VIE_SENDER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_SENDER_H_

#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"
#include "webrtc/video_engine/vie_defines.h"

namespace webrtc {

class CriticalSectionWrapper;
class RtpDump;
class Transport;
class VideoCodingModule;

class ViESender: public Transport {
 public:
  explicit ViESender(const int32_t channel_id);
  ~ViESender();

  
  int RegisterSendTransport(Transport* transport);
  int DeregisterSendTransport();

  
  int StartRTPDump(const char file_nameUTF8[1024]);
  int StopRTPDump();

  
  virtual int SendPacket(int vie_id, const void* data, int len);
  virtual int SendRTCPPacket(int vie_id, const void* data, int len);

 private:
  const int32_t channel_id_;

  scoped_ptr<CriticalSectionWrapper> critsect_;

  Transport* transport_;
  RtpDump* rtp_dump_;
};

}  

#endif  
