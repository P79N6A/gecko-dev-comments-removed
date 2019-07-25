









#include "tmmbr_help.h"

#include <limits>
#include "rtp_rtcp_config.h"

namespace webrtc {
TMMBRSet::TMMBRSet() :
    ptrTmmbrSet(0),
    ptrPacketOHSet(0),
    ptrSsrcSet(0),
    sizeOfSet(0),
    lengthOfSet(0)
{
}

TMMBRSet::~TMMBRSet()
{
    delete [] ptrTmmbrSet;
    delete [] ptrPacketOHSet;
    delete [] ptrSsrcSet;
    ptrTmmbrSet = 0;
    ptrPacketOHSet = 0;
    ptrSsrcSet = 0;
    sizeOfSet = 0;
    lengthOfSet = 0;
}

void
TMMBRSet::VerifyAndAllocateSet(WebRtc_UWord32 minimumSize)
{
    if(minimumSize > sizeOfSet)
    {
        
        if(ptrTmmbrSet)
        {
            delete [] ptrTmmbrSet;
            delete [] ptrPacketOHSet;
            delete [] ptrSsrcSet;
        }
        ptrTmmbrSet = new WebRtc_UWord32[minimumSize];
        ptrPacketOHSet = new WebRtc_UWord32[minimumSize];
        ptrSsrcSet = new WebRtc_UWord32[minimumSize];
        sizeOfSet = minimumSize;
    }
    
    for(WebRtc_UWord32 i = 0; i < sizeOfSet; i++)
    {
        ptrTmmbrSet[i] = 0;
        ptrPacketOHSet[i] = 0;
        ptrSsrcSet[i] = 0;
    }
    lengthOfSet = 0;
}

TMMBRHelp::TMMBRHelp()
    : _criticalSection(CriticalSectionWrapper::CreateCriticalSection()),
      _candidateSet(),
      _boundingSet(),
      _boundingSetToSend(),
      _ptrIntersectionBoundingSet(NULL),
      _ptrMaxPRBoundingSet(NULL) {
}

TMMBRHelp::~TMMBRHelp() {
  delete [] _ptrIntersectionBoundingSet;
  delete [] _ptrMaxPRBoundingSet;
  _ptrIntersectionBoundingSet = 0;
  _ptrMaxPRBoundingSet = 0;
  delete _criticalSection;
}

TMMBRSet*
TMMBRHelp::VerifyAndAllocateBoundingSet(WebRtc_UWord32 minimumSize)
{
    CriticalSectionScoped lock(_criticalSection);

    if(minimumSize > _boundingSet.sizeOfSet)
    {
        
        if(_ptrIntersectionBoundingSet)
        {
            delete [] _ptrIntersectionBoundingSet;
            delete [] _ptrMaxPRBoundingSet;
        }
        _ptrIntersectionBoundingSet = new float[minimumSize];
        _ptrMaxPRBoundingSet = new float[minimumSize];
    }
    _boundingSet.VerifyAndAllocateSet(minimumSize);
    return &_boundingSet;
}

TMMBRSet* TMMBRHelp::BoundingSet() {
  return &_boundingSet;
}

WebRtc_Word32
TMMBRHelp::SetTMMBRBoundingSetToSend(const TMMBRSet* boundingSetToSend,
                                     const WebRtc_UWord32 maxBitrateKbit)
{
    CriticalSectionScoped lock(_criticalSection);

    if (boundingSetToSend == NULL)
    {
        _boundingSetToSend.lengthOfSet = 0;
        return 0;
    }

    VerifyAndAllocateBoundingSetToSend(boundingSetToSend->lengthOfSet);

    for (WebRtc_UWord32 i = 0; i < boundingSetToSend->lengthOfSet; i++)
    {
        
        WebRtc_UWord32 bitrate = boundingSetToSend->ptrTmmbrSet[i];
        if(maxBitrateKbit)
        {
            
            if(bitrate > maxBitrateKbit)
            {
                bitrate = maxBitrateKbit;
            }
        }

        _boundingSetToSend.ptrTmmbrSet[i]    = bitrate;
        _boundingSetToSend.ptrPacketOHSet[i] = boundingSetToSend->ptrPacketOHSet[i];
        _boundingSetToSend.ptrSsrcSet[i]     = boundingSetToSend->ptrSsrcSet[i];
    }
    _boundingSetToSend.lengthOfSet = boundingSetToSend->lengthOfSet;
    return 0;
}

WebRtc_Word32
TMMBRHelp::VerifyAndAllocateBoundingSetToSend(WebRtc_UWord32 minimumSize)
{
    CriticalSectionScoped lock(_criticalSection);

    _boundingSetToSend.VerifyAndAllocateSet(minimumSize);
    return 0;
}

TMMBRSet*
TMMBRHelp::VerifyAndAllocateCandidateSet(WebRtc_UWord32 minimumSize)
{
    CriticalSectionScoped lock(_criticalSection);

    _candidateSet.VerifyAndAllocateSet(minimumSize);
    return &_candidateSet;
}

TMMBRSet*
TMMBRHelp::CandidateSet()
{
    return &_candidateSet;
}

TMMBRSet*
TMMBRHelp::BoundingSetToSend()
{
    return &_boundingSetToSend;
}

WebRtc_Word32
TMMBRHelp::FindTMMBRBoundingSet(TMMBRSet*& boundingSet)
{
    CriticalSectionScoped lock(_criticalSection);

    
    TMMBRSet    candidateSet;
    candidateSet.VerifyAndAllocateSet(_candidateSet.sizeOfSet);

    
    WebRtc_Word32 numSetCandidates = 0;
    for (WebRtc_UWord32 i = 0; i < _candidateSet.sizeOfSet; i++)
    {
        if(_candidateSet.ptrTmmbrSet[i])
        {
            numSetCandidates++;
            candidateSet.ptrTmmbrSet[i]    = _candidateSet.ptrTmmbrSet[i];
            candidateSet.ptrPacketOHSet[i] = _candidateSet.ptrPacketOHSet[i];
            candidateSet.ptrSsrcSet[i]     = _candidateSet.ptrSsrcSet[i];
        }
        else
        {
            
            _candidateSet.ptrPacketOHSet[i] = 0;
        }
    }
    candidateSet.lengthOfSet = numSetCandidates;

    
    WebRtc_UWord32 numBoundingSet = 0;
    if (numSetCandidates > 0)
    {
        numBoundingSet =  FindTMMBRBoundingSet(numSetCandidates, candidateSet);
        if(numBoundingSet < 1 || (numBoundingSet > _candidateSet.sizeOfSet))
        {
            return -1;
        }
        boundingSet = &_boundingSet;
    }
    return numBoundingSet;
}


WebRtc_Word32
TMMBRHelp::FindTMMBRBoundingSet(WebRtc_Word32 numCandidates, TMMBRSet& candidateSet)
{
    CriticalSectionScoped lock(_criticalSection);

    WebRtc_UWord32 numBoundingSet = 0;
    VerifyAndAllocateBoundingSet(candidateSet.sizeOfSet);

    if (numCandidates == 1)
    {
        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet; i++)
        {
            if(candidateSet.ptrTmmbrSet[i] > 0)
            {
                _boundingSet.ptrTmmbrSet[numBoundingSet]    = candidateSet.ptrTmmbrSet[i];
                _boundingSet.ptrPacketOHSet[numBoundingSet] = candidateSet.ptrPacketOHSet[i];
                _boundingSet.ptrSsrcSet[numBoundingSet]     = candidateSet.ptrSsrcSet[i];
                numBoundingSet++;
            }
        }
        if (numBoundingSet != 1)
        {
            numBoundingSet = -1;
        }
    } else
    {
        
        WebRtc_UWord32 temp;
        for (int i = candidateSet.sizeOfSet - 1; i >= 0; i--)
        {
            for (int j = 1; j <= i; j++)
            {
                if (candidateSet.ptrPacketOHSet[j-1] > candidateSet.ptrPacketOHSet[j])
                {
                    temp = candidateSet.ptrPacketOHSet[j-1];
                    candidateSet.ptrPacketOHSet[j-1] = candidateSet.ptrPacketOHSet[j];
                    candidateSet.ptrPacketOHSet[j] = temp;
                    temp = candidateSet.ptrTmmbrSet[j-1];
                    candidateSet.ptrTmmbrSet[j-1] = candidateSet.ptrTmmbrSet[j];
                    candidateSet.ptrTmmbrSet[j] = temp;
                    temp = candidateSet.ptrSsrcSet[j-1];
                    candidateSet.ptrSsrcSet[j-1] = candidateSet.ptrSsrcSet[j];
                    candidateSet.ptrSsrcSet[j] = temp;
                }
            }
        }
        
        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet; i++)
        {
            if (candidateSet.ptrTmmbrSet[i] > 0)
            {
                
                WebRtc_UWord32 currentPacketOH = candidateSet.ptrPacketOHSet[i];
                WebRtc_UWord32 currentMinTMMBR = candidateSet.ptrTmmbrSet[i];
                WebRtc_UWord32 currentMinIndexTMMBR = i;
                for (WebRtc_UWord32 j = i+1; j < candidateSet.sizeOfSet; j++)
                {
                    if(candidateSet.ptrPacketOHSet[j] == currentPacketOH)
                    {
                        if(candidateSet.ptrTmmbrSet[j] < currentMinTMMBR)
                        {
                            currentMinTMMBR = candidateSet.ptrTmmbrSet[j];
                            currentMinIndexTMMBR = j;
                        }
                    }
                }
                
                for (WebRtc_UWord32 j = 0; j < candidateSet.sizeOfSet; j++)
                {
                    if(candidateSet.ptrPacketOHSet[j] == currentPacketOH && j != currentMinIndexTMMBR)
                    {
                        candidateSet.ptrTmmbrSet[j]    = 0;
                        candidateSet.ptrPacketOHSet[j] = 0;
                        candidateSet.ptrSsrcSet[j]     = 0;
                        numCandidates--;
                    }
                }
            }
        }
        
