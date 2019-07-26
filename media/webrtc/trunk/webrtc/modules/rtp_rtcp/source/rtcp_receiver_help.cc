









#include "webrtc/modules/rtp_rtcp/source/rtcp_receiver_help.h"

#include <assert.h>  
#include <string.h>  

#include "webrtc/modules/rtp_rtcp/source/rtp_utility.h"

namespace webrtc {
using namespace RTCPHelp;

RTCPPacketInformation::RTCPPacketInformation()
    : rtcpPacketTypeFlags(0),
      remoteSSRC(0),
      nackSequenceNumbers(),
      applicationSubType(0),
      applicationName(0),
      applicationData(),
      applicationLength(0),
      rtt(0),
      interArrivalJitter(0),
      sliPictureId(0),
      rpsiPictureId(0),
      receiverEstimatedMaxBitrate(0),
      ntp_secs(0),
      ntp_frac(0),
      rtp_timestamp(0),
      xr_originator_ssrc(0),
      xr_dlrr_item(false),
      VoIPMetric(NULL) {
}

RTCPPacketInformation::~RTCPPacketInformation()
{
    delete [] applicationData;
    delete VoIPMetric;
}

void
RTCPPacketInformation::AddVoIPMetric(const RTCPVoIPMetric* metric)
{
    VoIPMetric = new RTCPVoIPMetric();
    memcpy(VoIPMetric, metric, sizeof(RTCPVoIPMetric));
}

void RTCPPacketInformation::AddApplicationData(const uint8_t* data,
                                               const uint16_t size) {
    uint8_t* oldData = applicationData;
    uint16_t oldLength = applicationLength;

    
    uint16_t copySize = size;
    if (size > kRtcpAppCode_DATA_SIZE) {
        copySize = kRtcpAppCode_DATA_SIZE;
    }

    applicationLength += copySize;
    applicationData = new uint8_t[applicationLength];

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
  nackSequenceNumbers.clear();
}

void
RTCPPacketInformation::AddNACKPacket(const uint16_t packetID)
{
  if (nackSequenceNumbers.size() >= kSendSideNackListSizeSanity) {
    return;
  }
  nackSequenceNumbers.push_back(packetID);
}

void
RTCPPacketInformation::AddReportInfo(
    const RTCPReportBlockInformation& report_block_info)
{
  this->rtt = report_block_info.RTT;
  report_blocks.push_back(report_block_info.remoteReceiveBlock);
}

RTCPReportBlockInformation::RTCPReportBlockInformation():
    remoteReceiveBlock(),
    remoteMaxJitter(0),
    remotePacketsReceived(0),
    remoteOctetsReceived(0),
    lastReceivedRRNTPsecs(0),
    lastReceivedRRNTPfrac(0),
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
    const uint32_t minimumSize) {
  if (minimumSize > TmmbrSet.sizeOfSet()) {
    TmmbrSet.VerifyAndAllocateSetKeepingData(minimumSize);
    
    _tmmbrSetTimeouts.reserve(minimumSize);
  }
}

void RTCPReceiveInformation::InsertTMMBRItem(
    const uint32_t senderSSRC,
    const RTCPUtility::RTCPPacketRTPFBTMMBRItem& TMMBRItem,
    const int64_t currentTimeMS) {
  
  for (uint32_t i = 0; i < TmmbrSet.lengthOfSet(); i++)  {
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

int32_t RTCPReceiveInformation::GetTMMBRSet(
    const uint32_t sourceIdx,
    const uint32_t targetIdx,
    TMMBRSet* candidateSet,
    const int64_t currentTimeMS) {
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
    const uint32_t minimumSize) {
  TmmbnBoundingSet.VerifyAndAllocateSet(minimumSize);
}
}  
