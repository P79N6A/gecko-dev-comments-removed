









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_INITIAL_DELAY_MANAGER_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_INITIAL_DELAY_MANAGER_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

namespace acm2 {

class InitialDelayManager {
 public:
  enum PacketType {
    kUndefinedPacket, kCngPacket, kAvtPacket, kAudioPacket, kSyncPacket };

  
  struct SyncStream {
    SyncStream()
        : num_sync_packets(0),
          receive_timestamp(0),
          timestamp_step(0) {
      memset(&rtp_info, 0, sizeof(rtp_info));
    }

    int num_sync_packets;

    
    WebRtcRTPHeader rtp_info;

    
    uint32_t receive_timestamp;

    
    uint32_t timestamp_step;
  };

  InitialDelayManager(int initial_delay_ms, int late_packet_threshold);

  
  
  
  
  
  
  
  
  void UpdateLastReceivedPacket(const WebRtcRTPHeader& header,
                                uint32_t receive_timestamp,
                                PacketType type,
                                bool new_codec,
                                int sample_rate_hz,
                                SyncStream* sync_stream);

  
  
  void LatePackets(uint32_t timestamp_now, SyncStream* sync_stream);

  
  uint32_t playout_timestamp() { return playout_timestamp_; }

  
  
  bool buffering() { return buffering_; }

  
  void DisableBuffering();

  
  bool PacketBuffered() { return last_packet_type_ != kUndefinedPacket; }

 private:
  static const uint8_t kInvalidPayloadType = 0xFF;

  
  
  void UpdatePlayoutTimestamp(const RTPHeader& current_header,
                              int sample_rate_hz);

  
  void RecordLastPacket(const WebRtcRTPHeader& rtp_info,
                        uint32_t receive_timestamp,
                        PacketType type);

  PacketType last_packet_type_;
  WebRtcRTPHeader last_packet_rtp_info_;
  uint32_t last_receive_timestamp_;
  uint32_t timestamp_step_;
  uint8_t audio_payload_type_;
  const int initial_delay_ms_;
  int buffered_audio_ms_;
  bool buffering_;

  
  
  
  
  uint32_t playout_timestamp_;

  
  
  
  const int late_packet_threshold_;
};

}  

}  

#endif  
