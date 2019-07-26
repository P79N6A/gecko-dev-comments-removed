








#include "modules/video_coding/main/source/jitter_buffer.h"

#include <algorithm>
#include <cassert>

#include "modules/video_coding/main/source/event.h"
#include "modules/video_coding/main/source/frame_buffer.h"
#include "modules/video_coding/main/source/inter_frame_delay.h"
#include "modules/video_coding/main/source/internal_defines.h"
#include "modules/video_coding/main/source/jitter_buffer_common.h"
#include "modules/video_coding/main/source/jitter_estimator.h"
#include "modules/video_coding/main/source/packet.h"
#include "modules/video_coding/main/source/tick_time_base.h"
#include "system_wrappers/interface/critical_section_wrapper.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {


class FrameSmallerTimestamp {
 public:
  FrameSmallerTimestamp(uint32_t timestamp) : timestamp_(timestamp) {}
  bool operator()(VCMFrameBuffer* frame) {
    return (LatestTimestamp(timestamp_, frame->TimeStamp(), NULL) ==
        timestamp_);
  }

 private:
  uint32_t timestamp_;
};

class FrameEqualTimestamp {
 public:
  FrameEqualTimestamp(uint32_t timestamp) : timestamp_(timestamp) {}
  bool operator()(VCMFrameBuffer* frame) {
    return (timestamp_ == frame->TimeStamp());
  }

 private:
  uint32_t timestamp_;
};

class CompleteDecodableKeyFrameCriteria {
 public:
  bool operator()(VCMFrameBuffer* frame) {
    return (frame->FrameType() == kVideoFrameKey) &&
           (frame->GetState() == kStateComplete ||
            frame->GetState() == kStateDecodable);
  }
};


VCMJitterBuffer::VCMJitterBuffer(TickTimeBase* clock,
                                 WebRtc_Word32 vcmId,
                                 WebRtc_Word32 receiverId,
                                 bool master) :
    _vcmId(vcmId),
    _receiverId(receiverId),
    _clock(clock),
    _running(false),
    _critSect(CriticalSectionWrapper::CreateCriticalSection()),
    _master(master),
    _frameEvent(),
    _packetEvent(),
    _maxNumberOfFrames(kStartNumberOfFrames),
    _frameBuffers(),
    _frameList(),
    _lastDecodedState(),
    _packetsNotDecodable(0),
    _receiveStatistics(),
    _incomingFrameRate(0),
    _incomingFrameCount(0),
    _timeLastIncomingFrameCount(0),
    _incomingBitCount(0),
    _incomingBitRate(0),
    _dropCount(0),
    _numConsecutiveOldFrames(0),
    _numConsecutiveOldPackets(0),
    _discardedPackets(0),
    _jitterEstimate(vcmId, receiverId),
    _delayEstimate(_clock->MillisecondTimestamp()),
    _rttMs(0),
    _nackMode(kNoNack),
    _lowRttNackThresholdMs(-1),
    _highRttNackThresholdMs(-1),
    _NACKSeqNum(),
    _NACKSeqNumLength(0),
    _waitingForKeyFrame(false),
    _firstPacket(true)
{
    memset(_frameBuffers, 0, sizeof(_frameBuffers));
    memset(_receiveStatistics, 0, sizeof(_receiveStatistics));
    memset(_NACKSeqNumInternal, -1, sizeof(_NACKSeqNumInternal));

    for (int i = 0; i< kStartNumberOfFrames; i++)
    {
        _frameBuffers[i] = new VCMFrameBuffer();
    }
}


VCMJitterBuffer::~VCMJitterBuffer()
{
    Stop();
    for (int i = 0; i< kMaxNumberOfFrames; i++)
    {
        if (_frameBuffers[i])
        {
            delete _frameBuffers[i];
        }
    }
    delete _critSect;
}

void
VCMJitterBuffer::CopyFrom(const VCMJitterBuffer& rhs)
{
    if (this != &rhs)
    {
        _critSect->Enter();
        rhs._critSect->Enter();
        _vcmId = rhs._vcmId;
        _receiverId = rhs._receiverId;
        _running = rhs._running;
        _master = !rhs._master;
        _maxNumberOfFrames = rhs._maxNumberOfFrames;
        _incomingFrameRate = rhs._incomingFrameRate;
        _incomingFrameCount = rhs._incomingFrameCount;
        _timeLastIncomingFrameCount = rhs._timeLastIncomingFrameCount;
        _incomingBitCount = rhs._incomingBitCount;
        _incomingBitRate = rhs._incomingBitRate;
        _dropCount = rhs._dropCount;
        _numConsecutiveOldFrames = rhs._numConsecutiveOldFrames;
        _numConsecutiveOldPackets = rhs._numConsecutiveOldPackets;
        _discardedPackets = rhs._discardedPackets;
        _jitterEstimate = rhs._jitterEstimate;
        _delayEstimate = rhs._delayEstimate;
        _waitingForCompletion = rhs._waitingForCompletion;
        _rttMs = rhs._rttMs;
        _NACKSeqNumLength = rhs._NACKSeqNumLength;
        _waitingForKeyFrame = rhs._waitingForKeyFrame;
        _firstPacket = rhs._firstPacket;
        _lastDecodedState =  rhs._lastDecodedState;
        _packetsNotDecodable = rhs._packetsNotDecodable;
        memcpy(_receiveStatistics, rhs._receiveStatistics,
               sizeof(_receiveStatistics));
        memcpy(_NACKSeqNumInternal, rhs._NACKSeqNumInternal,
               sizeof(_NACKSeqNumInternal));
        memcpy(_NACKSeqNum, rhs._NACKSeqNum, sizeof(_NACKSeqNum));
        for (int i = 0; i < kMaxNumberOfFrames; i++)
        {
            if (_frameBuffers[i] != NULL)
            {
                delete _frameBuffers[i];
                _frameBuffers[i] = NULL;
            }
        }
        _frameList.clear();
        for (int i = 0; i < _maxNumberOfFrames; i++)
        {
            _frameBuffers[i] = new VCMFrameBuffer(*(rhs._frameBuffers[i]));
            if (_frameBuffers[i]->Length() > 0)
            {
                FrameList::reverse_iterator rit = std::find_if(
                    _frameList.rbegin(), _frameList.rend(),
                    FrameSmallerTimestamp(_frameBuffers[i]->TimeStamp()));
                _frameList.insert(rit.base(), _frameBuffers[i]);
            }
        }
        rhs._critSect->Leave();
        _critSect->Leave();
    }
}


