









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_MAIN_INTERFACE_I420_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_MAIN_INTERFACE_I420_H_

#include <vector>

#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/typedefs.h"

namespace webrtc {

enum { kI420HeaderSize = 4 };

class I420Encoder : public VideoEncoder {
 public:
  I420Encoder();

  virtual ~I420Encoder();











  virtual int InitEncode(const VideoCodec* codecSettings,
                         int ,
                         uint32_t ) OVERRIDE;











  virtual int Encode(
      const I420VideoFrame& inputImage,
      const CodecSpecificInfo* ,
      const std::vector<VideoFrameType>* ) OVERRIDE;







  virtual int RegisterEncodeCompleteCallback(
      EncodedImageCallback* callback) OVERRIDE;




  virtual int Release() OVERRIDE;

  virtual int SetRates(uint32_t ,
                       uint32_t ) OVERRIDE {
    return WEBRTC_VIDEO_CODEC_OK;
  }

  virtual int SetChannelParameters(uint32_t ,
                                   int ) OVERRIDE {
    return WEBRTC_VIDEO_CODEC_OK;
  }

  virtual int CodecConfigParameters(uint8_t* ,
                                    int ) OVERRIDE {
    return WEBRTC_VIDEO_CODEC_OK;
  }

 private:
  static uint8_t* InsertHeader(uint8_t* buffer, uint16_t width,
                               uint16_t height);

  bool                     _inited;
  EncodedImage             _encodedImage;
  EncodedImageCallback*    _encodedCompleteCallback;
};  

class I420Decoder : public VideoDecoder {
 public:
  I420Decoder();

  virtual ~I420Decoder();






  virtual int InitDecode(const VideoCodec* codecSettings,
                         int ) OVERRIDE;

  virtual int SetCodecConfigParameters(const uint8_t* ,
                                       int ) OVERRIDE {
    return WEBRTC_VIDEO_CODEC_OK;
  }













  virtual int Decode(const EncodedImage& inputImage,
                     bool missingFrames,
                     const RTPFragmentationHeader* ,
                     const CodecSpecificInfo* ,
                     int64_t ) OVERRIDE;







  virtual int RegisterDecodeCompleteCallback(
      DecodedImageCallback* callback) OVERRIDE;





  virtual int Release() OVERRIDE;





  virtual int Reset() OVERRIDE;

 private:
  static const uint8_t* ExtractHeader(const uint8_t* buffer,
                                      uint16_t* width,
                                      uint16_t* height);

  I420VideoFrame              _decodedImage;
  int                         _width;
  int                         _height;
  bool                        _inited;
  DecodedImageCallback*       _decodeCompleteCallback;
};  

}  

#endif  
