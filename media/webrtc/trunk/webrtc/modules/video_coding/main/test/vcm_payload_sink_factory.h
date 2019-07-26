









#include <string>
#include <vector>

#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/modules/video_coding/main/test/rtp_player.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

class NullEventFactory;

namespace webrtc {
class Clock;
class CriticalSectionWrapper;

namespace rtpplayer {
class VcmPayloadSinkFactory : public PayloadSinkFactoryInterface {
 public:
  VcmPayloadSinkFactory(const std::string& base_out_filename,
                        Clock* clock, bool protection_enabled,
                        VCMVideoProtection protection_method,
                        uint32_t rtt_ms, uint32_t render_delay_ms,
                        uint32_t min_playout_delay_ms);
  virtual ~VcmPayloadSinkFactory();

  
  virtual PayloadSinkInterface* Create(RtpStreamInterface* stream);

  int DecodeAndProcessAll(bool decode_dual_frame);
  bool ProcessAll();
  bool DecodeAll();

 private:
  class VcmPayloadSink;
  friend class VcmPayloadSink;
  typedef std::vector<VcmPayloadSink*> Sinks;

  void Remove(VcmPayloadSink* sink);

  std::string base_out_filename_;
  Clock* clock_;
  bool protection_enabled_;
  VCMVideoProtection protection_method_;
  uint32_t rtt_ms_;
  uint32_t render_delay_ms_;
  uint32_t min_playout_delay_ms_;
  scoped_ptr<NullEventFactory> null_event_factory_;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  Sinks sinks_;
  int next_id_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(VcmPayloadSinkFactory);
};
}  
}  
