









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_GENERIC_CODEC_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_GENERIC_CODEC_H_

#include "webrtc/base/thread_annotations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_common_defs.h"
#include "webrtc/modules/audio_coding/neteq/interface/neteq.h"
#include "webrtc/modules/audio_coding/neteq/interface/audio_decoder.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"

#define MAX_FRAME_SIZE_10MSEC 6


struct WebRtcVadInst;
struct WebRtcCngEncInst;

namespace webrtc {

struct WebRtcACMCodecParams;
struct CodecInst;

namespace acm2 {


class AcmReceiver;

class ACMGenericCodec {
 public:
  
  
  
  ACMGenericCodec();

  
  
  
  virtual ~ACMGenericCodec();

  
  
  
  
  virtual ACMGenericCodec* CreateInstance() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t Encode(uint8_t* bitstream,
                 int16_t* bitstream_len_byte,
                 uint32_t* timestamp,
                 WebRtcACMEncodingType* encoding_type);

  
  
  
  
  
  
  
  bool EncoderInitialized();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t EncoderParams(WebRtcACMCodecParams* enc_params);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t InitEncoder(WebRtcACMCodecParams* codec_params,
                      bool force_initialization);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t Add10MsData(const uint32_t timestamp,
                      const int16_t* data,
                      const uint16_t length,
                      const uint8_t audio_channel);

  
  
  
  
  
  
  
  
  
  
  uint32_t NoMissedSamples() const;

  
  
  
  
  
  void ResetNoMissedSamples();

  
  
  
  
  
  
  
  
  
  
  
  
  int16_t SetBitRate(const int32_t bitrate_bps);

  
  
  
  
  
  
  
  
  uint32_t EarliestTimestamp() const;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t SetVAD(bool* enable_dtx, bool* enable_vad, ACMVADMode* mode);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t ReplaceInternalDTX(const bool replace_internal_dtx);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t IsInternalDTXReplaced(bool* internal_dtx_replaced);

  
  
  
  
  
  
  
  
  bool HasInternalDTX() const {
    ReadLockScoped rl(codec_wrapper_lock_);
    return has_internal_dtx_;
  }

  
  
  
  
  
  
  
  
  
  int32_t GetEstimatedBandwidth();

  
  
  
  
  
  
  
  
  
  
  
  
  int32_t SetEstimatedBandwidth(int32_t estimated_bandwidth);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t GetRedPayload(uint8_t* red_payload, int16_t* payload_bytes);

  
  
  
  
  
  
  
  
  
  
  int16_t ResetEncoder();

  
  
  
  
  
  
  
  void DestructEncoder();

  
  
  
  
  
  
  
  int16_t SamplesLeftToEncode();

  
  
  
  
  
  
  
  void SetUniqueID(const uint32_t id);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t UpdateDecoderSampFreq(int16_t ) { return 0; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t UpdateEncoderSampFreq(uint16_t samp_freq_hz)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t EncoderSampFreq(uint16_t* samp_freq_hz)
      SHARED_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t ConfigISACBandwidthEstimator(
      const uint8_t init_frame_size_msec,
      const uint16_t init_rate_bps,
      const bool enforce_frame_size);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetISACMaxPayloadSize(const uint16_t max_payload_len_bytes);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetISACMaxRate(const uint32_t max_rate_bps);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t REDPayloadISAC(const int32_t isac_rate,
                                 const int16_t isac_bw_estimate,
                                 uint8_t* payload,
                                 int16_t* payload_len_bytes);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetOpusMaxPlaybackRate(int );

  
  
  
  
  
  bool HasFrameToEncode() const;

  
  
  
  
  
  
  
  virtual AudioDecoder* Decoder(int ) { return NULL; }

  
  
  
  
  
  
  
  
  bool HasInternalFEC() const {
    ReadLockScoped rl(codec_wrapper_lock_);
    return has_internal_fec_;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetFEC(bool enable_fec);

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetPacketLossRate(int ) { return 0; }

 protected:
  
  
  
  
  
  
  

  
  
  
  
  virtual int32_t Add10MsDataSafe(const uint32_t timestamp,
                                  const int16_t* data,
                                  const uint16_t length,
                                  const uint8_t audio_channel)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  int16_t EncoderParamsSafe(WebRtcACMCodecParams* enc_params)
      SHARED_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  int16_t ResetEncoderSafe() EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  int16_t InitEncoderSafe(WebRtcACMCodecParams* codec_params,
                          bool force_initialization)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  int16_t InitDecoderSafe(WebRtcACMCodecParams* codec_params,
                          bool force_initialization);

  
  
  
  
  virtual void DestructEncoderSafe()
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_) = 0;

  
  
  
  
  
  
  virtual int16_t SetBitRateSafe(const int32_t bitrate_bps)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  virtual int32_t GetEstimatedBandwidthSafe();

  
  
  
  
  virtual int32_t SetEstimatedBandwidthSafe(int32_t estimated_bandwidth);

  
  
  
  
  virtual int32_t GetRedPayloadSafe(uint8_t* red_payload,
                                    int16_t* payload_bytes);

  
  
  
  
  int16_t SetVADSafe(bool* enable_dtx, bool* enable_vad, ACMVADMode* mode)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  virtual int32_t ReplaceInternalDTXSafe(const bool replace_internal_dtx);

  
  
  
  
  virtual int32_t IsInternalDTXReplacedSafe(bool* internal_dtx_replaced);

  
  
  
  
  
  
  
  
  int16_t CreateEncoder() EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t EnableVAD(ACMVADMode mode)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  int16_t DisableVAD() EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  virtual int16_t EnableDTX() EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  virtual int16_t DisableDTX() EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalEncode(uint8_t* bitstream,
                                 int16_t* bitstream_len_byte)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalInitEncoder(WebRtcACMCodecParams* codec_params)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_) = 0;

  
  
  
  
  
  
  
  
  
  void IncreaseNoMissedSamples(const int16_t num_samples)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalCreateEncoder() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int16_t InternalResetEncoder()
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int16_t ProcessFrameVADDTX(uint8_t* bitstream,
                             int16_t* bitstream_len_byte,
                             int16_t* samples_processed)
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  
  
  
  
  
  
  
  
  
  
