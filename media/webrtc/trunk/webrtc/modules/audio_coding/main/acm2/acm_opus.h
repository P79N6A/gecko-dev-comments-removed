









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_OPUS_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_OPUS_H_

#include "webrtc/common_audio/resampler/include/resampler.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_generic_codec.h"

struct WebRtcOpusEncInst;
struct WebRtcOpusDecInst;

namespace webrtc {

namespace acm2 {

class ACMOpus : public ACMGenericCodec {
 public:
  explicit ACMOpus(int16_t codec_id);
  ~ACMOpus();

  ACMGenericCodec* CreateInstance(void);

  int16_t InternalEncode(uint8_t* bitstream,
                         int16_t* bitstream_len_byte) OVERRIDE
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  int16_t InternalInitEncoder(WebRtcACMCodecParams *codec_params);

  virtual int SetFEC(bool enable_fec) OVERRIDE;

  virtual int SetPacketLossRate(int loss_rate) OVERRIDE;

  virtual int SetOpusMaxPlaybackRate(int frequency_hz) OVERRIDE;

 protected:
  void DestructEncoderSafe();

  int16_t InternalCreateEncoder();

  int16_t SetBitRateSafe(const int32_t rate) OVERRIDE
      EXCLUSIVE_LOCKS_REQUIRED(codec_wrapper_lock_);

  WebRtcOpusEncInst* encoder_inst_ptr_;
  uint16_t sample_freq_;
  int32_t bitrate_;
  int channels_;

  int packet_loss_rate_;
};

}  

}  

#endif  
