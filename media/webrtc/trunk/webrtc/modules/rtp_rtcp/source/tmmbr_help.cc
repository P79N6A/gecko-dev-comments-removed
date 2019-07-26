









#include "tmmbr_help.h"

#include <assert.h>
#include <limits>
#include <string.h>
#include "rtp_rtcp_config.h"

namespace webrtc {
TMMBRSet::TMMBRSet() :
    _sizeOfSet(0),
    _lengthOfSet(0)
{
}

TMMBRSet::~TMMBRSet()
{
    _sizeOfSet = 0;
    _lengthOfSet = 0;
}

void
TMMBRSet::VerifyAndAllocateSet(WebRtc_UWord32 minimumSize)
{
    if(minimumSize > _sizeOfSet)
    {
        
        _data.resize(minimumSize);
        _sizeOfSet = minimumSize;
    }
    
    for(WebRtc_UWord32 i = 0; i < _sizeOfSet; i++)
    {
        _data.at(i).tmmbr = 0;
        _data.at(i).packet_oh = 0;
        _data.at(i).ssrc = 0;
    }
    _lengthOfSet = 0;
}

void
TMMBRSet::VerifyAndAllocateSetKeepingData(WebRtc_UWord32 minimumSize)
{
    if(minimumSize > _sizeOfSet)
    {
        {
          _data.resize(minimumSize);
        }
        _sizeOfSet = minimumSize;
    }
}

void TMMBRSet::SetEntry(unsigned int i,
                         WebRtc_UWord32 tmmbrSet,
                         WebRtc_UWord32 packetOHSet,
                         WebRtc_UWord32 ssrcSet) {
  assert(i < _sizeOfSet);
  _data.at(i).tmmbr = tmmbrSet;
  _data.at(i).packet_oh = packetOHSet;
  _data.at(i).ssrc = ssrcSet;
  if (i >= _lengthOfSet) {
    _lengthOfSet = i + 1;
  }
}

void TMMBRSet::AddEntry(WebRtc_UWord32 tmmbrSet,
                        WebRtc_UWord32 packetOHSet,
                        WebRtc_UWord32 ssrcSet) {
  assert(_lengthOfSet < _sizeOfSet);
  SetEntry(_lengthOfSet, tmmbrSet, packetOHSet, ssrcSet);
}

void TMMBRSet::RemoveEntry(WebRtc_UWord32 sourceIdx) {
  assert(sourceIdx < _lengthOfSet);
  _data.erase(_data.begin() + sourceIdx);
  _lengthOfSet--;
  _data.resize(_sizeOfSet);  
}

void TMMBRSet::SwapEntries(WebRtc_UWord32 i, WebRtc_UWord32 j) {
    SetElement temp;
    temp = _data[i];
    _data[i] = _data[j];
    _data[j] = temp;
}

void TMMBRSet::ClearEntry(WebRtc_UWord32 idx) {
  SetEntry(idx, 0, 0, 0);
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

    if(minimumSize > _boundingSet.sizeOfSet())
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
        _boundingSetToSend.clearSet();
        return 0;
    }

    VerifyAndAllocateBoundingSetToSend(boundingSetToSend->lengthOfSet());
    _boundingSetToSend.clearSet();
    for (WebRtc_UWord32 i = 0; i < boundingSetToSend->lengthOfSet(); i++)
    {
        
        WebRtc_UWord32 bitrate = boundingSetToSend->Tmmbr(i);
        if(maxBitrateKbit)
        {
            
            if(bitrate > maxBitrateKbit)
            {
                bitrate = maxBitrateKbit;
            }
        }
        _boundingSetToSend.SetEntry(i, bitrate,
                                    boundingSetToSend->PacketOH(i),
                                    boundingSetToSend->Ssrc(i));
    }
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
    candidateSet.VerifyAndAllocateSet(_candidateSet.sizeOfSet());

    
    for (WebRtc_UWord32 i = 0; i < _candidateSet.sizeOfSet(); i++)
    {
        if(_candidateSet.Tmmbr(i))
        {
            candidateSet.AddEntry(_candidateSet.Tmmbr(i),
                                  _candidateSet.PacketOH(i),
                                  _candidateSet.Ssrc(i));
        }
        else
        {
            
            assert(_candidateSet.PacketOH(i) == 0);
            
            
        }
    }

    
    WebRtc_Word32 numSetCandidates = candidateSet.lengthOfSet();
    
