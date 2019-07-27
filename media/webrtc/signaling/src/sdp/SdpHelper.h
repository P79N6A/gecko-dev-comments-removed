



#ifndef _SDPHELPER_H_
#define _SDPHELPER_H_

#include "signaling/src/sdp/SdpMediaSection.h"
#include "signaling/src/sdp/SdpAttribute.h"

#include "m_cpp_utils.h"

#include <string>
#include <map>
#include <vector>

namespace mozilla {
class SdpMediaSection;
class Sdp;

class SdpHelper {
  public:
    
    
    explicit SdpHelper(std::string* errorDest) : mLastError(*errorDest) {}
    ~SdpHelper() {}

    nsresult GetComponent(const std::string& candidate, size_t* component);
    nsresult CopyTransportParams(size_t numComponents,
                                 const SdpMediaSection& source,
                                 SdpMediaSection* dest);
    bool AreOldTransportParamsValid(const Sdp& oldAnswer,
                                    const Sdp& newOffer,
                                    size_t level);

    bool MsectionIsDisabled(const SdpMediaSection& msection) const;
    void DisableMsection(Sdp* sdp, SdpMediaSection* msection) const;

    
    
    typedef std::map<std::string, const SdpMediaSection*> BundledMids;

    nsresult GetBundledMids(const Sdp& sdp, BundledMids* bundledMids);

    bool IsBundleSlave(const Sdp& localSdp, uint16_t level);
    void GetBundleGroups(
        const Sdp& sdp,
        std::vector<SdpGroupAttributeList::Group>* groups) const;

    nsresult GetIdsFromMsid(const Sdp& sdp,
                            const SdpMediaSection& msection,
                            std::string* streamId,
                            std::string* trackId);
    nsresult GetMsids(const SdpMediaSection& msection,
                      std::vector<SdpMsidAttributeList::Msid>* msids);
    nsresult ParseMsid(const std::string& msidAttribute,
                       std::string* streamId,
                       std::string* trackId);
    nsresult AddCandidateToSdp(Sdp* sdp,
                               const std::string& candidate,
                               const std::string& mid,
                               uint16_t level);
    void SetDefaultAddresses(const std::string& defaultCandidateAddr,
                             uint16_t defaultCandidatePort,
                             const std::string& defaultRtcpCandidateAddr,
                             uint16_t defaultRtcpCandidatePort,
                             SdpMediaSection* msection);
    void SetupMsidSemantic(const std::vector<std::string>& msids,
                           Sdp* sdp) const;

    std::string GetCNAME(const SdpMediaSection& msection) const;
    void SetSsrcs(const std::vector<uint32_t>& ssrcs,
                  const std::string& cname,
                  SdpMediaSection* msection) const;

    SdpMediaSection* FindMsectionByMid(Sdp& sdp,
                                       const std::string& mid) const;

    const SdpMediaSection* FindMsectionByMid(const Sdp& sdp,
                                             const std::string& mid) const;

    nsresult CopyStickyParams(const SdpMediaSection& source,
                              SdpMediaSection* dest);
    bool HasRtcp(SdpMediaSection::Protocol proto) const;
    SdpMediaSection::Protocol GetProtocolForMediaType(
        SdpMediaSection::MediaType type);
    void appendSdpParseErrors(
          const std::vector<std::pair<size_t, std::string> >& aErrors,
          std::string* aErrorString);

  private:
    std::string& mLastError;

    DISALLOW_COPY_ASSIGN(SdpHelper);
};
} 

#endif 

