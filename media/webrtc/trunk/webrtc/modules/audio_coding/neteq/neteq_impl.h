









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_NETEQ_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_NETEQ_IMPL_H_

#include <vector>

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/modules/audio_coding/neteq/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq/defines.h"
#include "webrtc/modules/audio_coding/neteq/interface/neteq.h"
#include "webrtc/modules/audio_coding/neteq/packet.h"  
#include "webrtc/modules/audio_coding/neteq/random_vector.h"
#include "webrtc/modules/audio_coding/neteq/rtcp.h"
#include "webrtc/modules/audio_coding/neteq/statistics_calculator.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class Accelerate;
class BackgroundNoise;
class BufferLevelFilter;
class ComfortNoise;
class CriticalSectionWrapper;
class DecisionLogic;
class DecoderDatabase;
class DelayManager;
class DelayPeakDetector;
class DtmfBuffer;
class DtmfToneGenerator;
class Expand;
class Merge;
class Normal;
class PacketBuffer;
class PayloadSplitter;
class PostDecodeVad;
class PreemptiveExpand;
class RandomVector;
class SyncBuffer;
class TimestampScaler;
struct AccelerateFactory;
struct DtmfEvent;
struct ExpandFactory;
struct PreemptiveExpandFactory;

class NetEqImpl : public webrtc::NetEq {
 public:
  
  
  NetEqImpl(const NetEq::Config& config,
            BufferLevelFilter* buffer_level_filter,
            DecoderDatabase* decoder_database,
            DelayManager* delay_manager,
            DelayPeakDetector* delay_peak_detector,
            DtmfBuffer* dtmf_buffer,
            DtmfToneGenerator* dtmf_tone_generator,
            PacketBuffer* packet_buffer,
            PayloadSplitter* payload_splitter,
            TimestampScaler* timestamp_scaler,
            AccelerateFactory* accelerate_factory,
            ExpandFactory* expand_factory,
            PreemptiveExpandFactory* preemptive_expand_factory,
            bool create_components = true);

  virtual ~NetEqImpl();

  
  
  
  
  virtual int InsertPacket(const WebRtcRTPHeader& rtp_header,
                           const uint8_t* payload,
                           int length_bytes,
                           uint32_t receive_timestamp) OVERRIDE;

  
  
  
  
  
  
  
  
  
  virtual int InsertSyncPacket(const WebRtcRTPHeader& rtp_header,
                               uint32_t receive_timestamp) OVERRIDE;

  
  
  
  
  
  
  
  
  virtual int GetAudio(size_t max_length, int16_t* output_audio,
                       int* samples_per_channel, int* num_channels,
                       NetEqOutputType* type) OVERRIDE;

  
  
  virtual int RegisterPayloadType(enum NetEqDecoder codec,
                                  uint8_t rtp_payload_type) OVERRIDE;

  
  
  
  
  virtual int RegisterExternalDecoder(AudioDecoder* decoder,
                                      enum NetEqDecoder codec,
                                      uint8_t rtp_payload_type) OVERRIDE;

  
  
  virtual int RemovePayloadType(uint8_t rtp_payload_type) OVERRIDE;

  virtual bool SetMinimumDelay(int delay_ms) OVERRIDE;

  virtual bool SetMaximumDelay(int delay_ms) OVERRIDE;

  virtual int LeastRequiredDelayMs() const OVERRIDE;

  virtual int SetTargetDelay() OVERRIDE { return kNotImplemented; }

  virtual int TargetDelay() OVERRIDE { return kNotImplemented; }

  virtual int CurrentDelay() OVERRIDE { return kNotImplemented; }

  
  
  
  virtual void SetPlayoutMode(NetEqPlayoutMode mode) OVERRIDE;

  
  
  
  virtual NetEqPlayoutMode PlayoutMode() const OVERRIDE;

  
  
  virtual int NetworkStatistics(NetEqNetworkStatistics* stats) OVERRIDE;

  
  
  
  virtual void WaitingTimes(std::vector<int>* waiting_times) OVERRIDE;

  
  
