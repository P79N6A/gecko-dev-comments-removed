









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_AUDIO_CODING_MODULE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_AUDIO_CODING_MODULE_IMPL_H_

#include <vector>

#include "webrtc/base/thread_annotations.h"
#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_receiver.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_resampler.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class CriticalSectionWrapper;

namespace acm2 {

class ACMDTMFDetection;
class ACMGenericCodec;

class AudioCodingModuleImpl : public AudioCodingModule {
 public:
  explicit AudioCodingModuleImpl(const AudioCodingModule::Config& config);
  ~AudioCodingModuleImpl();

  
  virtual int32_t ChangeUniqueId(const int32_t id) OVERRIDE;

  
  
  virtual int32_t TimeUntilNextProcess() OVERRIDE;

  
  virtual int32_t Process() OVERRIDE;

  
  
  

  
  virtual int InitializeSender() OVERRIDE;

  
  virtual int ResetEncoder() OVERRIDE;

  
  virtual int RegisterSendCodec(const CodecInst& send_codec) OVERRIDE;

  
  
  virtual int RegisterSecondarySendCodec(const CodecInst& send_codec) OVERRIDE;

  
  
  virtual void UnregisterSecondarySendCodec() OVERRIDE;

  
  virtual int SecondarySendCodec(CodecInst* secondary_codec) const OVERRIDE;

  
  virtual int SendCodec(CodecInst* current_codec) const OVERRIDE;

  
  virtual int SendFrequency() const OVERRIDE;

  
  
  
  virtual int SendBitrate() const OVERRIDE;

  
  
  virtual int SetReceivedEstimatedBandwidth(int bw) OVERRIDE;

  
  
  virtual int RegisterTransportCallback(
      AudioPacketizationCallback* transport) OVERRIDE;

  
  virtual int Add10MsData(const AudioFrame& audio_frame) OVERRIDE;

  
  
  

  
  virtual int SetREDStatus(bool enable_red) OVERRIDE;

  
  virtual bool REDStatus() const OVERRIDE;

  
  
  

  
  virtual int SetCodecFEC(bool enabled_codec_fec) OVERRIDE;

  
  virtual bool CodecFEC() const OVERRIDE;

  
  virtual int SetPacketLossRate(int loss_rate) OVERRIDE;

  
  
  
  
  

  virtual int SetVAD(bool enable_dtx = true,
                     bool enable_vad = false,
                     ACMVADMode mode = VADNormal) OVERRIDE;

  virtual int VAD(bool* dtx_enabled,
                  bool* vad_enabled,
                  ACMVADMode* mode) const OVERRIDE;

  virtual int RegisterVADCallback(ACMVADCallback* vad_callback) OVERRIDE;

  
  
  

  
  virtual int InitializeReceiver() OVERRIDE;

  
  virtual int ResetDecoder() OVERRIDE;

  
  virtual int ReceiveFrequency() const OVERRIDE;

  
  virtual int PlayoutFrequency() const OVERRIDE;

  
  
  virtual int RegisterReceiveCodec(const CodecInst& receive_codec) OVERRIDE;

  
  virtual int ReceiveCodec(CodecInst* current_codec) const OVERRIDE;

  
  virtual int IncomingPacket(const uint8_t* incoming_payload,
                             int payload_length,
                             const WebRtcRTPHeader& rtp_info) OVERRIDE;

  
  
  virtual int IncomingPayload(const uint8_t* incoming_payload,
                              int payload_length,
                              uint8_t payload_type,
                              uint32_t timestamp) OVERRIDE;

  
  virtual int SetMinimumPlayoutDelay(int time_ms) OVERRIDE;

  
  virtual int SetMaximumPlayoutDelay(int time_ms) OVERRIDE;

  
  virtual int LeastRequiredDelayMs() const OVERRIDE;

  
  
  virtual int SetInitialPlayoutDelay(int delay_ms) OVERRIDE;

  
  
  
  
  
  virtual int SetDtmfPlayoutStatus(bool enable) OVERRIDE { return 0; }

  
  virtual bool DtmfPlayoutStatus() const OVERRIDE { return true; }

  
  
  
  virtual int DecoderEstimatedBandwidth() const OVERRIDE;

  
  virtual int SetPlayoutMode(AudioPlayoutMode mode) OVERRIDE;

  
  virtual AudioPlayoutMode PlayoutMode() const OVERRIDE;

  
  virtual int PlayoutTimestamp(uint32_t* timestamp) OVERRIDE;

  
  
  virtual int PlayoutData10Ms(int desired_freq_hz,
                              AudioFrame* audio_frame) OVERRIDE;

  
  
  

  virtual int NetworkStatistics(ACMNetworkStatistics* statistics) OVERRIDE;

  
  
  
  int REDPayloadISAC(int isac_rate,
                     int isac_bw_estimate,
                     uint8_t* payload,
                     int16_t* length_bytes);

