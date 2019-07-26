









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_GENERIC_CODEC_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_GENERIC_CODEC_H_

#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/main/source/acm_common_defs.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

#define MAX_FRAME_SIZE_10MSEC 6


struct WebRtcVadInst;
struct WebRtcCngEncInst;

namespace webrtc {


struct CodecInst;
class ACMNetEQ;

class ACMGenericCodec {
 public:
  
  
  
  ACMGenericCodec();

  
  
  
  virtual ~ACMGenericCodec();

  
  
  
  
  virtual ACMGenericCodec* CreateInstance() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 Encode(WebRtc_UWord8* bitstream,
                       WebRtc_Word16* bitstream_len_byte,
                       WebRtc_UWord32* timestamp,
                       WebRtcACMEncodingType* encoding_type);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 Decode(WebRtc_UWord8* bitstream,
                       WebRtc_Word16 bitstream_len_byte,
                       WebRtc_Word16* audio,
                       WebRtc_Word16* audio_samples,
                       WebRtc_Word8* speech_type);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void SplitStereoPacket(WebRtc_UWord8* ,
                                 WebRtc_Word32* ) {}

  
  
  
  
  
  
  
  bool EncoderInitialized();

  
  
  
  
  
  
  
  bool DecoderInitialized();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 EncoderParams(WebRtcACMCodecParams *enc_params);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool DecoderParams(WebRtcACMCodecParams *dec_params,
                     const WebRtc_UWord8 payload_type);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 InitEncoder(WebRtcACMCodecParams* codec_params,
                            bool force_initialization);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 InitDecoder(WebRtcACMCodecParams* codec_params,
                            bool force_initialization);

  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 RegisterInNetEq(ACMNetEQ* neteq, const CodecInst& codec_inst);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 Add10MsData(const WebRtc_UWord32 timestamp,
                            const WebRtc_Word16* data,
                            const WebRtc_UWord16 length,
                            const WebRtc_UWord8 audio_channel);

  
  
  
  
  
  
  
  
  
  
  WebRtc_UWord32 NoMissedSamples() const;

  
  
  
  
  
  void ResetNoMissedSamples();

  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 SetBitRate(const WebRtc_Word32 bitrate_bps);

  
  
  
  
  
  
  
  
  
  
  void DestructEncoderInst(void* ptr_inst);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 AudioBuffer(WebRtcACMAudioBuff& audio_buff);

  
  
  
  
  
  
  
  
  WebRtc_UWord32 EarliestTimestamp() const;

  
  
  
  
  
  
  
  
  
  WebRtc_Word16 SetAudioBuffer(WebRtcACMAudioBuff& audio_buff);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 SetVAD(const bool enable_dtx = true,
                       const bool enable_vad = false,
                       const ACMVADMode mode = VADNormal);

  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 ReplaceInternalDTX(const bool replace_internal_dtx);

  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 IsInternalDTXReplaced(bool* internal_dtx_replaced);

  
  
  
  
  
  
  
  void SetNetEqDecodeLock(RWLockWrapper* neteq_decode_lock) {
    neteq_decode_lock_ = neteq_decode_lock;
  }

  
  
  
  
  
  
  
  
  bool HasInternalDTX() const {
    return has_internal_dtx_;
  }

  
  
  
  
  
  
  
  
  
  WebRtc_Word32 GetEstimatedBandwidth();

  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 SetEstimatedBandwidth(WebRtc_Word32 estimated_bandwidth);

  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 GetRedPayload(WebRtc_UWord8* red_payload,
                              WebRtc_Word16* payload_bytes);

  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 ResetEncoder();

  
  
  
  
  
  
  
  
  
  WebRtc_Word16 ResetDecoder(WebRtc_Word16 payload_type);

  
  
  
  
  
  
  
  void DestructEncoder();

  
  
  
  
  
  
  
  
  void DestructDecoder();

  
  
  
  
  
  
  
  WebRtc_Word16 SamplesLeftToEncode();

  
  
  
  
  
  
  
  WebRtc_UWord32 LastEncodedTimestamp() const;

  
  
  
  
  
  
  
  void SetUniqueID(const WebRtc_UWord32 id);

  
  
  
  
  
  
  
  
  bool IsAudioBufferFresh() const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 UpdateDecoderSampFreq(WebRtc_Word16 ) {
    return 0;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 UpdateEncoderSampFreq(
      WebRtc_UWord16 samp_freq_hz);

  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 EncoderSampFreq(WebRtc_UWord16& samp_freq_hz);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ConfigISACBandwidthEstimator(
      const WebRtc_UWord8 init_frame_size_msec,
      const WebRtc_UWord16 init_rate_bps,
      const bool enforce_frame_size);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetISACMaxPayloadSize(
      const WebRtc_UWord16 max_payload_len_bytes);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetISACMaxRate(const WebRtc_UWord32 max_rate_bps);

  
  
  
  
  
  
  
  
  void SaveDecoderParam(const WebRtcACMCodecParams* codec_params);

  WebRtc_Word32 FrameSize() {
    return frame_len_smpl_;
  }

  void SetIsMaster(bool is_master);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 REDPayloadISAC(const WebRtc_Word32 isac_rate,
                                       const WebRtc_Word16 isac_bw_estimate,
                                       WebRtc_UWord8* payload,
                                       WebRtc_Word16* payload_len_bytes);

  
  
  
  
  
  
  
  
  virtual bool IsTrueStereoCodec() {
    return false;
  }

  
  
  
  
  
  bool HasFrameToEncode() const;

 protected:
  
  
  
  
  
  
  

  
  
  
  
  virtual WebRtc_Word16 DecodeSafe(WebRtc_UWord8* bitstream,
                                   WebRtc_Word16 bitstream_len_byte,
                                   WebRtc_Word16* audio,
                                   WebRtc_Word16* audio_samples,
                                   WebRtc_Word8* speech_type) = 0;

  
  
  
  
  virtual WebRtc_Word32 Add10MsDataSafe(const WebRtc_UWord32 timestamp,
                                        const WebRtc_Word16* data,
                                        const WebRtc_UWord16 length,
                                        const WebRtc_UWord8 audio_channel);

  
  
  
  
  virtual WebRtc_Word32 CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                                 const CodecInst& codec_inst) = 0;

  
  
  
  
