









#include "structs.h"
#include "bandwidth_estimator.h"
#include "entropy_coding.h"
#include "codec.h"


int
WebRtcIsac_EstimateBandwidth(
    BwEstimatorstr*           bwest_str,
    Bitstr*                   streamdata,
    WebRtc_Word32               packet_size,
    WebRtc_UWord16              rtp_seq_number,
    WebRtc_UWord32              send_ts,
    WebRtc_UWord32              arr_ts,
    enum IsacSamplingRate encoderSampRate,
    enum IsacSamplingRate decoderSampRate)
{
  WebRtc_Word16  index;
  WebRtc_Word16  frame_samples;
  WebRtc_UWord32 sendTimestampIn16kHz;
  WebRtc_UWord32 arrivalTimestampIn16kHz;
  WebRtc_UWord32 diffSendTime;
  WebRtc_UWord32 diffArrivalTime;
  int err;

  
  err = WebRtcIsac_DecodeFrameLen(streamdata, &frame_samples);
  if(err < 0)  
  {
    return err;
  }
  err = WebRtcIsac_DecodeSendBW(streamdata, &index);
  if(err < 0)  
  {
    return err;
  }

  
  err = WebRtcIsac_UpdateUplinkBwImpl(bwest_str, index, encoderSampRate);
  if(err < 0)
  {
    return err;
  }

  
  
  
  diffSendTime = (WebRtc_UWord32)((WebRtc_UWord32)send_ts -
                                  (WebRtc_UWord32)bwest_str->senderTimestamp);
  bwest_str->senderTimestamp = send_ts;

  diffArrivalTime = (WebRtc_UWord32)((WebRtc_UWord32)arr_ts -
                                     (WebRtc_UWord32)bwest_str->receiverTimestamp);
  bwest_str->receiverTimestamp = arr_ts;

  if(decoderSampRate == kIsacSuperWideband)
  {
    diffArrivalTime = (WebRtc_UWord32)diffArrivalTime >> 1;
    diffSendTime = (WebRtc_UWord32)diffSendTime >> 1;
  }

  
  arrivalTimestampIn16kHz = (WebRtc_UWord32)((WebRtc_UWord32)
                                             bwest_str->prev_rec_arr_ts + (WebRtc_UWord32)diffArrivalTime);
  
  sendTimestampIn16kHz = (WebRtc_UWord32)((WebRtc_UWord32)
                                          bwest_str->prev_rec_send_ts + (WebRtc_UWord32)diffSendTime);

  err = WebRtcIsac_UpdateBandwidthEstimator(bwest_str, rtp_seq_number,
                                            (frame_samples * 1000) / FS, sendTimestampIn16kHz,
                                            arrivalTimestampIn16kHz, packet_size);
  
  if(err < 0)
  {
    return err;
  }

  return 0;
}
