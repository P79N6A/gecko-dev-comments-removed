












#ifndef WEBRTC_VIDEO_ENGINE_VIE_SENDER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_SENDER_H_

#include "common_types.h"
#include "engine_configurations.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"
#include "vie_defines.h"

namespace webrtc {

class CriticalSectionWrapper;
class RtpDump;
class Transport;
class VideoCodingModule;

class ViESender: public Transport {
 public:
  ViESender(int engine_id, int channel_id);
  ~ViESender();

  
  int RegisterExternalEncryption(Encryption* encryption);
  int DeregisterExternalEncryption();

  
  int RegisterSendTransport(Transport* transport);
  int DeregisterSendTransport();

  
  int StartRTPDump(const char file_nameUTF8[1024]);
  int StopRTPDump();

  
  virtual int SendPacket(int vie_id, const void* data, int len);
  virtual int SendRTCPPacket(int vie_id, const void* data, int len);

 private:
  int engine_id_;
  int channel_id_;

  scoped_ptr<CriticalSectionWrapper> critsect_;

  Encryption* external_encryption_;
  WebRtc_UWord8* encryption_buffer_;
  Transport* transport_;
  RtpDump* rtp_dump_;
};

}  

#endif  
