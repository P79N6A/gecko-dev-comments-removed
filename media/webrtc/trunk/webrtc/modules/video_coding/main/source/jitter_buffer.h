









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_JITTER_BUFFER_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_JITTER_BUFFER_H_

#include <list>

#include "modules/interface/module_common_types.h"
#include "modules/video_coding/main/interface/video_coding_defines.h"
#include "modules/video_coding/main/source/decoding_state.h"
#include "modules/video_coding/main/source/event.h"
#include "modules/video_coding/main/source/inter_frame_delay.h"
#include "modules/video_coding/main/source/jitter_buffer_common.h"
#include "modules/video_coding/main/source/jitter_estimator.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "typedefs.h"

namespace webrtc {

enum VCMNackMode {
  kNackInfinite,
  kNackHybrid,
  kNoNack
};

typedef std::list<VCMFrameBuffer*> FrameList;


class TickTimeBase;
class VCMFrameBuffer;
class VCMPacket;
class VCMEncodedFrame;

struct VCMJitterSample {
  VCMJitterSample() : timestamp(0), frame_size(0), latest_packet_time(-1) {}
  uint32_t timestamp;
  uint32_t frame_size;
  int64_t latest_packet_time;
};

class VCMJitterBuffer {
 public:
  VCMJitterBuffer(TickTimeBase* clock, int vcm_id = -1, int receiver_id = -1,
                  bool master = true);
  virtual ~VCMJitterBuffer();

  
  void CopyFrom(const VCMJitterBuffer& rhs);

  
  void Start();

  
  void Stop();

  
  bool Running() const;

  
  void Flush();

  
  
  void FrameStatistics(uint32_t* received_delta_frames,
                       uint32_t* received_key_frames) const;

  
  
  int num_not_decodable_packets() const;

  
  int num_discarded_packets() const;

  
  void IncomingRateStatistics(unsigned int* framerate,
                              unsigned int* bitrate);

  
  
  
  
  
  int64_t NextTimestamp(uint32_t max_wait_time_ms,
                        FrameType* incoming_frame_type,
                        int64_t* render_time_ms);

  
  
  
  
  bool CompleteSequenceWithNextFrame();

  
  
  
  VCMEncodedFrame* GetCompleteFrameForDecoding(uint32_t max_wait_time_ms);

  
  VCMEncodedFrame* GetFrameForDecoding();

  
  
  void ReleaseFrame(VCMEncodedFrame* frame);

  
  int GetFrame(const VCMPacket& packet, VCMEncodedFrame*&);
  VCMEncodedFrame* GetFrame(const VCMPacket& packet);  

  
  
  
  int64_t LastPacketTime(VCMEncodedFrame* frame, bool* retransmitted) const;

  
  VCMFrameBufferEnum InsertPacket(VCMEncodedFrame* frame,
                                  const VCMPacket& packet);

  
  uint32_t EstimatedJitterMs();

  
  void UpdateRtt(uint32_t rtt_ms);

  
  
  
  
  
  
  void SetNackMode(VCMNackMode mode, int low_rtt_nack_threshold_ms,
                   int high_rtt_nack_threshold_ms);

  
  VCMNackMode nack_mode() const;

  
  uint16_t* CreateNackList(uint16_t* nack_list_size, bool* list_extended);

  int64_t LastDecodedTimestamp() const;

 private:
  
  
  
  VCMEncodedFrame* GetFrameForDecodingNACK();

  void ReleaseFrameIfNotDecoding(VCMFrameBuffer* frame);

  
  
  VCMFrameBuffer* GetEmptyFrame();

  
  
  bool RecycleFramesUntilKeyFrame();

  
  
  
  VCMFrameBufferEnum UpdateFrameState(VCMFrameBuffer* frame);

  
  
  FrameList::iterator FindOldestCompleteContinuousFrame(bool enable_decodable);

  void CleanUpOldFrames();

  
  
  
  
  
  
  void VerifyAndSetPreviousFrameLost(VCMFrameBuffer* frame);

  
  bool IsPacketRetransmitted(const VCMPacket& packet) const;

  
  
  void UpdateJitterEstimate(const VCMJitterSample& sample,
                            bool incomplete_frame);
  void UpdateJitterEstimate(const VCMFrameBuffer& frame, bool incomplete_frame);
  void UpdateJitterEstimate(int64_t latest_packet_time_ms,
                            uint32_t timestamp,
                            unsigned int frame_size,
                            bool incomplete_frame);

  
  
  
  void GetLowHighSequenceNumbers(int32_t* low_seq_num,
                                 int32_t* high_seq_num) const;

  
  bool WaitForRetransmissions();

  int vcm_id_;
  int receiver_id_;
  TickTimeBase* clock_;
  
  bool running_;
  CriticalSectionWrapper* crit_sect_;
  bool master_;
  
  VCMEvent frame_event_;
  
  VCMEvent packet_event_;
  
  int max_number_of_frames_;
  
  VCMFrameBuffer* frame_buffers_[kMaxNumberOfFrames];
  FrameList frame_list_;
  VCMDecodingState last_decoded_state_;
  bool first_packet_;

  
  int num_not_decodable_packets_;
  
  unsigned int receive_statistics_[4];
  
  unsigned int incoming_frame_rate_;
  unsigned int incoming_frame_count_;
  int64_t time_last_incoming_frame_count_;
  unsigned int incoming_bit_count_;
  unsigned int incoming_bit_rate_;
  unsigned int drop_count_;  
  
  int num_consecutive_old_frames_;
  
  int num_consecutive_old_packets_;
  
  int num_discarded_packets_;

  
  
  VCMJitterEstimator jitter_estimate_;
  
  VCMInterFrameDelay inter_frame_delay_;
  VCMJitterSample waiting_for_completion_;
  WebRtc_UWord32 rtt_ms_;

  
  VCMNackMode nack_mode_;
  int low_rtt_nack_threshold_ms_;
  int high_rtt_nack_threshold_ms_;
  
  int32_t nack_seq_nums_internal_[kNackHistoryLength];
  uint16_t nack_seq_nums_[kNackHistoryLength];
  unsigned int nack_seq_nums_length_;
  bool waiting_for_key_frame_;

  DISALLOW_COPY_AND_ASSIGN(VCMJitterBuffer);
};
}  

#endif  
