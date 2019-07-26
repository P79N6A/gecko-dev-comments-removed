









#include "Bitrate.h"
#include "rtp_utility.h"

namespace webrtc {
Bitrate::Bitrate(RtpRtcpClock* clock) :
    _clock(*clock),
    _packetRate(0),
    _bitrate(0),
    _bitrateNextIdx(0),
    _timeLastRateUpdate(0),
    _bytesCount(0),
    _packetCount(0)
{
    memset(_packetRateArray, 0, sizeof(_packetRateArray));
    memset(_bitrateDiffMS, 0, sizeof(_bitrateDiffMS));
    memset(_bitrateArray, 0, sizeof(_bitrateArray));
}

void
Bitrate::Update(const WebRtc_Word32 bytes)
{
    _bytesCount += bytes;
    _packetCount++;
}

WebRtc_UWord32
Bitrate::PacketRate() const
{
    return _packetRate;
}

WebRtc_UWord32 Bitrate::BitrateLast() const {
  return _bitrate;
}

WebRtc_UWord32 Bitrate::BitrateNow() const {
  WebRtc_Word64 now = _clock.GetTimeInMS();
  WebRtc_Word64 diffMS = now -_timeLastRateUpdate;

  if(diffMS > 10000) {  
    
    return _bitrate; 
  }
  WebRtc_Word64 bitsSinceLastRateUpdate = 8 * _bytesCount * 1000;

  
  
  WebRtc_Word64 bitrate = (static_cast<WebRtc_UWord64>(_bitrate) * 1000 +
      bitsSinceLastRateUpdate) / (1000 + diffMS);
  return static_cast<WebRtc_UWord32>(bitrate);
}

void Bitrate::Process() {
  
  WebRtc_Word64 now = _clock.GetTimeInMS();
  WebRtc_Word64 diffMS = now -_timeLastRateUpdate;

  if (diffMS < 100) {
    
    return;
  }
  if (diffMS > 10000) {  
    
    _timeLastRateUpdate = now;
    _bytesCount = 0;
    _packetCount = 0;
    return;
  }
  _packetRateArray[_bitrateNextIdx] = (_packetCount * 1000) / diffMS;
  _bitrateArray[_bitrateNextIdx] = 8 * ((_bytesCount * 1000) / diffMS);
  _bitrateDiffMS[_bitrateNextIdx] = diffMS;
  _bitrateNextIdx++;
  if (_bitrateNextIdx >= 10) {
    _bitrateNextIdx = 0;
  }
  WebRtc_Word64 sumDiffMS = 0;
  WebRtc_Word64 sumBitrateMS = 0;
  WebRtc_Word64 sumPacketrateMS = 0;
  for (int i = 0; i < 10; i++) {
    sumDiffMS += _bitrateDiffMS[i];
    sumBitrateMS += _bitrateArray[i] * _bitrateDiffMS[i];
    sumPacketrateMS += _packetRateArray[i] * _bitrateDiffMS[i];
  }
  _timeLastRateUpdate = now;
  _bytesCount = 0;
  _packetCount = 0;
  _packetRate = static_cast<WebRtc_UWord32>(sumPacketrateMS / sumDiffMS);
  _bitrate = static_cast<WebRtc_UWord32>(sumBitrateMS / sumDiffMS);
}

} 
