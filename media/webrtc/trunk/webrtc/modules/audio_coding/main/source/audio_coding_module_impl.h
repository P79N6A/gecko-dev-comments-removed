









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_AUDIO_CODING_MODULE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_AUDIO_CODING_MODULE_IMPL_H_

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/main/source/acm_resampler.h"
#include "webrtc/modules/audio_coding/main/acm2/call_statistics.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

struct WebRtcACMAudioBuff;
struct WebRtcACMCodecParams;
class CriticalSectionWrapper;
class RWLockWrapper;
class Clock;

namespace acm2 {
class Nack;
}

namespace acm1 {

class ACMDTMFDetection;
class ACMGenericCodec;

class AudioCodingModuleImpl : public AudioCodingModule {
 public:
  AudioCodingModuleImpl(const int32_t id, Clock* clock);
  ~AudioCodingModuleImpl();

  virtual const char* Version() const;

  
  virtual int32_t ChangeUniqueId(const int32_t id);

  
  
  int32_t TimeUntilNextProcess();

  
  int32_t Process();

  
  
  

  
  int32_t InitializeSender();

  
  int32_t ResetEncoder();

  
  int32_t RegisterSendCodec(const CodecInst& send_codec);

  
  
  int RegisterSecondarySendCodec(const CodecInst& send_codec);

  
  
  void UnregisterSecondarySendCodec();

  
  int SecondarySendCodec(CodecInst* secondary_codec) const;

  
  int32_t SendCodec(CodecInst* current_codec) const;

  
  int32_t SendFrequency() const;

  
  
  
  int32_t SendBitrate() const;

  
  
  virtual int32_t SetReceivedEstimatedBandwidth(const int32_t bw);

  
  
  int32_t RegisterTransportCallback(AudioPacketizationCallback* transport);

  
  int32_t Add10MsData(const AudioFrame& audio_frame);

  
  
  

  
  int32_t SetFECStatus(const bool enable_fec);

  
  bool FECStatus() const;

  
  
  
  
  

  int32_t SetVAD(bool enable_dtx = true,
                 bool enable_vad = false,
                 ACMVADMode mode = VADNormal);

  int32_t VAD(bool* dtx_enabled, bool* vad_enabled, ACMVADMode* mode) const;

  int32_t RegisterVADCallback(ACMVADCallback* vad_callback);

  
  
  

  
  int32_t InitializeReceiver();

  
  int32_t ResetDecoder();

  
  int32_t ReceiveFrequency() const;

  
  int32_t PlayoutFrequency() const;

  
  
  int32_t RegisterReceiveCodec(const CodecInst& receive_codec);

  
  int32_t ReceiveCodec(CodecInst* current_codec) const;

  
  int32_t IncomingPacket(const uint8_t* incoming_payload,
                         const int32_t payload_length,
                         const WebRtcRTPHeader& rtp_info);

  
  
  int32_t IncomingPayload(const uint8_t* incoming_payload,
                          const int32_t payload_length,
                          const uint8_t payload_type,
                          const uint32_t timestamp = 0);

  
  
  int SetMinimumPlayoutDelay(int time_ms);

  
  
  int SetMaximumPlayoutDelay(int time_ms);

  
  
  
  
  
  int LeastRequiredDelayMs() const ;

  
  
  int32_t SetDtmfPlayoutStatus(const bool enable);

  
  bool DtmfPlayoutStatus() const;

  
  
  
  int32_t DecoderEstimatedBandwidth() const;

  
  int32_t SetPlayoutMode(const AudioPlayoutMode mode);

  
  AudioPlayoutMode PlayoutMode() const;

  
  int32_t PlayoutTimestamp(uint32_t* timestamp);

  
  
  int32_t PlayoutData10Ms(int32_t desired_freq_hz,
                          AudioFrame* audio_frame);

  
  
  

  int32_t NetworkStatistics(ACMNetworkStatistics* statistics);

  void DestructEncoderInst(void* inst);

  int16_t AudioBuffer(WebRtcACMAudioBuff& buffer);

  
  
  int32_t REDPayloadISAC(const int32_t isac_rate,
                         const int16_t isac_bw_estimate,
                         uint8_t* payload,
                         int16_t* length_bytes);

  int16_t SetAudioBuffer(WebRtcACMAudioBuff& buffer);

  uint32_t EarliestTimestamp() const;

  int32_t LastEncodedTimestamp(uint32_t& timestamp) const;

  int32_t ReplaceInternalDTXWithWebRtc(const bool use_webrtc_dtx);

  int32_t IsInternalDTXReplacedWithWebRtc(bool* uses_webrtc_dtx);

  int SetISACMaxRate(int max_bit_per_sec);

  int SetISACMaxPayloadSize(int max_size_bytes);

  int32_t ConfigISACBandwidthEstimator(
      int frame_size_ms,
      int rate_bit_per_sec,
      bool enforce_frame_size = false);

  int UnregisterReceiveCodec(uint8_t payload_type);

  std::vector<uint16_t> GetNackList(int round_trip_time_ms) const;

 protected:
  void UnregisterSendCodec();

  int32_t UnregisterReceiveCodecSafe(const int16_t id);