void
VCMJitterBuffer::Start()
{
    CriticalSectionScoped cs(_critSect);
    _running = true;
    _incomingFrameCount = 0;
    _incomingFrameRate = 0;
    _incomingBitCount = 0;
    _incomingBitRate = 0;
    _timeLastIncomingFrameCount = _clock->MillisecondTimestamp();
    memset(_receiveStatistics, 0, sizeof(_receiveStatistics));

    _numConsecutiveOldFrames = 0;
    _numConsecutiveOldPackets = 0;
    _discardedPackets = 0;

    _frameEvent.Reset(); 
    _packetEvent.Reset(); 
    _waitingForCompletion.frameSize = 0;
    _waitingForCompletion.timestamp = 0;
    _waitingForCompletion.latestPacketTime = -1;
    _firstPacket = true;
    _NACKSeqNumLength = 0;
    _waitingForKeyFrame = false;
    _rttMs = 0;
    _packetsNotDecodable = 0;

    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId,
                 _receiverId), "JB(0x%x): Jitter buffer: start", this);
}



void
VCMJitterBuffer::Stop()
{
    _critSect->Enter();
    _running = false;
    _lastDecodedState.Reset();
    _frameList.clear();
    for (int i = 0; i < kMaxNumberOfFrames; i++)
    {
        if (_frameBuffers[i] != NULL)
        {
            static_cast<VCMFrameBuffer*>(_frameBuffers[i])->SetState(kStateFree);
        }
    }

    _critSect->Leave();
    _frameEvent.Set(); 
    _packetEvent.Set(); 
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId,
                 _receiverId), "JB(0x%x): Jitter buffer: stop", this);
}

bool
VCMJitterBuffer::Running() const
{
    CriticalSectionScoped cs(_critSect);
    return _running;
}


void
VCMJitterBuffer::Flush()
{
    CriticalSectionScoped cs(_critSect);
    FlushInternal();
}


void
VCMJitterBuffer::FlushInternal()
{
    
    _frameList.clear();
    for (WebRtc_Word32 i = 0; i < _maxNumberOfFrames; i++)
    {
        ReleaseFrameInternal(_frameBuffers[i]);
    }
    _lastDecodedState.Reset(); 
    _packetsNotDecodable = 0;

    _frameEvent.Reset();
    _packetEvent.Reset();

    _numConsecutiveOldFrames = 0;
    _numConsecutiveOldPackets = 0;

    
    _jitterEstimate.Reset();
    _delayEstimate.Reset(_clock->MillisecondTimestamp());

    _waitingForCompletion.frameSize = 0;
    _waitingForCompletion.timestamp = 0;
    _waitingForCompletion.latestPacketTime = -1;

    _firstPacket = true;

    _NACKSeqNumLength = 0;

    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId,
                 _receiverId), "JB(0x%x): Jitter buffer: flush", this);
}



void
VCMJitterBuffer::ReleaseFrameInternal(VCMFrameBuffer* frame)
{
    if (frame != NULL && frame->GetState() != kStateDecoding)
    {
        frame->SetState(kStateFree);
    }
}





VCMFrameBufferEnum
VCMJitterBuffer::UpdateFrameState(VCMFrameBuffer* frame)
{
    if (frame == NULL)
    {
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId), "JB(0x%x) FB(0x%x): "
                         "UpdateFrameState NULL frame pointer", this, frame);
        return kNoError;
    }

    int length = frame->Length();
    if (_master)
    {
        
        
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId),
                     "JB(0x%x) FB(0x%x): Complete frame added to jitter buffer,"
                     " size:%d type %d",
                     this, frame,length,frame->FrameType());
    }

    if (length != 0 && !frame->GetCountedFrame())
    {
        
        _incomingFrameCount++;
        frame->SetCountedFrame(true);
    }

    
    
    if (_lastDecodedState.IsOldFrame(frame))
    {
        
        
        frame->Reset();
        frame->SetState(kStateEmpty);

        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId),
                     "JB(0x%x) FB(0x%x): Dropping old frame in Jitter buffer",
                     this, frame);
        _dropCount++;
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId),
                     "Jitter buffer drop count: %d, consecutive drops: %u",
                     _dropCount, _numConsecutiveOldFrames);
        
        _numConsecutiveOldFrames++;
        if (_numConsecutiveOldFrames > kMaxConsecutiveOldFrames) {
          FlushInternal();
          return kFlushIndicator;
        }
        return kNoError;
    }
    _numConsecutiveOldFrames = 0;
    frame->SetState(kStateComplete);


    
    
    if (frame->IsSessionComplete())
    {
        switch (frame->FrameType())
        {
        case kVideoFrameKey:
            {
                _receiveStatistics[0]++;
                break;
            }
        case kVideoFrameDelta:
            {
                _receiveStatistics[1]++;
                break;
            }
        case kVideoFrameGolden:
            {
                _receiveStatistics[2]++;
                break;
            }
        case kVideoFrameAltRef:
            {
                _receiveStatistics[3]++;
                break;
            }
        default:
            assert(false);

        }
    }
    const FrameList::iterator it = FindOldestCompleteContinuousFrame(false);
    VCMFrameBuffer* oldFrame = NULL;
    if (it != _frameList.end())
    {
        oldFrame = *it;
    }

    
    
    if (!WaitForNack() || (oldFrame != NULL && oldFrame == frame))
    {
        _frameEvent.Set();
    }
    return kNoError;
}