  WebRtc_Word16 EncoderParamsSafe(WebRtcACMCodecParams *enc_params);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool DecoderParamsSafe(WebRtcACMCodecParams *dec_params,
                                 const WebRtc_UWord8 payload_type);

  
  
  
  
  WebRtc_Word16 ResetEncoderSafe();

  
  
  
  
  WebRtc_Word16 InitEncoderSafe(WebRtcACMCodecParams *codec_params,
                                bool force_initialization);

  
  
  
  
  WebRtc_Word16 InitDecoderSafe(WebRtcACMCodecParams *codec_params,
                                bool force_initialization);

  
  
  
  
  WebRtc_Word16 ResetDecoderSafe(WebRtc_Word16 payload_type);

  
  
  
  
  virtual void DestructEncoderSafe() = 0;

  
  
  
  
  virtual void DestructDecoderSafe() = 0;

  
  
  
  
  
  
  virtual WebRtc_Word16 SetBitRateSafe(const WebRtc_Word32 bitrate_bps);

  
  
  
  
  virtual WebRtc_Word32 GetEstimatedBandwidthSafe();

  
  
  
  
  virtual WebRtc_Word32 SetEstimatedBandwidthSafe(
      WebRtc_Word32 estimated_bandwidth);

  
  
  
  
  virtual WebRtc_Word32 GetRedPayloadSafe(WebRtc_UWord8* red_payload,
                                          WebRtc_Word16* payload_bytes);

  
  
  
  
  WebRtc_Word16 SetVADSafe(const bool enable_dtx = true,
                           const bool enable_vad = false,
                           const ACMVADMode mode = VADNormal);

  
  
  
  
  virtual WebRtc_Word32 ReplaceInternalDTXSafe(const bool replace_internal_dtx);

  
  
  
  
  virtual WebRtc_Word32 IsInternalDTXReplacedSafe(bool* internal_dtx_replaced);

  
  
  
  
  
  
  
  
  WebRtc_Word16 CreateEncoder();

  
  
  
  
  
  
  
  
  WebRtc_Word16 CreateDecoder();

  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 EnableVAD(ACMVADMode mode);

  
  
  
  
  
  
  
  
  WebRtc_Word16 DisableVAD();

  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 EnableDTX();

  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 DisableDTX();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 InternalEncode(WebRtc_UWord8* bitstream,
                                       WebRtc_Word16* bitstream_len_byte) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 InternalInitEncoder(
      WebRtcACMCodecParams *codec_params) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 InternalInitDecoder(
      WebRtcACMCodecParams *codec_params) = 0;

  
  
  
  
  
  
  
  
  
  void IncreaseNoMissedSamples(const WebRtc_Word16 num_samples);

  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 InternalCreateEncoder() = 0;

  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 InternalCreateDecoder() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void InternalDestructEncoderInst(void* ptr_inst) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 InternalResetEncoder();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 ProcessFrameVADDTX(WebRtc_UWord8* bitstream,
                                   WebRtc_Word16* bitstream_len_byte,
                                   WebRtc_Word16* samples_processed);

  
  
  
  
  
  
  
  
  
  virtual bool CanChangeEncodingParam(CodecInst& codec_inst);

  
  
  
  
  
  
  
  
  
  
  virtual void CurrentRate(WebRtc_Word32& ) {
    return;
  }

  virtual void SaveDecoderParamSafe(const WebRtcACMCodecParams* codec_params);

  
  
  WebRtc_Word16 in_audio_ix_write_;

  
  WebRtc_Word16 in_audio_ix_read_;

  WebRtc_Word16 in_timestamp_ix_write_;

  
  
  
  
  WebRtc_Word16* in_audio_;
  WebRtc_UWord32* in_timestamp_;

  WebRtc_Word16 frame_len_smpl_;
  WebRtc_UWord16 num_channels_;

  
  WebRtc_Word16 codec_id_;

  
  
  
  WebRtc_UWord32 num_missed_samples_;

  
  bool encoder_exist_;
  bool decoder_exist_;
  
  bool encoder_initialized_;
  bool decoder_initialized_;

  bool registered_in_neteq_;

  
  bool has_internal_dtx_;
  WebRtcVadInst* ptr_vad_inst_;
  bool vad_enabled_;
  ACMVADMode vad_mode_;
  WebRtc_Word16 vad_label_[MAX_FRAME_SIZE_10MSEC];
  bool dtx_enabled_;
  WebRtcCngEncInst* ptr_dtx_inst_;
  WebRtc_UWord8 num_lpc_params_;
  bool sent_cn_previous_;
  bool is_master_;
  int16_t prev_frame_cng_;

  WebRtcACMCodecParams encoder_params_;
  WebRtcACMCodecParams decoder_params_;

  
  
  RWLockWrapper* neteq_decode_lock_;
  
  
  RWLockWrapper& codec_wrapper_lock_;

  WebRtc_UWord32 last_encoded_timestamp_;
  WebRtc_UWord32 last_timestamp_;
  bool is_audio_buff_fresh_;
  WebRtc_UWord32 unique_id_;
};

}  

#endif  
