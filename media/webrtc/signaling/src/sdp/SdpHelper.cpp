



#include "signaling/src/sdp/SdpHelper.h"

#include "signaling/src/sdp/Sdp.h"
#include "signaling/src/sdp/SdpMediaSection.h"
#include "logging.h"

#include "nsDebug.h"
#include "nsError.h"
#include "nspr/prprf.h"

#include <set>

namespace mozilla {
MOZ_MTLOG_MODULE("sdp")

#define SDP_SET_ERROR(error)                                                   \
  do {                                                                         \
    std::ostringstream os;                                                     \
    os << error;                                                               \
    mLastError = os.str();                                                     \
    MOZ_MTLOG(ML_ERROR, mLastError);                                           \
  } while (0);

nsresult
SdpHelper::CopyTransportParams(size_t numComponents,
                               const SdpMediaSection& oldLocal,
                               SdpMediaSection* newLocal)
{
  
  newLocal->SetPort(oldLocal.GetPort());
  newLocal->GetConnection() = oldLocal.GetConnection();

  const SdpAttributeList& oldLocalAttrs = oldLocal.GetAttributeList();
  SdpAttributeList& newLocalAttrs = newLocal->GetAttributeList();

  
  if (oldLocalAttrs.HasAttribute(SdpAttribute::kCandidateAttribute) &&
      numComponents) {
    UniquePtr<SdpMultiStringAttribute> candidateAttrs(
      new SdpMultiStringAttribute(SdpAttribute::kCandidateAttribute));
    for (const std::string& candidate : oldLocalAttrs.GetCandidate()) {
      size_t component;
      nsresult rv = GetComponent(candidate, &component);
      NS_ENSURE_SUCCESS(rv, rv);
      if (numComponents >= component) {
        candidateAttrs->mValues.push_back(candidate);
      }
    }
    if (candidateAttrs->mValues.size()) {
      newLocalAttrs.SetAttribute(candidateAttrs.release());
    }
  }

  if (numComponents == 2 &&
      oldLocalAttrs.HasAttribute(SdpAttribute::kRtcpAttribute)) {
    
    newLocalAttrs.SetAttribute(new SdpRtcpAttribute(oldLocalAttrs.GetRtcp()));
  }

  return NS_OK;
}

bool
SdpHelper::AreOldTransportParamsValid(const Sdp& oldAnswer,
                                      const Sdp& newOffer,
                                      size_t level)
{
  if (MsectionIsDisabled(oldAnswer.GetMediaSection(level)) ||
      MsectionIsDisabled(newOffer.GetMediaSection(level))) {
    
    return false;
  }

  if (IsBundleSlave(oldAnswer, level)) {
    
    
    return false;
  }

  if (newOffer.GetMediaSection(level).GetAttributeList().HasAttribute(
        SdpAttribute::kBundleOnlyAttribute) &&
      IsBundleSlave(newOffer, level)) {
    
    
    return false;
  }

  
  

  return true;
}

nsresult
SdpHelper::GetComponent(const std::string& candidate, size_t* component)
{
  unsigned int temp;
  int32_t result = PR_sscanf(candidate.c_str(), "%*s %u", &temp);
  if (result == 1) {
    *component = temp;
    return NS_OK;
  }
  SDP_SET_ERROR("Malformed ICE candidate: " << candidate);
  return NS_ERROR_INVALID_ARG;
}

bool
SdpHelper::MsectionIsDisabled(const SdpMediaSection& msection) const
{
  return !msection.GetPort() &&
         !msection.GetAttributeList().HasAttribute(
             SdpAttribute::kBundleOnlyAttribute);
}

void
SdpHelper::DisableMsection(Sdp* sdp, SdpMediaSection* msection) const
{
  
  if (msection->GetAttributeList().HasAttribute(SdpAttribute::kMidAttribute)) {
    std::string mid = msection->GetAttributeList().GetMid();
    if (sdp->GetAttributeList().HasAttribute(SdpAttribute::kGroupAttribute)) {
      UniquePtr<SdpGroupAttributeList> newGroupAttr(new SdpGroupAttributeList(
            sdp->GetAttributeList().GetGroup()));
      newGroupAttr->RemoveMid(mid);
      sdp->GetAttributeList().SetAttribute(newGroupAttr.release());
    }
  }

  
  msection->GetAttributeList().Clear();

  auto* direction =
    new SdpDirectionAttribute(SdpDirectionAttribute::kInactive);
  msection->GetAttributeList().SetAttribute(direction);
  msection->SetPort(0);
}

void
SdpHelper::GetBundleGroups(
    const Sdp& sdp,
    std::vector<SdpGroupAttributeList::Group>* bundleGroups) const
{
  if (sdp.GetAttributeList().HasAttribute(SdpAttribute::kGroupAttribute)) {
    for (auto& group : sdp.GetAttributeList().GetGroup().mGroups) {
      if (group.semantics == SdpGroupAttributeList::kBundle) {
        bundleGroups->push_back(group);
      }
    }
  }
}

nsresult
SdpHelper::GetBundledMids(const Sdp& sdp, BundledMids* bundledMids)
{
  std::vector<SdpGroupAttributeList::Group> bundleGroups;
  GetBundleGroups(sdp, &bundleGroups);

  for (SdpGroupAttributeList::Group& group : bundleGroups) {
    if (group.tags.empty()) {
      SDP_SET_ERROR("Empty BUNDLE group");
      return NS_ERROR_INVALID_ARG;
    }

    const SdpMediaSection* masterBundleMsection(
        FindMsectionByMid(sdp, group.tags[0]));

    if (!masterBundleMsection) {
      SDP_SET_ERROR("mid specified for bundle transport in group attribute"
          " does not exist in the SDP. (mid=" << group.tags[0] << ")");
      return NS_ERROR_INVALID_ARG;
    }

    if (MsectionIsDisabled(*masterBundleMsection)) {
      SDP_SET_ERROR("mid specified for bundle transport in group attribute"
          " points at a disabled m-section. (mid=" << group.tags[0] << ")");
      return NS_ERROR_INVALID_ARG;
    }

    for (const std::string& mid : group.tags) {
      if (bundledMids->count(mid)) {
        SDP_SET_ERROR("mid \'" << mid << "\' appears more than once in a "
                       "BUNDLE group");
        return NS_ERROR_INVALID_ARG;
      }

      (*bundledMids)[mid] = masterBundleMsection;
    }
  }

  return NS_OK;
}

bool
SdpHelper::IsBundleSlave(const Sdp& sdp, uint16_t level)
{
  auto& msection = sdp.GetMediaSection(level);

  if (!msection.GetAttributeList().HasAttribute(SdpAttribute::kMidAttribute)) {
    
    return false;
  }
  std::string mid(msection.GetAttributeList().GetMid());

  BundledMids bundledMids;
  nsresult rv = GetBundledMids(sdp, &bundledMids);
  if (NS_FAILED(rv)) {
    
    MOZ_ASSERT(false);
    return false;
  }

  if (bundledMids.count(mid) && level != bundledMids[mid]->GetLevel()) {
    
    return true;
  }

  return false;
}

nsresult
SdpHelper::AddCandidateToSdp(Sdp* sdp,
                             const std::string& candidateUntrimmed,
                             const std::string& mid,
                             uint16_t level)
{

  if (level >= sdp->GetMediaSectionCount()) {
    SDP_SET_ERROR("Index " << level << " out of range");
    return NS_ERROR_INVALID_ARG;
  }

  
  size_t begin = candidateUntrimmed.find(':');
  if (begin == std::string::npos) {
    SDP_SET_ERROR("Invalid candidate, no ':' (" << candidateUntrimmed << ")");
    return NS_ERROR_INVALID_ARG;
  }
  ++begin;

  std::string candidate = candidateUntrimmed.substr(begin);

  

  SdpMediaSection& msection = sdp->GetMediaSection(level);
  SdpAttributeList& attrList = msection.GetAttributeList();

  UniquePtr<SdpMultiStringAttribute> candidates;
  if (!attrList.HasAttribute(SdpAttribute::kCandidateAttribute)) {
    
    candidates.reset(
        new SdpMultiStringAttribute(SdpAttribute::kCandidateAttribute));
  } else {
    
    candidates.reset(new SdpMultiStringAttribute(
        *static_cast<const SdpMultiStringAttribute*>(
            attrList.GetAttribute(SdpAttribute::kCandidateAttribute))));
  }
  candidates->PushEntry(candidate);
  attrList.SetAttribute(candidates.release());

  return NS_OK;
}

void
SdpHelper::SetDefaultAddresses(const std::string& defaultCandidateAddr,
                               uint16_t defaultCandidatePort,
                               const std::string& defaultRtcpCandidateAddr,
                               uint16_t defaultRtcpCandidatePort,
                               SdpMediaSection* msection)
{
  msection->GetConnection().SetAddress(defaultCandidateAddr);
  msection->SetPort(defaultCandidatePort);

  if (!defaultRtcpCandidateAddr.empty()) {
    sdp::AddrType ipVersion = sdp::kIPv4;
    if (defaultRtcpCandidateAddr.find(':') != std::string::npos) {
      ipVersion = sdp::kIPv6;
    }
    msection->GetAttributeList().SetAttribute(new SdpRtcpAttribute(
          defaultRtcpCandidatePort,
          sdp::kInternet,
          ipVersion,
          defaultRtcpCandidateAddr));
  }
}

nsresult
SdpHelper::GetIdsFromMsid(const Sdp& sdp,
                                const SdpMediaSection& msection,
                                std::string* streamId,
                                std::string* trackId)
{
  if (!sdp.GetAttributeList().HasAttribute(
        SdpAttribute::kMsidSemanticAttribute)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  auto& msidSemantics = sdp.GetAttributeList().GetMsidSemantic().mMsidSemantics;
  std::vector<SdpMsidAttributeList::Msid> allMsids;
  nsresult rv = GetMsids(msection, &allMsids);
  NS_ENSURE_SUCCESS(rv, rv);

  bool allMsidsAreWebrtc = false;
  std::set<std::string> webrtcMsids;

  for (auto i = msidSemantics.begin(); i != msidSemantics.end(); ++i) {
    if (i->semantic == "WMS") {
      for (auto j = i->msids.begin(); j != i->msids.end(); ++j) {
        if (*j == "*") {
          allMsidsAreWebrtc = true;
        } else {
          webrtcMsids.insert(*j);
        }
      }
      break;
    }
  }

  bool found = false;

  for (auto i = allMsids.begin(); i != allMsids.end(); ++i) {
    if (allMsidsAreWebrtc || webrtcMsids.count(i->identifier)) {
      if (i->appdata.empty()) {
        SDP_SET_ERROR("Invalid webrtc msid at level " << msection.GetLevel()
                       << ": Missing track id.");
        return NS_ERROR_INVALID_ARG;
      }
      if (!found) {
        *streamId = i->identifier;
        *trackId = i->appdata;
        found = true;
      } else if ((*streamId != i->identifier) || (*trackId != i->appdata)) {
        SDP_SET_ERROR("Found multiple different webrtc msids in m-section "
                       << msection.GetLevel() << ". The behavior here is "
                       "undefined.");
        return NS_ERROR_INVALID_ARG;
      }
    }
  }

  if (!found) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}

nsresult
SdpHelper::GetMsids(const SdpMediaSection& msection,
                    std::vector<SdpMsidAttributeList::Msid>* msids)
{
  if (msection.GetAttributeList().HasAttribute(SdpAttribute::kMsidAttribute)) {
    *msids = msection.GetAttributeList().GetMsid().mMsids;
  }

  
  
  if (msection.GetAttributeList().HasAttribute(SdpAttribute::kSsrcAttribute)) {
    auto& ssrcs = msection.GetAttributeList().GetSsrc().mSsrcs;

    for (auto i = ssrcs.begin(); i != ssrcs.end(); ++i) {
      if (i->attribute.find("msid:") == 0) {
        std::string streamId;
        std::string trackId;
        nsresult rv = ParseMsid(i->attribute, &streamId, &trackId);
        NS_ENSURE_SUCCESS(rv, rv);
        msids->push_back({streamId, trackId});
      }
    }
  }

  return NS_OK;
}

nsresult
SdpHelper::ParseMsid(const std::string& msidAttribute,
                     std::string* streamId,
                     std::string* trackId)
{
  
  
  
  
  size_t streamIdStart = msidAttribute.find_first_not_of(" \t", 5);
  
  
  if (streamIdStart == std::string::npos) {
    SDP_SET_ERROR("Malformed source-level msid attribute: "
        << msidAttribute);
    return NS_ERROR_INVALID_ARG;
  }

  size_t streamIdEnd = msidAttribute.find_first_of(" \t", streamIdStart);
  if (streamIdEnd == std::string::npos) {
    streamIdEnd = msidAttribute.size();
  }

  size_t trackIdStart =
    msidAttribute.find_first_not_of(" \t", streamIdEnd);
  if (trackIdStart == std::string::npos) {
    trackIdStart = msidAttribute.size();
  }

  size_t trackIdEnd = msidAttribute.find_first_of(" \t", trackIdStart);
  if (trackIdEnd == std::string::npos) {
    trackIdEnd = msidAttribute.size();
  }

  size_t streamIdSize = streamIdEnd - streamIdStart;
  size_t trackIdSize = trackIdEnd - trackIdStart;

  *streamId = msidAttribute.substr(streamIdStart, streamIdSize);
  *trackId = msidAttribute.substr(trackIdStart, trackIdSize);
  return NS_OK;
}

void
SdpHelper::SetupMsidSemantic(const std::vector<std::string>& msids,
                             Sdp* sdp) const
{
  if (!msids.empty()) {
    UniquePtr<SdpMsidSemanticAttributeList> msidSemantics(
        new SdpMsidSemanticAttributeList);
    msidSemantics->PushEntry("WMS", msids);
    sdp->GetAttributeList().SetAttribute(msidSemantics.release());
  }
}

std::string
SdpHelper::GetCNAME(const SdpMediaSection& msection) const
{
  if (msection.GetAttributeList().HasAttribute(SdpAttribute::kSsrcAttribute)) {
    auto& ssrcs = msection.GetAttributeList().GetSsrc().mSsrcs;
    for (auto i = ssrcs.begin(); i != ssrcs.end(); ++i) {
      if (i->attribute.find("cname:") == 0) {
        return i->attribute.substr(6);
      }
    }
  }
  return "";
}

const SdpMediaSection*
SdpHelper::FindMsectionByMid(const Sdp& sdp,
                             const std::string& mid) const
{
  for (size_t i = 0; i < sdp.GetMediaSectionCount(); ++i) {
    auto& attrs = sdp.GetMediaSection(i).GetAttributeList();
    if (attrs.HasAttribute(SdpAttribute::kMidAttribute) &&
        attrs.GetMid() == mid) {
      return &sdp.GetMediaSection(i);
    }
  }
  return nullptr;
}

SdpMediaSection*
SdpHelper::FindMsectionByMid(Sdp& sdp,
                             const std::string& mid) const
{
  for (size_t i = 0; i < sdp.GetMediaSectionCount(); ++i) {
    auto& attrs = sdp.GetMediaSection(i).GetAttributeList();
    if (attrs.HasAttribute(SdpAttribute::kMidAttribute) &&
        attrs.GetMid() == mid) {
      return &sdp.GetMediaSection(i);
    }
  }
  return nullptr;
}

void
SdpHelper::SetSsrcs(const std::vector<uint32_t>& ssrcs,
                    const std::string& cname,
                    SdpMediaSection* msection) const
{
  if (ssrcs.empty()) {
    msection->GetAttributeList().RemoveAttribute(SdpAttribute::kSsrcAttribute);
    return;
  }

  UniquePtr<SdpSsrcAttributeList> ssrcAttr(new SdpSsrcAttributeList);
  for (auto ssrc : ssrcs) {
    
    
    std::string cnameAttr("cname:");
    cnameAttr += cname;
    ssrcAttr->PushEntry(ssrc, cnameAttr);
  }

  msection->GetAttributeList().SetAttribute(ssrcAttr.release());
}

nsresult
SdpHelper::CopyStickyParams(const SdpMediaSection& source,
                            SdpMediaSection* dest)
{
  auto& sourceAttrs = source.GetAttributeList();
  auto& destAttrs = dest->GetAttributeList();

  
  if (sourceAttrs.HasAttribute(SdpAttribute::kRtcpMuxAttribute)) {
    destAttrs.SetAttribute(
        new SdpFlagAttribute(SdpAttribute::kRtcpMuxAttribute));
  }

  
  if (sourceAttrs.HasAttribute(SdpAttribute::kMidAttribute)) {
    destAttrs.SetAttribute(
        new SdpStringAttribute(SdpAttribute::kMidAttribute,
          sourceAttrs.GetMid()));
  }

  return NS_OK;
}

bool
SdpHelper::HasRtcp(SdpMediaSection::Protocol proto) const
{
  switch (proto) {
    case SdpMediaSection::kRtpAvpf:
    case SdpMediaSection::kDccpRtpAvpf:
    case SdpMediaSection::kDccpRtpSavpf:
    case SdpMediaSection::kRtpSavpf:
    case SdpMediaSection::kUdpTlsRtpSavpf:
    case SdpMediaSection::kTcpTlsRtpSavpf:
    case SdpMediaSection::kDccpTlsRtpSavpf:
      return true;
    case SdpMediaSection::kRtpAvp:
    case SdpMediaSection::kUdp:
    case SdpMediaSection::kVat:
    case SdpMediaSection::kRtp:
    case SdpMediaSection::kUdptl:
    case SdpMediaSection::kTcp:
    case SdpMediaSection::kTcpRtpAvp:
    case SdpMediaSection::kRtpSavp:
    case SdpMediaSection::kTcpBfcp:
    case SdpMediaSection::kTcpTlsBfcp:
    case SdpMediaSection::kTcpTls:
    case SdpMediaSection::kFluteUdp:
    case SdpMediaSection::kTcpMsrp:
    case SdpMediaSection::kTcpTlsMsrp:
    case SdpMediaSection::kDccp:
    case SdpMediaSection::kDccpRtpAvp:
    case SdpMediaSection::kDccpRtpSavp:
    case SdpMediaSection::kUdpTlsRtpSavp:
    case SdpMediaSection::kTcpTlsRtpSavp:
    case SdpMediaSection::kDccpTlsRtpSavp:
    case SdpMediaSection::kUdpMbmsFecRtpAvp:
    case SdpMediaSection::kUdpMbmsFecRtpSavp:
    case SdpMediaSection::kUdpMbmsRepair:
    case SdpMediaSection::kFecUdp:
    case SdpMediaSection::kUdpFec:
    case SdpMediaSection::kTcpMrcpv2:
    case SdpMediaSection::kTcpTlsMrcpv2:
    case SdpMediaSection::kPstn:
    case SdpMediaSection::kUdpTlsUdptl:
    case SdpMediaSection::kSctp:
    case SdpMediaSection::kSctpDtls:
    case SdpMediaSection::kDtlsSctp:
      return false;
  }
  MOZ_CRASH("Unknown protocol, probably corruption.");
}

SdpMediaSection::Protocol
SdpHelper::GetProtocolForMediaType(SdpMediaSection::MediaType type)
{
  if (type == SdpMediaSection::kApplication) {
    return SdpMediaSection::kDtlsSctp;
  }

  
  return SdpMediaSection::kRtpSavpf;
}

void
SdpHelper::appendSdpParseErrors(
    const std::vector<std::pair<size_t, std::string> >& aErrors,
    std::string* aErrorString)
{
  std::ostringstream os;
  for (auto i = aErrors.begin(); i != aErrors.end(); ++i) {
    os << "SDP Parse Error on line " << i->first << ": " + i->second
       << std::endl;
  }
  *aErrorString += os.str();
}

} 