  virtual int ReplaceInternalDTXWithWebRtc(bool use_webrtc_dtx) OVERRIDE;

  virtual int IsInternalDTXReplacedWithWebRtc(bool* uses_webrtc_dtx) OVERRIDE;

  virtual int SetISACMaxRate(int max_bit_per_sec) OVERRIDE;

  virtual int SetISACMaxPayloadSize(int max_size_bytes) OVERRIDE;

  virtual int ConfigISACBandwidthEstimator(
      int frame_size_ms,
      int rate_bit_per_sec,
      bool enforce_frame_size = false) OVERRIDE;

  
  
  virtual int SetOpusMaxPlaybackRate(int frequency_hz) OVERRIDE;

  virtual int UnregisterReceiveCodec(uint8_t payload_type) OVERRIDE;

  virtual int EnableNack(size_t max_nack_list_size) OVERRIDE;

  virtual void DisableNack() OVERRIDE;

  virtual std::vector<uint16_t> GetNackList(
      int round_trip_time_ms) const OVERRIDE;

  virtual void GetDecodingCallStatistics(
      AudioDecodingCallStats* stats) const OVERRIDE;

 private:
  int UnregisterReceiveCodecSafe(int payload_type);

  ACMGenericCodec* CreateCodec(const CodecInst& codec);

  int InitializeReceiverSafe() EXCLUSIVE_LOCKS_REQUIRED(acm_crit_sect_);

  bool HaveValidEncoder(const char* caller_name) const
      EXCLUSIVE_LOCKS_REQUIRED(acm_crit_sect_);

  
  
  int SetVADSafe(bool enable_dtx, bool enable_vad, ACMVADMode mode)
      EXCLUSIVE_LOCKS_REQUIRED(acm_crit_sect_);

  
  
  int ProcessSingleStream();

  
  
  int ProcessDualStream();

  
  
  
  
  
  
  
  
  
  
  
  int PreprocessToAddData(const AudioFrame& in_frame,
                          const AudioFrame** ptr_out)
      EXCLUSIVE_LOCKS_REQUIRED(acm_crit_sect_);

  
  
  int UpdateUponReceivingCodec(int index);

  int EncodeFragmentation(int fragmentation_index,
                          int payload_type,
                          uint32_t current_timestamp,
                          ACMGenericCodec* encoder,
                          uint8_t* stream)
      EXCLUSIVE_LOCKS_REQUIRED(acm_crit_sect_);

  void ResetFragmentation(int vector_size)
      EXCLUSIVE_LOCKS_REQUIRED(acm_crit_sect_);

  
  
  
  
  
  
  
  
  
  
  int GetAudioDecoder(const CodecInst& codec, int codec_id,
                      int mirror_id, AudioDecoder** decoder)
      EXCLUSIVE_LOCKS_REQUIRED(acm_crit_sect_);

  CriticalSectionWrapper* acm_crit_sect_;
  int id_;  
  uint32_t expected_codec_ts_ GUARDED_BY(acm_crit_sect_);
  uint32_t expected_in_ts_ GUARDED_BY(acm_crit_sect_);
  CodecInst send_codec_inst_ GUARDED_BY(acm_crit_sect_);

  uint8_t cng_nb_pltype_ GUARDED_BY(acm_crit_sect_);
  uint8_t cng_wb_pltype_ GUARDED_BY(acm_crit_sect_);
  uint8_t cng_swb_pltype_ GUARDED_BY(acm_crit_sect_);
  uint8_t cng_fb_pltype_ GUARDED_BY(acm_crit_sect_);

  uint8_t red_pltype_ GUARDED_BY(acm_crit_sect_);
  bool vad_enabled_ GUARDED_BY(acm_crit_sect_);
  bool dtx_enabled_ GUARDED_BY(acm_crit_sect_);
  ACMVADMode vad_mode_ GUARDED_BY(acm_crit_sect_);
  ACMGenericCodec* codecs_[ACMCodecDB::kMaxNumCodecs]
      GUARDED_BY(acm_crit_sect_);
  int mirror_codec_idx_[ACMCodecDB::kMaxNumCodecs] GUARDED_BY(acm_crit_sect_);
  bool stereo_send_ GUARDED_BY(acm_crit_sect_);
  int current_send_codec_idx_ GUARDED_BY(acm_crit_sect_);
  bool send_codec_registered_ GUARDED_BY(acm_crit_sect_);
  ACMResampler resampler_ GUARDED_BY(acm_crit_sect_);
  AcmReceiver receiver_;  

  
  bool is_first_red_ GUARDED_BY(acm_crit_sect_);
  bool red_enabled_ GUARDED_BY(acm_crit_sect_);

  
  
  
  
