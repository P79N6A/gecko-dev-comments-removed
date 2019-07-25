









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_BANDWIDTH_MANAGEMENT_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_BANDWIDTH_MANAGEMENT_H_

#include "typedefs.h"
#include "rtp_rtcp_config.h"
#include "critical_section_wrapper.h"





namespace webrtc {
class BandwidthManagement
{
public:
    BandwidthManagement(const WebRtc_Word32 id);
    ~BandwidthManagement();

    
    WebRtc_Word32 UpdateBandwidthEstimate(const WebRtc_UWord16 bandWidthKbit,
                                          WebRtc_UWord32* newBitrate,
                                          WebRtc_UWord8* fractionLost,
                                          WebRtc_UWord16* roundTripTime);

   
    WebRtc_Word32 UpdatePacketLoss(
        const WebRtc_UWord32 lastReceivedExtendedHighSeqNum,
        WebRtc_UWord32 sentBitrate,
        const WebRtc_UWord16 rtt,
        WebRtc_UWord8* loss,
        WebRtc_UWord32* newBitrate,
        WebRtc_Word64 nowMS);

    
    
    WebRtc_Word32 AvailableBandwidth(WebRtc_UWord32* bandwidthKbit) const;

    void SetSendBitrate(const WebRtc_UWord32 startBitrate,
                        const WebRtc_UWord16 minBitrateKbit,
                        const WebRtc_UWord16 maxBitrateKbit);

    WebRtc_Word32 MaxConfiguredBitrate(WebRtc_UWord16* maxBitrateKbit);

protected:
    WebRtc_UWord32 ShapeSimple(WebRtc_Word32 packetLoss,
                               WebRtc_Word32 rtt,
                               WebRtc_UWord32 sentBitrate,
                               WebRtc_Word64 nowMS);

    WebRtc_Word32 CalcTFRCbps(WebRtc_Word16 avgPackSizeBytes,
                              WebRtc_Word32 rttMs,
                              WebRtc_Word32 packetLoss);

private:
    enum { kBWEUpdateIntervalMs = 1000 };

    WebRtc_Word32         _id;

    CriticalSectionWrapper* _critsect;

    
    WebRtc_UWord32        _lastPacketLossExtendedHighSeqNum;
    bool                  _lastReportAllLost;
    WebRtc_UWord8         _lastLoss;
    int                   _accumulateLostPacketsQ8;
    int                   _accumulateExpectedPackets;

    
    WebRtc_UWord32        _bitRate;
    WebRtc_UWord32        _minBitRateConfigured;
    WebRtc_UWord32        _maxBitRateConfigured;

    WebRtc_UWord8         _last_fraction_loss;
    WebRtc_UWord16        _last_round_trip_time;

    
    WebRtc_UWord32        _bwEstimateIncoming;
    WebRtc_Word64         _timeLastIncrease;
};
} 

#endif
