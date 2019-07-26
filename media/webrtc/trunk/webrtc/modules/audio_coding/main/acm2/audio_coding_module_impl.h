









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_AUDIO_CODING_MODULE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_AUDIO_CODING_MODULE_IMPL_H_

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_receiver.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_resampler.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class CriticalSectionWrapper;
class RWLockWrapper;

namespace acm2 {

class ACMDTMFDetection;
class ACMGenericCodec;

class AudioCodingModuleImpl : public AudioCodingModule {
 public:
  explicit AudioCodingModuleImpl(int id);
  ~AudioCodingModuleImpl();

  virtual const char* Version() const;

  
  virtual int32_t ChangeUniqueId(const int32_t id);

  
  
  int32_t TimeUntilNextProcess();

  
  int32_t Process();

  
  
  

  
  int InitializeSender();

  
  int ResetEncoder();

  
  int RegisterSendCodec(const CodecInst& send_codec);

  
  
  int RegisterSecondarySendCodec(const CodecInst& send_codec);

  
  
  void UnregisterSecondarySendCodec();

  
  int SecondarySendCodec(CodecInst* secondary_codec) const;

  
  int SendCodec(CodecInst* current_codec) const;

  
  int SendFrequency() const;

  
  
  
  int SendBitrate() const;

  
  
  virtual int SetReceivedEstimatedBandwidth(int bw);

  
  
  int RegisterTransportCallback(AudioPacketizationCallback* transport);

  
  int Add10MsData(const AudioFrame& audio_frame);

  
  
  

  
  int SetFECStatus(bool enable_fec);

  
  bool FECStatus() const;

  
  
  
  
  

  int SetVAD(bool enable_dtx = true,
             bool enable_vad = false,
             ACMVADMode mode = VADNormal);

  int VAD(bool* dtx_enabled, bool* vad_enabled, ACMVADMode* mode) const;

  int RegisterVADCallback(ACMVADCallback* vad_callback);

  
  
  

  
  int InitializeReceiver();

  
  int ResetDecoder();

  
  int ReceiveFrequency() const;

  
  int PlayoutFrequency() const;

  
  
  int RegisterReceiveCodec(const CodecInst& receive_codec);

  
  int ReceiveCodec(CodecInst* current_codec) const;

  
  int IncomingPacket(const uint8_t* incoming_payload,
                     int payload_length,
                     const WebRtcRTPHeader& rtp_info);

  
  
  int IncomingPayload(const uint8_t* incoming_payload,
                      int payload_length,
                      uint8_t payload_type,
                      uint32_t timestamp);

  
  int SetMinimumPlayoutDelay(int time_ms);

  
  int SetMaximumPlayoutDelay(int time_ms);

  
  int LeastRequiredDelayMs() const;

  
  
  int SetInitialPlayoutDelay(int delay_ms);

  
  
  
  
  
  int SetDtmfPlayoutStatus(bool enable) { return 0; }

  
  bool DtmfPlayoutStatus() const { return true; }

  
  
  
  int DecoderEstimatedBandwidth() const;

  
  int SetPlayoutMode(AudioPlayoutMode mode);

  
  AudioPlayoutMode PlayoutMode() const;

  
  int PlayoutTimestamp(uint32_t* timestamp);

  
  
  int PlayoutData10Ms(int desired_freq_hz, AudioFrame* audio_frame);

  
  
  

  int NetworkStatistics(ACMNetworkStatistics* statistics);

  void DestructEncoderInst(void* inst);

  
  
  int REDPayloadISAC(int isac_rate,
                     int isac_bw_estimate,
                     uint8_t* payload,
                     int16_t* length_bytes);

  int ReplaceInternalDTXWithWebRtc(bool use_webrtc_dtx);

  int IsInternalDTXReplacedWithWebRtc(bool* uses_webrtc_dtx);

  int SetISACMaxRate(int max_bit_per_sec);

  int SetISACMaxPayloadSize(int max_size_bytes);

  int ConfigISACBandwidthEstimator(int frame_size_ms,
                                   int rate_bit_per_sec,
                                   bool enforce_frame_size = false);

  int UnregisterReceiveCodec(uint8_t payload_type);

  int EnableNack(size_t max_nack_list_size);

  void DisableNack();

  std::vector<uint16_t> GetNackList(int round_trip_time_ms) const;

  void GetDecodingCallStatistics(AudioDecodingCallStats* stats) const;

 private:
  int UnregisterReceiveCodecSafe(int payload_type);

  ACMGenericCodec* CreateCodec(const CodecInst& codec);

  int InitializeReceiverSafe();

  bool HaveValidEncoder(const char* caller_name) const;

  
  
  int SetVADSafe(bool enable_dtx, bool enable_vad, ACMVADMode mode);

  
  
  int ProcessSingleStream();

  
  
  int ProcessDualStream();

  
  
  
  
  
  
  
  
  
  
  
  int PreprocessToAddData(const AudioFrame& in_frame,
                          const AudioFrame** ptr_out);

  
  
  int UpdateUponReceivingCodec(int index);

  int EncodeFragmentation(int fragmentation_index, int payload_type,
                          uint32_t current_timestamp,
                          ACMGenericCodec* encoder,
                          uint8_t* stream);

  void ResetFragmentation(int vector_size);

  
  
  
  
  
  
  
  
  
  
  int GetAudioDecoder(const CodecInst& codec, int codec_id,
                      int mirror_id, AudioDecoder** decoder);

  AudioPacketizationCallback* packetization_callback_;

  int id_;
  uint32_t expected_codec_ts_;
  uint32_t expected_in_ts_;
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
  int mirror_codec_idx_[ACMCodecDB::kMaxNumCodecs];
  bool stereo_send_;
  int current_send_codec_idx_;
  bool send_codec_registered_;
  ACMResampler resampler_;
  AcmReceiver receiver_;
  CriticalSectionWrapper* acm_crit_sect_;
  ACMVADCallback* vad_callback_;

  
  bool is_first_red_;
  bool fec_enabled_;

  
  
  
  
  uint8_t* red_buffer_;

  
  
  
  RTPFragmentationHeader fragmentation_;
  uint32_t last_fec_timestamp_;

  
  uint8_t previous_pltype_;

  
  
  
  
  
  
  WebRtcRTPHeader* aux_rtp_header_;

  bool receiver_initialized_;

  CriticalSectionWrapper* callback_crit_sect_;

  AudioFrame preprocess_frame_;
  CodecInst secondary_send_codec_inst_;
  scoped_ptr<ACMGenericCodec> secondary_encoder_;
  uint32_t codec_timestamp_;
  bool first_10ms_data_;
};

}  

}  

#endif  
