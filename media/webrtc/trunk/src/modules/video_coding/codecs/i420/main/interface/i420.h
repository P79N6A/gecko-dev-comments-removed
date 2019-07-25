









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_I420_H_

#include "video_codec_interface.h"
#include "typedefs.h"

namespace webrtc
{

class I420Encoder : public VideoEncoder
{
public:

    I420Encoder();

    virtual ~I420Encoder();











    virtual WebRtc_Word32 InitEncode(const VideoCodec* codecSettings, WebRtc_Word32 , WebRtc_UWord32 );











    virtual WebRtc_Word32
        Encode(const RawImage& inputImage,
               const CodecSpecificInfo* ,
               const VideoFrameType );







    virtual WebRtc_Word32 RegisterEncodeCompleteCallback(EncodedImageCallback* callback);




    virtual WebRtc_Word32 Release();

    virtual WebRtc_Word32 SetRates(WebRtc_UWord32 ,
                                   WebRtc_UWord32 )
    {return WEBRTC_VIDEO_CODEC_OK;}

    virtual WebRtc_Word32 SetChannelParameters(WebRtc_UWord32 ,
                                               int )
    {return WEBRTC_VIDEO_CODEC_OK;}

    virtual WebRtc_Word32 CodecConfigParameters(WebRtc_UWord8* ,
                                                WebRtc_Word32 )
    {return WEBRTC_VIDEO_CODEC_OK;}

private:
    bool                     _inited;
    EncodedImage             _encodedImage;
    EncodedImageCallback*    _encodedCompleteCallback;

}; 

class I420Decoder : public VideoDecoder
{
public:

    I420Decoder();

    virtual ~I420Decoder();






    virtual WebRtc_Word32 InitDecode(const VideoCodec* codecSettings, WebRtc_Word32 );

    virtual WebRtc_Word32 SetCodecConfigParameters(const WebRtc_UWord8* , WebRtc_Word32 ){return WEBRTC_VIDEO_CODEC_OK;};













    virtual WebRtc_Word32 Decode(
        const EncodedImage& inputImage,
        bool missingFrames,
        const RTPFragmentationHeader* ,
        const CodecSpecificInfo* ,
        WebRtc_Word64 );







    virtual WebRtc_Word32 RegisterDecodeCompleteCallback(DecodedImageCallback* callback);





    virtual WebRtc_Word32 Release();





    virtual WebRtc_Word32 Reset();

private:

    RawImage                    _decodedImage;
    WebRtc_Word32               _width;
    WebRtc_Word32               _height;
    bool                        _inited;
    DecodedImageCallback*       _decodeCompleteCallback;


}; 

} 

#endif 
