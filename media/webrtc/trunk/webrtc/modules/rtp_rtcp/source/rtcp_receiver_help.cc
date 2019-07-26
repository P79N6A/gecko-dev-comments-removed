









#include "rtcp_receiver_help.h"

#include <string.h>  
#include <cassert>  

#include "modules/rtp_rtcp/source/rtp_utility.h"

namespace webrtc {
using namespace RTCPHelp;

RTCPPacketInformation::RTCPPacketInformation()
    : rtcpPacketTypeFlags(0),
      remoteSSRC(0),
      nackSequenceNumbers(0),
      nackSequenceNumbersLength(0),
      applicationSubType(0),
      applicationName(0),
      applicationData(),
      applicationLength(0),
      reportBlock(false),
      fractionLost(0),
      roundTripTime(0),
      lastReceivedExtendedHighSeqNum(0),
      jitter(0),
      interArrivalJitter(0),
      sliPictureId(0),
      rpsiPictureId(0),
      receiverEstimatedMaxBitrate(0),
      ntp_secs(0),
      ntp_frac(0),
      rtp_timestamp(0),
      VoIPMetric(NULL) {
}

RTCPPacketInformation::~RTCPPacketInformation()
{
    delete [] nackSequenceNumbers;
    delete [] applicationData;
    delete VoIPMetric;
}

void
RTCPPacketInformation::AddVoIPMetric(const RTCPVoIPMetric* metric)
{
    VoIPMetric = new RTCPVoIPMetric();
    memcpy(VoIPMetric, metric, sizeof(RTCPVoIPMetric));
}

void RTCPPacketInformation::AddApplicationData(const WebRtc_UWord8* data,
                                               const WebRtc_UWord16 size) {
    WebRtc_UWord8* oldData = applicationData;
    WebRtc_UWord16 oldLength = applicationLength;

    
    WebRtc_UWord16 copySize = size;
    if (size > kRtcpAppCode_DATA_SIZE) {
        copySize = kRtcpAppCode_DATA_SIZE;
    }

    applicationLength += copySize;
    applicationData = new WebRtc_UWord8[applicationLength];

    if (oldData)
    {
        memcpy(applicationData, oldData, oldLength);
        memcpy(applicationData+oldLength, data, copySize);
        delete [] oldData;
    } else
    {
        memcpy(applicationData, data, copySize);
    }
}

void
RTCPPacketInformation::ResetNACKPacketIdArray()
{
    if (NULL == nackSequenceNumbers)
    {
        nackSequenceNumbers = new WebRtc_UWord16[NACK_PACKETS_MAX_SIZE];
    }
    nackSequenceNumbersLength = 0;
}

void
RTCPPacketInformation::AddNACKPacket(const WebRtc_UWord16 packetID)
{
    assert(nackSequenceNumbers);

    WebRtc_UWord16& idx = nackSequenceNumbersLength;
    if (idx < NACK_PACKETS_MAX_SIZE)
    {
        nackSequenceNumbers[idx++] = packetID;
    }
}

void
RTCPPacketInformation::AddReportInfo(const WebRtc_UWord8 fraction,
                                     const WebRtc_UWord16 rtt,
                                     const WebRtc_UWord32 extendedHighSeqNum,
                                     const WebRtc_UWord32 j)
{
    reportBlock = true;
    fractionLost = fraction;
    roundTripTime = rtt;
    jitter = j;
    lastReceivedExtendedHighSeqNum = extendedHighSeqNum;
}

RTCPReportBlockInformation::RTCPReportBlockInformation():
    remoteReceiveBlock(),
    remoteMaxJitter(0),
    RTT(0),
    minRTT(0),
    maxRTT(0),
    avgRTT(0),
    numAverageCalcs(0)
{
    memset(&remoteReceiveBlock,0,sizeof(remoteReceiveBlock));
}

RTCPReportBlockInformation::~RTCPReportBlockInformation()
{
}

RTCPReceiveInformation::RTCPReceiveInformation()
    : lastTimeReceived(0),
      lastFIRSequenceNumber(-1),
      lastFIRRequest(0),
      readyForDelete(false) {
}

RTCPReceiveInformation::~RTCPReceiveInformation() {
}



void RTCPReceiveInformation::VerifyAndAllocateTMMBRSet(
    const WebRtc_UWord32 minimumSize) {
  if (minimumSize > TmmbrSet.sizeOfSet()) {
    TmmbrSet.VerifyAndAllocateSetKeepingData(minimumSize);
    
    _tmmbrSetTimeouts.reserve(minimumSize);
  }
}

void RTCPReceiveInformation::InsertTMMBRItem(
    const WebRtc_UWord32 senderSSRC,
    const RTCPUtility::RTCPPacketRTPFBTMMBRItem& TMMBRItem,
    const WebRtc_Word64 currentTimeMS) {
  
  for (WebRtc_UWord32 i = 0; i < TmmbrSet.lengthOfSet(); i++)  {
    if (TmmbrSet.Ssrc(i) == senderSSRC) {
      
      TmmbrSet.SetEntry(i,
                        TMMBRItem.MaxTotalMediaBitRate,
                        TMMBRItem.MeasuredOverhead,
                        senderSSRC);
      _tmmbrSetTimeouts[i] = currentTimeMS;
      return;
    }
  }
  VerifyAndAllocateTMMBRSet(TmmbrSet.lengthOfSet() + 1);
  TmmbrSet.AddEntry(TMMBRItem.MaxTotalMediaBitRate,
                    TMMBRItem.MeasuredOverhead,
                    senderSSRC);
  _tmmbrSetTimeouts.push_back(currentTimeMS);
}

WebRtc_Word32 RTCPReceiveInformation::GetTMMBRSet(
    const WebRtc_UWord32 sourceIdx,
    const WebRtc_UWord32 targetIdx,
    TMMBRSet* candidateSet,
    const WebRtc_Word64 currentTimeMS) {
  if (sourceIdx >= TmmbrSet.lengthOfSet()) {
    return -1;
  }
  if (targetIdx >= candidateSet->sizeOfSet()) {
    return -1;
  }
  
  if (currentTimeMS - _tmmbrSetTimeouts[sourceIdx] >
      5 * RTCP_INTERVAL_AUDIO_MS) {
    
    TmmbrSet.RemoveEntry(sourceIdx);
    _tmmbrSetTimeouts.erase(_tmmbrSetTimeouts.begin() + sourceIdx);
    return -1;
  }
  candidateSet->SetEntry(targetIdx,
                         TmmbrSet.Tmmbr(sourceIdx),
                         TmmbrSet.PacketOH(sourceIdx),
                         TmmbrSet.Ssrc(sourceIdx));
  return 0;
}

void RTCPReceiveInformation::VerifyAndAllocateBoundingSet(
    const WebRtc_UWord32 minimumSize) {
  TmmbnBoundingSet.VerifyAndAllocateSet(minimumSize);
}
} 
