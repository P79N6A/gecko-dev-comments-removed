









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_AUDIO_CODING_MODULE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_AUDIO_CODING_MODULE_IMPL_H_

#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/source/acm_neteq.h"
#include "webrtc/modules/audio_coding/main/source/acm_resampler.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class ACMDTMFDetection;
class ACMGenericCodec;
class CriticalSectionWrapper;
class RWLockWrapper;

class AudioCodingModuleImpl : public AudioCodingModule {
 public:
  
  explicit AudioCodingModuleImpl(const WebRtc_Word32 id);

  
  ~AudioCodingModuleImpl();

  
  virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

  
  
  WebRtc_Word32 TimeUntilNextProcess();

  
  WebRtc_Word32 Process();

  
  
  

  
  WebRtc_Word32 InitializeSender();

  
  WebRtc_Word32 ResetEncoder();

  
  WebRtc_Word32 RegisterSendCodec(const CodecInst& send_codec);

  
  
  int RegisterSecondarySendCodec(const CodecInst& send_codec);

  
  
  void UnregisterSecondarySendCodec();

  
  int SecondarySendCodec(CodecInst* secondary_codec) const;

  
  WebRtc_Word32 SendCodec(CodecInst& current_codec) const;

  
  WebRtc_Word32 SendFrequency() const;

  
  
  
  WebRtc_Word32 SendBitrate() const;

  
  
  virtual WebRtc_Word32 SetReceivedEstimatedBandwidth(const WebRtc_Word32 bw);

  
  
  WebRtc_Word32 RegisterTransportCallback(
      AudioPacketizationCallback* transport);

  
  
  WebRtc_Word32 RegisterIncomingMessagesCallback(
      AudioCodingFeedback* incoming_message, const ACMCountries cpt);

  
  WebRtc_Word32 Add10MsData(const AudioFrame& audio_frame);

  
  WebRtc_Word32 SetBackgroundNoiseMode(const ACMBackgroundNoiseMode mode);

  
  WebRtc_Word32 BackgroundNoiseMode(ACMBackgroundNoiseMode& mode);

  
  
  

  
  WebRtc_Word32 SetFECStatus(const bool enable_fec);

  
  bool FECStatus() const;

  
  
  
  
  

  WebRtc_Word32 SetVAD(const bool enable_dtx = true,
                       const bool enable_vad = false,
                       const ACMVADMode mode = VADNormal);

  WebRtc_Word32 VAD(bool& dtx_enabled, bool& vad_enabled,
                    ACMVADMode& mode) const;

  WebRtc_Word32 RegisterVADCallback(ACMVADCallback* vad_callback);

  
  ACMVADMode ReceiveVADMode() const;

  
  WebRtc_Word16 SetReceiveVADMode(const ACMVADMode mode);

  
  
  

  
  WebRtc_Word32 InitializeReceiver();

  
  WebRtc_Word32 ResetDecoder();

  
  WebRtc_Word32 ReceiveFrequency() const;

  
  WebRtc_Word32 PlayoutFrequency() const;

  
  
  WebRtc_Word32 RegisterReceiveCodec(const CodecInst& receive_codec);

  
  WebRtc_Word32 ReceiveCodec(CodecInst& current_codec) const;

  
  WebRtc_Word32 IncomingPacket(const WebRtc_UWord8* incoming_payload,
                               const WebRtc_Word32 payload_length,
                               const WebRtcRTPHeader& rtp_info);

  
  
  WebRtc_Word32 IncomingPayload(const WebRtc_UWord8* incoming_payload,
                                const WebRtc_Word32 payload_length,
                                const WebRtc_UWord8 payload_type,
                                const WebRtc_UWord32 timestamp = 0);

  
  WebRtc_Word32 SetMinimumPlayoutDelay(const WebRtc_Word32 time_ms);

  
  
  WebRtc_Word32 SetDtmfPlayoutStatus(const bool enable);

  
  bool DtmfPlayoutStatus() const;

  
  
  
  WebRtc_Word32 DecoderEstimatedBandwidth() const;

  
  WebRtc_Word32 SetPlayoutMode(const AudioPlayoutMode mode);

  
  AudioPlayoutMode PlayoutMode() const;

  
  WebRtc_Word32 PlayoutTimestamp(WebRtc_UWord32& timestamp);

  
  
  WebRtc_Word32 PlayoutData10Ms(const WebRtc_Word32 desired_freq_hz,
                                AudioFrame &audio_frame);

  
  
  

  WebRtc_Word32 NetworkStatistics(ACMNetworkStatistics& statistics) const;

  void DestructEncoderInst(void* inst);

  WebRtc_Word16 AudioBuffer(WebRtcACMAudioBuff& buffer);

  
  
  WebRtc_Word32 REDPayloadISAC(const WebRtc_Word32 isac_rate,
                               const WebRtc_Word16 isac_bw_estimate,
                               WebRtc_UWord8* payload,
                               WebRtc_Word16* length_bytes);

