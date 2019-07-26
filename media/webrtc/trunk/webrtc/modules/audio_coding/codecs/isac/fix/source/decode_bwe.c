

















#include "bandwidth_estimator.h"
#include "codec.h"
#include "entropy_coding.h"
#include "structs.h"




int WebRtcIsacfix_EstimateBandwidth(BwEstimatorstr *bwest_str,
                                    Bitstr_dec  *streamdata,
                                    int32_t  packet_size,
                                    uint16_t rtp_seq_number,
                                    uint32_t send_ts,
                                    uint32_t arr_ts)
{
  int16_t index;
  int16_t frame_samples;
  int err;

  
  err = WebRtcIsacfix_DecodeFrameLen(streamdata, &frame_samples);
  
  if (err<0) {
    return err;
  }

  
  err = WebRtcIsacfix_DecodeSendBandwidth(streamdata, &index);
  
  if (err<0) {
    return err;
  }

  
  err = WebRtcIsacfix_UpdateUplinkBwImpl(
      bwest_str,
      rtp_seq_number,
      (uint16_t)WEBRTC_SPL_UDIV(WEBRTC_SPL_UMUL(frame_samples,1000), FS),
      send_ts,
      arr_ts,
      (int16_t) packet_size,  
      index);

  
  if (err<0) {
    return err;
  }

  
  return 0;
}