WebRtc_Word32
VCMJitterBuffer::GetFrameStatistics(WebRtc_UWord32& receivedDeltaFrames,
                                    WebRtc_UWord32& receivedKeyFrames) const
{
    {
        CriticalSectionScoped cs(_critSect);
        receivedDeltaFrames = _receiveStatistics[1] + _receiveStatistics[3];
        receivedKeyFrames = _receiveStatistics[0] + _receiveStatistics[2];
    }
    return 0;
}

WebRtc_UWord32 VCMJitterBuffer::NumNotDecodablePackets() const {
  CriticalSectionScoped cs(_critSect);
  return _packetsNotDecodable;
}

WebRtc_UWord32 VCMJitterBuffer::DiscardedPackets() const {
  CriticalSectionScoped cs(_critSect);
  return _discardedPackets;
}


WebRtc_Word32
VCMJitterBuffer::GetFrame(const VCMPacket& packet, VCMEncodedFrame*& frame)
{
    if (!_running) 
    {
        return VCM_UNINITIALIZED;
    }

    _critSect->Enter();
    
    if (_lastDecodedState.IsOldPacket(&packet))
    {
        
        if (packet.sizeBytes > 0)
        {
            _discardedPackets++;
            _numConsecutiveOldPackets++;
        }
        
        
        
        _lastDecodedState.UpdateOldPacket(&packet);

        if (_numConsecutiveOldPackets > kMaxConsecutiveOldPackets)
        {
            FlushInternal();
            _critSect->Leave();
            return VCM_FLUSH_INDICATOR;
        }
        _critSect->Leave();
        return VCM_OLD_PACKET_ERROR;
    }
    _numConsecutiveOldPackets = 0;

    FrameList::iterator it = std::find_if(
        _frameList.begin(),
        _frameList.end(),
        FrameEqualTimestamp(packet.timestamp));

    if (it != _frameList.end()) {
      frame = *it;
      _critSect->Leave();
      return VCM_OK;
    }

    _critSect->Leave();

    
    frame = GetEmptyFrame();
    if (frame != NULL)
    {
        return VCM_OK;
    }
    
    _critSect->Enter();
    RecycleFramesUntilKeyFrame();
    _critSect->Leave();

    frame = GetEmptyFrame();
    if (frame != NULL)
    {
        return VCM_OK;
    }
    return VCM_JITTER_BUFFER_ERROR;
}


VCMEncodedFrame*
VCMJitterBuffer::GetFrame(const VCMPacket& packet)
{
    VCMEncodedFrame* frame = NULL;
    if (GetFrame(packet, frame) < 0)
    {
        return NULL;
    }
    return frame;
}


VCMFrameBuffer*
VCMJitterBuffer::GetEmptyFrame()
{
    if (!_running) 
    {
        return NULL;
    }

    _critSect->Enter();

    for (int i = 0; i <_maxNumberOfFrames; ++i)
    {
        if (kStateFree == _frameBuffers[i]->GetState())
        {
            
            _frameBuffers[i]->SetState(kStateEmpty);
            _critSect->Leave();
            return _frameBuffers[i];
        }
    }

    
    if (_maxNumberOfFrames < kMaxNumberOfFrames)
    {
        VCMFrameBuffer* ptrNewBuffer = new VCMFrameBuffer();
        ptrNewBuffer->SetState(kStateEmpty);
        _frameBuffers[_maxNumberOfFrames] = ptrNewBuffer;
        _maxNumberOfFrames++;

        _critSect->Leave();
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
        VCMId(_vcmId, _receiverId), "JB(0x%x) FB(0x%x): Jitter buffer "
        "increased to:%d frames", this, ptrNewBuffer, _maxNumberOfFrames);
        return ptrNewBuffer;
    }
    _critSect->Leave();

    
    return NULL;
}




FrameList::iterator
VCMJitterBuffer::FindOldestCompleteContinuousFrame(bool enable_decodable) {
  
  VCMFrameBuffer* oldest_frame = NULL;
  FrameList::iterator it = _frameList.begin();

  
  
  
  
  for (; it != _frameList.end(); ++it)  {
    oldest_frame = *it;
    VCMFrameBufferStateEnum state = oldest_frame->GetState();
    
    if ((state == kStateComplete ||
        (enable_decodable && state == kStateDecodable)) &&
        _lastDecodedState.ContinuousFrame(oldest_frame)) {
      break;
    } else {
      int temporal_id = oldest_frame->TemporalId();
      oldest_frame = NULL;
      if (temporal_id <= 0) {
        
        
        break;
      }
    }
  }

  if (oldest_frame == NULL) {
    
    return _frameList.end();
  } else  if (_waitingForKeyFrame &&
              oldest_frame->FrameType() != kVideoFrameKey) {
    
    return _frameList.end();
  }

  
  return it;
}


void
VCMJitterBuffer::RecycleFrame(VCMFrameBuffer* frame)
{
    if (frame == NULL)
    {
        return;
    }

    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(_vcmId, _receiverId),
                 "JB(0x%x) FB(0x%x): RecycleFrame, size:%d",
                 this, frame, frame->Length());

    ReleaseFrameInternal(frame);
}