  ACMGenericCodec* CreateCodec(const CodecInst& codec);

  int16_t DecoderParamByPlType(const uint8_t payload_type,
                               WebRtcACMCodecParams& codec_params) const;

  int16_t DecoderListIDByPlName(
      const char* name, const uint16_t frequency = 0) const;

  int32_t InitializeReceiverSafe();

  bool HaveValidEncoder(const char* caller_name) const;

  int32_t RegisterRecCodecMSSafe(const CodecInst& receive_codec,
                                 int16_t codec_id,
                                 int16_t mirror_id,
                                 ACMNetEQ::JitterBuffer jitter_buffer);

  
  
  int SetVADSafe(bool enable_dtx, bool enable_vad, ACMVADMode mode);

  
  
  int ProcessSingleStream();

  
  
  int ProcessDualStream();

  
  
  
  
  
  
  
  
  
  
  
  int PreprocessToAddData(const AudioFrame& in_frame,
                          const AudioFrame** ptr_out);

  
  
  
  
  
  
  int SetInitialPlayoutDelay(int delay_ms);

  
  int EnableNack(size_t max_nack_list_size);

  
  void DisableNack();

  void GetDecodingCallStatistics(AudioDecodingCallStats* call_stats) const;

 private:
  
  
  int UpdateUponReceivingCodec(int index);

  
  
  int InitStereoSlave();

  
  
  bool IsCodecForSlave(int index) const;

  int EncodeFragmentation(int fragmentation_index, int payload_type,
                          uint32_t current_timestamp,
                          ACMGenericCodec* encoder,
                          uint8_t* stream);

  void ResetFragmentation(int vector_size);

  bool GetSilence(int desired_sample_rate_hz, AudioFrame* frame);

  
  
  
  
  int PushSyncPacketSafe();

  
  
  
  void UpdateBufferingSafe(const WebRtcRTPHeader& rtp_info,
                           int payload_len_bytes);

  
  
  
  
  uint32_t NowTimestamp(int codec_id);

  AudioPacketizationCallback* packetization_callback_;
  int32_t id_;
  uint32_t last_timestamp_;
  uint32_t last_in_timestamp_;
  CodecInst send_codec_inst_;
  uint8_t cng_nb_pltype_;
  uint8_t cng_wb_pltype_;
  uint8_t cng_swb_pltype_;
  uint8_t cng_fb_pltype_;
  uint8_t red_pltype_;
  bool vad_enabled_;
  bool dtx_enabled_;
  ACMVADMode vad_mode_;
  ACMGenericCodec* codecs_[ACMCodecDB::kMaxNumCodecs];
  ACMGenericCodec* slave_codecs_[ACMCodecDB::kMaxNumCodecs];
  int16_t mirror_codec_idx_[ACMCodecDB::kMaxNumCodecs];
  bool stereo_receive_[ACMCodecDB::kMaxNumCodecs];
  bool stereo_receive_registered_;
  bool stereo_send_;
  int prev_received_channel_;
  int expected_channels_;
  int32_t current_send_codec_idx_;
  int current_receive_codec_idx_;
  bool send_codec_registered_;
  ACMResampler input_resampler_;
  ACMResampler output_resampler_;
  ACMNetEQ neteq_;
  CriticalSectionWrapper* acm_crit_sect_;
  ACMVADCallback* vad_callback_;
  uint8_t last_recv_audio_codec_pltype_;

  
  bool is_first_red_;
  bool fec_enabled_;
  
  
  
  
  uint8_t* red_buffer_;
  
  
  
  RTPFragmentationHeader fragmentation_;
  uint32_t last_fec_timestamp_;
  
  
  uint8_t receive_red_pltype_;

  
  uint8_t previous_pltype_;

  
  
  
  int16_t registered_pltypes_[ACMCodecDB::kMaxNumCodecs];

  
  
  
  WebRtcRTPHeader* dummy_rtp_header_;
  uint16_t recv_pl_frame_size_smpls_;

  bool receiver_initialized_;
  ACMDTMFDetection* dtmf_detector_;

  AudioCodingFeedback* dtmf_callback_;
  int16_t last_detected_tone_;
  CriticalSectionWrapper* callback_crit_sect_;

  AudioFrame audio_frame_;
  AudioFrame preprocess_frame_;
  CodecInst secondary_send_codec_inst_;
  scoped_ptr<ACMGenericCodec> secondary_encoder_;

  
  int initial_delay_ms_;
  int num_packets_accumulated_;
  int num_bytes_accumulated_;
  int accumulated_audio_ms_;
  int first_payload_received_;
  uint32_t last_incoming_send_timestamp_;
  bool track_neteq_buffer_;
  uint32_t playout_ts_;

  
  
  bool av_sync_;

  
  uint32_t last_timestamp_diff_;
  uint16_t last_sequence_number_;
  uint32_t last_ssrc_;
  bool last_packet_was_sync_;
  int64_t last_receive_timestamp_;

  Clock* clock_;
  scoped_ptr<acm2::Nack> nack_;
  bool nack_enabled_;

  acm2::CallStatistics call_stats_;
};

}  

}  

#endif  