  virtual void GetRtcpStatistics(RtcpStatistics* stats) OVERRIDE;

  
  virtual void GetRtcpStatisticsNoReset(RtcpStatistics* stats) OVERRIDE;

  
  
  virtual void EnableVad() OVERRIDE;

  
  virtual void DisableVad() OVERRIDE;

  virtual bool GetPlayoutTimestamp(uint32_t* timestamp) OVERRIDE;

  virtual int SetTargetNumberOfChannels() OVERRIDE { return kNotImplemented; }

  virtual int SetTargetSampleRate() OVERRIDE { return kNotImplemented; }

  
  
  virtual int LastError() const OVERRIDE;

  
  
  
  virtual int LastDecoderError() OVERRIDE;

  
  virtual void FlushBuffers() OVERRIDE;

  virtual void PacketBufferStatistics(int* current_num_packets,
                                      int* max_num_packets) const OVERRIDE;

  
  
  virtual int DecodedRtpInfo(int* sequence_number,
                             uint32_t* timestamp) const OVERRIDE;

  
  const SyncBuffer* sync_buffer_for_test() const;

 protected:
  static const int kOutputSizeMs = 10;
  static const int kMaxFrameSize = 2880;  
  
  static const int kSyncBufferSize = 2 * kMaxFrameSize;

  
  
  
  int InsertPacketInternal(const WebRtcRTPHeader& rtp_header,
                           const uint8_t* payload,
                           int length_bytes,
                           uint32_t receive_timestamp,
                           bool is_sync_packet)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  
  
  
  