WebRtc_Word32
VCMJitterBuffer::GetUpdate(WebRtc_UWord32& frameRate, WebRtc_UWord32& bitRate)
{
    CriticalSectionScoped cs(_critSect);
    const WebRtc_Word64 now = _clock->MillisecondTimestamp();
    WebRtc_Word64 diff = now - _timeLastIncomingFrameCount;
    if (diff < 1000 && _incomingFrameRate > 0 && _incomingBitRate > 0)
    {
        
        
        frameRate = _incomingFrameRate;
        bitRate = _incomingBitRate;
    }
    else if (_incomingFrameCount != 0)
    {
        

        
        if (diff <= 0)
        {
            diff = 1;
        }
        
        float rate = 0.5f + ((_incomingFrameCount * 1000.0f) / diff);
        if (rate < 1.0f) 
        {
            rate = 1.0f;
        }

        
        
        
        
        
        
        frameRate = (_incomingFrameRate + (WebRtc_Word32)rate) >> 1;
        _incomingFrameRate = (WebRtc_UWord8)rate;

        
        if (_incomingBitCount == 0)
        {
            bitRate = 0;
        }
        else
        {
            bitRate = 10 * ((100 * _incomingBitCount) /
                      static_cast<WebRtc_UWord32>(diff));
        }
        _incomingBitRate = bitRate;

        
        _incomingFrameCount = 0;
        _incomingBitCount = 0;
        _timeLastIncomingFrameCount = now;

    }
    else
    {
        
        _timeLastIncomingFrameCount = _clock->MillisecondTimestamp();
        frameRate = 0;
        bitRate = 0;
        _incomingBitRate = 0;
    }

    return 0;
}



VCMEncodedFrame*
VCMJitterBuffer::GetCompleteFrameForDecoding(WebRtc_UWord32 maxWaitTimeMS)
{
    if (!_running)
    {
        return NULL;
    }

    _critSect->Enter();

    CleanUpOldFrames();

    if (_lastDecodedState.init() && WaitForNack()) {
      _waitingForKeyFrame = true;
    }

    FrameList::iterator it = FindOldestCompleteContinuousFrame(false);
    if (it == _frameList.end())
    {
        if (maxWaitTimeMS == 0)
        {
            _critSect->Leave();
            return NULL;
        }
        const WebRtc_Word64 endWaitTimeMs = _clock->MillisecondTimestamp()
                                            + maxWaitTimeMS;
        WebRtc_Word64 waitTimeMs = maxWaitTimeMS;
        while (waitTimeMs > 0)
        {
            _critSect->Leave();
            const EventTypeWrapper ret =
                  _frameEvent.Wait(static_cast<WebRtc_UWord32>(waitTimeMs));
            _critSect->Enter();
            if (ret == kEventSignaled)
            {
                
                if (!_running)
                {
                    _critSect->Leave();
                    return NULL;
                }

                
                
                CleanUpOldFrames();
                it = FindOldestCompleteContinuousFrame(false);
                if (it == _frameList.end())
                {
                    waitTimeMs = endWaitTimeMs -
                                 _clock->MillisecondTimestamp();
                }
                else
                {
                    break;
                }
            }
            else
            {
                _critSect->Leave();
                return NULL;
            }
        }
        
    }
    else
    {
        
        _frameEvent.Reset();
    }

    if (it == _frameList.end())
    {
        
        _critSect->Leave();
        return NULL;
    }

    VCMFrameBuffer* oldestFrame = *it;
    it = _frameList.erase(it);

    
    const bool retransmitted = (oldestFrame->GetNackCount() > 0);
    if (retransmitted)
    {
        _jitterEstimate.FrameNacked();
    }
    else if (oldestFrame->Length() > 0)
    {
        
        UpdateJitterAndDelayEstimates(*oldestFrame, false);
    }

    oldestFrame->SetState(kStateDecoding);

    CleanUpOldFrames();

    if (oldestFrame->FrameType() == kVideoFrameKey)
    {
        _waitingForKeyFrame = false;
    }

    _critSect->Leave();

    
    _lastDecodedState.SetState(oldestFrame);

    return oldestFrame;
}

WebRtc_UWord32
VCMJitterBuffer::GetEstimatedJitterMS()
{
    CriticalSectionScoped cs(_critSect);
    return GetEstimatedJitterMsInternal();
}

WebRtc_UWord32
VCMJitterBuffer::GetEstimatedJitterMsInternal()
{
    WebRtc_UWord32 estimate = VCMJitterEstimator::OPERATING_SYSTEM_JITTER;

    
    
    double rttMult = 1.0f;
    if (_nackMode == kNackHybrid && (_lowRttNackThresholdMs >= 0 &&
        static_cast<int>(_rttMs) > _lowRttNackThresholdMs))
    {
        
        rttMult = 0.0f;
    }
    estimate += static_cast<WebRtc_UWord32>
                (_jitterEstimate.GetJitterEstimate(rttMult) + 0.5);
    return estimate;
}

void
VCMJitterBuffer::UpdateRtt(WebRtc_UWord32 rttMs)
{
    CriticalSectionScoped cs(_critSect);
    _rttMs = rttMs;
    _jitterEstimate.UpdateRtt(rttMs);
}


