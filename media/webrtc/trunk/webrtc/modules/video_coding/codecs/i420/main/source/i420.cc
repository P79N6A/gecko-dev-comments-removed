









#include "modules/video_coding/codecs/i420/main/interface/i420.h"

#include <string.h>

#include "common_video/libyuv/include/webrtc_libyuv.h"

namespace webrtc
{

I420Encoder::I420Encoder():
_inited(false),
_encodedImage(),
_encodedCompleteCallback(NULL)
{}

I420Encoder::~I420Encoder() {
  _inited = false;
  if (_encodedImage._buffer != NULL) {
    delete [] _encodedImage._buffer;
    _encodedImage._buffer = NULL;
  }
}

int I420Encoder::Release() {
  
  
  if (_encodedImage._buffer != NULL) {
    delete [] _encodedImage._buffer;
    _encodedImage._buffer = NULL;
  }
  _inited = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

int I420Encoder::InitEncode(const VideoCodec* codecSettings,
                            int ,
                            uint32_t ) {
  if (codecSettings == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (codecSettings->width < 1 || codecSettings->height < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  
  if (_encodedImage._buffer != NULL) {
    delete [] _encodedImage._buffer;
    _encodedImage._buffer = NULL;
    _encodedImage._size = 0;
  }
  const uint32_t newSize = CalcBufferSize(kI420,
                                          codecSettings->width,
                                          codecSettings->height);
  uint8_t* newBuffer = new uint8_t[newSize];
  if (newBuffer == NULL) {
    return WEBRTC_VIDEO_CODEC_MEMORY;
  }
  _encodedImage._size = newSize;
  _encodedImage._buffer = newBuffer;

  
  _inited = true;
  return WEBRTC_VIDEO_CODEC_OK;
}



int I420Encoder::Encode(const I420VideoFrame& inputImage,
                        const CodecSpecificInfo* ,
                        const std::vector<VideoFrameType>* ) {
  if (!_inited) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (_encodedCompleteCallback == NULL) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  _encodedImage._frameType = kKeyFrame; 
  _encodedImage._timeStamp = inputImage.timestamp();
  _encodedImage._encodedHeight = inputImage.height();
  _encodedImage._encodedWidth = inputImage.width();

  int req_length = CalcBufferSize(kI420, inputImage.width(),
                                  inputImage.height());
  if (_encodedImage._size > static_cast<unsigned int>(req_length)) {
    
    if (_encodedImage._buffer != NULL) {
      delete [] _encodedImage._buffer;
      _encodedImage._buffer = NULL;
      _encodedImage._size = 0;
    }
    uint8_t* newBuffer = new uint8_t[req_length];
    if (newBuffer == NULL) {
      return WEBRTC_VIDEO_CODEC_MEMORY;
    }
    _encodedImage._size = req_length;
    _encodedImage._buffer = newBuffer;
  }

  int ret_length = ExtractBuffer(inputImage, req_length, _encodedImage._buffer);
  if (ret_length < 0)
    return WEBRTC_VIDEO_CODEC_MEMORY;
  _encodedImage._length = ret_length;

  _encodedCompleteCallback->Encoded(_encodedImage);
  return WEBRTC_VIDEO_CODEC_OK;
}


int
I420Encoder::RegisterEncodeCompleteCallback(EncodedImageCallback* callback) {
  _encodedCompleteCallback = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}


I420Decoder::I420Decoder():
_decodedImage(),
_width(0),
_height(0),
_inited(false),
_decodeCompleteCallback(NULL)
{}

I420Decoder::~I420Decoder() {
  Release();
}

int
I420Decoder::Reset() {
  return WEBRTC_VIDEO_CODEC_OK;
}


int
I420Decoder::InitDecode(const VideoCodec* codecSettings,
                        int ) {
  if (codecSettings == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  } else if (codecSettings->width < 1 || codecSettings->height < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  _width = codecSettings->width;
  _height = codecSettings->height;
  _inited = true;
  return WEBRTC_VIDEO_CODEC_OK;
}

int
I420Decoder::Decode(const EncodedImage& inputImage,
                    bool ,
                    const RTPFragmentationHeader* ,
                    const CodecSpecificInfo* ,
                    int64_t ) {
  if (inputImage._buffer == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (_decodeCompleteCallback == NULL) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (inputImage._length <= 0) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inputImage._completeFrame == false) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (!_inited) {
   return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  
  int req_length = CalcBufferSize(kI420, _width, _height);
  if (req_length > static_cast<int>(inputImage._length)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  
  int half_width = (_width + 1) / 2;
  int half_height = (_height + 1) / 2;
  int size_y = _width * _height;
  int size_uv = half_width * half_height;

  const uint8_t* buffer_y = inputImage._buffer;
  const uint8_t* buffer_u = buffer_y + size_y;
  const uint8_t* buffer_v = buffer_u + size_uv;
  
  int ret = _decodedImage.CreateFrame(size_y, buffer_y,
                                      size_uv, buffer_u,
                                      size_uv, buffer_v,
                                      _width, _height,
                                      _width, half_width, half_width);
  if (ret < 0) {
    return WEBRTC_VIDEO_CODEC_MEMORY;
  }
  _decodedImage.set_timestamp(inputImage._timeStamp);

  _decodeCompleteCallback->Decoded(_decodedImage);
  return WEBRTC_VIDEO_CODEC_OK;
}

int
I420Decoder::RegisterDecodeCompleteCallback(DecodedImageCallback* callback) {
  _decodeCompleteCallback = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int
I420Decoder::Release() {
  _inited = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

}
