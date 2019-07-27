









#ifndef WEBRTC_MODULES_VIDEO_CODING_GENERIC_DECODER_H_
#define WEBRTC_MODULES_VIDEO_CODING_GENERIC_DECODER_H_

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/video_coding/codecs/interface/video_codec_interface.h"
#include "webrtc/modules/video_coding/main/source/encoded_frame.h"
#include "webrtc/modules/video_coding/main/source/timestamp_map.h"
#include "webrtc/modules/video_coding/main/source/timing.h"

namespace webrtc
{

class VCMReceiveCallback;

enum { kDecoderFrameMemoryLength = 30 };

struct VCMFrameInformation
{
    int64_t     renderTimeMs;
    int64_t     decodeStartTimeMs;
    void*             userData;
};

class VCMDecodedFrameCallback : public DecodedImageCallback
{
public:
    VCMDecodedFrameCallback(VCMTiming& timing, Clock* clock);
    virtual ~VCMDecodedFrameCallback();
    void SetUserReceiveCallback(VCMReceiveCallback* receiveCallback);
    VCMReceiveCallback* UserReceiveCallback();

    virtual int32_t Decoded(I420VideoFrame& decodedImage);
    virtual int32_t ReceivedDecodedReferenceFrame(const uint64_t pictureId);
    virtual int32_t ReceivedDecodedFrame(const uint64_t pictureId);

    uint64_t LastReceivedPictureID() const;

    int32_t Map(uint32_t timestamp, VCMFrameInformation* frameInfo);
    int32_t Pop(uint32_t timestamp);

private:
    
    CriticalSectionWrapper* _critSect;
    Clock* _clock;
    VCMReceiveCallback* _receiveCallback;  
    VCMTiming& _timing;
    VCMTimestampMap _timestampMap;  
    uint64_t _lastReceivedPictureID;
};


class VCMGenericDecoder
{
    friend class VCMCodecDataBase;
public:
    VCMGenericDecoder(VideoDecoder& decoder, int32_t id = 0, bool isExternal = false);
    ~VCMGenericDecoder();

    


    int32_t InitDecode(const VideoCodec* settings,
                             int32_t numberOfCores);

    




    int32_t Decode(const VCMEncodedFrame& inputFrame, int64_t nowMs);

    


    int32_t Release();

    


    int32_t Reset();

    





    int32_t SetCodecConfigParameters(const uint8_t* ,
                                           int32_t );

    


    int32_t RegisterDecodeCompleteCallback(VCMDecodedFrameCallback* callback);

    bool External() const;

protected:

    int32_t               _id;
    VCMDecodedFrameCallback*    _callback;
    VCMFrameInformation         _frameInfos[kDecoderFrameMemoryLength];
    uint32_t              _nextFrameInfoIdx;
    VideoDecoder&               _decoder;
    VideoCodecType              _codecType;
    bool                        _isExternal;
    bool                        _keyFrameDecoded;

};

}  

#endif 