  uint8_t* red_buffer_ GUARDED_BY(acm_crit_sect_);

  
  
  
  RTPFragmentationHeader fragmentation_ GUARDED_BY(acm_crit_sect_);
  uint32_t last_red_timestamp_ GUARDED_BY(acm_crit_sect_);

  
  bool codec_fec_enabled_ GUARDED_BY(acm_crit_sect_);

  
  uint8_t previous_pltype_ GUARDED_BY(acm_crit_sect_);

  
  
  
  
  
  
  WebRtcRTPHeader* aux_rtp_header_;

  bool receiver_initialized_ GUARDED_BY(acm_crit_sect_);

  AudioFrame preprocess_frame_ GUARDED_BY(acm_crit_sect_);
  CodecInst secondary_send_codec_inst_ GUARDED_BY(acm_crit_sect_);
  scoped_ptr<ACMGenericCodec> secondary_encoder_ GUARDED_BY(acm_crit_sect_);
  uint32_t codec_timestamp_ GUARDED_BY(acm_crit_sect_);
  bool first_10ms_data_ GUARDED_BY(acm_crit_sect_);

  CriticalSectionWrapper* callback_crit_sect_;
  AudioPacketizationCallback* packetization_callback_
      GUARDED_BY(callback_crit_sect_);
  ACMVADCallback* vad_callback_ GUARDED_BY(callback_crit_sect_);
};

}  

class AudioCodingImpl : public AudioCoding {
 public:
  AudioCodingImpl(const Config& config) {
    AudioCodingModule::Config config_old = config.ToOldConfig();
    acm_old_.reset(new acm2::AudioCodingModuleImpl(config_old));
    acm_old_->RegisterTransportCallback(config.transport);
    acm_old_->RegisterVADCallback(config.vad_callback);
    acm_old_->SetDtmfPlayoutStatus(config.play_dtmf);
    if (config.initial_playout_delay_ms > 0) {
      acm_old_->SetInitialPlayoutDelay(config.initial_playout_delay_ms);
    }
    playout_frequency_hz_ = config.playout_frequency_hz;
  }

  virtual ~AudioCodingImpl() OVERRIDE {};

  virtual bool RegisterSendCodec(AudioEncoder* send_codec) OVERRIDE;

  virtual bool RegisterSendCodec(int encoder_type,
                                 uint8_t payload_type,
                                 int frame_size_samples = 0) OVERRIDE;

  virtual const AudioEncoder* GetSenderInfo() const OVERRIDE;

  virtual const CodecInst* GetSenderCodecInst() OVERRIDE;

  virtual int Add10MsAudio(const AudioFrame& audio_frame) OVERRIDE;

  virtual const ReceiverInfo* GetReceiverInfo() const OVERRIDE;

  virtual bool RegisterReceiveCodec(AudioDecoder* receive_codec) OVERRIDE;

  virtual bool RegisterReceiveCodec(int decoder_type,
                                    uint8_t payload_type) OVERRIDE;

  virtual bool InsertPacket(const uint8_t* incoming_payload,
                            int32_t payload_len_bytes,
                            const WebRtcRTPHeader& rtp_info) OVERRIDE;

  virtual bool InsertPayload(const uint8_t* incoming_payload,
                             int32_t payload_len_byte,
                             uint8_t payload_type,
                             uint32_t timestamp) OVERRIDE;

  virtual bool SetMinimumPlayoutDelay(int time_ms) OVERRIDE;

  virtual bool SetMaximumPlayoutDelay(int time_ms) OVERRIDE;

  virtual int LeastRequiredDelayMs() const OVERRIDE;

  virtual bool PlayoutTimestamp(uint32_t* timestamp) OVERRIDE;

  virtual bool Get10MsAudio(AudioFrame* audio_frame) OVERRIDE;

  virtual bool NetworkStatistics(
      ACMNetworkStatistics* network_statistics) OVERRIDE;

  virtual bool EnableNack(size_t max_nack_list_size) OVERRIDE;

  virtual void DisableNack() OVERRIDE;

  virtual bool SetVad(bool enable_dtx,
                      bool enable_vad,
                      ACMVADMode vad_mode) OVERRIDE;

  virtual std::vector<uint16_t> GetNackList(
      int round_trip_time_ms) const OVERRIDE;

  virtual void GetDecodingCallStatistics(
      AudioDecodingCallStats* call_stats) const OVERRIDE;

 private:
  
  
  
  
  static bool MapCodecTypeToParameters(int codec_type,
                                       std::string* codec_name,
                                       int* sample_rate_hz,
                                       int* channels);

  int playout_frequency_hz_;
  
  
  scoped_ptr<acm2::AudioCodingModuleImpl> acm_old_;
  CodecInst current_send_codec_;
};

}  

#endif  
