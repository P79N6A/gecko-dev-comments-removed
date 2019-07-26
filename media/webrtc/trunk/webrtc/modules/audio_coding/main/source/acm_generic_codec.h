









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_GENERIC_CODEC_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_GENERIC_CODEC_H_

#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

#define MAX_FRAME_SIZE_10MSEC 6


struct WebRtcVadInst;
struct WebRtcCngEncInst;

namespace webrtc {


struct CodecInst;
struct WebRtcACMCodecParams;

namespace acm1 {

class ACMNetEQ;

class ACMGenericCodec {
 public:
  
  
  
  ACMGenericCodec();

  
  
  
  virtual ~ACMGenericCodec();

  
  
  
  
  virtual ACMGenericCodec* CreateInstance() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t Encode(uint8_t* bitstream,
                 int16_t* bitstream_len_byte,
                 uint32_t* timestamp,
                 WebRtcACMEncodingType* encoding_type);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t Decode(uint8_t* bitstream,
                 int16_t bitstream_len_byte,
                 int16_t* audio,
                 int16_t* audio_samples,
                 int8_t* speech_type);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void SplitStereoPacket(uint8_t* ,
                                 int32_t* ) {}

  
  
  
  
  
  
  
  bool EncoderInitialized();

  
  
  
  
  
  
  
  bool DecoderInitialized();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t EncoderParams(WebRtcACMCodecParams *enc_params);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool DecoderParams(WebRtcACMCodecParams *dec_params,
                     const uint8_t payload_type);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t InitEncoder(WebRtcACMCodecParams* codec_params,
                      bool force_initialization);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t InitDecoder(WebRtcACMCodecParams* codec_params,
                      bool force_initialization);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t RegisterInNetEq(ACMNetEQ* neteq, const CodecInst& codec_inst);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t Add10MsData(const uint32_t timestamp,
                      const int16_t* data,
                      const uint16_t length,
                      const uint8_t audio_channel);

  
  
  
  
  
  
  
  
  
  
  uint32_t NoMissedSamples() const;

  
  
  
  
  
  void ResetNoMissedSamples();

  
  
  
  
  
  
  
  
  
  
  
  
  int16_t SetBitRate(const int32_t bitrate_bps);

  
  
  
  
  
  
  
  
  
  
  void DestructEncoderInst(void* ptr_inst);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t AudioBuffer(WebRtcACMAudioBuff& audio_buff);

  
  
  
  
  
  
  
  
  uint32_t EarliestTimestamp() const;

  
  
  
  
  
  
  
  
  
  int16_t SetAudioBuffer(WebRtcACMAudioBuff& audio_buff);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t SetVAD(bool* enable_dtx,
                 bool* enable_vad,
                 ACMVADMode* mode);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t ReplaceInternalDTX(const bool replace_internal_dtx);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t IsInternalDTXReplaced(bool* internal_dtx_replaced);

  
  
  
  
  
  
  
  void SetNetEqDecodeLock(RWLockWrapper* neteq_decode_lock) {
    neteq_decode_lock_ = neteq_decode_lock;
  }

  
  
  
  
  
  
  
  
  bool HasInternalDTX() const {
    return has_internal_dtx_;
  }

  
  
  
  
  
  
  
  
  
  int32_t GetEstimatedBandwidth();

  
  
  
  
  
  
  
  
  
  
  
  
  int32_t SetEstimatedBandwidth(int32_t estimated_bandwidth);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t GetRedPayload(uint8_t* red_payload,
                        int16_t* payload_bytes);

  
  
  
  
  
  
  
  
  
  
  int16_t ResetEncoder();

  
  
  
  
  
  
  
  
  
  int16_t ResetDecoder(int16_t payload_type);

  
  
  
  
  
  
  
  void DestructEncoder();

  
  
  
  
  
  
  
  
  void DestructDecoder();

  
  
  
  
  
  
  
  int16_t SamplesLeftToEncode();

  
  
  
  
  
  
  
  uint32_t LastEncodedTimestamp() const;

  
  
  
  
  
  
  
  void SetUniqueID(const uint32_t id);

  
  
  
  
  
  
  
  
  bool IsAudioBufferFresh() const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t UpdateDecoderSampFreq(int16_t );

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t UpdateEncoderSampFreq(
      uint16_t samp_freq_hz);

  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t EncoderSampFreq(uint16_t& samp_freq_hz);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t ConfigISACBandwidthEstimator(
      const uint8_t init_frame_size_msec,
      const uint16_t init_rate_bps,
      const bool enforce_frame_size);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetISACMaxPayloadSize(
      const uint16_t max_payload_len_bytes);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetISACMaxRate(const uint32_t max_rate_bps);

  
  
  
  
  
  
  
  
  void SaveDecoderParam(const WebRtcACMCodecParams* codec_params);

  int32_t FrameSize() {
    return frame_len_smpl_;
  }

  void SetIsMaster(bool is_master);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t REDPayloadISAC(const int32_t isac_rate,
                                 const int16_t isac_bw_estimate,
                                 uint8_t* payload,
                                 int16_t* payload_len_bytes);

  
  
  
  
  
  
  
  
  virtual bool IsTrueStereoCodec();

  
  
  
  
  
  bool HasFrameToEncode() const;

