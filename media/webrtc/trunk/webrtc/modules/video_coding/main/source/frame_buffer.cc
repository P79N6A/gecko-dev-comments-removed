









#include "frame_buffer.h"
#include "packet.h"

#include <cassert>
#include <string.h>

namespace webrtc {

VCMFrameBuffer::VCMFrameBuffer()
  :
    _state(kStateFree),
    _frameCounted(false),
    _nackCount(0),
    _latestPacketTimeMs(-1) {
}

VCMFrameBuffer::~VCMFrameBuffer() {
}

VCMFrameBuffer::VCMFrameBuffer(VCMFrameBuffer& rhs)
:
VCMEncodedFrame(rhs),
_state(rhs._state),
_frameCounted(rhs._frameCounted),
_sessionInfo(),
_nackCount(rhs._nackCount),
_latestPacketTimeMs(rhs._latestPacketTimeMs)
{
    _sessionInfo = rhs._sessionInfo;
    _sessionInfo.UpdateDataPointers(rhs._buffer, _buffer);
}

webrtc::FrameType
VCMFrameBuffer::FrameType() const
{
    return _sessionInfo.FrameType();
}

void
VCMFrameBuffer::SetPreviousFrameLoss()
{
    _sessionInfo.SetPreviousFrameLoss();
}

int32_t
VCMFrameBuffer::GetLowSeqNum() const
{
    return _sessionInfo.LowSequenceNumber();
}

int32_t
VCMFrameBuffer::GetHighSeqNum() const
{
    return _sessionInfo.HighSequenceNumber();
}

int VCMFrameBuffer::PictureId() const {
  return _sessionInfo.PictureId();
}

int VCMFrameBuffer::TemporalId() const {
  return _sessionInfo.TemporalId();
}

bool VCMFrameBuffer::LayerSync() const {
  return _sessionInfo.LayerSync();
}

int VCMFrameBuffer::Tl0PicId() const {
  return _sessionInfo.Tl0PicId();
}

bool VCMFrameBuffer::NonReference() const {
  return _sessionInfo.NonReference();
}

bool
VCMFrameBuffer::IsSessionComplete() const
{
    return _sessionInfo.complete();
}


VCMFrameBufferEnum
VCMFrameBuffer::InsertPacket(const VCMPacket& packet, int64_t timeInMs,
                             bool enableDecodableState, uint32_t rttMS)
{
    if (_state == kStateDecoding)
    {
        
        return kNoError;
    }

    
    if (_state == kStateFree)
    {
        return kStateError;
    }

    
    if (TimeStamp() && (TimeStamp() != packet.timestamp))
    {
        return kTimeStampError;
    }

    
    if (_size + packet.sizeBytes +
        (packet.insertStartCode ?  kH264StartCodeLengthBytes : 0 )
        > kMaxJBFrameSizeBytes)
    {
        return kSizeError;
    }
    if (NULL == packet.dataPtr && packet.sizeBytes > 0)
    {
        return kSizeError;
    }
    if (packet.dataPtr != NULL)
    {
        _payloadType = packet.payloadType;
    }

    if (kStateEmpty == _state)
    {
        
        
        _timeStamp = packet.timestamp;
        _codec = packet.codec;
        if (packet.frameType != kFrameEmpty)
        {
            
            SetState(kStateIncomplete);
        }
    }

    uint32_t requiredSizeBytes = Length() + packet.sizeBytes +
                   (packet.insertStartCode ? kH264StartCodeLengthBytes : 0);
    if (requiredSizeBytes >= _size)
    {
        const uint8_t* prevBuffer = _buffer;
        const uint32_t increments = requiredSizeBytes /
                                          kBufferIncStepSizeBytes +
                                        (requiredSizeBytes %
                                         kBufferIncStepSizeBytes > 0);
        const uint32_t newSize = _size +
                                       increments * kBufferIncStepSizeBytes;
        if (newSize > kMaxJBFrameSizeBytes)
        {
            return kSizeError;
        }
        if (VerifyAndAllocate(newSize) == -1)
        {
            return kSizeError;
        }
        _sessionInfo.UpdateDataPointers(prevBuffer, _buffer);
    }

    CopyCodecSpecific(&packet.codecSpecificHeader);

    int retVal = _sessionInfo.InsertPacket(packet, _buffer,
                                           enableDecodableState,
                                           rttMS);
    if (retVal == -1)
    {
        return kSizeError;
    }
    else if (retVal == -2)
    {
        return kDuplicatePacket;
    }
    
    _length = Length() + static_cast<uint32_t>(retVal);

    _latestPacketTimeMs = timeInMs;

    if (_sessionInfo.complete()) {
      return kCompleteSession;
    } else if (_sessionInfo.decodable()) {
      SetState(kStateDecodable);
      return kDecodableSession;
    } else {
      
      if (_state == kStateComplete) {
        
        
        _state = kStateIncomplete;
      }
    }
    return kIncomplete;
}

int64_t
VCMFrameBuffer::LatestPacketTimeMs() const
{
    return _latestPacketTimeMs;
}

void
VCMFrameBuffer::IncrementNackCount()
{
    _nackCount++;
}

int16_t
VCMFrameBuffer::GetNackCount() const
{
    return _nackCount;
}

bool
VCMFrameBuffer::HaveFirstPacket() const
{
    return _sessionInfo.HaveFirstPacket();
}

bool
VCMFrameBuffer::HaveLastPacket() const
{
    return _sessionInfo.HaveLastPacket();
}

void
VCMFrameBuffer::Reset()
{
    _length = 0;
    _timeStamp = 0;
    _sessionInfo.Reset();
    _frameCounted = false;
    _payloadType = 0;
    _nackCount = 0;
    _latestPacketTimeMs = -1;
    _state = kStateFree;
    VCMEncodedFrame::Reset();
}


void
VCMFrameBuffer::MakeSessionDecodable()
{
    uint32_t retVal;
#ifdef INDEPENDENT_PARTITIONS
    if (_codec != kVideoCodecVP8) {
        retVal = _sessionInfo.MakeDecodable();
        _length -= retVal;
    }
#else
    retVal = _sessionInfo.MakeDecodable();
    _length -= retVal;
#endif
}


void
VCMFrameBuffer::SetState(VCMFrameBufferStateEnum state)
{
    if (_state == state)
    {
        return;
    }
    switch (state)
    {
    case kStateFree:
        
        
        
        
        
        Reset();
        break;

    case kStateIncomplete:
        
        assert(_state == kStateEmpty ||
            _state == kStateDecoding);

        
        break;

    case kStateComplete:
        assert(_state == kStateEmpty ||
               _state == kStateIncomplete ||
               _state == kStateDecodable);

        break;

    case kStateEmpty:
        assert(_state == kStateFree);
        
        break;

    case kStateDecoding:
        
        
        
        
        assert(_state == kStateComplete || _state == kStateIncomplete ||
               _state == kStateDecodable || _state == kStateEmpty);
        
        
        RestructureFrameInformation();
        break;

    case kStateDecodable:
        assert(_state == kStateEmpty ||
               _state == kStateIncomplete);
        break;
    }
    _state = state;
}

void
VCMFrameBuffer::RestructureFrameInformation()
{
    PrepareForDecode();
    _frameType = ConvertFrameType(_sessionInfo.FrameType());
    _completeFrame = _sessionInfo.complete();
    _missingFrame = _sessionInfo.PreviousFrameLoss();
}

int32_t
VCMFrameBuffer::ExtractFromStorage(const EncodedVideoData& frameFromStorage)
{
    _frameType = ConvertFrameType(frameFromStorage.frameType);
    _timeStamp = frameFromStorage.timeStamp;
    _payloadType = frameFromStorage.payloadType;
    _encodedWidth = frameFromStorage.encodedWidth;
    _encodedHeight = frameFromStorage.encodedHeight;
    _missingFrame = frameFromStorage.missingFrame;
    _completeFrame = frameFromStorage.completeFrame;
    _renderTimeMs = frameFromStorage.renderTimeMs;
    _codec = frameFromStorage.codec;
    const uint8_t *prevBuffer = _buffer;
    if (VerifyAndAllocate(frameFromStorage.payloadSize) < 0)
    {
        return VCM_MEMORY;
    }
    _sessionInfo.UpdateDataPointers(prevBuffer, _buffer);
    memcpy(_buffer, frameFromStorage.payloadData, frameFromStorage.payloadSize);
    _length = frameFromStorage.payloadSize;
    return VCM_OK;
}

int VCMFrameBuffer::NotDecodablePackets() const {
  return _sessionInfo.packets_not_decodable();
}


void VCMFrameBuffer::SetCountedFrame(bool frameCounted)
{
    _frameCounted = frameCounted;
}

bool VCMFrameBuffer::GetCountedFrame() const
{
    return _frameCounted;
}


VCMFrameBufferStateEnum
VCMFrameBuffer::GetState() const
{
    return _state;
}


VCMFrameBufferStateEnum
VCMFrameBuffer::GetState(uint32_t& timeStamp) const
{
    timeStamp = TimeStamp();
    return GetState();
}

bool
VCMFrameBuffer::IsRetransmitted() const
{
    return _sessionInfo.session_nack();
}

void
VCMFrameBuffer::PrepareForDecode()
{
#ifdef INDEPENDENT_PARTITIONS
    if (_codec == kVideoCodecVP8)
    {
        _length =
            _sessionInfo.BuildVP8FragmentationHeader(_buffer, _length,
                                                     &_fragmentation);
    }
#endif
}

}
