









#ifndef WEBRTC_MODULES_VIDEO_CODING_GENERIC_DECODER_H_
#define WEBRTC_MODULES_VIDEO_CODING_GENERIC_DECODER_H_

#include "timing.h"
#include "timestamp_map.h"
#include "video_codec_interface.h"
#include "encoded_frame.h"
#include "module_common_types.h"

namespace webrtc
{

class VCMReceiveCallback;

enum { kDecoderFrameMemoryLength = 10 };

struct VCMFrameInformation
{
    WebRtc_Word64     renderTimeMs;
    WebRtc_Word64     decodeStartTimeMs;
    void*             userData;
};

class VCMDecodedFrameCallback : public DecodedImageCallback
{
public:
    VCMDecodedFrameCallback(VCMTiming& timing, TickTimeBase* clock);
    virtual ~VCMDecodedFrameCallback();
    void SetUserReceiveCallback(VCMReceiveCallback* receiveCallback);

    virtual WebRtc_Word32 Decoded(I420VideoFrame& decodedImage);
    virtual WebRtc_Word32 ReceivedDecodedReferenceFrame(const WebRtc_UWord64 pictureId);
    virtual WebRtc_Word32 ReceivedDecodedFrame(const WebRtc_UWord64 pictureId);

    WebRtc_UWord64 LastReceivedPictureID() const;

    WebRtc_Word32 Map(WebRtc_UWord32 timestamp, VCMFrameInformation* frameInfo);
    WebRtc_Word32 Pop(WebRtc_UWord32 timestamp);

private:
    CriticalSectionWrapper* _critSect;
    TickTimeBase* _clock;
    I420VideoFrame _frame;
    VCMReceiveCallback* _receiveCallback;
    VCMTiming& _timing;
    VCMTimestampMap _timestampMap;
    WebRtc_UWord64 _lastReceivedPictureID;
};


class VCMGenericDecoder
{
    friend class VCMCodecDataBase;
public:
    VCMGenericDecoder(VideoDecoder& decoder, WebRtc_Word32 id = 0, bool isExternal = false);
    ~VCMGenericDecoder();

    


    WebRtc_Word32 InitDecode(const VideoCodec* settings,
                             WebRtc_Word32 numberOfCores,
                             bool requireKeyFrame);

    




    WebRtc_Word32 Decode(const VCMEncodedFrame& inputFrame, int64_t nowMs);

    


    WebRtc_Word32 Release();

    


    WebRtc_Word32 Reset();

    





    WebRtc_Word32 SetCodecConfigParameters(const WebRtc_UWord8* ,
                                           WebRtc_Word32 );

    WebRtc_Word32 RegisterDecodeCompleteCallback(VCMDecodedFrameCallback* callback);

    bool External() const;

protected:

    WebRtc_Word32               _id;
    VCMDecodedFrameCallback*    _callback;
    VCMFrameInformation         _frameInfos[kDecoderFrameMemoryLength];
    WebRtc_UWord32              _nextFrameInfoIdx;
    VideoDecoder&               _decoder;
    VideoCodecType              _codecType;
    bool                        _isExternal;
    bool                        _requireKeyFrame;
    bool                        _keyFrameDecoded;

};

} 

#endif 