    WebRtc_UWord32 numBoundingSet = 0;
    if (numSetCandidates > 0)
    {
        numBoundingSet =  FindTMMBRBoundingSet(numSetCandidates, candidateSet);
        if(numBoundingSet < 1 || (numBoundingSet > _candidateSet.sizeOfSet()))
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
    VerifyAndAllocateBoundingSet(candidateSet.sizeOfSet());

    if (numCandidates == 1)
    {
        
        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet(); i++)
        {
            if (candidateSet.Tmmbr(i) > 0)
            {
                _boundingSet.AddEntry(candidateSet.Tmmbr(i),
                                    candidateSet.PacketOH(i),
                                    candidateSet.Ssrc(i));
                numBoundingSet++;
            }
        }
        if (numBoundingSet != 1)
        {
            numBoundingSet = -1;
        }
    } else
    {
        
        for (int i = candidateSet.sizeOfSet() - 1; i >= 0; i--)
        {
            for (int j = 1; j <= i; j++)
            {
                if (candidateSet.PacketOH(j-1) > candidateSet.PacketOH(j))
                {
                    candidateSet.SwapEntries(j-1, j);
                }
            }
        }
        
        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet(); i++)
        {
            if (candidateSet.Tmmbr(i) > 0)
            {
                
                WebRtc_UWord32 currentPacketOH = candidateSet.PacketOH(i);
                WebRtc_UWord32 currentMinTMMBR = candidateSet.Tmmbr(i);
                WebRtc_UWord32 currentMinIndexTMMBR = i;
                for (WebRtc_UWord32 j = i+1; j < candidateSet.sizeOfSet(); j++)
                {
                    if(candidateSet.PacketOH(j) == currentPacketOH)
                    {
                        if(candidateSet.Tmmbr(j) < currentMinTMMBR)
                        {
                            currentMinTMMBR = candidateSet.Tmmbr(j);
                            currentMinIndexTMMBR = j;
                        }
                    }
                }
                
                for (WebRtc_UWord32 j = 0; j < candidateSet.sizeOfSet(); j++)
                {
                  if(candidateSet.PacketOH(j) == currentPacketOH
                     && j != currentMinIndexTMMBR)
                    {
                        candidateSet.ClearEntry(j);
                    }
                }
            }
        }
        
        
        WebRtc_UWord32 minTMMBR = 0;
        WebRtc_UWord32 minIndexTMMBR = 0;
        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet(); i++)
        {
            if (candidateSet.Tmmbr(i) > 0)
            {
                minTMMBR = candidateSet.Tmmbr(i);
                minIndexTMMBR = i;
                break;
            }
        }

        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet(); i++)
        {
            if (candidateSet.Tmmbr(i) > 0 && candidateSet.Tmmbr(i) <= minTMMBR)
            {
                
                minTMMBR = candidateSet.Tmmbr(i);
                minIndexTMMBR = i;
            }
        }
        
        _boundingSet.SetEntry(numBoundingSet,
                              candidateSet.Tmmbr(minIndexTMMBR),
                              candidateSet.PacketOH(minIndexTMMBR),
                              candidateSet.Ssrc(minIndexTMMBR));

        
        _ptrIntersectionBoundingSet[numBoundingSet] = 0;
        
        _ptrMaxPRBoundingSet[numBoundingSet]
            = _boundingSet.Tmmbr(numBoundingSet) * 1000
            / float(8 * _boundingSet.PacketOH(numBoundingSet));
        numBoundingSet++;
        
        candidateSet.ClearEntry(minIndexTMMBR);
        numCandidates--;

        
        
        for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet(); i++)
        {
            if(candidateSet.Tmmbr(i) > 0
               && candidateSet.PacketOH(i) < _boundingSet.PacketOH(0))
            {
                candidateSet.ClearEntry(i);
                numCandidates--;
            }
        }

        if (numCandidates == 0)
        {
            
            assert(_boundingSet.lengthOfSet() == numBoundingSet);
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
                
                for (WebRtc_UWord32 i = 0; i < candidateSet.sizeOfSet(); i++)
                {
                    if (candidateSet.Tmmbr(i) > 0)
                    {
                        curCandidateTMMBR    = candidateSet.Tmmbr(i);
                        curCandidatePacketOH = candidateSet.PacketOH(i);
                        curCandidateSSRC     = candidateSet.Ssrc(i);
                        curCandidateIndex    = i;
                        candidateSet.ClearEntry(curCandidateIndex);
                        break;
                    }
                }
            }

            
            
            float packetRate
                = float(curCandidateTMMBR
                        - _boundingSet.Tmmbr(numBoundingSet-1))*1000
                / (8*(curCandidatePacketOH
                      - _boundingSet.PacketOH(numBoundingSet-1)));

            
            
            
            if(packetRate <= _ptrIntersectionBoundingSet[numBoundingSet-1])
            {
                
                numBoundingSet--;
                _boundingSet.ClearEntry(numBoundingSet);
                _ptrIntersectionBoundingSet[numBoundingSet] = 0;
                _ptrMaxPRBoundingSet[numBoundingSet]        = 0;
                getNewCandidate = false;
            } else
            {
                
                
                
                if (packetRate < _ptrMaxPRBoundingSet[numBoundingSet-1])
                {
                    _boundingSet.SetEntry(numBoundingSet,
                                          curCandidateTMMBR,
                                          curCandidatePacketOH,
                                          curCandidateSSRC);
                    _ptrIntersectionBoundingSet[numBoundingSet] = packetRate;
                    _ptrMaxPRBoundingSet[numBoundingSet]
                        = _boundingSet.Tmmbr(numBoundingSet)*1000
                        / float(8*_boundingSet.PacketOH(numBoundingSet));
                    numBoundingSet++;
                }
                numCandidates--;
                getNewCandidate = true;
            }

            
        } while (numCandidates > 0);
    }
    return numBoundingSet;
}

