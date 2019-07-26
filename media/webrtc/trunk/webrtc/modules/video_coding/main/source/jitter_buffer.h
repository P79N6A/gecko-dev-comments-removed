









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_JITTER_BUFFER_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_JITTER_BUFFER_H_

#include <list>
#include <map>
#include <set>
#include <vector>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/main/interface/video_coding.h"
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


class Clock;
class EventFactory;
class EventWrapper;
class VCMFrameBuffer;
class VCMPacket;
class VCMEncodedFrame;

typedef std::list<VCMFrameBuffer*> UnorderedFrameList;

struct VCMJitterSample {
  VCMJitterSample() : timestamp(0), frame_size(0), latest_packet_time(-1) {}
  uint32_t timestamp;
  uint32_t frame_size;
  int64_t latest_packet_time;
};

class TimestampLessThan {
 public:
  bool operator() (const uint32_t& timestamp1,
                   const uint32_t& timestamp2) const {
    return IsNewerTimestamp(timestamp2, timestamp1);
  }
};

class FrameList
    : public std::map<uint32_t, VCMFrameBuffer*, TimestampLessThan> {
 public:
  void InsertFrame(VCMFrameBuffer* frame);
  VCMFrameBuffer* FindFrame(uint16_t seq_num, uint32_t timestamp) const;
  VCMFrameBuffer* PopFrame(uint32_t timestamp);
  VCMFrameBuffer* Front() const;
  VCMFrameBuffer* Back() const;
  int RecycleFramesUntilKeyFrame(FrameList::iterator* key_frame_it,
      UnorderedFrameList* free_frames);
  int CleanUpOldOrEmptyFrames(VCMDecodingState* decoding_state,
      UnorderedFrameList* free_frames);
  void Reset(UnorderedFrameList* free_frames);
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

  
  
  std::map<FrameType, uint32_t> FrameStatistics() const;

  
  
  int num_not_decodable_packets() const;

  
  int num_discarded_packets() const;

  
  void IncomingRateStatistics(unsigned int* framerate,
                              unsigned int* bitrate);

  
  
  
  
  bool CompleteSequenceWithNextFrame();

  
  
  
  bool NextCompleteTimestamp(uint32_t max_wait_time_ms, uint32_t* timestamp);

  
  
  
  bool NextMaybeIncompleteTimestamp(uint32_t* timestamp);

  
  
  VCMEncodedFrame* ExtractAndSetDecode(uint32_t timestamp);

  
  
  void ReleaseFrame(VCMEncodedFrame* frame);

  
  
  
  int64_t LastPacketTime(const VCMEncodedFrame* frame,
                         bool* retransmitted) const;

  
  
  
  VCMFrameBufferEnum InsertPacket(const VCMPacket& packet,
                                  bool* retransmitted);

  
  uint32_t EstimatedJitterMs();

  
  void UpdateRtt(uint32_t rtt_ms);

  
  
  
  
  
  
  void SetNackMode(VCMNackMode mode, int low_rtt_nack_threshold_ms,
                   int high_rtt_nack_threshold_ms);

  void SetNackSettings(size_t max_nack_list_size,
                       int max_packet_age_to_nack,
                       int max_incomplete_time_ms);

  
  VCMNackMode nack_mode() const;

  
  uint16_t* GetNackList(uint16_t* nack_list_size, bool* request_key_frame);

  
  
  void SetDecodeErrorMode(VCMDecodeErrorMode error_mode);
  int64_t LastDecodedTimestamp() const;
  VCMDecodeErrorMode decode_error_mode() const {return decode_error_mode_;}

  
  
  void RenderBufferSize(uint32_t* timestamp_start, uint32_t* timestamp_end);

 private:
  class SequenceNumberLessThan {
   public:
    bool operator() (const uint16_t& sequence_number1,
                     const uint16_t& sequence_number2) const {
      return IsNewerSequenceNumber(sequence_number2, sequence_number1);
    }
  };
  typedef std::set<uint16_t, SequenceNumberLessThan> SequenceNumberSet;

  
  
  
  VCMFrameBufferEnum GetFrame(const VCMPacket& packet, VCMFrameBuffer** frame);
  void CopyFrames(FrameList* to_list, const FrameList& from_list);
  void CopyFrames(FrameList* to_list, const FrameList& from_list, int* index);
  
  
  bool IsContinuousInState(const VCMFrameBuffer& frame,
      const VCMDecodingState& decoding_state) const;
  
  
  bool IsContinuous(const VCMFrameBuffer& frame) const;
  
  
  
  void FindAndInsertContinuousFrames(const VCMFrameBuffer& new_frame);
  VCMFrameBuffer* NextFrame() const;
  
  
  
  bool UpdateNackList(uint16_t sequence_number);
  bool TooLargeNackList() const;
  
  
  bool HandleTooLargeNackList();
  bool MissingTooOldPacket(uint16_t latest_sequence_number) const;
  
  
  
  bool HandleTooOldPackets(uint16_t latest_sequence_number);
  
  void DropPacketsFromNackList(uint16_t last_decoded_sequence_number);

  void ReleaseFrameIfNotDecoding(VCMFrameBuffer* frame);

  
  
  VCMFrameBuffer* GetEmptyFrame();

  
  
  bool TryToIncreaseJitterBufferSize();

  
  
  bool RecycleFramesUntilKeyFrame();

  
  
  
  void CountFrame(const VCMFrameBuffer& frame);

  
  void UpdateAveragePacketsPerFrame(int current_number_packets_);

  
  
  void CleanUpOldOrEmptyFrames();

  
  bool IsPacketRetransmitted(const VCMPacket& packet) const;

  
  
  void UpdateJitterEstimate(const VCMJitterSample& sample,
                            bool incomplete_frame);
  void UpdateJitterEstimate(const VCMFrameBuffer& frame, bool incomplete_frame);
  void UpdateJitterEstimate(int64_t latest_packet_time_ms,
                            uint32_t timestamp,
                            unsigned int frame_size,
                            bool incomplete_frame);

  
  bool WaitForRetransmissions();

  int NonContinuousOrIncompleteDuration();

  uint16_t EstimatedLowSequenceNumber(const VCMFrameBuffer& frame) const;

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
  UnorderedFrameList free_frames_;
  FrameList decodable_frames_;
  FrameList incomplete_frames_;
  VCMDecodingState last_decoded_state_;
  bool first_packet_since_reset_;

  
  
  std::map<FrameType, uint32_t> receive_statistics_;
  
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
  int max_incomplete_time_ms_;

  VCMDecodeErrorMode decode_error_mode_;
  
  float average_packets_per_frame_;
  
  
  int frame_counter_;
  DISALLOW_COPY_AND_ASSIGN(VCMJitterBuffer);
};
}  

#endif  