 protected:
  
  
  
  
  
  
  

  
  
  
  
  virtual int16_t DecodeSafe(uint8_t* bitstream,
                             int16_t bitstream_len_byte,
                             int16_t* audio,
                             int16_t* audio_samples,
                             int8_t* speech_type) = 0;

  
  
  
  
  virtual int32_t Add10MsDataSafe(const uint32_t timestamp,
                                  const int16_t* data,
                                  const uint16_t length,
                                  const uint8_t audio_channel);

  
  
  
  
  virtual int32_t CodecDef(WebRtcNetEQ_CodecDef& codec_def,
                           const CodecInst& codec_inst) = 0;

  
  
  
  
  int16_t EncoderParamsSafe(WebRtcACMCodecParams *enc_params);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual bool DecoderParamsSafe(WebRtcACMCodecParams *dec_params,
                                 const uint8_t payload_type);

  
  
  
  
  int16_t ResetEncoderSafe();

  
  
  
  
  int16_t InitEncoderSafe(WebRtcACMCodecParams *codec_params,
                          bool force_initialization);

  
  
  
  
  int16_t InitDecoderSafe(WebRtcACMCodecParams *codec_params,
                          bool force_initialization);

  
  
  
  
  int16_t ResetDecoderSafe(int16_t payload_type);

  
  
  
  
  virtual void DestructEncoderSafe() = 0;

  
  
  
  
  virtual void DestructDecoderSafe() = 0;

  
  
  
  
  
  
  virtual int16_t SetBitRateSafe(const int32_t bitrate_bps);

  
  
  
  
  virtual int32_t GetEstimatedBandwidthSafe();

  
  
  
  
  virtual int32_t SetEstimatedBandwidthSafe(
      int32_t estimated_bandwidth);

  
  
  
  
  virtual int32_t GetRedPayloadSafe(uint8_t* red_payload,
                                    int16_t* payload_bytes);

  
  
  
  
  int16_t SetVADSafe(bool* enable_dtx,
                     bool* enable_vad,
                     ACMVADMode* mode);

  
  
  
  
  virtual int32_t ReplaceInternalDTXSafe(const bool replace_internal_dtx);

  
  
  
  
  virtual int32_t IsInternalDTXReplacedSafe(bool* internal_dtx_replaced);

  
  
  
  
  
  
  
  
  int16_t CreateEncoder();

  
  
  
  
  
  
  
  
  int16_t CreateDecoder();

  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t EnableVAD(ACMVADMode mode);

  
  
  
  
  
  
  
  
  int16_t DisableVAD();

  
  
  
  
  
  
  
  
  
  virtual int16_t EnableDTX();

  
  
  
  
  
  
  
  
  
  virtual int16_t DisableDTX();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalEncode(uint8_t* bitstream,
                                 int16_t* bitstream_len_byte) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalInitEncoder(
      WebRtcACMCodecParams *codec_params) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalInitDecoder(
      WebRtcACMCodecParams *codec_params) = 0;

  
  
  
  
  
  
  
  
  
  void IncreaseNoMissedSamples(const int16_t num_samples);

  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalCreateEncoder() = 0;

  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalCreateDecoder() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void InternalDestructEncoderInst(void* ptr_inst) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalResetEncoder();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t ProcessFrameVADDTX(uint8_t* bitstream,
                             int16_t* bitstream_len_byte,
                             int16_t* samples_processed);

  
  
  
  
  
  
  
  
  
  virtual bool CanChangeEncodingParam(CodecInst& codec_inst);

  
  
  
  
  
  
  
  
  
  
  virtual void CurrentRate(int32_t& );

  virtual void SaveDecoderParamSafe(const WebRtcACMCodecParams* codec_params);

  
  
  int16_t in_audio_ix_write_;

  
  int16_t in_audio_ix_read_;

  int16_t in_timestamp_ix_write_;

  
  
  
  
  int16_t* in_audio_;
  uint32_t* in_timestamp_;

  int16_t frame_len_smpl_;
  uint16_t num_channels_;

  
  int16_t codec_id_;

  
  
  
  uint32_t num_missed_samples_;

  
  bool encoder_exist_;
  bool decoder_exist_;
  
  bool encoder_initialized_;
  bool decoder_initialized_;

  bool registered_in_neteq_;

  
  bool has_internal_dtx_;
  WebRtcVadInst* ptr_vad_inst_;
  bool vad_enabled_;
  ACMVADMode vad_mode_;
  int16_t vad_label_[MAX_FRAME_SIZE_10MSEC];
  bool dtx_enabled_;
  WebRtcCngEncInst* ptr_dtx_inst_;
  uint8_t num_lpc_params_;
  bool sent_cn_previous_;
  bool is_master_;
  int16_t prev_frame_cng_;

  WebRtcACMCodecParams encoder_params_;
  WebRtcACMCodecParams decoder_params_;

  
  
  RWLockWrapper* neteq_decode_lock_;
  
  
  RWLockWrapper& codec_wrapper_lock_;

  uint32_t last_encoded_timestamp_;
  uint32_t last_timestamp_;
  bool is_audio_buff_fresh_;
  uint32_t unique_id_;
};

}  

}  

#endif  
