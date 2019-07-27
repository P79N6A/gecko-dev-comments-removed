









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RECEIVER_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RECEIVER_H_

#include <vector>

#include "webrtc/base/thread_annotations.h"
#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_resampler.h"
#include "webrtc/modules/audio_coding/main/acm2/call_statistics.h"
#include "webrtc/modules/audio_coding/main/acm2/initial_delay_manager.h"
#include "webrtc/modules/audio_coding/neteq/interface/neteq.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

struct CodecInst;
class CriticalSectionWrapper;
class NetEq;

namespace acm2 {

class Nack;

class AcmReceiver {
 public:
  struct Decoder {
    bool registered;
    uint8_t payload_type;
    
    
    int channels;
  };

  
  explicit AcmReceiver(const AudioCodingModule::Config& config);

  
  ~AcmReceiver();

  
  
  
  
  
  
  
  
  
  
  
  
  
  int InsertPacket(const WebRtcRTPHeader& rtp_header,
                   const uint8_t* incoming_payload,
                   int length_payload);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int GetAudio(int desired_freq_hz, AudioFrame* audio_frame);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int AddCodec(int acm_codec_id,
               uint8_t payload_type,
               int channels,
               AudioDecoder* audio_decoder);

  
  
  
  
  
  
  
  
  
  
  int SetMinimumDelay(int delay_ms);

  
  
  
  
  
  
  
  
  
  
  int SetMaximumDelay(int delay_ms);

  
  
  
  
  
  int LeastRequiredDelayMs() const;

  
  
  
  
  
  
  
  
  
  
  
  int SetInitialDelay(int delay_ms);

  
  
  
  void ResetInitialDelay();

  
  
  
  
  
  int current_sample_rate_hz() const;

  
  
  
  
  
  
  void SetPlayoutMode(AudioPlayoutMode mode);

  
  
  
  
  
  AudioPlayoutMode PlayoutMode() const;

  
  
  
  
  
  
  void NetworkStatistics(ACMNetworkStatistics* statistics);

  
  
  
  void EnableVad();

  
  
  
  void DisableVad();

  
  
  
  bool vad_enabled() const { return vad_enabled_; }

  
  
  
  void FlushBuffers();

  
  
  
  
  
  
  
  
  
  int RemoveCodec(uint8_t payload_type);

  
  
  
  int RemoveAllCodecs();

  
  
  
  void set_id(int id);  

  
  
  
  
  bool GetPlayoutTimestamp(uint32_t* timestamp);

  
  
  
  
  
  int last_audio_codec_id() const;  

  
  
  
  
  int last_audio_payload_type() const;  

  
  
  
  
  
  int LastAudioCodec(CodecInst* codec) const;

  
  
  
  int RedPayloadType() const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  int DecoderByPayloadType(uint8_t payload_type,
                           CodecInst* codec) const;

  
  
  
  
  
  
  
  
  
  
  
  
  int EnableNack(size_t max_nack_list_size);

  
  void DisableNack();

  
  
  
  
  
  
  
  std::vector<uint16_t> GetNackList(int round_trip_time_ms) const;

  
  
  void GetDecodingCallStatistics(AudioDecodingCallStats* stats) const;

 private:
  int PayloadType2CodecIndex(uint8_t payload_type) const;

  bool GetSilence(int desired_sample_rate_hz, AudioFrame* frame)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  int GetNumSyncPacketToInsert(uint16_t received_squence_number);

  int RtpHeaderToCodecIndex(
      const RTPHeader& rtp_header, const uint8_t* payload) const;

  uint32_t NowInTimestamp(int decoder_sampling_rate) const;

  void InsertStreamOfSyncPackets(InitialDelayManager::SyncStream* sync_stream);

  scoped_ptr<CriticalSectionWrapper> crit_sect_;
  int id_;  
  int last_audio_decoder_ GUARDED_BY(crit_sect_);
  AudioFrame::VADActivity previous_audio_activity_ GUARDED_BY(crit_sect_);
  int current_sample_rate_hz_ GUARDED_BY(crit_sect_);
  ACMResampler resampler_ GUARDED_BY(crit_sect_);
  
  
  scoped_ptr<int16_t[]> audio_buffer_ GUARDED_BY(crit_sect_);
  scoped_ptr<int16_t[]> last_audio_buffer_ GUARDED_BY(crit_sect_);
  scoped_ptr<Nack> nack_ GUARDED_BY(crit_sect_);
  bool nack_enabled_ GUARDED_BY(crit_sect_);
  CallStatistics call_stats_ GUARDED_BY(crit_sect_);
  NetEq* neteq_;
  Decoder decoders_[ACMCodecDB::kMaxNumCodecs];
  bool vad_enabled_;
  Clock* clock_;  
  bool resampled_last_output_frame_ GUARDED_BY(crit_sect_);

  
  
  bool av_sync_;
  scoped_ptr<InitialDelayManager> initial_delay_manager_;

  
  
  
  
  
  scoped_ptr<InitialDelayManager::SyncStream> missing_packets_sync_stream_;
  scoped_ptr<InitialDelayManager::SyncStream> late_packets_sync_stream_;
};

}  

}  

#endif  
