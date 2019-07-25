









#ifndef WEBRTC_MODULES_VIDEO_CODING_JITTER_BUFFER_H_
#define WEBRTC_MODULES_VIDEO_CODING_JITTER_BUFFER_H_

#include <list>

#include "modules/interface/module_common_types.h"
#include "modules/video_coding/main/interface/video_coding_defines.h"
#include "modules/video_coding/main/source/decoding_state.h"
#include "modules/video_coding/main/source/event.h"
#include "modules/video_coding/main/source/inter_frame_delay.h"
#include "modules/video_coding/main/source/jitter_buffer_common.h"
#include "modules/video_coding/main/source/jitter_estimator.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "typedefs.h"

namespace webrtc
{

enum VCMNackMode
{
    kNackInfinite,
    kNackHybrid,
    kNoNack
};

typedef std::list<VCMFrameBuffer*> FrameList;


class TickTimeBase;
class VCMFrameBuffer;
class VCMPacket;
class VCMEncodedFrame;

class VCMJitterSample
{
public:
    VCMJitterSample() : timestamp(0), frameSize(0), latestPacketTime(-1) {}
    WebRtc_UWord32 timestamp;
    WebRtc_UWord32 frameSize;
    WebRtc_Word64 latestPacketTime;
};

class VCMJitterBuffer
{
public:
    VCMJitterBuffer(TickTimeBase* clock,
                    WebRtc_Word32 vcmId = -1,
                    WebRtc_Word32 receiverId = -1,
                    bool master = true);
    virtual ~VCMJitterBuffer();

    void CopyFrom(const VCMJitterBuffer& rhs);

    
    
    void Start();
    void Stop();
    bool Running() const;

    
    void Flush();

    
    WebRtc_Word32 GetFrameStatistics(WebRtc_UWord32& receivedDeltaFrames,
                                     WebRtc_UWord32& receivedKeyFrames) const;

    
    
    WebRtc_UWord32 NumNotDecodablePackets() const;
    
    WebRtc_UWord32 DiscardedPackets() const;

    
    WebRtc_Word32 GetUpdate(WebRtc_UWord32& frameRate, WebRtc_UWord32& bitRate);

    
    
    WebRtc_Word64 GetNextTimeStamp(WebRtc_UWord32 maxWaitTimeMS,
                                   FrameType& incomingFrameType,
                                   WebRtc_Word64& renderTimeMs);

    
    
    
    
    bool CompleteSequenceWithNextFrame();

    
    
    
    VCMEncodedFrame* GetCompleteFrameForDecoding(WebRtc_UWord32 maxWaitTimeMS);

    
    VCMEncodedFrame* GetFrameForDecoding();

    VCMEncodedFrame* GetFrameForDecodingNACK();

    
    void ReleaseFrame(VCMEncodedFrame* frame);

    
    WebRtc_Word32 GetFrame(const VCMPacket& packet, VCMEncodedFrame*&);
    VCMEncodedFrame* GetFrame(const VCMPacket& packet); 

    
    
    
    WebRtc_Word64 LastPacketTime(VCMEncodedFrame* frame,
                                 bool& retransmitted) const;

    
    VCMFrameBufferEnum InsertPacket(VCMEncodedFrame* frame,
                                    const VCMPacket& packet);

    
    WebRtc_UWord32 GetEstimatedJitterMS();
    void UpdateRtt(WebRtc_UWord32 rttMs);

    
    
    
    
    
    
    
    void SetNackMode(VCMNackMode mode,
                     int lowRttNackThresholdMs,
                     int highRttNackThresholdMs);
    VCMNackMode GetNackMode() const;    
    
    WebRtc_UWord16* GetNackList(WebRtc_UWord16& nackSize,
                                bool& listExtended);

    WebRtc_Word64 LastDecodedTimestamp() const;

private:
    
    
    void RecycleFrame(VCMFrameBuffer* frame);
    void ReleaseFrameInternal(VCMFrameBuffer* frame);
    
    void FlushInternal();

    
    
    VCMFrameBuffer* GetEmptyFrame();
    
    bool RecycleFramesUntilKeyFrame();
    
    
    VCMFrameBufferEnum UpdateFrameState(VCMFrameBuffer* frameListItem);

    
    
    
    FrameList::iterator FindOldestCompleteContinuousFrame(bool enableDecodable);

    void CleanUpOldFrames();

    void VerifyAndSetPreviousFrameLost(VCMFrameBuffer& frame);
    bool IsPacketRetransmitted(const VCMPacket& packet) const;

    void UpdateJitterAndDelayEstimates(VCMJitterSample& sample,
                                       bool incompleteFrame);
    void UpdateJitterAndDelayEstimates(VCMFrameBuffer& frame,
                                       bool incompleteFrame);
    void UpdateJitterAndDelayEstimates(WebRtc_Word64 latestPacketTimeMs,
                                       WebRtc_UWord32 timestamp,
                                       WebRtc_UWord32 frameSize,
                                       bool incompleteFrame);
    void UpdateOldJitterSample(const VCMPacket& packet);
    WebRtc_UWord32 GetEstimatedJitterMsInternal();

    
    WebRtc_UWord16* CreateNackList(WebRtc_UWord16& nackSize,
                                   bool& listExtended);
    WebRtc_Word32 GetLowHighSequenceNumbers(WebRtc_Word32& lowSeqNum,
                                            WebRtc_Word32& highSeqNum) const;

    
    bool WaitForNack();

    WebRtc_Word32                 _vcmId;
    WebRtc_Word32                 _receiverId;
    TickTimeBase*                 _clock;
    
    bool                          _running;
    CriticalSectionWrapper*       _critSect;
    bool                          _master;
    
    VCMEvent                      _frameEvent;
    
    VCMEvent                      _packetEvent;
    
    WebRtc_Word32                 _maxNumberOfFrames;
    
    VCMFrameBuffer*               _frameBuffers[kMaxNumberOfFrames];
    FrameList _frameList;

    
    VCMDecodingState       _lastDecodedState;
    WebRtc_UWord32          _packetsNotDecodable;

    
    
    WebRtc_UWord8           _receiveStatistics[4];
    
    WebRtc_UWord8           _incomingFrameRate;
    
    WebRtc_UWord32          _incomingFrameCount;
    
    WebRtc_Word64           _timeLastIncomingFrameCount;
    
    WebRtc_UWord32          _incomingBitCount;
    WebRtc_UWord32          _incomingBitRate;
    WebRtc_UWord32          _dropCount;            
    
    WebRtc_UWord32          _numConsecutiveOldFrames;
    
    WebRtc_UWord32          _numConsecutiveOldPackets;
    
    WebRtc_UWord32          _discardedPackets;

    
    VCMJitterEstimator      _jitterEstimate;
    
    VCMInterFrameDelay      _delayEstimate;
    VCMJitterSample         _waitingForCompletion;
    WebRtc_UWord32          _rttMs;

    
    VCMNackMode             _nackMode;
    int                     _lowRttNackThresholdMs;
    int                     _highRttNackThresholdMs;
    
    WebRtc_Word32           _NACKSeqNumInternal[kNackHistoryLength];
    WebRtc_UWord16          _NACKSeqNum[kNackHistoryLength];
    WebRtc_UWord32          _NACKSeqNumLength;
    bool                    _waitingForKeyFrame;

    bool                    _firstPacket;

    DISALLOW_COPY_AND_ASSIGN(VCMJitterBuffer);
};

} 

#endif 
