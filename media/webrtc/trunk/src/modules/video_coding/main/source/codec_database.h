









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODEC_DATABASE_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODEC_DATABASE_H_

#include <map>

#include "modules/video_coding/codecs/interface/video_codec_interface.h"
#include "modules/video_coding/main/interface/video_coding.h"
#include "modules/video_coding/main/source/generic_decoder.h"
#include "modules/video_coding/main/source/generic_encoder.h"
#include "typedefs.h"

namespace webrtc
{

enum VCMCodecDBProperties
{
    kDefaultPayloadSize = 1440
};

class VCMDecoderMapItem {
public:
    VCMDecoderMapItem(VideoCodec* settings,
                      WebRtc_UWord32 numberOfCores,
                      bool requireKeyFrame);

    VideoCodec*     _settings;
    WebRtc_UWord32  _numberOfCores;
    bool            _requireKeyFrame;
};

class VCMExtDecoderMapItem {
public:
    VCMExtDecoderMapItem(VideoDecoder* externalDecoderInstance,
                         WebRtc_UWord8 payloadType,
                         bool internalRenderTiming);

    WebRtc_UWord8   _payloadType;
    VideoDecoder*   _externalDecoderInstance;
    bool            _internalRenderTiming;
};




class VCMCodecDataBase
{
public:
    VCMCodecDataBase(WebRtc_Word32 id);
    ~VCMCodecDataBase();
    


    WebRtc_Word32 Reset();
    


    


    static WebRtc_UWord8 NumberOfCodecs();
    






    static WebRtc_Word32 Codec(WebRtc_UWord8 listId, VideoCodec* settings);
    static WebRtc_Word32 Codec(VideoCodecType codecType, VideoCodec* settings);
    


    WebRtc_Word32 ResetSender();
    




    WebRtc_Word32 RegisterSendCodec(const VideoCodec* sendCodec,
                                  WebRtc_UWord32 numberOfCores,
                                  WebRtc_UWord32 maxPayloadSize);
    


    WebRtc_Word32 SendCodec(VideoCodec* currentSendCodec) const;
    


    VideoCodecType SendCodec() const;
    




    WebRtc_Word32 DeRegisterExternalEncoder(WebRtc_UWord8 payloadType, bool& wasSendCodec);
    WebRtc_Word32 RegisterExternalEncoder(VideoEncoder* externalEncoder,
                                        WebRtc_UWord8 payloadType,
                                        bool internalSource);
    






    VCMGenericEncoder* SetEncoder(const VideoCodec* settings,
                                  VCMEncodedFrameCallback* VCMencodedFrameCallback);

    WebRtc_Word32 SetPeriodicKeyFrames(bool enable);

    bool InternalSource() const;

    


    WebRtc_Word32 ResetReceiver();
    


    WebRtc_Word32 DeRegisterExternalDecoder(WebRtc_UWord8 payloadType);
    WebRtc_Word32 RegisterExternalDecoder(VideoDecoder* externalDecoder,
                                        WebRtc_UWord8 payloadType,
                                        bool internalRenderTiming);

    bool DecoderRegistered() const;
    


    WebRtc_Word32 RegisterReceiveCodec(const VideoCodec* receiveCodec,
                                     WebRtc_UWord32 numberOfCores,
                                     bool requireKeyFrame);
    WebRtc_Word32 DeRegisterReceiveCodec(WebRtc_UWord8 payloadType);
    


    WebRtc_Word32 ReceiveCodec(VideoCodec* currentReceiveCodec) const;
    


    VideoCodecType ReceiveCodec() const;
    






    VCMGenericDecoder* SetDecoder(WebRtc_UWord8 payloadType, VCMDecodedFrameCallback& callback);

    VCMGenericDecoder* CreateAndInitDecoder(WebRtc_UWord8 payloadType,
                                            VideoCodec& newCodec,
                                            bool &external) const;

    VCMGenericDecoder* CreateDecoderCopy() const;

    void ReleaseDecoder(VCMGenericDecoder* decoder) const;

    void CopyDecoder(const VCMGenericDecoder& decoder);

    bool RenderTiming() const;

protected:
    


    VCMGenericEncoder* CreateEncoder(const VideoCodecType type) const;

    void DeleteEncoder();
    


    VCMGenericDecoder* CreateDecoder(VideoCodecType type) const;

    VCMDecoderMapItem* FindDecoderItem(WebRtc_UWord8 payloadType) const;

    VCMExtDecoderMapItem* FindExternalDecoderItem(WebRtc_UWord8 payloadType) const;

private:
    typedef std::map<uint8_t, VCMDecoderMapItem*> DecoderMap;
    typedef std::map<uint8_t, VCMExtDecoderMapItem*> ExternalDecoderMap;
    WebRtc_Word32 _id;
    WebRtc_UWord32 _numberOfCores;
    WebRtc_UWord32 _maxPayloadSize;
    bool _periodicKeyFrames;
    bool _currentEncIsExternal;
    VideoCodec _sendCodec;
    VideoCodec _receiveCodec;
    WebRtc_UWord8 _externalPayloadType;
    VideoEncoder* _externalEncoder;
    bool _internalSource;
    VCMGenericEncoder* _ptrEncoder;
    VCMGenericDecoder* _ptrDecoder;
    bool _currentDecIsExternal;
    DecoderMap _decMap;
    ExternalDecoderMap _decExternalMap;
}; 

} 

#endif 