WebRtc_Word64
VCMJitterBuffer::GetNextTimeStamp(WebRtc_UWord32 maxWaitTimeMS,
                                  FrameType& incomingFrameType,
                                  WebRtc_Word64& renderTimeMs)
{
    if (!_running)
    {
        return -1;
    }

    _critSect->Enter();

    
    CleanUpOldFrames();

    FrameList::iterator it = _frameList.begin();

    if (it == _frameList.end())
    {
        _packetEvent.Reset();
        _critSect->Leave();

        if (_packetEvent.Wait(maxWaitTimeMS) == kEventSignaled)
        {
            
            if (!_running)
            {
                return -1;
            }
            _critSect->Enter();

            CleanUpOldFrames();
            it = _frameList.begin();
        }
        else
        {
            _critSect->Enter();
        }
    }

    if (it == _frameList.end())
    {
        _critSect->Leave();
        return -1;
    }
    

    
    
    incomingFrameType = (*it)->FrameType();

    renderTimeMs = (*it)->RenderTimeMs();

    const WebRtc_UWord32 timestamp = (*it)->TimeStamp();

    _critSect->Leave();

    
    return timestamp;
}






bool
VCMJitterBuffer::CompleteSequenceWithNextFrame()
{
    CriticalSectionScoped cs(_critSect);
    
    CleanUpOldFrames();

    if (_frameList.empty())
      return true;

    VCMFrameBuffer* oldestFrame = _frameList.front();
    if (_frameList.size() <= 1 &&
        oldestFrame->GetState() != kStateComplete)
    {
        
        return true;
    }
    if (!oldestFrame->Complete())
    {
        return false;
    }

    
    if (_lastDecodedState.init())
    {
        
        if (oldestFrame->FrameType() != kVideoFrameKey)
        {
            return false;
        }
    }
    else if (oldestFrame->GetLowSeqNum() == -1)
    {
        return false;
    }
    else if (!_lastDecodedState.ContinuousFrame(oldestFrame))
    {
        return false;
    }
    return true;
}


VCMEncodedFrame*
VCMJitterBuffer::GetFrameForDecoding()
{
    CriticalSectionScoped cs(_critSect);
    if (!_running)
    {
        return NULL;
    }

    if (WaitForNack())
    {
        return GetFrameForDecodingNACK();
    }

    CleanUpOldFrames();

    if (_frameList.empty()) {
      return NULL;
    }

    VCMFrameBuffer* oldestFrame = _frameList.front();
    if (_frameList.size() <= 1 &&
        oldestFrame->GetState() != kStateComplete) {
      return NULL;
    }

    
    
    
    
    const bool retransmitted = (oldestFrame->GetNackCount() > 0);
    if (retransmitted)
    {
        _jitterEstimate.FrameNacked();
    }
    else if (oldestFrame->Length() > 0)
    {
        
        
        if (_waitingForCompletion.latestPacketTime >= 0)
        {
            UpdateJitterAndDelayEstimates(_waitingForCompletion, true);
        }
        
        _waitingForCompletion.frameSize = oldestFrame->Length();
        _waitingForCompletion.latestPacketTime =
                              oldestFrame->LatestPacketTimeMs();
        _waitingForCompletion.timestamp = oldestFrame->TimeStamp();
    }
    _frameList.erase(_frameList.begin());

    
    VerifyAndSetPreviousFrameLost(*oldestFrame);

    
    
    
    
    oldestFrame->SetState(kStateDecoding);

    CleanUpOldFrames();

    if (oldestFrame->FrameType() == kVideoFrameKey)
    {
        _waitingForKeyFrame = false;
    }

    _packetsNotDecodable += oldestFrame->NotDecodablePackets();

    
    _lastDecodedState.SetState(oldestFrame);

    return oldestFrame;
}

VCMEncodedFrame*
VCMJitterBuffer::GetFrameForDecodingNACK()
{
    
    
    

    
    CleanUpOldFrames();

    
    
    
    if (_lastDecodedState.init()) {
      _waitingForKeyFrame = true;
    }

    
    bool enableDecodable = _nackMode == kNackHybrid ? true : false;
    FrameList::iterator it = FindOldestCompleteContinuousFrame(enableDecodable);
    if (it == _frameList.end())
    {
        
        it = find_if(_frameList.begin(), _frameList.end(),
                     CompleteDecodableKeyFrameCriteria());
        if (it == _frameList.end())
        {
            return NULL;
        }
    }
    VCMFrameBuffer* oldestFrame = *it;
    
    const bool retransmitted = (oldestFrame->GetNackCount() > 0);
    if (retransmitted)
    {
        _jitterEstimate.FrameNacked();
    }
    else if (oldestFrame->Length() > 0)
    {
        
        UpdateJitterAndDelayEstimates(*oldestFrame, false);
    }
    it = _frameList.erase(it);

    
    VerifyAndSetPreviousFrameLost(*oldestFrame);

    
    
    
    oldestFrame->SetState(kStateDecoding);

    
    CleanUpOldFrames();

    if (oldestFrame->FrameType() == kVideoFrameKey)
    {
        _waitingForKeyFrame = false;
    }

    
    _lastDecodedState.SetState(oldestFrame);

    return oldestFrame;
}




void
VCMJitterBuffer::UpdateJitterAndDelayEstimates(VCMJitterSample& sample,
                                               bool incompleteFrame)
{
    if (sample.latestPacketTime == -1)
    {
        return;
    }
    if (incompleteFrame)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId), "Received incomplete frame "
                     "timestamp %u frame size %u at time %u",
                     sample.timestamp, sample.frameSize,
                     MaskWord64ToUWord32(sample.latestPacketTime));
    }
    else
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId), "Received complete frame "
                     "timestamp %u frame size %u at time %u",
                     sample.timestamp, sample.frameSize,
                     MaskWord64ToUWord32(sample.latestPacketTime));
    }
    UpdateJitterAndDelayEstimates(sample.latestPacketTime,
                                  sample.timestamp,
                                  sample.frameSize,
                                  incompleteFrame);
}




