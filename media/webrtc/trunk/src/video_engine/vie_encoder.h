









#ifndef WEBRTC_VIDEO_ENGINE_VIE_ENCODER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_ENCODER_H_

#include "common_types.h"
#include "rtp_rtcp_defines.h"
#include "typedefs.h"
#include "video_coding_defines.h"
#include "video_processing.h"
#include "vie_defines.h"
#include "vie_file_recorder.h"
#include "vie_frame_provider_base.h"
#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class CriticalSectionWrapper;
class ProcessThread;
class QMVideoSettingsCallback;
class RtpRtcp;
class VideoCodingModule;
class ViEEffectFilter;
class ViEEncoderObserver;

class ViEEncoder
    : public RtpVideoFeedback,
      public RtcpFeedback,
      public VCMPacketizationCallback,
      public VCMProtectionCallback,
      public VCMSendStatisticsCallback,
      public ViEFrameCallback {
 public:
  ViEEncoder(WebRtc_Word32 engine_id,
             WebRtc_Word32 channel_id,
             WebRtc_UWord32 number_of_cores,
             ProcessThread& module_process_thread);
  ~ViEEncoder();

  bool Init();

  
  int Owner() const;

  
  void Pause();
  void Restart();

  WebRtc_Word32 DropDeltaAfterKey(bool enable);

  
  WebRtc_UWord8 NumberOfCodecs();
  WebRtc_Word32 GetCodec(WebRtc_UWord8 list_index, VideoCodec& video_codec);
  WebRtc_Word32 RegisterExternalEncoder(VideoEncoder* encoder,
                                        WebRtc_UWord8 pl_type);
  WebRtc_Word32 DeRegisterExternalEncoder(WebRtc_UWord8 pl_type);
  WebRtc_Word32 SetEncoder(const VideoCodec& video_codec);
  WebRtc_Word32 GetEncoder(VideoCodec& video_codec);

  WebRtc_Word32 GetCodecConfigParameters(
    unsigned char config_parameters[kConfigParameterSize],
    unsigned char& config_parameters_size);

  
  WebRtc_Word32 ScaleInputImage(bool enable);

  
  RtpRtcp* SendRtpRtcpModule();

  
  virtual void DeliverFrame(int id,
                            VideoFrame& video_frame,
                            int num_csrcs = 0,
                            const WebRtc_UWord32 CSRC[kRtpCsrcSize] = NULL);
  virtual void DelayChanged(int id, int frame_delay);
  virtual int GetPreferedFrameSettings(int& width,
                                       int& height,
                                       int& frame_rate);

  virtual void ProviderDestroyed(int id) {
    return;
  }

  WebRtc_Word32 EncodeFrame(VideoFrame& video_frame);
  WebRtc_Word32 SendKeyFrame();
  WebRtc_Word32 SendCodecStatistics(WebRtc_UWord32& num_key_frames,
                                    WebRtc_UWord32& num_delta_frames);
  WebRtc_Word32 EstimatedSendBandwidth(
      WebRtc_UWord32* available_bandwidth) const;
  int CodecTargetBitrate(WebRtc_UWord32* bitrate) const;
  
  WebRtc_Word32 UpdateProtectionMethod();

  
  virtual WebRtc_Word32 SendData(
    const FrameType frame_type,
    const WebRtc_UWord8 payload_type,
    const WebRtc_UWord32 time_stamp,
    const WebRtc_UWord8* payload_data,
    const WebRtc_UWord32 payload_size,
    const RTPFragmentationHeader& fragmentation_header,
    const RTPVideoHeader* rtp_video_hdr);

  
  virtual int ProtectionRequest(
      const FecProtectionParams* delta_fec_params,
      const FecProtectionParams* key_fec_params,
      WebRtc_UWord32* sent_video_rate_bps,
      WebRtc_UWord32* sent_nack_rate_bps,
      WebRtc_UWord32* sent_fec_rate_bps);

  
  virtual WebRtc_Word32 SendStatistics(const WebRtc_UWord32 bit_rate,
                                       const WebRtc_UWord32 frame_rate);
  WebRtc_Word32 RegisterCodecObserver(ViEEncoderObserver* observer);

  
  virtual void OnSLIReceived(const WebRtc_Word32 id,
                             const WebRtc_UWord8 picture_id);
  virtual void OnRPSIReceived(const WebRtc_Word32 id,
                              const WebRtc_UWord64 picture_id);

  
  virtual void OnReceivedIntraFrameRequest(const WebRtc_Word32 id,
                                           const FrameType type,
                                           const WebRtc_UWord8 stream_idx);

  virtual void OnNetworkChanged(const WebRtc_Word32 id,
                                const WebRtc_UWord32 bitrate_bps,
                                const WebRtc_UWord8 fraction_lost,
                                const WebRtc_UWord16 round_trip_time_ms);

  
  WebRtc_Word32 RegisterEffectFilter(ViEEffectFilter* effect_filter);

  
  ViEFileRecorder& GetOutgoingFileRecorder();

 private:
  WebRtc_Word32 engine_id_;
  const int channel_id_;
  const WebRtc_UWord32 number_of_cores_;

  VideoCodingModule& vcm_;
  VideoProcessingModule& vpm_;
  RtpRtcp& default_rtp_rtcp_;
  scoped_ptr<CriticalSectionWrapper> callback_cs_;
  scoped_ptr<CriticalSectionWrapper> data_cs_;
  VideoCodec send_codec_;

  bool paused_;
  WebRtc_Word64 time_last_intra_request_ms_;
  WebRtc_Word32 channels_dropping_delta_frames_;
  bool drop_next_frame_;

  bool fec_enabled_;
  bool nack_enabled_;

  ViEEncoderObserver* codec_observer_;
  ViEEffectFilter* effect_filter_;
  ProcessThread& module_process_thread_;

  bool has_received_sli_;
  WebRtc_UWord8 picture_id_sli_;
  bool has_received_rpsi_;
  WebRtc_UWord64 picture_id_rpsi_;

  ViEFileRecorder file_recorder_;

  
  QMVideoSettingsCallback* qm_callback_;
};

}  

#endif  