        WebRtc_UWord32 minTMMBR = 0;
        WebRtc_UWord32 minIndexTMMBR = 0;
        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet; i++)
        {
            if (candidateSet.ptrTmmbrSet[i] > 0)
            {
                minTMMBR = candidateSet.ptrTmmbrSet[i];
                minIndexTMMBR = i;
                break;
            }
        }

        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet; i++)
        {
            if (candidateSet.ptrTmmbrSet[i] > 0 && candidateSet.ptrTmmbrSet[i] <= minTMMBR)
            {
                
                minTMMBR = candidateSet.ptrTmmbrSet[i];
                minIndexTMMBR = i;
            }
        }
        
        _boundingSet.ptrTmmbrSet[numBoundingSet]    = candidateSet.ptrTmmbrSet[minIndexTMMBR];
        _boundingSet.ptrPacketOHSet[numBoundingSet] = candidateSet.ptrPacketOHSet[minIndexTMMBR];
        _boundingSet.ptrSsrcSet[numBoundingSet]     = candidateSet.ptrSsrcSet[minIndexTMMBR];
        
        _ptrIntersectionBoundingSet[numBoundingSet] = 0;
        
        _ptrMaxPRBoundingSet[numBoundingSet] = _boundingSet.ptrTmmbrSet[numBoundingSet]*1000 / float(8*_boundingSet.ptrPacketOHSet[numBoundingSet]);
        numBoundingSet++;
        
        candidateSet.ptrTmmbrSet[minIndexTMMBR]    = 0;
        candidateSet.ptrPacketOHSet[minIndexTMMBR] = 0;
        candidateSet.ptrSsrcSet[minIndexTMMBR]     = 0;
        numCandidates--;

        
        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet; i++)
        {
            if(candidateSet.ptrTmmbrSet[i] > 0 && candidateSet.ptrPacketOHSet[i] < _boundingSet.ptrPacketOHSet[0])
            {
                candidateSet.ptrTmmbrSet[i]    = 0;
                candidateSet.ptrPacketOHSet[i] = 0;
                candidateSet.ptrSsrcSet[i]     = 0;
                numCandidates--;
            }
        }

        if (numCandidates == 0)
        {
            _boundingSet.lengthOfSet = numBoundingSet;
            return numBoundingSet;
        }

        bool getNewCandidate = true;
        int curCandidateTMMBR = 0;
        int curCandidateIndex = 0;
        int curCandidatePacketOH = 0;
        int curCandidateSSRC = 0;
        do
        {
            if (getNewCandidate)
            {
                
                for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet; i++)
                {
                    if (candidateSet.ptrTmmbrSet[i] > 0)
                    {
                        curCandidateTMMBR    = candidateSet.ptrTmmbrSet[i];
                        curCandidatePacketOH = candidateSet.ptrPacketOHSet[i];
                        curCandidateSSRC     = candidateSet.ptrSsrcSet[i];
                        curCandidateIndex    = i;
                        candidateSet.ptrTmmbrSet[curCandidateIndex]    = 0;
                        candidateSet.ptrPacketOHSet[curCandidateIndex] = 0;
                        candidateSet.ptrSsrcSet[curCandidateIndex]     = 0;
                        break;
                    }
                }
            }

            
            float packetRate = float(curCandidateTMMBR - _boundingSet.ptrTmmbrSet[numBoundingSet-1])*1000 / (8*(curCandidatePacketOH - _boundingSet.ptrPacketOHSet[numBoundingSet-1]));

            
            
            if(packetRate <= _ptrIntersectionBoundingSet[numBoundingSet-1])
            {
                
                numBoundingSet--;
                _boundingSet.ptrTmmbrSet[numBoundingSet]    = 0;
                _boundingSet.ptrPacketOHSet[numBoundingSet] = 0;
                _boundingSet.ptrSsrcSet[numBoundingSet]     = 0;
                _ptrIntersectionBoundingSet[numBoundingSet] = 0;
                _ptrMaxPRBoundingSet[numBoundingSet]        = 0;
                getNewCandidate = false;
            } else
            {
                
                if (packetRate < _ptrMaxPRBoundingSet[numBoundingSet-1])
                {
                    _boundingSet.ptrTmmbrSet[numBoundingSet]    = curCandidateTMMBR;
                    _boundingSet.ptrPacketOHSet[numBoundingSet] = curCandidatePacketOH;
                    _boundingSet.ptrSsrcSet[numBoundingSet]     = curCandidateSSRC;
                    _ptrIntersectionBoundingSet[numBoundingSet] = packetRate;
                    _ptrMaxPRBoundingSet[numBoundingSet] = _boundingSet.ptrTmmbrSet[numBoundingSet]*1000 / float(8*_boundingSet.ptrPacketOHSet[numBoundingSet]);
                    numBoundingSet++;
                }
                numCandidates--;
                getNewCandidate = true;
            }

            
        } while (numCandidates > 0);
    }
    _boundingSet.lengthOfSet = numBoundingSet;
    return numBoundingSet;
}

