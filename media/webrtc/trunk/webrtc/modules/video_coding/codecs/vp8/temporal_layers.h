










#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_TEMPORAL_LAYERS_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_TEMPORAL_LAYERS_H_

#include "webrtc/common_video/interface/video_image.h"
#include "webrtc/typedefs.h"


typedef struct vpx_codec_enc_cfg vpx_codec_enc_cfg_t;

namespace webrtc {

struct CodecSpecificInfoVP8;

class TemporalLayers {
 public:
  
  
  struct Factory {
    Factory() {}
    virtual ~Factory() {}
    virtual TemporalLayers* Create(int temporal_layers,
                                   uint8_t initial_tl0_pic_idx) const;
  };

  virtual ~TemporalLayers() {}

  
  
  virtual int EncodeFlags(uint32_t timestamp) = 0;

  virtual bool ConfigureBitrates(int bitrate_kbit,
                                 int max_bitrate_kbit,
                                 int framerate,
                                 vpx_codec_enc_cfg_t* cfg) = 0;

  virtual void PopulateCodecSpecific(bool base_layer_sync,
                                     CodecSpecificInfoVP8* vp8_info,
                                     uint32_t timestamp) = 0;

  virtual void FrameEncoded(unsigned int size, uint32_t timestamp) = 0;

  virtual int CurrentLayerId() const = 0;
};




struct RealTimeTemporalLayersFactory : TemporalLayers::Factory {
  virtual ~RealTimeTemporalLayersFactory() {}
  virtual TemporalLayers* Create(int num_temporal_layers,
                                 uint8_t initial_tl0_pic_idx) const;
};

}  
#endif  