bool TMMBRHelp::IsOwner(const WebRtc_UWord32 ssrc,
                        const WebRtc_UWord32 length) const {
  CriticalSectionScoped lock(_criticalSection);

  if (length == 0) {
    
    return false;
  }
  for(WebRtc_UWord32 i = 0;
      (i < length) && (i < _boundingSet.sizeOfSet()); ++i) {
    if(_boundingSet.Ssrc(i) == ssrc) {
      return true;
    }
  }
  return false;
}

bool TMMBRHelp::CalcMinBitRate( WebRtc_UWord32* minBitrateKbit) const {
  CriticalSectionScoped lock(_criticalSection);

  if (_candidateSet.sizeOfSet() == 0) {
    
    return false;
  }
  *minBitrateKbit = std::numeric_limits<uint32_t>::max();

  for (WebRtc_UWord32 i = 0; i < _candidateSet.lengthOfSet(); ++i) {
    WebRtc_UWord32 curNetBitRateKbit = _candidateSet.Tmmbr(i);
    if (curNetBitRateKbit < MIN_VIDEO_BW_MANAGEMENT_BITRATE) {
      curNetBitRateKbit = MIN_VIDEO_BW_MANAGEMENT_BITRATE;
    }
    *minBitrateKbit = curNetBitRateKbit < *minBitrateKbit ?
        curNetBitRateKbit : *minBitrateKbit;
  }
  return true;
}
} 