  int GetAudioInternal(size_t max_length,
                       int16_t* output,
                       int* samples_per_channel,
                       int* num_channels) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  
  
  
  int GetDecision(Operations* operation,
                  PacketList* packet_list,
                  DtmfEvent* dtmf_event,
                  bool* play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  
  
  
  
  int Decode(PacketList* packet_list,
             Operations* operation,
             int* decoded_length,
             AudioDecoder::SpeechType* speech_type)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  int DecodeLoop(PacketList* packet_list,
                 Operations* operation,
                 AudioDecoder* decoder,
                 int* decoded_length,
                 AudioDecoder::SpeechType* speech_type)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  void DoNormal(const int16_t* decoded_buffer,
                size_t decoded_length,
                AudioDecoder::SpeechType speech_type,
                bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  void DoMerge(int16_t* decoded_buffer,
               size_t decoded_length,
               AudioDecoder::SpeechType speech_type,
               bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  int DoExpand(bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  int DoAccelerate(int16_t* decoded_buffer,
                   size_t decoded_length,
                   AudioDecoder::SpeechType speech_type,
                   bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  int DoPreemptiveExpand(int16_t* decoded_buffer,
                         size_t decoded_length,
                         AudioDecoder::SpeechType speech_type,
                         bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  
  
  int DoRfc3389Cng(PacketList* packet_list, bool play_dtmf)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  void DoCodecInternalCng() EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  int DoDtmf(const DtmfEvent& dtmf_event, bool* play_dtmf)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  
  void DoAlternativePlc(bool increase_timestamp)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  int DtmfOverdub(const DtmfEvent& dtmf_event,
                  size_t num_channels,
                  int16_t* output) const EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  
  
  int ExtractPackets(int required_samples, PacketList* packet_list)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  void SetSampleRateAndChannels(int fs_hz, size_t channels)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  
  NetEqOutputType LastOutputType() EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  virtual void UpdatePlcComponents(int fs_hz, size_t channels)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  
  virtual void CreateDecisionLogic() EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  const scoped_ptr<CriticalSectionWrapper> crit_sect_;
  const scoped_ptr<BufferLevelFilter> buffer_level_filter_
      GUARDED_BY(crit_sect_);
  const scoped_ptr<DecoderDatabase> decoder_database_ GUARDED_BY(crit_sect_);
  const scoped_ptr<DelayManager> delay_manager_ GUARDED_BY(crit_sect_);
  const scoped_ptr<DelayPeakDetector> delay_peak_detector_
      GUARDED_BY(crit_sect_);
  const scoped_ptr<DtmfBuffer> dtmf_buffer_ GUARDED_BY(crit_sect_);
  const scoped_ptr<DtmfToneGenerator> dtmf_tone_generator_
      GUARDED_BY(crit_sect_);
  const scoped_ptr<PacketBuffer> packet_buffer_ GUARDED_BY(crit_sect_);
  const scoped_ptr<PayloadSplitter> payload_splitter_ GUARDED_BY(crit_sect_);
  const scoped_ptr<TimestampScaler> timestamp_scaler_ GUARDED_BY(crit_sect_);
  const scoped_ptr<PostDecodeVad> vad_ GUARDED_BY(crit_sect_);
  const scoped_ptr<ExpandFactory> expand_factory_ GUARDED_BY(crit_sect_);
  const scoped_ptr<AccelerateFactory> accelerate_factory_
      GUARDED_BY(crit_sect_);
  const scoped_ptr<PreemptiveExpandFactory> preemptive_expand_factory_
      GUARDED_BY(crit_sect_);

  scoped_ptr<BackgroundNoise> background_noise_ GUARDED_BY(crit_sect_);
  scoped_ptr<DecisionLogic> decision_logic_ GUARDED_BY(crit_sect_);
  scoped_ptr<AudioMultiVector> algorithm_buffer_ GUARDED_BY(crit_sect_);
  scoped_ptr<SyncBuffer> sync_buffer_ GUARDED_BY(crit_sect_);
  scoped_ptr<Expand> expand_ GUARDED_BY(crit_sect_);
  scoped_ptr<Normal> normal_ GUARDED_BY(crit_sect_);
  scoped_ptr<Merge> merge_ GUARDED_BY(crit_sect_);
  scoped_ptr<Accelerate> accelerate_ GUARDED_BY(crit_sect_);
  scoped_ptr<PreemptiveExpand> preemptive_expand_ GUARDED_BY(crit_sect_);
  RandomVector random_vector_ GUARDED_BY(crit_sect_);
  scoped_ptr<ComfortNoise> comfort_noise_ GUARDED_BY(crit_sect_);
  Rtcp rtcp_ GUARDED_BY(crit_sect_);
  StatisticsCalculator stats_ GUARDED_BY(crit_sect_);
  int fs_hz_ GUARDED_BY(crit_sect_);
  int fs_mult_ GUARDED_BY(crit_sect_);
  int output_size_samples_ GUARDED_BY(crit_sect_);
  int decoder_frame_length_ GUARDED_BY(crit_sect_);
  Modes last_mode_ GUARDED_BY(crit_sect_);
  scoped_ptr<int16_t[]> mute_factor_array_ GUARDED_BY(crit_sect_);
  size_t decoded_buffer_length_ GUARDED_BY(crit_sect_);
  scoped_ptr<int16_t[]> decoded_buffer_ GUARDED_BY(crit_sect_);
  uint32_t playout_timestamp_ GUARDED_BY(crit_sect_);
  bool new_codec_ GUARDED_BY(crit_sect_);
  uint32_t timestamp_ GUARDED_BY(crit_sect_);
  bool reset_decoder_ GUARDED_BY(crit_sect_);
  uint8_t current_rtp_payload_type_ GUARDED_BY(crit_sect_);
  uint8_t current_cng_rtp_payload_type_ GUARDED_BY(crit_sect_);
  uint32_t ssrc_ GUARDED_BY(crit_sect_);
  bool first_packet_ GUARDED_BY(crit_sect_);
  int error_code_ GUARDED_BY(crit_sect_);  
  int decoder_error_code_ GUARDED_BY(crit_sect_);
  const BackgroundNoiseMode background_noise_mode_ GUARDED_BY(crit_sect_);
  NetEqPlayoutMode playout_mode_ GUARDED_BY(crit_sect_);

  
  
  
  
  
  
  
  int decoded_packet_sequence_number_ GUARDED_BY(crit_sect_);
  uint32_t decoded_packet_timestamp_ GUARDED_BY(crit_sect_);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetEqImpl);
};

}  
#endif  