void
VCMJitterBuffer::UpdateJitterAndDelayEstimates(VCMFrameBuffer& frame,
                                               bool incompleteFrame)
{
    if (frame.LatestPacketTimeMs() == -1)
    {
        return;
    }
    
    
    if (incompleteFrame)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId),
                   "Received incomplete frame timestamp %u frame type %d "
                   "frame size %u at time %u, jitter estimate was %u",
                   frame.TimeStamp(), frame.FrameType(), frame.Length(),
                   MaskWord64ToUWord32(frame.LatestPacketTimeMs()),
                   GetEstimatedJitterMsInternal());
    }
    else
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId),"Received complete frame "
                     "timestamp %u frame type %d frame size %u at time %u, "
                     "jitter estimate was %u",
                     frame.TimeStamp(), frame.FrameType(), frame.Length(),
                     MaskWord64ToUWord32(frame.LatestPacketTimeMs()),
                     GetEstimatedJitterMsInternal());
    }
    UpdateJitterAndDelayEstimates(frame.LatestPacketTimeMs(), frame.TimeStamp(),
                                  frame.Length(), incompleteFrame);
}




void
VCMJitterBuffer::UpdateJitterAndDelayEstimates(WebRtc_Word64 latestPacketTimeMs,
                                               WebRtc_UWord32 timestamp,
                                               WebRtc_UWord32 frameSize,
                                               bool incompleteFrame)
{
    if (latestPacketTimeMs == -1)
    {
        return;
    }
    WebRtc_Word64 frameDelay;
    
    WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding,
                 VCMId(_vcmId, _receiverId),
                 "Packet received and sent to jitter estimate with: "
                 "timestamp=%u wallClock=%u", timestamp,
                 MaskWord64ToUWord32(latestPacketTimeMs));
    bool notReordered = _delayEstimate.CalculateDelay(timestamp,
                                                      &frameDelay,
                                                      latestPacketTimeMs);
    
    if (notReordered)
    {
        
        _jitterEstimate.UpdateEstimate(frameDelay, frameSize, incompleteFrame);
    }
}

WebRtc_UWord16*
VCMJitterBuffer::GetNackList(WebRtc_UWord16& nackSize,bool& listExtended)
{
    return CreateNackList(nackSize,listExtended);
}


WebRtc_Word32
VCMJitterBuffer::GetLowHighSequenceNumbers(WebRtc_Word32& lowSeqNum,
                                           WebRtc_Word32& highSeqNum) const
{
    
    WebRtc_Word32 i = 0;
    WebRtc_Word32 seqNum = -1;

    highSeqNum = -1;
    lowSeqNum = -1;
    if (!_lastDecodedState.init())
      lowSeqNum = _lastDecodedState.sequence_num();

    
    for (i = 0; i < _maxNumberOfFrames; ++i)
    {
        seqNum = _frameBuffers[i]->GetHighSeqNum();

        
        VCMFrameBufferStateEnum state = _frameBuffers[i]->GetState();

        if ((kStateFree != state) &&
            (kStateEmpty != state) &&
            (kStateDecoding != state) &&
             seqNum != -1)
        {
            bool wrap;
            highSeqNum = LatestSequenceNumber(seqNum, highSeqNum, &wrap);
        }
    } 
    return 0;
}


