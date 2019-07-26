









#ifndef WEBRTC_MODULES_INTERFACE_VIDEO_CODING_DEFINES_H_
#define WEBRTC_MODULES_INTERFACE_VIDEO_CODING_DEFINES_H_

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {


#define VCM_FRAME_NOT_READY      3
#define VCM_REQUEST_SLI          2
#define VCM_MISSING_CALLBACK     1
#define VCM_OK                   0
#define VCM_GENERAL_ERROR       -1
#define VCM_LEVEL_EXCEEDED      -2
#define VCM_MEMORY              -3
#define VCM_PARAMETER_ERROR     -4
#define VCM_UNKNOWN_PAYLOAD     -5
#define VCM_CODEC_ERROR         -6
#define VCM_UNINITIALIZED       -7
#define VCM_NO_CODEC_REGISTERED -8
#define VCM_JITTER_BUFFER_ERROR -9
#define VCM_OLD_PACKET_ERROR    -10
#define VCM_NO_FRAME_DECODED    -11
#define VCM_ERROR_REQUEST_SLI   -12
#define VCM_NOT_IMPLEMENTED     -20

#define VCM_RED_PAYLOAD_TYPE        96
#define VCM_ULPFEC_PAYLOAD_TYPE     97
#define VCM_VP8_PAYLOAD_TYPE       100
#define VCM_I420_PAYLOAD_TYPE      124

enum VCMVideoProtection {
  kProtectionNack,                
  kProtectionNackSender,          
  kProtectionNackReceiver,        
  kProtectionDualDecoder,
  kProtectionFEC,
  kProtectionNackFEC,
  kProtectionKeyOnLoss,
  kProtectionKeyOnKeyLoss,
  kProtectionPeriodicKeyFrames
};

enum VCMTemporalDecimation {
  kBitrateOverUseDecimation,
};

struct VCMFrameCount {
  uint32_t numKeyFrames;
  uint32_t numDeltaFrames;
};


class VCMPacketizationCallback {
 public:
  virtual int32_t SendData(
      FrameType frameType,
      uint8_t payloadType,
      uint32_t timeStamp,
      int64_t capture_time_ms,
      const uint8_t* payloadData,
      uint32_t payloadSize,
      const RTPFragmentationHeader& fragmentationHeader,
      const RTPVideoHeader* rtpVideoHdr) = 0;
 protected:
  virtual ~VCMPacketizationCallback() {
  }
};


class VCMReceiveCallback {
 public:
  virtual int32_t FrameToRender(I420VideoFrame& videoFrame) = 0;
  virtual int32_t ReceivedDecodedReferenceFrame(
      const uint64_t pictureId) {
    return -1;
  }
  
  virtual void IncomingCodecChanged(const VideoCodec& codec) {}

 protected:
  virtual ~VCMReceiveCallback() {
  }
};



class VCMSendStatisticsCallback {
 public:
  virtual int32_t SendStatistics(const uint32_t bitRate,
                                       const uint32_t frameRate) = 0;

 protected:
  virtual ~VCMSendStatisticsCallback() {
  }
};


class VCMReceiveStatisticsCallback {
 public:
  virtual int32_t OnReceiveStatisticsUpdate(const uint32_t bitRate,
                                            const uint32_t frameRate) = 0;

 protected:
  virtual ~VCMReceiveStatisticsCallback() {
  }
};


class VCMDecoderTimingCallback {
 public:
  virtual void OnDecoderTiming(int decode_ms,
                               int max_decode_ms,
                               int current_delay_ms,
                               int target_delay_ms,
                               int jitter_buffer_ms,
                               int min_playout_delay_ms,
                               int render_delay_ms) = 0;

 protected:
  virtual ~VCMDecoderTimingCallback() {}
};



class VCMProtectionCallback {
 public:
  virtual int ProtectionRequest(const FecProtectionParams* delta_params,
                                const FecProtectionParams* key_params,
                                uint32_t* sent_video_rate_bps,
                                uint32_t* sent_nack_rate_bps,
                                uint32_t* sent_fec_rate_bps) = 0;

 protected:
  virtual ~VCMProtectionCallback() {
  }
};



class VCMFrameTypeCallback {
 public:
  virtual int32_t RequestKeyFrame() = 0;
  virtual int32_t SliceLossIndicationRequest(
      const uint64_t pictureId) {
    return -1;
  }

 protected:
  virtual ~VCMFrameTypeCallback() {
  }
};



class VCMPacketRequestCallback {
 public:
  virtual int32_t ResendPackets(const uint16_t* sequenceNumbers,
                                      uint16_t length) = 0;

 protected:
  virtual ~VCMPacketRequestCallback() {
  }
};



class VCMQMSettingsCallback {
 public:
  virtual int32_t SetVideoQMSettings(const uint32_t frameRate,
                                           const uint32_t width,
                                           const uint32_t height) = 0;

 protected:
  virtual ~VCMQMSettingsCallback() {
  }
};



class VCMRenderBufferSizeCallback {
 public:
  virtual void RenderBufferSizeMs(int buffer_size_ms) = 0;

 protected:
  virtual ~VCMRenderBufferSizeCallback() {
  }
};

}  

#endif 