bool TMMBRHelp::IsOwner(const WebRtc_UWord32 ssrc,
                        const WebRtc_UWord32 length) const {
  CriticalSectionScoped lock(_criticalSection);

  if (length == 0) {
    
    return false;
  }
  for(WebRtc_UWord32 i = 0; (i < length) && (i < _boundingSet.sizeOfSet); ++i) {
    if(_boundingSet.ptrSsrcSet[i] == ssrc) {
      return true;
    }
  }
  return false;
}

bool TMMBRHelp::CalcMinBitRate( WebRtc_UWord32* minBitrateKbit) const {
  CriticalSectionScoped lock(_criticalSection);

  if (_candidateSet.sizeOfSet == 0) {
    
    return false;
  }
  *minBitrateKbit = std::numeric_limits<uint32_t>::max();

  for (WebRtc_UWord32 i = 0; i < _candidateSet.sizeOfSet; ++i) {
    WebRtc_UWord32 curNetBitRateKbit = _candidateSet.ptrTmmbrSet[i];
    if (curNetBitRateKbit < MIN_VIDEO_BW_MANAGEMENT_BITRATE) {
      curNetBitRateKbit = MIN_VIDEO_BW_MANAGEMENT_BITRATE;
    }
    *minBitrateKbit = curNetBitRateKbit < *minBitrateKbit ?
        curNetBitRateKbit : *minBitrateKbit;
  }
  return true;
}
} 
