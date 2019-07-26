










#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_TEMPORAL_LAYERS_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_TEMPORAL_LAYERS_H_

#include <typedefs.h>

 
typedef struct vpx_codec_enc_cfg vpx_codec_enc_cfg_t;

namespace webrtc {

struct CodecSpecificInfoVP8;

class TemporalLayers {
 public:
  TemporalLayers(int number_of_temporal_layers);

  
  
  int EncodeFlags();

  bool ConfigureBitrates(int bitrate_kbit, vpx_codec_enc_cfg_t* cfg);

  void PopulateCodecSpecific(bool base_layer_sync,
                             CodecSpecificInfoVP8* vp8_info,
                             uint32_t timestamp);

 private:
  enum TemporalReferences {
    
    
    kTemporalUpdateLastRefAll = 12,
    
    
    kTemporalUpdateLastAndGoldenRefAltRef = 11,
    
    kTemporalUpdateGoldenRefAltRef = 10,
    
    kTemporalUpdateGoldenWithoutDependencyRefAltRef = 9,
    
    kTemporalUpdateLastRefAltRef = 8,
    
    
    kTemporalUpdateNoneNoRefGoldenRefAltRef = 7,
    
    kTemporalUpdateNoneNoRefAltref = 6,
    
    kTemporalUpdateNone = 5,
    
    kTemporalUpdateAltref = 4,
    
    
    kTemporalUpdateAltrefWithoutDependency = 3,
    
    kTemporalUpdateGolden = 2,
    
    
    kTemporalUpdateGoldenWithoutDependency = 1,
    
    kTemporalUpdateLast = 0,
  };
  enum { kMaxTemporalPattern = 16 };

  int number_of_temporal_layers_;
  int temporal_ids_length_;
  int temporal_ids_[kMaxTemporalPattern];
  int temporal_pattern_length_;
  TemporalReferences temporal_pattern_[kMaxTemporalPattern];
  uint8_t tl0_pic_idx_;
  uint8_t pattern_idx_;
  uint32_t timestamp_;
  bool last_base_layer_sync_;
};
}  
#endif

