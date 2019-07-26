









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_NETEQ_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_NETEQ_IMPL_H_

#include <vector>

#include "webrtc/modules/audio_coding/neteq4/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq4/defines.h"
#include "webrtc/modules/audio_coding/neteq4/interface/neteq.h"
#include "webrtc/modules/audio_coding/neteq4/packet.h"  
#include "webrtc/modules/audio_coding/neteq4/random_vector.h"
#include "webrtc/modules/audio_coding/neteq4/rtcp.h"
#include "webrtc/modules/audio_coding/neteq4/statistics_calculator.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
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
  
  
  NetEqImpl(int fs,
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
            PreemptiveExpandFactory* preemptive_expand_factory);

  virtual ~NetEqImpl();

  
  
  
  
  virtual int InsertPacket(const WebRtcRTPHeader& rtp_header,
                           const uint8_t* payload,
                           int length_bytes,
                           uint32_t receive_timestamp);

  
  
  
  
  
  
  
  
  
  virtual int InsertSyncPacket(const WebRtcRTPHeader& rtp_header,
                               uint32_t receive_timestamp);

  
  
  
  
  
  
  
  
  virtual int GetAudio(size_t max_length, int16_t* output_audio,
                       int* samples_per_channel, int* num_channels,
                       NetEqOutputType* type);

  
  
  virtual int RegisterPayloadType(enum NetEqDecoder codec,
                                  uint8_t rtp_payload_type);

  
  
  
  
  virtual int RegisterExternalDecoder(AudioDecoder* decoder,
                                      enum NetEqDecoder codec,
                                      int sample_rate_hz,
                                      uint8_t rtp_payload_type);

  
  
  virtual int RemovePayloadType(uint8_t rtp_payload_type);

  virtual bool SetMinimumDelay(int delay_ms);

  virtual bool SetMaximumDelay(int delay_ms);

  virtual int LeastRequiredDelayMs() const;

  virtual int SetTargetDelay() { return kNotImplemented; }

  virtual int TargetDelay() { return kNotImplemented; }

  virtual int CurrentDelay() { return kNotImplemented; }

  
  virtual void SetPlayoutMode(NetEqPlayoutMode mode);

  
  virtual NetEqPlayoutMode PlayoutMode() const;

  
  
  virtual int NetworkStatistics(NetEqNetworkStatistics* stats);

  
  
  
  virtual void WaitingTimes(std::vector<int>* waiting_times);

  
  
  virtual void GetRtcpStatistics(RtcpStatistics* stats);

  
  virtual void GetRtcpStatisticsNoReset(RtcpStatistics* stats);

  
  
  virtual void EnableVad();

  
  virtual void DisableVad();

  
  virtual uint32_t PlayoutTimestamp();

  virtual int SetTargetNumberOfChannels() { return kNotImplemented; }

  virtual int SetTargetSampleRate() { return kNotImplemented; }

  
  
  virtual int LastError();

  
  
  
  virtual int LastDecoderError();

  
  virtual void FlushBuffers();

  virtual void PacketBufferStatistics(int* current_num_packets,
                                      int* max_num_packets,
                                      int* current_memory_size_bytes,
                                      int* max_memory_size_bytes) const;

  
  
  virtual int DecodedRtpInfo(int* sequence_number, uint32_t* timestamp) const;

  
  virtual void SetBackgroundNoiseMode(NetEqBackgroundNoiseMode mode);

  
  virtual NetEqBackgroundNoiseMode BackgroundNoiseMode() const;

 private:
  static const int kOutputSizeMs = 10;
  static const int kMaxFrameSize = 2880;  
  
  static const int kSyncBufferSize = 2 * kMaxFrameSize;

  
  
  
  int InsertPacketInternal(const WebRtcRTPHeader& rtp_header,
                           const uint8_t* payload,
                           int length_bytes,
                           uint32_t receive_timestamp,
                           bool is_sync_packet);


  
  
  
  
  
  
  int GetAudioInternal(size_t max_length, int16_t* output,
                       int* samples_per_channel, int* num_channels);


  
  
  
  
  
  int GetDecision(Operations* operation,
                  PacketList* packet_list,
                  DtmfEvent* dtmf_event,
                  bool* play_dtmf);

  
  
  
  
  
  
  int Decode(PacketList* packet_list, Operations* operation,
             int* decoded_length, AudioDecoder::SpeechType* speech_type);

  
  int DecodeLoop(PacketList* packet_list, Operations* operation,
                 AudioDecoder* decoder, int* decoded_length,
                 AudioDecoder::SpeechType* speech_type);

  
  void DoNormal(const int16_t* decoded_buffer, size_t decoded_length,
                AudioDecoder::SpeechType speech_type, bool play_dtmf);

  
  void DoMerge(int16_t* decoded_buffer, size_t decoded_length,
               AudioDecoder::SpeechType speech_type, bool play_dtmf);

  
  int DoExpand(bool play_dtmf);

  
  
  int DoAccelerate(int16_t* decoded_buffer, size_t decoded_length,
                   AudioDecoder::SpeechType speech_type, bool play_dtmf);

  
  
  int DoPreemptiveExpand(int16_t* decoded_buffer, size_t decoded_length,
                         AudioDecoder::SpeechType speech_type, bool play_dtmf);

  
  
  
  
  int DoRfc3389Cng(PacketList* packet_list, bool play_dtmf);

  
  
  void DoCodecInternalCng();

  
  int DoDtmf(const DtmfEvent& dtmf_event, bool* play_dtmf);

  
  
  
  void DoAlternativePlc(bool increase_timestamp);

  
  int DtmfOverdub(const DtmfEvent& dtmf_event, size_t num_channels,
                  int16_t* output) const;

  
  
  
  
  int ExtractPackets(int required_samples, PacketList* packet_list);

  
  
  void SetSampleRateAndChannels(int fs_hz, size_t channels);

  
  
  NetEqOutputType LastOutputType();

  scoped_ptr<BackgroundNoise> background_noise_;
  scoped_ptr<BufferLevelFilter> buffer_level_filter_;
  scoped_ptr<DecoderDatabase> decoder_database_;
  scoped_ptr<DelayManager> delay_manager_;
  scoped_ptr<DelayPeakDetector> delay_peak_detector_;
  scoped_ptr<DtmfBuffer> dtmf_buffer_;
  scoped_ptr<DtmfToneGenerator> dtmf_tone_generator_;
  scoped_ptr<PacketBuffer> packet_buffer_;
  scoped_ptr<PayloadSplitter> payload_splitter_;
  scoped_ptr<TimestampScaler> timestamp_scaler_;
  scoped_ptr<DecisionLogic> decision_logic_;
  scoped_ptr<PostDecodeVad> vad_;
  scoped_ptr<AudioMultiVector> algorithm_buffer_;
  scoped_ptr<SyncBuffer> sync_buffer_;
  scoped_ptr<Expand> expand_;
  scoped_ptr<ExpandFactory> expand_factory_;
  scoped_ptr<Normal> normal_;
  scoped_ptr<Merge> merge_;
  scoped_ptr<Accelerate> accelerate_;
  scoped_ptr<AccelerateFactory> accelerate_factory_;
  scoped_ptr<PreemptiveExpand> preemptive_expand_;
  scoped_ptr<PreemptiveExpandFactory> preemptive_expand_factory_;
  RandomVector random_vector_;
  scoped_ptr<ComfortNoise> comfort_noise_;
  Rtcp rtcp_;
  StatisticsCalculator stats_;
  int fs_hz_;
  int fs_mult_;
  int output_size_samples_;
  int decoder_frame_length_;
  Modes last_mode_;
  scoped_array<int16_t> mute_factor_array_;
  size_t decoded_buffer_length_;
  scoped_array<int16_t> decoded_buffer_;
  uint32_t playout_timestamp_;
  bool new_codec_;
  uint32_t timestamp_;
  bool reset_decoder_;
  uint8_t current_rtp_payload_type_;
  uint8_t current_cng_rtp_payload_type_;
  uint32_t ssrc_;
  bool first_packet_;
  int error_code_;  
  int decoder_error_code_;
  scoped_ptr<CriticalSectionWrapper> crit_sect_;

  
  
  
  
  
  
  
  int decoded_packet_sequence_number_;
  uint32_t decoded_packet_timestamp_;

  DISALLOW_COPY_AND_ASSIGN(NetEqImpl);
};

}  
#endif  
