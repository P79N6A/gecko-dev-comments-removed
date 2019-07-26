









#ifndef WEBRTC_MODULES_VIDEO_CODING_FRAME_BUFFER_H_
#define WEBRTC_MODULES_VIDEO_CODING_FRAME_BUFFER_H_

#include "modules/interface/module_common_types.h"
#include "modules/video_coding/main/source/encoded_frame.h"
#include "modules/video_coding/main/source/jitter_buffer_common.h"
#include "modules/video_coding/main/source/session_info.h"
#include "typedefs.h"

namespace webrtc
{

class VCMFrameBuffer : public VCMEncodedFrame
{
public:
    VCMFrameBuffer();
    virtual ~VCMFrameBuffer();

    VCMFrameBuffer(VCMFrameBuffer& rhs);

    virtual void Reset();

    VCMFrameBufferEnum InsertPacket(const VCMPacket& packet,
                                    WebRtc_Word64 timeInMs,
                                    bool enableDecodableState,
                                    WebRtc_UWord32 rttMs);

    
    
    VCMFrameBufferStateEnum GetState() const;
    
    VCMFrameBufferStateEnum GetState(WebRtc_UWord32& timeStamp) const;
    void SetState(VCMFrameBufferStateEnum state); 

    bool IsRetransmitted() const;
    bool IsSessionComplete() const;
    bool HaveLastPacket() const;
    
    void MakeSessionDecodable();

    
    
    WebRtc_Word32 GetLowSeqNum() const;
    
    WebRtc_Word32 GetHighSeqNum() const;

    int PictureId() const;
    int TemporalId() const;
    bool LayerSync() const;
    int Tl0PicId() const;
    bool NonReference() const;

    
    void SetCountedFrame(bool frameCounted);
    bool GetCountedFrame() const;

    
    
    
    int BuildHardNackList(int* list, int num);
    
    
    int BuildSoftNackList(int* list, int num, int rttMs);
    void IncrementNackCount();
    WebRtc_Word16 GetNackCount() const;

    WebRtc_Word64 LatestPacketTimeMs() const;

    webrtc::FrameType FrameType() const;
    void SetPreviousFrameLoss();

    WebRtc_Word32 ExtractFromStorage(const EncodedVideoData& frameFromStorage);

    
    
    int NotDecodablePackets() const;

protected:
    void RestructureFrameInformation();
    void PrepareForDecode();

private:
    VCMFrameBufferStateEnum    _state;         
    bool                       _frameCounted;  
    VCMSessionInfo             _sessionInfo;
    WebRtc_UWord16             _nackCount;
    WebRtc_Word64              _latestPacketTimeMs;
};

} 

#endif 
