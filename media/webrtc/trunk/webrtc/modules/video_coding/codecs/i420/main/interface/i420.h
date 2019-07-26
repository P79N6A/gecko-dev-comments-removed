









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_H_

#include "video_codec_interface.h"
#include "typedefs.h"

namespace webrtc {

class I420Encoder : public VideoEncoder {
public:

  I420Encoder();

  virtual ~I420Encoder();











  virtual int InitEncode(const VideoCodec* codecSettings,
                         int ,
                         uint32_t );











  virtual int Encode(const I420VideoFrame& inputImage,
                     const CodecSpecificInfo* ,
                     const std::vector<VideoFrameType>* );







  virtual int RegisterEncodeCompleteCallback(EncodedImageCallback* callback);




  virtual int Release();

  virtual int SetRates(uint32_t , uint32_t )
    {return WEBRTC_VIDEO_CODEC_OK;}

  virtual int SetChannelParameters(uint32_t , int )
    {return WEBRTC_VIDEO_CODEC_OK;}

  virtual int CodecConfigParameters(uint8_t* , int )
    {return WEBRTC_VIDEO_CODEC_OK;}

private:
  bool                     _inited;
  EncodedImage             _encodedImage;
  EncodedImageCallback*    _encodedCompleteCallback;

}; 

class I420Decoder : public VideoDecoder {
public:

  I420Decoder();

  virtual ~I420Decoder();






  virtual int InitDecode(const VideoCodec* codecSettings,
                         int );

  virtual int SetCodecConfigParameters(const uint8_t* , int )
    {return WEBRTC_VIDEO_CODEC_OK;};













  virtual int Decode(const EncodedImage& inputImage,
                     bool missingFrames,
                     const RTPFragmentationHeader* ,
                     const CodecSpecificInfo* ,
                     int64_t );







  virtual int RegisterDecodeCompleteCallback(DecodedImageCallback* callback);





  virtual int Release();





  virtual int Reset();

private:

  I420VideoFrame              _decodedImage;
  int                         _width;
  int                         _height;
  bool                        _inited;
  DecodedImageCallback*       _decodeCompleteCallback;

}; 

} 

#endif 
