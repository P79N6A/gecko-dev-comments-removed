









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_JITTER_BUFFER_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_JITTER_BUFFER_H_

#include <list>
#include <set>
#include <vector>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/modules/video_coding/main/source/decoding_state.h"
#include "webrtc/modules/video_coding/main/source/inter_frame_delay.h"
#include "webrtc/modules/video_coding/main/source/jitter_buffer_common.h"
#include "webrtc/modules/video_coding/main/source/jitter_estimator.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {

enum VCMNackMode {
  kNack,
  kNoNack
};

typedef std::list<VCMFrameBuffer*> FrameList;


class Clock;
class EventFactory;
class EventWrapper;
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
  VCMJitterBuffer(Clock* clock,
                  EventFactory* event_factory,
                  int vcm_id,
                  int receiver_id,
                  bool master);
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

  
  
  
  
  VCMEncodedFrame* MaybeGetIncompleteFrameForDecoding();

  
  
  void ReleaseFrame(VCMEncodedFrame* frame);

  
  int GetFrame(const VCMPacket& packet, VCMEncodedFrame*&);
  VCMEncodedFrame* GetFrame(const VCMPacket& packet);  

  
  
  
  int64_t LastPacketTime(VCMEncodedFrame* frame, bool* retransmitted) const;

  
  VCMFrameBufferEnum InsertPacket(VCMEncodedFrame* frame,
                                  const VCMPacket& packet);

  
  
  
  void SetMaxJitterEstimate(uint32_t initial_delay_ms);

  
  uint32_t EstimatedJitterMs();

  
  void UpdateRtt(uint32_t rtt_ms);

  
  
  
  
  
  
  void SetNackMode(VCMNackMode mode, int low_rtt_nack_threshold_ms,
                   int high_rtt_nack_threshold_ms);

  void SetNackSettings(size_t max_nack_list_size,
                       int max_packet_age_to_nack);

  
  VCMNackMode nack_mode() const;

  
  uint16_t* GetNackList(uint16_t* nack_list_size, bool* request_key_frame);

  
  void DecodeWithErrors(bool enable) {decode_with_errors_ = enable;}
  int64_t LastDecodedTimestamp() const;
  bool decode_with_errors() const {return decode_with_errors_;}

  
  int RenderBufferSizeMs();

 private:
  class SequenceNumberLessThan {
   public:
    bool operator() (const uint16_t& sequence_number1,
                     const uint16_t& sequence_number2) const {
      return IsNewerSequenceNumber(sequence_number2, sequence_number1);
    }
  };
  typedef std::set<uint16_t, SequenceNumberLessThan> SequenceNumberSet;

  
  
  
  bool UpdateNackList(uint16_t sequence_number);
  bool TooLargeNackList() const;
  
  
  bool HandleTooLargeNackList();
  bool MissingTooOldPacket(uint16_t latest_sequence_number) const;
  
  
  
  bool HandleTooOldPackets(uint16_t latest_sequence_number);
  
  void DropPacketsFromNackList(uint16_t last_decoded_sequence_number);

  void ReleaseFrameIfNotDecoding(VCMFrameBuffer* frame);

  
  
  VCMFrameBuffer* GetEmptyFrame();

  
  
  bool RecycleFramesUntilKeyFrame();

  
  
  
  VCMFrameBufferEnum UpdateFrameState(VCMFrameBuffer* frame);

  
  
  FrameList::iterator FindOldestCompleteContinuousFrame();

  
  
  void CleanUpOldOrEmptyFrames();

  
  
  
  
  
  
  void VerifyAndSetPreviousFrameLost(VCMFrameBuffer* frame);

  
  bool IsPacketRetransmitted(const VCMPacket& packet) const;

  
  
  void UpdateJitterEstimate(const VCMJitterSample& sample,
                            bool incomplete_frame);
  void UpdateJitterEstimate(const VCMFrameBuffer& frame, bool incomplete_frame);
  void UpdateJitterEstimate(int64_t latest_packet_time_ms,
                            uint32_t timestamp,
                            unsigned int frame_size,
                            bool incomplete_frame);

  
  bool WaitForRetransmissions();

  int vcm_id_;
  int receiver_id_;
  Clock* clock_;
  
  bool running_;
  CriticalSectionWrapper* crit_sect_;
  bool master_;
  
  scoped_ptr<EventWrapper> frame_event_;
  
  scoped_ptr<EventWrapper> packet_event_;
  
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
  uint32_t rtt_ms_;

  
  VCMNackMode nack_mode_;
  int low_rtt_nack_threshold_ms_;
  int high_rtt_nack_threshold_ms_;
  
  SequenceNumberSet missing_sequence_numbers_;
  uint16_t latest_received_sequence_number_;
  std::vector<uint16_t> nack_seq_nums_;
  size_t max_nack_list_size_;
  int max_packet_age_to_nack_;  

  bool decode_with_errors_;
  DISALLOW_COPY_AND_ASSIGN(VCMJitterBuffer);
};
}  

#endif  
