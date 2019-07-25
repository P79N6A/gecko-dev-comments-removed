









#include "rtcp_receiver_help.h"
#include "rtp_utility.h"

#include <string.h> 
#include <cassert> 

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

void
RTCPPacketInformation::AddApplicationData(const WebRtc_UWord8* data, const WebRtc_UWord16 size)
{
    WebRtc_UWord8* oldData = applicationData;
    WebRtc_UWord16 oldLength = applicationLength;

    
    WebRtc_UWord16 copySize = size;
    if (size > kRtcpAppCode_DATA_SIZE) {
        copySize = kRtcpAppCode_DATA_SIZE;
    }

    applicationLength += copySize;
    applicationData = new WebRtc_UWord8[applicationLength];

    if(oldData)
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
    if(NULL == nackSequenceNumbers)
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

RTCPReceiveInformation::RTCPReceiveInformation() :

    lastTimeReceived(0),
    lastFIRSequenceNumber(-1),
    lastFIRRequest(0),
    readyForDelete(false),
    _tmmbrSetTimeouts(NULL)
{
}

RTCPReceiveInformation::~RTCPReceiveInformation()
{
    if(_tmmbrSetTimeouts)
    {
        delete [] _tmmbrSetTimeouts;
    }
}


void
RTCPReceiveInformation::VerifyAndAllocateTMMBRSet(const WebRtc_UWord32 minimumSize)
{
    if(minimumSize > TmmbrSet.sizeOfSet)
    {
        
        WebRtc_UWord32* ptrTmmbrSet = new WebRtc_UWord32[minimumSize];
        WebRtc_UWord32* ptrTmmbrPacketOHSet = new WebRtc_UWord32[minimumSize];
        WebRtc_UWord32* ptrTmmbrSsrcSet = new WebRtc_UWord32[minimumSize];
        WebRtc_UWord32* tmmbrSetTimeouts = new WebRtc_UWord32[minimumSize];

        if(TmmbrSet.lengthOfSet > 0)
        {
            
            memcpy(ptrTmmbrSet, TmmbrSet.ptrTmmbrSet, sizeof(WebRtc_UWord32) * TmmbrSet.lengthOfSet);
            memcpy(ptrTmmbrPacketOHSet, TmmbrSet.ptrPacketOHSet, sizeof(WebRtc_UWord32) * TmmbrSet.lengthOfSet);
            memcpy(ptrTmmbrSsrcSet, TmmbrSet.ptrSsrcSet, sizeof(WebRtc_UWord32) * TmmbrSet.lengthOfSet);
            memcpy(tmmbrSetTimeouts, _tmmbrSetTimeouts, sizeof(WebRtc_UWord32) * TmmbrSet.lengthOfSet);
        }
        if(TmmbrSet.ptrTmmbrSet)
        {
            delete [] TmmbrSet.ptrTmmbrSet;
            delete [] TmmbrSet.ptrPacketOHSet;
            delete [] TmmbrSet.ptrSsrcSet;
        }
        if(_tmmbrSetTimeouts)
        {
            delete [] _tmmbrSetTimeouts;
        }
        TmmbrSet.ptrTmmbrSet = ptrTmmbrSet;
        TmmbrSet.ptrPacketOHSet = ptrTmmbrPacketOHSet;
        TmmbrSet.ptrSsrcSet = ptrTmmbrSsrcSet;
        TmmbrSet.sizeOfSet = minimumSize;
        _tmmbrSetTimeouts = tmmbrSetTimeouts;
    }
}

void
RTCPReceiveInformation::InsertTMMBRItem(const WebRtc_UWord32 senderSSRC,
                                        const RTCPUtility::RTCPPacketRTPFBTMMBRItem& TMMBRItem,
                                        const WebRtc_UWord32 currentTimeMS)
{
    
    for(WebRtc_UWord32 i = 0; i < TmmbrSet.lengthOfSet; i++)
    {
        if(TmmbrSet.ptrSsrcSet[i] == senderSSRC)
        {
            
            
            TmmbrSet.ptrPacketOHSet[i] = TMMBRItem.MeasuredOverhead;
            TmmbrSet.ptrTmmbrSet[i] = TMMBRItem.MaxTotalMediaBitRate;
            _tmmbrSetTimeouts[i] = currentTimeMS;
            return;
        }
    }
    VerifyAndAllocateTMMBRSet(TmmbrSet.lengthOfSet+1);

    const WebRtc_UWord32 idx = TmmbrSet.lengthOfSet;
    TmmbrSet.ptrPacketOHSet[idx] = TMMBRItem.MeasuredOverhead;
    TmmbrSet.ptrTmmbrSet[idx] = TMMBRItem.MaxTotalMediaBitRate;
    TmmbrSet.ptrSsrcSet[idx] = senderSSRC;
    _tmmbrSetTimeouts[idx] = currentTimeMS;
    TmmbrSet.lengthOfSet++;
}

WebRtc_Word32
RTCPReceiveInformation::GetTMMBRSet(const WebRtc_UWord32 sourceIdx,
                                    const WebRtc_UWord32 targetIdx,
                                    TMMBRSet* candidateSet,
                                    const WebRtc_UWord32 currentTimeMS)
{
    if(sourceIdx >= TmmbrSet.lengthOfSet)
    {
        return -1;
    }
    if(targetIdx >= candidateSet->sizeOfSet)
    {
        return -1;
    }
    WebRtc_UWord32 timeNow = currentTimeMS;

    
    if(timeNow - _tmmbrSetTimeouts[sourceIdx] > 5*RTCP_INTERVAL_AUDIO_MS)
    {
        
        const WebRtc_UWord32 move = TmmbrSet.lengthOfSet - (sourceIdx + 1);
        if(move > 0)
        {
            memmove(&(TmmbrSet.ptrTmmbrSet[sourceIdx]), &(TmmbrSet.ptrTmmbrSet[sourceIdx+1]), move* sizeof(WebRtc_UWord32));
            memmove(&(TmmbrSet.ptrPacketOHSet[sourceIdx]),&(TmmbrSet.ptrPacketOHSet[sourceIdx+1]), move* sizeof(WebRtc_UWord32));
            memmove(&(TmmbrSet.ptrSsrcSet[sourceIdx]),&(TmmbrSet.ptrSsrcSet[sourceIdx+1]), move* sizeof(WebRtc_UWord32));
            memmove(&(_tmmbrSetTimeouts[sourceIdx]),&(_tmmbrSetTimeouts[sourceIdx+1]), move* sizeof(WebRtc_UWord32));
        }
        TmmbrSet.lengthOfSet--;
        return -1;
    }

    candidateSet->ptrTmmbrSet[targetIdx] = TmmbrSet.ptrTmmbrSet[sourceIdx];
    candidateSet->ptrPacketOHSet[targetIdx] = TmmbrSet.ptrPacketOHSet[sourceIdx];
    candidateSet->ptrSsrcSet[targetIdx] = TmmbrSet.ptrSsrcSet[sourceIdx];
    return 0;
}

void RTCPReceiveInformation::VerifyAndAllocateBoundingSet(const WebRtc_UWord32 minimumSize)
{
    TmmbnBoundingSet.VerifyAndAllocateSet(minimumSize);
}
} 