  WebRtc_Word16 SetAudioBuffer(WebRtcACMAudioBuff& buffer);

  WebRtc_UWord32 EarliestTimestamp() const;

  WebRtc_Word32 LastEncodedTimestamp(WebRtc_UWord32& timestamp) const;

  WebRtc_Word32 ReplaceInternalDTXWithWebRtc(const bool use_webrtc_dtx);

  WebRtc_Word32 IsInternalDTXReplacedWithWebRtc(bool& uses_webrtc_dtx);

  WebRtc_Word32 SetISACMaxRate(const WebRtc_UWord32 max_bit_per_sec);

  WebRtc_Word32 SetISACMaxPayloadSize(const WebRtc_UWord16 max_size_bytes);

  WebRtc_Word32 ConfigISACBandwidthEstimator(
      const WebRtc_UWord8 frame_size_ms,
      const WebRtc_UWord16 rate_bit_per_sec,
      const bool enforce_frame_size = false);

  WebRtc_Word32 UnregisterReceiveCodec(const WebRtc_Word16 payload_type);

 protected:
  void UnregisterSendCodec();

  WebRtc_Word32 UnregisterReceiveCodecSafe(const WebRtc_Word16 id);

  ACMGenericCodec* CreateCodec(const CodecInst& codec);

  WebRtc_Word16 DecoderParamByPlType(const WebRtc_UWord8 payload_type,
                                     WebRtcACMCodecParams& codec_params) const;

  WebRtc_Word16 DecoderListIDByPlName(
      const char* name, const WebRtc_UWord16 frequency = 0) const;

  WebRtc_Word32 InitializeReceiverSafe();

  bool HaveValidEncoder(const char* caller_name) const;

  WebRtc_Word32 RegisterRecCodecMSSafe(const CodecInst& receive_codec,
                                       WebRtc_Word16 codec_id,
                                       WebRtc_Word16 mirror_id,
                                       ACMNetEQ::JitterBuffer jitter_buffer);

  
  
  int SetVADSafe(bool enable_dtx, bool enable_vad, ACMVADMode mode);

  
  
  int ProcessSingleStream();

  
  
  int ProcessDualStream();

  
  
  
  
  
  
  
  
  
  
  
  int PreprocessToAddData(const AudioFrame& in_frame,
                          const AudioFrame** ptr_out);

 private:
  
  
  int UpdateUponReceivingCodec(int index);

  
  
  int InitStereoSlave();

  
  
  bool IsCodecForSlave(int index) const;

  int EncodeFragmentation(int fragmentation_index, int payload_type,
                          uint32_t current_timestamp,
                          ACMGenericCodec* encoder,
                          uint8_t* stream);

  void ResetFragmentation(int vector_size);

  AudioPacketizationCallback* packetization_callback_;
  WebRtc_Word32 id_;
  WebRtc_UWord32 last_timestamp_;
  WebRtc_UWord32 last_in_timestamp_;
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
  WebRtc_Word16 mirror_codec_idx_[ACMCodecDB::kMaxNumCodecs];
  bool stereo_receive_[ACMCodecDB::kMaxNumCodecs];
  bool stereo_receive_registered_;
  bool stereo_send_;
  int prev_received_channel_;
  int expected_channels_;
  WebRtc_Word32 current_send_codec_idx_;
  int current_receive_codec_idx_;
  bool send_codec_registered_;
  ACMResampler input_resampler_;
  ACMResampler output_resampler_;
  ACMNetEQ neteq_;
  CriticalSectionWrapper* acm_crit_sect_;
  ACMVADCallback* vad_callback_;
  WebRtc_UWord8 last_recv_audio_codec_pltype_;

  
  bool is_first_red_;
  bool fec_enabled_;
  
  
  
  
  WebRtc_UWord8* red_buffer_;
  
  
  
  RTPFragmentationHeader fragmentation_;
  WebRtc_UWord32 last_fec_timestamp_;
  
  
  WebRtc_UWord8 receive_red_pltype_;

  
  WebRtc_UWord8 previous_pltype_;

  
  
  
  WebRtc_Word16 registered_pltypes_[ACMCodecDB::kMaxNumCodecs];

  
  
  
  WebRtcRTPHeader* dummy_rtp_header_;
  WebRtc_UWord16 recv_pl_frame_size_smpls_;

  bool receiver_initialized_;
  ACMDTMFDetection* dtmf_detector_;

  AudioCodingFeedback* dtmf_callback_;
  WebRtc_Word16 last_detected_tone_;
  CriticalSectionWrapper* callback_crit_sect_;

  AudioFrame audio_frame_;
  AudioFrame preprocess_frame_;
  CodecInst secondary_send_codec_inst_;
  scoped_ptr<ACMGenericCodec> secondary_encoder_;
};

}  

#endif  
