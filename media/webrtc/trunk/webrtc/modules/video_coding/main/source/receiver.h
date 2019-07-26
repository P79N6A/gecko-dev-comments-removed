









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_RECEIVER_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_RECEIVER_H_

#include "webrtc/modules/video_coding/main/source/jitter_buffer.h"
#include "webrtc/modules/video_coding/main/source/packet.h"
#include "webrtc/modules/video_coding/main/source/timing.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"

namespace webrtc {

class Clock;
class VCMEncodedFrame;

enum VCMNackStatus {
  kNackOk,
  kNackNeedMoreMemory,
  kNackKeyFrameRequest
};

enum VCMReceiverState {
  kReceiving,
  kPassive,
  kWaitForPrimaryDecode
};

class VCMReceiver {
 public:
  VCMReceiver(VCMTiming* timing,
              Clock* clock,
              EventFactory* event_factory,
              int32_t vcm_id,
              int32_t receiver_id,
              bool master);
  ~VCMReceiver();

  void Reset();
  int32_t Initialize();
  void UpdateRtt(uint32_t rtt);
  int32_t InsertPacket(const VCMPacket& packet,
                       uint16_t frame_width,
                       uint16_t frame_height);
  VCMEncodedFrame* FrameForDecoding(uint16_t max_wait_time_ms,
                                    int64_t& next_render_time_ms,
                                    bool render_timing = true,
                                    VCMReceiver* dual_receiver = NULL);
  void ReleaseFrame(VCMEncodedFrame* frame);
  void ReceiveStatistics(uint32_t* bitrate, uint32_t* framerate);
  void ReceivedFrameCount(VCMFrameCount* frame_count) const;
  uint32_t DiscardedPackets() const;

  
  void SetNackMode(VCMNackMode nackMode,
                   int low_rtt_nack_threshold_ms,
                   int high_rtt_nack_threshold_ms);
  void SetNackSettings(size_t max_nack_list_size,
                       int max_packet_age_to_nack,
                       int max_incomplete_time_ms);
  VCMNackMode NackMode() const;
  VCMNackStatus NackList(uint16_t* nackList, uint16_t size,
                         uint16_t* nack_list_length);

  
  bool DualDecoderCaughtUp(VCMEncodedFrame* dual_frame,
                           VCMReceiver& dual_receiver) const;
  VCMReceiverState State() const;

  
  int SetMinReceiverDelay(int desired_delay_ms);

  
  void SetDecodeErrorMode(VCMDecodeErrorMode decode_error_mode);
  VCMDecodeErrorMode DecodeErrorMode() const;

  
  
  
  int RenderBufferSizeMs();

 private:
  void CopyJitterBufferStateFromReceiver(const VCMReceiver& receiver);
  void UpdateState(VCMReceiverState new_state);
  void UpdateState(const VCMEncodedFrame& frame);
  static int32_t GenerateReceiverId();

  CriticalSectionWrapper* crit_sect_;
  int32_t vcm_id_;
  Clock* clock_;
  int32_t receiver_id_;
  bool master_;
  VCMJitterBuffer jitter_buffer_;
  VCMTiming* timing_;
  scoped_ptr<EventWrapper> render_wait_event_;
  VCMReceiverState state_;
  int max_video_delay_ms_;

  static int32_t receiver_id_counter_;
};

}  

#endif  
