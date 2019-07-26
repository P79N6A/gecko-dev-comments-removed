









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RECEIVER_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_RECEIVER_H_

#include <vector>

#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_resampler.h"
#include "webrtc/modules/audio_coding/main/acm2/call_statistics.h"
#include "webrtc/modules/audio_coding/main/acm2/initial_delay_manager.h"
#include "webrtc/modules/audio_coding/neteq4/interface/neteq.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {

struct CodecInst;
class CriticalSectionWrapper;
class RWLockWrapper;
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

  
  AcmReceiver();

  explicit AcmReceiver(NetEq* neteq);

  
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

  
  
  
  
  
  RWLockWrapper* DecodeLock() const { return decode_lock_; }

  
  
  
  void FlushBuffers();

  
  
  
  
  
  
  
  
  
  int RemoveCodec(uint8_t payload_type);

  
  
  
  int RemoveAllCodecs();

  
  
  
  void set_id(int id);  

  
  
  
  uint32_t PlayoutTimestamp();

  
  
  
  
  
  int last_audio_codec_id() const;  

  
  
  
  
  int last_audio_payload_type() const;  

  
  
  
  
  
  int LastAudioCodec(CodecInst* codec) const;

  
  
  
  int RedPayloadType() const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  int DecoderByPayloadType(uint8_t payload_type,
                           CodecInst* codec) const;

  
  
  
  
  
  
  
  
  
  
  
  
  int EnableNack(size_t max_nack_list_size);

  
  void DisableNack();

  
  
  
  
  
  
  
  std::vector<uint16_t> GetNackList(int round_trip_time_ms) const;

  
  
  
  
  NetEqBackgroundNoiseMode BackgroundNoiseModeForTest() const;

  
  
  void GetDecodingCallStatistics(AudioDecodingCallStats* stats) const;

 private:
  int PayloadType2CodecIndex(uint8_t payload_type) const;

  bool GetSilence(int desired_sample_rate_hz, AudioFrame* frame);

  int GetNumSyncPacketToInsert(uint16_t received_squence_number);

  int RtpHeaderToCodecIndex(
      const RTPHeader& rtp_header, const uint8_t* payload) const;

  uint32_t NowInTimestamp(int decoder_sampling_rate) const;

  void InsertStreamOfSyncPackets(InitialDelayManager::SyncStream* sync_stream);

  int id_;
  NetEq* neteq_;
  Decoder decoders_[ACMCodecDB::kMaxNumCodecs];
  int last_audio_decoder_;
  RWLockWrapper* decode_lock_;
  CriticalSectionWrapper* neteq_crit_sect_;
  bool vad_enabled_;
  AudioFrame::VADActivity previous_audio_activity_;
  int current_sample_rate_hz_;
  ACMResampler resampler_;
  
  int16_t audio_buffer_[AudioFrame::kMaxDataSizeSamples];
  scoped_ptr<Nack> nack_;
  bool nack_enabled_;

  
  
  bool av_sync_;
  scoped_ptr<InitialDelayManager> initial_delay_manager_;

  
  
  
  
  
  scoped_ptr<InitialDelayManager::SyncStream> missing_packets_sync_stream_;
  scoped_ptr<InitialDelayManager::SyncStream> late_packets_sync_stream_;

  CallStatistics call_stats_;
};

}  

}  

#endif  
