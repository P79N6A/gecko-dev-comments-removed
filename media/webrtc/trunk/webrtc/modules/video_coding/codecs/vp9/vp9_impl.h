










#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP9_IMPL_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP9_IMPL_H_

#include "webrtc/modules/video_coding/codecs/vp9/include/vp9.h"


typedef struct vpx_codec_ctx vpx_codec_ctx_t;
typedef struct vpx_codec_ctx vpx_dec_ctx_t;
typedef struct vpx_codec_enc_cfg vpx_codec_enc_cfg_t;
typedef struct vpx_image vpx_image_t;
typedef struct vpx_ref_frame vpx_ref_frame_t;
struct vpx_codec_cx_pkt;

namespace webrtc {

class VP9EncoderImpl : public VP9Encoder {
 public:
  VP9EncoderImpl();

  virtual ~VP9EncoderImpl();

  virtual int Release() OVERRIDE;

  virtual int InitEncode(const VideoCodec* codec_settings,
                         int number_of_cores,
                         uint32_t max_payload_size) OVERRIDE;

  virtual int Encode(const I420VideoFrame& input_image,
                     const CodecSpecificInfo* codec_specific_info,
                     const std::vector<VideoFrameType>* frame_types) OVERRIDE;

  virtual int RegisterEncodeCompleteCallback(EncodedImageCallback* callback)
  OVERRIDE;

  virtual int SetChannelParameters(uint32_t packet_loss, int rtt) OVERRIDE;

  virtual int SetRates(uint32_t new_bitrate_kbit, uint32_t frame_rate) OVERRIDE;

 private:
  
  int InitAndSetControlSettings(const VideoCodec* inst);

  void PopulateCodecSpecific(CodecSpecificInfo* codec_specific,
                             const vpx_codec_cx_pkt& pkt,
                             uint32_t timestamp);

  int GetEncodedPartitions(const I420VideoFrame& input_image);

  
  
  
  
  
  
  uint32_t MaxIntraTarget(uint32_t optimal_buffer_size);

  EncodedImage encoded_image_;
  EncodedImageCallback* encoded_complete_callback_;
  VideoCodec codec_;
  bool inited_;
  int64_t timestamp_;
  uint16_t picture_id_;
  int cpu_speed_;
  uint32_t rc_max_intra_target_;
  vpx_codec_ctx_t* encoder_;
  vpx_codec_enc_cfg_t* config_;
  vpx_image_t* raw_;
};


class VP9DecoderImpl : public VP9Decoder {
 public:
  VP9DecoderImpl();

  virtual ~VP9DecoderImpl();

  virtual int InitDecode(const VideoCodec* inst, int number_of_cores) OVERRIDE;

  virtual int Decode(const EncodedImage& input_image,
                     bool missing_frames,
                     const RTPFragmentationHeader* fragmentation,
                     const CodecSpecificInfo* codec_specific_info,
                     int64_t ) OVERRIDE;

  virtual int RegisterDecodeCompleteCallback(DecodedImageCallback* callback)
  OVERRIDE;

  virtual int Release() OVERRIDE;

  virtual int Reset() OVERRIDE;

 private:
  int ReturnFrame(const vpx_image_t* img, uint32_t timeStamp);

  I420VideoFrame decoded_image_;
  DecodedImageCallback* decode_complete_callback_;
  bool inited_;
  vpx_dec_ctx_t* decoder_;
  VideoCodec codec_;
  bool key_frame_required_;
};
}  

#endif  
