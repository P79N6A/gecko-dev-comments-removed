









#include "bandwidth_management.h"
#include "trace.h"
#include "rtp_utility.h"
#include "rtp_rtcp_config.h"

#include <math.h>   

namespace webrtc {

BandwidthManagement::BandwidthManagement(const WebRtc_Word32 id) :
    _id(id),
    _critsect(CriticalSectionWrapper::CreateCriticalSection()),
    _lastPacketLossExtendedHighSeqNum(0),
    _lastReportAllLost(false),
    _lastLoss(0),
    _accumulateLostPacketsQ8(0),
    _accumulateExpectedPackets(0),
    _bitRate(0),
    _minBitRateConfigured(0),
    _maxBitRateConfigured(0),
    _last_fraction_loss(0),
    _last_round_trip_time(0),
    _bwEstimateIncoming(0),
    _timeLastIncrease(0)
{
}

BandwidthManagement::~BandwidthManagement()
{
    delete _critsect;
}

void
BandwidthManagement::SetSendBitrate(const WebRtc_UWord32 startBitrate,
                                    const WebRtc_UWord16 minBitrateKbit,
                                    const WebRtc_UWord16 maxBitrateKbit)
{
    CriticalSectionScoped cs(_critsect);

    _bitRate = startBitrate;
    _minBitRateConfigured = minBitrateKbit*1000;
    if(maxBitrateKbit == 0)
    {
        
        _maxBitRateConfigured = 1000000000;
    } else
    {
        _maxBitRateConfigured = maxBitrateKbit*1000;
    }
}

WebRtc_Word32
BandwidthManagement::MaxConfiguredBitrate(WebRtc_UWord16* maxBitrateKbit)
{
    CriticalSectionScoped cs(_critsect);

    if(_maxBitRateConfigured == 0)
    {
        return -1;
    }
    *maxBitrateKbit = (WebRtc_UWord16)(_maxBitRateConfigured/1000);
    return 0;
}

WebRtc_Word32
BandwidthManagement::UpdateBandwidthEstimate(const WebRtc_UWord16 bandWidthKbit,
                                             WebRtc_UWord32* newBitrate,
                                             WebRtc_UWord8* fractionLost,
                                             WebRtc_UWord16* roundTripTime)
{
    *newBitrate = 0;
    CriticalSectionScoped cs(_critsect);

    _bwEstimateIncoming = bandWidthKbit*1000;

    if(_bitRate == 0)
    {
        
        return -1;
    }
    if (_bwEstimateIncoming > 0 && _bitRate > _bwEstimateIncoming)
    {
        _bitRate   = _bwEstimateIncoming;
    } else
    {
        return -1;
    }
    *newBitrate = _bitRate;
    *fractionLost = _last_fraction_loss;
    *roundTripTime = _last_round_trip_time;
    return 0;
}

WebRtc_Word32 BandwidthManagement::UpdatePacketLoss(
    const WebRtc_UWord32 lastReceivedExtendedHighSeqNum,
    WebRtc_UWord32 sentBitrate,
    const WebRtc_UWord16 rtt,
    WebRtc_UWord8* loss,
    WebRtc_UWord32* newBitrate,
    WebRtc_Word64 nowMS)
{
    CriticalSectionScoped cs(_critsect);

    _last_fraction_loss = *loss;
    _last_round_trip_time = rtt;

    if(_bitRate == 0)
    {
        
        return -1;
    }

    
    if (_lastPacketLossExtendedHighSeqNum > 0 &&
        (lastReceivedExtendedHighSeqNum >= _lastPacketLossExtendedHighSeqNum))
    {
        
        
        WebRtc_UWord32 seqNumDiff = lastReceivedExtendedHighSeqNum
            - _lastPacketLossExtendedHighSeqNum;

        
        
        
        if (!(_lastReportAllLost && *loss == 255))
        {
            _lastReportAllLost = (*loss == 255);

            
            
            const int numLostPacketsQ8 = *loss * seqNumDiff;

            
            _accumulateLostPacketsQ8 += numLostPacketsQ8;
            _accumulateExpectedPackets += seqNumDiff;

            
            
            const int limitNumPackets = 10;
            if (_accumulateExpectedPackets >= limitNumPackets)
            {
                *loss = _accumulateLostPacketsQ8 / _accumulateExpectedPackets;

                
                _accumulateLostPacketsQ8 = 0;
                _accumulateExpectedPackets = 0;
            }
            else
            {
                
                
                *loss = 0;
            }
        }
    }
    
    _lastLoss = *loss;

    
    _lastPacketLossExtendedHighSeqNum = lastReceivedExtendedHighSeqNum;

    WebRtc_UWord32 bitRate = ShapeSimple(*loss, rtt, sentBitrate, nowMS);
    if (bitRate == 0)
    {
        
        return -1;
    }
    _bitRate = bitRate;
    *newBitrate = bitRate;
    return 0;
}

WebRtc_Word32 BandwidthManagement::AvailableBandwidth(
    WebRtc_UWord32* bandwidthKbit) const {
  CriticalSectionScoped cs(_critsect);
  if (_bitRate == 0) {
    return -1;
  }
  if (!bandwidthKbit) {
    return -1;
  }
  *bandwidthKbit = _bitRate;
  return 0;
}






WebRtc_Word32 BandwidthManagement::CalcTFRCbps(WebRtc_Word16 avgPackSizeBytes,
                                               WebRtc_Word32 rttMs,
                                               WebRtc_Word32 packetLoss)
{
    if (avgPackSizeBytes <= 0 || rttMs <= 0 || packetLoss <= 0)
    {
        
        return -1;
    }

    double R = static_cast<double>(rttMs)/1000; 
    int b = 1; 
    double t_RTO = 4.0 * R; 
    double p = static_cast<double>(packetLoss)/255; 
    double s = static_cast<double>(avgPackSizeBytes);

    
    double X = s / (R * sqrt(2 * b * p / 3) + (t_RTO * (3 * sqrt( 3 * b * p / 8) * p * (1 + 32 * p * p))));

    return (static_cast<WebRtc_Word32>(X*8)); 
}





WebRtc_UWord32 BandwidthManagement::ShapeSimple(WebRtc_Word32 packetLoss,
                                                WebRtc_Word32 rtt,
                                                WebRtc_UWord32 sentBitrate,
                                                WebRtc_Word64 nowMS)
{
    WebRtc_UWord32 newBitRate = 0;
    bool reducing = false;

    
    if (packetLoss <= 5)
    {
        if ((nowMS - _timeLastIncrease) <
            kBWEUpdateIntervalMs)
        {
            return _bitRate;
        }
        _timeLastIncrease = nowMS;
    }

    if (packetLoss > 5 && packetLoss <= 26)
    {
        
        newBitRate = _bitRate;
    }
    else if (packetLoss > 26)
    {
        
        
        
        newBitRate = static_cast<WebRtc_UWord32>(
            (sentBitrate * static_cast<double>(512 - packetLoss)) / 512.0);
        reducing = true;
    }
    else
    {
        
        newBitRate = static_cast<WebRtc_UWord32>(_bitRate * 1.08 + 0.5);

        
        
        newBitRate += 1000;
    }

    
    WebRtc_Word32 tfrcRate = CalcTFRCbps(1000, rtt, packetLoss); 

    if (reducing &&
        tfrcRate > 0 &&
        static_cast<WebRtc_UWord32>(tfrcRate) > newBitRate)
    {
        
        newBitRate = tfrcRate;
    }

    if (_bwEstimateIncoming > 0 && newBitRate > _bwEstimateIncoming)
    {
        newBitRate = _bwEstimateIncoming;
    }
    if (newBitRate > _maxBitRateConfigured)
    {
        newBitRate = _maxBitRateConfigured;
    }
    if (newBitRate < _minBitRateConfigured)
    {
        WEBRTC_TRACE(kTraceWarning,
                     kTraceRtpRtcp,
                     _id,
                     "The configured min bitrate (%u kbps) is greater than the "
                     "estimated available bandwidth (%u kbps).\n",
                     _minBitRateConfigured / 1000, newBitRate / 1000);
        newBitRate = _minBitRateConfigured;
    }
    return newBitRate;
}
} 