WebRtc_UWord16*
VCMJitterBuffer::CreateNackList(WebRtc_UWord16& nackSize, bool& listExtended)
{
    
    CriticalSectionScoped cs(_critSect);
    int i = 0;
    WebRtc_Word32 lowSeqNum = -1;
    WebRtc_Word32 highSeqNum = -1;
    listExtended = false;

    
    if (!WaitForNack())
    {
        nackSize = 0;
        return NULL;
    }

    
    
    
    
    GetLowHighSequenceNumbers(lowSeqNum, highSeqNum);

    
    if (lowSeqNum == -1 || highSeqNum == -1)
    {
        
        if (highSeqNum == -1)
        {
            
            nackSize = 0;
        }
        else
        {
            
            nackSize = 0xffff;
        }
        return NULL;
    }

    int numberOfSeqNum = 0;
    if (lowSeqNum > highSeqNum)
    {
        if (lowSeqNum - highSeqNum > 0x00ff)
        {
            
            numberOfSeqNum = (0xffff-lowSeqNum) + highSeqNum + 1;
        }
    }
    else
    {
        numberOfSeqNum = highSeqNum - lowSeqNum;
    }

    if (numberOfSeqNum > kNackHistoryLength)
    {
        
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId),
                     "Nack list too large, try to find a key frame and restart "
                     "from seq: %d. Lowest seq in jb %d", highSeqNum,lowSeqNum);

        
        bool foundKeyFrame = false;

        while (numberOfSeqNum > kNackHistoryLength)
        {
            foundKeyFrame = RecycleFramesUntilKeyFrame();

            if (!foundKeyFrame)
            {
                break;
            }

            
            lowSeqNum = -1;
            highSeqNum = -1;
            GetLowHighSequenceNumbers(lowSeqNum, highSeqNum);

            if (highSeqNum == -1)
            {
                assert(lowSeqNum != -1); 
                
                return NULL;
            }

            numberOfSeqNum = 0;
            if (lowSeqNum > highSeqNum)
            {
                if (lowSeqNum - highSeqNum > 0x00ff)
                {
                    
                    numberOfSeqNum = (0xffff-lowSeqNum) + highSeqNum + 1;
                    highSeqNum=lowSeqNum;
                }
            }
            else
            {
                numberOfSeqNum = highSeqNum - lowSeqNum;
            }

        } 

        if (!foundKeyFrame)
        {
            

            
            
            _lastDecodedState.SetSeqNum(static_cast<uint16_t>(highSeqNum));
            
            nackSize = 0xffff;
            listExtended = true;
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, -1,
                    "\tNo key frame found, request one. _lastDecodedSeqNum[0] "
                    "%d", _lastDecodedState.sequence_num());
        }
        else
        {
            
            
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, -1,
                    "\tKey frame found. _lastDecodedSeqNum[0] %d",
                    _lastDecodedState.sequence_num());
            nackSize = 0;
        }

        return NULL;
    }

    WebRtc_UWord16 seqNumberIterator = (WebRtc_UWord16)(lowSeqNum + 1);
    for (i = 0; i < numberOfSeqNum; i++)
    {
        _NACKSeqNumInternal[i] = seqNumberIterator;
        seqNumberIterator++;
    }

    

    
    for (i = 0; i < _maxNumberOfFrames; i++)
    {
        
        
        
        
        VCMFrameBufferStateEnum state = _frameBuffers[i]->GetState();

        if ((kStateFree != state) &&
            (kStateEmpty != state) &&
            (kStateDecoding != state))
        {
            
            
            if (_nackMode == kNackHybrid)
            {
                _frameBuffers[i]->BuildSoftNackList(_NACKSeqNumInternal,
                                                    numberOfSeqNum,
                                                    _rttMs);
            }
            else
            {
                
                
                _frameBuffers[i]->BuildHardNackList(_NACKSeqNumInternal,
                                                    numberOfSeqNum);
            }
        }
    }

    
    int emptyIndex = -1;
    for (i = 0; i < numberOfSeqNum; i++)
    {
        if (_NACKSeqNumInternal[i] == -1 || _NACKSeqNumInternal[i] == -2 )
        {
            
            if (emptyIndex == -1)
            {
                
                emptyIndex = i;
            }
        }
        else
        {
            
            if (emptyIndex == -1)
            {
                
            }
            else
            {
                _NACKSeqNumInternal[emptyIndex] = _NACKSeqNumInternal[i];
                _NACKSeqNumInternal[i] = -1;
                emptyIndex++;
            }
        }
    } 

    if (emptyIndex == -1)
    {
        
        nackSize = numberOfSeqNum;
    }
    else
    {
        nackSize = emptyIndex;
    }

    if (nackSize > _NACKSeqNumLength)
    {
        
        listExtended = true;
    }

    for (WebRtc_UWord32 j = 0; j < nackSize; j++)
    {
        
        
        if (_NACKSeqNumLength > j && !listExtended)
        {
            WebRtc_UWord32 k = 0;
            for (k = j; k < _NACKSeqNumLength; k++)
            {
                
                if (_NACKSeqNum[k] == (WebRtc_UWord16)_NACKSeqNumInternal[j])
                {
                   break;
                }
            }
            if (k == _NACKSeqNumLength) 
            {
                listExtended = true;
            }
        }
        else
        {
            listExtended = true;
        }
        _NACKSeqNum[j] = (WebRtc_UWord16)_NACKSeqNumInternal[j];
    }

    _NACKSeqNumLength = nackSize;

    return _NACKSeqNum;
}



void
VCMJitterBuffer::ReleaseFrame(VCMEncodedFrame* frame)
{
    CriticalSectionScoped cs(_critSect);
    VCMFrameBuffer* frameBuffer = static_cast<VCMFrameBuffer*>(frame);
    if (frameBuffer != NULL)
        frameBuffer->SetState(kStateFree);
}

WebRtc_Word64
VCMJitterBuffer::LastPacketTime(VCMEncodedFrame* frame,
                                bool& retransmitted) const
{
    CriticalSectionScoped cs(_critSect);
    retransmitted = (static_cast<VCMFrameBuffer*>(frame)->GetNackCount() > 0);
    return static_cast<VCMFrameBuffer*>(frame)->LatestPacketTimeMs();
}

WebRtc_Word64
VCMJitterBuffer::LastDecodedTimestamp() const
{
    CriticalSectionScoped cs(_critSect);
    return _lastDecodedState.time_stamp();
}



VCMFrameBufferEnum
VCMJitterBuffer::InsertPacket(VCMEncodedFrame* buffer, const VCMPacket& packet)
{
    CriticalSectionScoped cs(_critSect);
    WebRtc_Word64 nowMs = _clock->MillisecondTimestamp();
    VCMFrameBufferEnum bufferReturn = kSizeError;
    VCMFrameBufferEnum ret = kSizeError;
    VCMFrameBuffer* frame = static_cast<VCMFrameBuffer*>(buffer);

    
    
    if (_firstPacket)
    {
        
        
        _delayEstimate.Reset(_clock->MillisecondTimestamp());
        _firstPacket = false;
    }

    
    
    if (packet.frameType != kFrameEmpty)
    {
        if (_waitingForCompletion.timestamp == packet.timestamp)
        {
            
            
            _waitingForCompletion.frameSize += packet.sizeBytes;
            _waitingForCompletion.latestPacketTime = nowMs;
        }
        else if (_waitingForCompletion.latestPacketTime >= 0 &&
                 _waitingForCompletion.latestPacketTime + 2000 <= nowMs)
        {
            
            UpdateJitterAndDelayEstimates(_waitingForCompletion, true);
            _waitingForCompletion.latestPacketTime = -1;
            _waitingForCompletion.frameSize = 0;
            _waitingForCompletion.timestamp = 0;
        }
    }

    if (frame != NULL)
    {
        VCMFrameBufferStateEnum state = frame->GetState();
        _lastDecodedState.UpdateOldPacket(&packet);
        
        
        
        
        bool first = (frame->GetHighSeqNum() == -1);
        
        
        
        
        bufferReturn = frame->InsertPacket(packet, nowMs,
                                           _nackMode == kNackHybrid,
                                           _rttMs);
        ret = bufferReturn;

        if (bufferReturn > 0)
        {
            _incomingBitCount += packet.sizeBytes << 3;

            
            if (IsPacketRetransmitted(packet))
            {
                frame->IncrementNackCount();
            }

            
            
            if (state == kStateEmpty && first)
            {
                ret = kFirstPacket;
                FrameList::reverse_iterator rit = std::find_if(
                    _frameList.rbegin(), _frameList.rend(),
                    FrameSmallerTimestamp(frame->TimeStamp()));
                _frameList.insert(rit.base(), frame);
            }
        }
    }
    switch(bufferReturn)
    {
    case kStateError:
    case kTimeStampError:
    case kSizeError:
        {
            if (frame != NULL)
            {
                
                frame->Reset();
                frame->SetState(kStateEmpty);
            }
            break;
        }
    case kCompleteSession:
        {
            
            if (UpdateFrameState(frame) == kFlushIndicator)
              ret = kFlushIndicator;
            
            _packetEvent.Set();
            break;
        }
    case kDecodableSession:
    case kIncomplete:
        {
          
          _packetEvent.Set();
          break;
        }
    case kNoError:
    case kDuplicatePacket:
        {
            break;
        }
    default:
        {
            assert(false && "JitterBuffer::InsertPacket: Undefined value");
        }
    }
   return ret;
}


