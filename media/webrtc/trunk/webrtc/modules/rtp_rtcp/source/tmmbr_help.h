









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_TMMBR_HELP_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_TMMBR_HELP_H_

#include <vector>

#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class TMMBRSet
{
public:
    TMMBRSet();
    ~TMMBRSet();

    void VerifyAndAllocateSet(uint32_t minimumSize);
    void VerifyAndAllocateSetKeepingData(uint32_t minimumSize);
    
    uint32_t lengthOfSet() const { return _lengthOfSet; }
    
    uint32_t sizeOfSet() const { return _sizeOfSet; }
    void clearSet() {
      _lengthOfSet = 0;
    }
    uint32_t Tmmbr(int i) const {
      return _data.at(i).tmmbr;
    }
    uint32_t PacketOH(int i) const {
      return _data.at(i).packet_oh;
    }
    uint32_t Ssrc(int i) const {
      return _data.at(i).ssrc;
    }
    void SetEntry(unsigned int i,
                  uint32_t tmmbrSet,
                  uint32_t packetOHSet,
                  uint32_t ssrcSet);

    void AddEntry(uint32_t tmmbrSet,
                  uint32_t packetOHSet,
                  uint32_t ssrcSet);

    
    void RemoveEntry(uint32_t sourceIdx);

    void SwapEntries(uint32_t firstIdx,
                     uint32_t secondIdx);

    
    void ClearEntry(uint32_t idx);

 private:
    class SetElement {
      public:
        SetElement() : tmmbr(0), packet_oh(0), ssrc(0) {}
        uint32_t tmmbr;
        uint32_t packet_oh;
        uint32_t ssrc;
    };

    std::vector<SetElement> _data;
    
    uint32_t    _sizeOfSet;
    
    uint32_t    _lengthOfSet;
};

class TMMBRHelp
{
public:
    TMMBRHelp();
    virtual ~TMMBRHelp();

    TMMBRSet* BoundingSet(); 
    TMMBRSet* CandidateSet();
    TMMBRSet* BoundingSetToSend();

    TMMBRSet* VerifyAndAllocateCandidateSet(const uint32_t minimumSize);
    int32_t FindTMMBRBoundingSet(TMMBRSet*& boundingSet);
    int32_t SetTMMBRBoundingSetToSend(
        const TMMBRSet* boundingSetToSend,
        const uint32_t maxBitrateKbit);

    bool IsOwner(const uint32_t ssrc, const uint32_t length) const;

    bool CalcMinBitRate(uint32_t* minBitrateKbit) const;

protected:
    TMMBRSet*   VerifyAndAllocateBoundingSet(uint32_t minimumSize);
    int32_t VerifyAndAllocateBoundingSetToSend(uint32_t minimumSize);

    int32_t FindTMMBRBoundingSet(int32_t numCandidates, TMMBRSet& candidateSet);

private:
    CriticalSectionWrapper* _criticalSection;
    TMMBRSet                _candidateSet;
    TMMBRSet                _boundingSet;
    TMMBRSet                _boundingSetToSend;

    float*                  _ptrIntersectionBoundingSet;
    float*                  _ptrMaxPRBoundingSet;
};
}  

#endif 