  virtual void CurrentRate(int32_t* ) {}

  
  
  int16_t in_audio_ix_write_ GUARDED_BY(codec_wrapper_lock_);

  
  int16_t in_audio_ix_read_ GUARDED_BY(codec_wrapper_lock_);

  int16_t in_timestamp_ix_write_ GUARDED_BY(codec_wrapper_lock_);

  
  
  
  
  int16_t* in_audio_ GUARDED_BY(codec_wrapper_lock_);
  uint32_t* in_timestamp_ GUARDED_BY(codec_wrapper_lock_);

  int16_t frame_len_smpl_ GUARDED_BY(codec_wrapper_lock_);
  uint16_t num_channels_ GUARDED_BY(codec_wrapper_lock_);

  
  int16_t codec_id_ GUARDED_BY(codec_wrapper_lock_);

  
  
  
  uint32_t num_missed_samples_ GUARDED_BY(codec_wrapper_lock_);

  
  bool encoder_exist_ GUARDED_BY(codec_wrapper_lock_);

  
  bool encoder_initialized_ GUARDED_BY(codec_wrapper_lock_);

  const bool registered_in_neteq_
      GUARDED_BY(codec_wrapper_lock_);  

  
  bool has_internal_dtx_ GUARDED_BY(codec_wrapper_lock_);
  WebRtcVadInst* ptr_vad_inst_ GUARDED_BY(codec_wrapper_lock_);
  bool vad_enabled_ GUARDED_BY(codec_wrapper_lock_);
  ACMVADMode vad_mode_ GUARDED_BY(codec_wrapper_lock_);
  int16_t vad_label_[MAX_FRAME_SIZE_10MSEC] GUARDED_BY(codec_wrapper_lock_);
  bool dtx_enabled_ GUARDED_BY(codec_wrapper_lock_);
  WebRtcCngEncInst* ptr_dtx_inst_ GUARDED_BY(codec_wrapper_lock_);
  uint8_t num_lpc_params_               
      GUARDED_BY(codec_wrapper_lock_);  
  bool sent_cn_previous_ GUARDED_BY(codec_wrapper_lock_);
  int16_t prev_frame_cng_ GUARDED_BY(codec_wrapper_lock_);

  
  bool has_internal_fec_ GUARDED_BY(codec_wrapper_lock_);

  WebRtcACMCodecParams encoder_params_ GUARDED_BY(codec_wrapper_lock_);

  
  
  RWLockWrapper& codec_wrapper_lock_;

  uint32_t last_timestamp_ GUARDED_BY(codec_wrapper_lock_);
  uint32_t unique_id_;
};

}  

}  

#endif  