void
VCMJitterBuffer::UpdateOldJitterSample(const VCMPacket& packet)
{
    if (_waitingForCompletion.timestamp != packet.timestamp &&
        LatestTimestamp(_waitingForCompletion.timestamp, packet.timestamp,
                        NULL) == packet.timestamp)
    {
        
        _waitingForCompletion.frameSize = packet.sizeBytes;
        _waitingForCompletion.timestamp = packet.timestamp;
    }
    else
    {
        
        
        _waitingForCompletion.frameSize += packet.sizeBytes;
        _jitterEstimate.UpdateMaxFrameSize(_waitingForCompletion.frameSize);
    }
}


bool
VCMJitterBuffer::IsPacketRetransmitted(const VCMPacket& packet) const
{
    if (_NACKSeqNumLength > 0)
    {
        for (WebRtc_UWord16 i = 0; i < _NACKSeqNumLength; i++)
        {
            if (packet.seqNum == _NACKSeqNum[i])
            {
                return true;
            }
        }
    }
    return false;
}


VCMNackMode
VCMJitterBuffer::GetNackMode() const
{
    CriticalSectionScoped cs(_critSect);
    return _nackMode;
}


void
VCMJitterBuffer::SetNackMode(VCMNackMode mode,
                             int lowRttNackThresholdMs,
                             int highRttNackThresholdMs)
{
    CriticalSectionScoped cs(_critSect);
    _nackMode = mode;
    assert(lowRttNackThresholdMs >= -1 && highRttNackThresholdMs >= -1);
    assert(highRttNackThresholdMs == -1 ||
           lowRttNackThresholdMs <= highRttNackThresholdMs);
    assert(lowRttNackThresholdMs > -1 || highRttNackThresholdMs == -1);
    _lowRttNackThresholdMs = lowRttNackThresholdMs;
    _highRttNackThresholdMs = highRttNackThresholdMs;
    if (_nackMode == kNoNack)
    {
        _jitterEstimate.ResetNackCount();
    }
}



bool
VCMJitterBuffer::RecycleFramesUntilKeyFrame()
{
    
    while (_frameList.size() > 0)
    {
        
        _dropCount++;
        FrameList::iterator it = _frameList.begin();
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCoding,
                     VCMId(_vcmId, _receiverId),
                     "Jitter buffer drop count:%d, lowSeq %d", _dropCount,
                     (*it)->GetLowSeqNum());
        RecycleFrame(*it);
        it = _frameList.erase(it);
        if (it != _frameList.end() && (*it)->FrameType() == kVideoFrameKey)
        {
            
            _lastDecodedState.SetStateOneBack(*it);
            return true;
        }
    }
    _waitingForKeyFrame = true;
    _lastDecodedState.Reset(); 
    return false;
}


void VCMJitterBuffer::CleanUpOldFrames() {
  while (_frameList.size() > 0) {
    VCMFrameBuffer* oldestFrame = _frameList.front();
    bool nextFrameEmpty = (_lastDecodedState.ContinuousFrame(oldestFrame) &&
        oldestFrame->GetState() == kStateEmpty);
    if (_lastDecodedState.IsOldFrame(oldestFrame) ||
        (nextFrameEmpty && _frameList.size() > 1)) {
      ReleaseFrameInternal(_frameList.front());
      _frameList.erase(_frameList.begin());
    } else {
      break;
    }
  }
}


void VCMJitterBuffer::VerifyAndSetPreviousFrameLost(VCMFrameBuffer& frame) {
  frame.MakeSessionDecodable();  
  if (frame.FrameType() == kVideoFrameKey)
    return;

  if (!_lastDecodedState.ContinuousFrame(&frame))
    frame.SetPreviousFrameLoss();
}

bool
VCMJitterBuffer::WaitForNack()
{
     
     if (_nackMode == kNoNack)
     {
         return false;
     }
     
     else if (_nackMode == kNackInfinite)
     {
         return true;
     }
     
     
     if (_highRttNackThresholdMs >= 0 &&
         _rttMs >= static_cast<unsigned int>(_highRttNackThresholdMs))
     {
         return false;
     }
     
     return true;
}

}  
