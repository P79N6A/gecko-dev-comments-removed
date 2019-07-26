

















#include "bandwidth_estimator.h"
#include "codec.h"
#include "entropy_coding.h"
#include "structs.h"




int WebRtcIsacfix_EstimateBandwidth(BwEstimatorstr *bwest_str,
                                    Bitstr_dec  *streamdata,
                                    WebRtc_Word32  packet_size,
                                    WebRtc_UWord16 rtp_seq_number,
                                    WebRtc_UWord32 send_ts,
                                    WebRtc_UWord32 arr_ts)
{
  WebRtc_Word16 index;
  WebRtc_Word16 frame_samples;
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
      (WebRtc_UWord16)WEBRTC_SPL_UDIV(WEBRTC_SPL_UMUL(frame_samples,1000), FS),
      send_ts,
      arr_ts,
      (WebRtc_Word16) packet_size,  
      index);

  
  if (err<0) {
    return err;
  }

  
  return 0;
}
