



#include "logging.h"

#include "signaling/src/jsep/JsepSessionImpl.h"
#include <string>
#include <set>
#include <stdlib.h>

#include "nspr.h"
#include "nss.h"
#include "pk11pub.h"
#include "nsDebug.h"

#include <mozilla/Move.h>
#include <mozilla/UniquePtr.h>

#include "signaling/src/jsep/JsepTrack.h"
#include "signaling/src/jsep/JsepTrack.h"
#include "signaling/src/jsep/JsepTrackImpl.h"
#include "signaling/src/jsep/JsepTransport.h"
#include "signaling/src/sdp/Sdp.h"
#include "signaling/src/sdp/SipccSdp.h"
#include "signaling/src/sdp/SipccSdpParser.h"
#include "mozilla/net/DataChannelProtocol.h"

namespace mozilla {

MOZ_MTLOG_MODULE("jsep")

#define JSEP_SET_ERROR(error)                                                  \
  do {                                                                         \
    std::ostringstream os;                                                     \
    os << error;                                                               \
    mLastError = os.str();                                                     \
    MOZ_MTLOG(ML_ERROR, mLastError);                                           \
  } while (0);

nsresult
JsepSessionImpl::Init()
{
  mLastError.clear();

  MOZ_ASSERT(!mSessionId, "Init called more than once");

  nsresult rv = SetupIds();
  NS_ENSURE_SUCCESS(rv, rv);

  SetupDefaultCodecs();
  SetupDefaultRtpExtensions();

  return NS_OK;
}


template <class T>
typename std::vector<T>::iterator
FindTrackByLevel(std::vector<T>& tracks, size_t level)
{
  for (auto t = tracks.begin(); t != tracks.end(); ++t) {
    if (t->mAssignedMLine.isSome() &&
        (*t->mAssignedMLine == level)) {
      return t;
    }
  }

  return tracks.end();
}

template <class T>
typename std::vector<T>::iterator
FindTrackByIds(std::vector<T>& tracks,
               const std::string& streamId,
               const std::string& trackId)
{
  for (auto t = tracks.begin(); t != tracks.end(); ++t) {
    if (t->mTrack->GetStreamId() == streamId &&
        (t->mTrack->GetTrackId() == trackId)) {
      return t;
    }
  }

  return tracks.end();
}

template <class T>
typename std::vector<T>::iterator
FindUnassignedTrackByType(std::vector<T>& tracks,
                          SdpMediaSection::MediaType type)
{
  for (auto t = tracks.begin(); t != tracks.end(); ++t) {
    if (!t->mAssignedMLine.isSome() &&
        (t->mTrack->GetMediaType() == type)) {
      return t;
    }
  }

  return tracks.end();
}

nsresult
JsepSessionImpl::AddTrack(const RefPtr<JsepTrack>& track)
{
  mLastError.clear();
  MOZ_ASSERT(track->GetDirection() == JsepTrack::kJsepTrackSending);

  if (track->GetMediaType() != SdpMediaSection::kApplication) {
    track->SetCNAME(mCNAME);

    if (track->GetSsrcs().empty()) {
      uint32_t ssrc;
      nsresult rv = CreateSsrc(&ssrc);
      NS_ENSURE_SUCCESS(rv, rv);
      track->AddSsrc(ssrc);
    }
  }

  JsepSendingTrack strack;
  strack.mTrack = track;
  strack.mNegotiated = false;

  mLocalTracks.push_back(strack);

  return NS_OK;
}

nsresult
JsepSessionImpl::RemoveTrack(const std::string& streamId,
                             const std::string& trackId)
{
  if (mState != kJsepStateStable) {
    JSEP_SET_ERROR("Removing tracks outside of stable is unsupported.");
    return NS_ERROR_UNEXPECTED;
  }

  auto track = FindTrackByIds(mLocalTracks, streamId, trackId);

  if (track == mLocalTracks.end()) {
    return NS_ERROR_INVALID_ARG;
  }

  mLocalTracks.erase(track);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetIceCredentials(const std::string& ufrag,
                                   const std::string& pwd)
{
  mLastError.clear();
  mIceUfrag = ufrag;
  mIcePwd = pwd;

  return NS_OK;
}

nsresult
JsepSessionImpl::SetBundlePolicy(JsepBundlePolicy policy)
{
  mLastError.clear();
  if (mCurrentLocalDescription) {
    JSEP_SET_ERROR("Changing the bundle policy is only supported before the "
                   "first SetLocalDescription.");
    return NS_ERROR_UNEXPECTED;
  }

  mBundlePolicy = policy;
  return NS_OK;
}

nsresult
JsepSessionImpl::AddDtlsFingerprint(const std::string& algorithm,
                                    const std::vector<uint8_t>& value)
{
  mLastError.clear();
  JsepDtlsFingerprint fp;

  fp.mAlgorithm = algorithm;
  fp.mValue = value;

  mDtlsFingerprints.push_back(fp);

  return NS_OK;
}

nsresult
JsepSessionImpl::AddAudioRtpExtension(const std::string& extensionName)
{
  mLastError.clear();

  if (mAudioRtpExtensions.size() + 1 > UINT16_MAX) {
    JSEP_SET_ERROR("Too many audio rtp extensions have been added");
    return NS_ERROR_FAILURE;
  }

  SdpExtmapAttributeList::Extmap extmap =
      { static_cast<uint16_t>(mAudioRtpExtensions.size() + 1),
        SdpDirectionAttribute::kSendrecv,
        false, 
        extensionName,
        "" };

  mAudioRtpExtensions.push_back(extmap);
  return NS_OK;
}

nsresult
JsepSessionImpl::AddVideoRtpExtension(const std::string& extensionName)
{
  mLastError.clear();

  if (mVideoRtpExtensions.size() + 1 > UINT16_MAX) {
    JSEP_SET_ERROR("Too many video rtp extensions have been added");
    return NS_ERROR_FAILURE;
  }

  SdpExtmapAttributeList::Extmap extmap =
      { static_cast<uint16_t>(mVideoRtpExtensions.size() + 1),
        SdpDirectionAttribute::kSendrecv,
        false, 
        extensionName, "" };

  mVideoRtpExtensions.push_back(extmap);
  return NS_OK;
}

template<class T>
std::vector<RefPtr<JsepTrack>>
GetTracks(const std::vector<T>& wrappedTracks)
{
  std::vector<RefPtr<JsepTrack>> result;
  for (auto i = wrappedTracks.begin(); i != wrappedTracks.end(); ++i) {
    result.push_back(i->mTrack);
  }
  return result;
}

nsresult
JsepSessionImpl::ReplaceTrack(const std::string& oldStreamId,
                              const std::string& oldTrackId,
                              const std::string& newStreamId,
                              const std::string& newTrackId)
{
  auto it = FindTrackByIds(mLocalTracks, oldStreamId, oldTrackId);

  if (it == mLocalTracks.end()) {
    JSEP_SET_ERROR("Track " << oldStreamId << "/" << oldTrackId
                   << " was never added.");
    return NS_ERROR_INVALID_ARG;
  }

  if (FindTrackByIds(mLocalTracks, newStreamId, newTrackId) !=
      mLocalTracks.end()) {
    JSEP_SET_ERROR("Track " << newStreamId << "/" << newTrackId
                   << " was already added.");
    return NS_ERROR_INVALID_ARG;
  }

  it->mTrack->SetStreamId(newStreamId);
  it->mTrack->SetTrackId(newTrackId);

  return NS_OK;
}

std::vector<RefPtr<JsepTrack>>
JsepSessionImpl::GetLocalTracks() const
{
  return GetTracks(mLocalTracks);
}

std::vector<RefPtr<JsepTrack>>
JsepSessionImpl::GetRemoteTracks() const
{
  return GetTracks(mRemoteTracks);
}

std::vector<RefPtr<JsepTrack>>
JsepSessionImpl::GetRemoteTracksAdded() const
{
  return GetTracks(mRemoteTracksAdded);
}

std::vector<RefPtr<JsepTrack>>
JsepSessionImpl::GetRemoteTracksRemoved() const
{
  return GetTracks(mRemoteTracksRemoved);
}

nsresult
JsepSessionImpl::AddOfferMSections(const JsepOfferOptions& options, Sdp* sdp)
{
  
  
  
  
  
  
  nsresult rv = AddOfferMSectionsByType(
      SdpMediaSection::kAudio, options.mOfferToReceiveAudio, sdp);

  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddOfferMSectionsByType(
      SdpMediaSection::kVideo, options.mOfferToReceiveVideo, sdp);

  NS_ENSURE_SUCCESS(rv, rv);

  if (!(options.mDontOfferDataChannel.isSome() &&
        *options.mDontOfferDataChannel)) {
    rv = AddOfferMSectionsByType(
        SdpMediaSection::kApplication, Maybe<size_t>(), sdp);

    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!sdp->GetMediaSectionCount()) {
    JSEP_SET_ERROR("Cannot create an offer with no local tracks, "
                   "no offerToReceiveAudio/Video, and no DataChannel.");
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::AddOfferMSectionsByType(SdpMediaSection::MediaType mediatype,
                                         Maybe<size_t> offerToReceiveMaybe,
                                         Sdp* sdp)
{
  
  
  size_t offerToReceiveCount;
  size_t* offerToReceiveCountPtr = nullptr;

  if (offerToReceiveMaybe) {
    offerToReceiveCount = *offerToReceiveMaybe;
    offerToReceiveCountPtr = &offerToReceiveCount;
  }

  
  nsresult rv = BindLocalTracks(mediatype, sdp);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = EnsureRecvForRemoteTracks(mediatype, sdp, offerToReceiveCountPtr);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = SetRecvAsNeededOrDisable(mediatype,
                                sdp,
                                offerToReceiveCountPtr);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (offerToReceiveCountPtr && *offerToReceiveCountPtr) {
    rv = AddRecvonlyMsections(mediatype, *offerToReceiveCountPtr, sdp);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::BindLocalTracks(SdpMediaSection::MediaType mediatype,
                                 Sdp* sdp)
{
  for (auto track = mLocalTracks.begin(); track != mLocalTracks.end();
       ++track) {
    if (mediatype != track->mTrack->GetMediaType()) {
      continue;
    }

    SdpMediaSection* msection;

    if (track->mAssignedMLine.isSome()) {
      
      msection = &sdp->GetMediaSection(*track->mAssignedMLine);
    } else {
      nsresult rv = GetFreeMsectionForSend(track->mTrack->GetMediaType(),
                                           sdp,
                                           &msection);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsresult rv = BindTrackToMsection(&(*track), msection);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
JsepSessionImpl::BindTrackToMsection(
    JsepSendingTrack* track,
    SdpMediaSection* msection)
{
  if (msection->GetMediaType() != SdpMediaSection::kApplication) {
    mSdpHelper.SetSsrcs(track->mTrack->GetSsrcs(), mCNAME, msection);
    AddLocalIds(*track->mTrack, msection);
  }
  msection->SetSending(true);
  track->mAssignedMLine = Some(msection->GetLevel());
  track->mNegotiated = false;
  return NS_OK;
}

nsresult
JsepSessionImpl::EnsureRecvForRemoteTracks(SdpMediaSection::MediaType mediatype,
                                           Sdp* sdp,
                                           size_t* offerToReceive)
{
  for (auto track = mRemoteTracks.begin(); track != mRemoteTracks.end();
       ++track) {
    if (mediatype != track->mTrack->GetMediaType()) {
      continue;
    }

    if (!track->mAssignedMLine.isSome()) {
      MOZ_ASSERT(false);
      continue;
    }

    auto& msection = sdp->GetMediaSection(*track->mAssignedMLine);

    if (mSdpHelper.MsectionIsDisabled(msection)) {
      
      
      continue;
    }

    msection.SetReceiving(true);
    if (offerToReceive && *offerToReceive) {
      --(*offerToReceive);
    }
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::SetRecvAsNeededOrDisable(SdpMediaSection::MediaType mediatype,
                                          Sdp* sdp,
                                          size_t* offerToRecv)
{
  for (size_t i = 0; i < sdp->GetMediaSectionCount(); ++i) {
    auto& msection = sdp->GetMediaSection(i);

    if (mSdpHelper.MsectionIsDisabled(msection) ||
        msection.GetMediaType() != mediatype ||
        msection.IsReceiving()) {
      continue;
    }

    if (offerToRecv) {
      if (*offerToRecv) {
        msection.SetReceiving(true);
        --(*offerToRecv);
        continue;
      }
    } else if (msection.IsSending()) {
      msection.SetReceiving(true);
      continue;
    }

    if (!msection.IsSending()) {
      
      DisableMsection(sdp, &msection);
    }
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::AddRecvonlyMsections(SdpMediaSection::MediaType mediatype,
                                      size_t count,
                                      Sdp* sdp)
{
  while (count--) {
    nsresult rv = CreateOfferMSection(
        mediatype, SdpDirectionAttribute::kRecvonly, sdp);

    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}



nsresult
JsepSessionImpl::CreateReoffer(const Sdp& oldLocalSdp,
                               const Sdp& oldAnswer,
                               Sdp* newSdp)
{
  nsresult rv;

  for (size_t i = 0; i < oldLocalSdp.GetMediaSectionCount(); ++i) {
    
    
    rv = CreateOfferMSection(oldLocalSdp.GetMediaSection(i).GetMediaType(),
                             SdpDirectionAttribute::kInactive,
                             newSdp);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mSdpHelper.CopyStickyParams(oldAnswer.GetMediaSection(i),
                                     &newSdp->GetMediaSection(i));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}



void
JsepSessionImpl::SetupBundle(Sdp* sdp) const
{
  std::vector<std::string> mids;
  std::set<SdpMediaSection::MediaType> observedTypes;

  
  

  for (size_t i = 0; i < sdp->GetMediaSectionCount(); ++i) {
    auto& attrs = sdp->GetMediaSection(i).GetAttributeList();
    if (attrs.HasAttribute(SdpAttribute::kMidAttribute)) {
      bool useBundleOnly = false;
      switch (mBundlePolicy) {
        case kBundleMaxCompat:
          
          break;
        case kBundleBalanced:
          
          
          if (observedTypes.count(sdp->GetMediaSection(i).GetMediaType())) {
            useBundleOnly = true;
          }
          observedTypes.insert(sdp->GetMediaSection(i).GetMediaType());
          break;
        case kBundleMaxBundle:
          
          
          useBundleOnly = !mids.empty();
          break;
      }

      if (useBundleOnly) {
        attrs.SetAttribute(
            new SdpFlagAttribute(SdpAttribute::kBundleOnlyAttribute));
      }

      mids.push_back(attrs.GetMid());
    }
  }

  if (mids.size() > 1) {
    UniquePtr<SdpGroupAttributeList> groupAttr(new SdpGroupAttributeList);
    groupAttr->PushEntry(SdpGroupAttributeList::kBundle, mids);
    sdp->GetAttributeList().SetAttribute(groupAttr.release());
  }
}

nsresult
JsepSessionImpl::GetRemoteIds(const Sdp& sdp,
                              const SdpMediaSection& msection,
                              std::string* streamId,
                              std::string* trackId)
{
  nsresult rv = mSdpHelper.GetIdsFromMsid(sdp, msection, streamId, trackId);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    *streamId = mDefaultRemoteStreamId;

    if (!mDefaultRemoteTrackIdsByLevel.count(msection.GetLevel())) {
      
      if (!mUuidGen->Generate(trackId)) {
        JSEP_SET_ERROR("Failed to generate UUID for JsepTrack");
        return NS_ERROR_FAILURE;
      }

      mDefaultRemoteTrackIdsByLevel[msection.GetLevel()] = *trackId;
    } else {
      *trackId = mDefaultRemoteTrackIdsByLevel[msection.GetLevel()];
    }
    return NS_OK;
  }

  if (NS_SUCCEEDED(rv)) {
    
    
    
    mDefaultRemoteTrackIdsByLevel.erase(msection.GetLevel());
  }

  return rv;
}

nsresult
JsepSessionImpl::CreateOffer(const JsepOfferOptions& options,
                             std::string* offer)
{
  mLastError.clear();

  if (mState != kJsepStateStable) {
    JSEP_SET_ERROR("Cannot create offer in state " << GetStateStr(mState));
    return NS_ERROR_UNEXPECTED;
  }

  UniquePtr<Sdp> sdp;

  
  nsresult rv = CreateGenericSDP(&sdp);
  NS_ENSURE_SUCCESS(rv, rv);
  ++mSessionVersion;

  if (mCurrentLocalDescription) {
    rv = CreateReoffer(*mCurrentLocalDescription, *GetAnswer(), sdp.get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(rv)) {
    for (auto i = mLocalTracks.begin(); i != mLocalTracks.end(); ++i) {
      if (!i->mNegotiated) {
        i->mAssignedMLine.reset();
      }
    }
  }

  
  rv = AddOfferMSections(options, sdp.get());
  NS_ENSURE_SUCCESS(rv, rv);

  SetupBundle(sdp.get());

  if (GetAnswer()) {
    
    rv = SetupTransportParams(*GetAnswer(), *sdp, sdp.get());
    NS_ENSURE_SUCCESS(rv,rv);
  }

  *offer = sdp->ToString();
  mGeneratedLocalDescription = Move(sdp);

  return NS_OK;
}

std::string
JsepSessionImpl::GetLocalDescription() const
{
  std::ostringstream os;
  if (mPendingLocalDescription) {
    mPendingLocalDescription->Serialize(os);
  } else if (mCurrentLocalDescription) {
    mCurrentLocalDescription->Serialize(os);
  }
  return os.str();
}

std::string
JsepSessionImpl::GetRemoteDescription() const
{
  std::ostringstream os;
  if (mPendingRemoteDescription) {
    mPendingRemoteDescription->Serialize(os);
  } else if (mCurrentRemoteDescription) {
    mCurrentRemoteDescription->Serialize(os);
  }
  return os.str();
}

void
JsepSessionImpl::AddCodecs(SdpMediaSection* msection) const
{
  msection->ClearCodecs();
  for (const JsepCodecDescription* codec : mCodecs.values) {
    codec->AddToMediaSection(*msection);
  }
}

void
JsepSessionImpl::AddExtmap(SdpMediaSection* msection) const
{
  const auto* extensions = GetRtpExtensions(msection->GetMediaType());

  if (extensions && !extensions->empty()) {
    SdpExtmapAttributeList* extmap = new SdpExtmapAttributeList;
    extmap->mExtmaps = *extensions;
    msection->GetAttributeList().SetAttribute(extmap);
  }
}

void
JsepSessionImpl::AddMid(const std::string& mid,
                        SdpMediaSection* msection) const
{
  msection->GetAttributeList().SetAttribute(new SdpStringAttribute(
        SdpAttribute::kMidAttribute, mid));
}

void
JsepSessionImpl::AddLocalIds(const JsepTrack& track,
                             SdpMediaSection* msection) const
{
  if (track.GetMediaType() == SdpMediaSection::kApplication) {
    return;
  }

  UniquePtr<SdpMsidAttributeList> msids(new SdpMsidAttributeList);
  if (msection->GetAttributeList().HasAttribute(SdpAttribute::kMsidAttribute)) {
    msids->mMsids = msection->GetAttributeList().GetMsid().mMsids;
  }

  msids->PushEntry(track.GetStreamId(), track.GetTrackId());

  msection->GetAttributeList().SetAttribute(msids.release());
}

JsepCodecDescription*
JsepSessionImpl::FindMatchingCodec(const std::string& fmt,
                                   const SdpMediaSection& msection) const
{
  for (JsepCodecDescription* codec : mCodecs.values) {
    if (codec->mEnabled && codec->Matches(fmt, msection)) {
      return codec;
    }
  }

  return nullptr;
}

const std::vector<SdpExtmapAttributeList::Extmap>*
JsepSessionImpl::GetRtpExtensions(SdpMediaSection::MediaType type) const
{
  switch (type) {
    case SdpMediaSection::kAudio:
      return &mAudioRtpExtensions;
    case SdpMediaSection::kVideo:
      return &mVideoRtpExtensions;
    default:
      return nullptr;
  }
}

static bool
CompareCodec(const JsepCodecDescription* lhs, const JsepCodecDescription* rhs)
{
  return lhs->mStronglyPreferred && !rhs->mStronglyPreferred;
}

PtrVector<JsepCodecDescription>
JsepSessionImpl::GetCommonCodecs(const SdpMediaSection& offerMsection)
{
  MOZ_ASSERT(!mIsOfferer);
  PtrVector<JsepCodecDescription> matchingCodecs;
  for (const std::string& fmt : offerMsection.GetFormats()) {
    JsepCodecDescription* codec = FindMatchingCodec(fmt, offerMsection);
    if (codec) {
      codec->mDefaultPt = fmt; 
      matchingCodecs.values.push_back(codec->Clone());
    }
  }

  std::stable_sort(matchingCodecs.values.begin(),
                   matchingCodecs.values.end(),
                   CompareCodec);

  return matchingCodecs;
}

void
JsepSessionImpl::AddCommonExtmaps(const SdpMediaSection& remoteMsection,
                                  SdpMediaSection* msection)
{
  if (!remoteMsection.GetAttributeList().HasAttribute(
        SdpAttribute::kExtmapAttribute)) {
    return;
  }

  auto* ourExtensions = GetRtpExtensions(remoteMsection.GetMediaType());

  if (!ourExtensions) {
    return;
  }

  UniquePtr<SdpExtmapAttributeList> ourExtmap(new SdpExtmapAttributeList);
  auto& theirExtmap = remoteMsection.GetAttributeList().GetExtmap().mExtmaps;
  for (auto i = theirExtmap.begin(); i != theirExtmap.end(); ++i) {
    for (auto j = ourExtensions->begin(); j != ourExtensions->end(); ++j) {
      if (i->extensionname == j->extensionname) {
        ourExtmap->mExtmaps.push_back(*i);

        
        
        
        if (ourExtmap->mExtmaps.back().entry >= 4096) {
          ourExtmap->mExtmaps.back().entry = j->entry;
        }
      }
    }
  }

  if (!ourExtmap->mExtmaps.empty()) {
    msection->GetAttributeList().SetAttribute(ourExtmap.release());
  }
}

nsresult
JsepSessionImpl::CreateAnswer(const JsepAnswerOptions& options,
                              std::string* answer)
{
  mLastError.clear();

  if (mState != kJsepStateHaveRemoteOffer) {
    JSEP_SET_ERROR("Cannot create answer in state " << GetStateStr(mState));
    return NS_ERROR_UNEXPECTED;
  }

  
  
  
  
  
  
  
  
  
  
  UniquePtr<Sdp> sdp;

  
  nsresult rv = CreateGenericSDP(&sdp);
  NS_ENSURE_SUCCESS(rv, rv);

  const Sdp& offer = *mPendingRemoteDescription;

  std::vector<SdpGroupAttributeList::Group> bundleGroups;

  
  UniquePtr<SdpGroupAttributeList> groupAttr(new SdpGroupAttributeList);
  mSdpHelper.GetBundleGroups(offer, &groupAttr->mGroups);
  sdp->GetAttributeList().SetAttribute(groupAttr.release());

  
  
  for (auto i = mLocalTracks.begin(); i != mLocalTracks.end(); ++i) {
    if (!i->mAssignedMLine.isSome()) {
      continue;
    }

    
    if (!i->mNegotiated) {
      i->mAssignedMLine.reset();
      continue;
    }

    if (!offer.GetMediaSection(*i->mAssignedMLine).IsReceiving()) {
      i->mAssignedMLine.reset();
    }
  }

  size_t numMsections = offer.GetMediaSectionCount();

  for (size_t i = 0; i < numMsections; ++i) {
    const SdpMediaSection& remoteMsection = offer.GetMediaSection(i);
    rv = CreateAnswerMSection(options, i, remoteMsection, sdp.get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (GetAnswer()) {
    
    rv = SetupTransportParams(*GetAnswer(), *sdp, sdp.get());
    NS_ENSURE_SUCCESS(rv,rv);
  }

  *answer = sdp->ToString();
  mGeneratedLocalDescription = Move(sdp);

  return NS_OK;
}

nsresult
JsepSessionImpl::CreateOfferMSection(SdpMediaSection::MediaType mediatype,
                                     SdpDirectionAttribute::Direction dir,
                                     Sdp* sdp)
{
  SdpMediaSection::Protocol proto =
    mSdpHelper.GetProtocolForMediaType(mediatype);

  SdpMediaSection* msection =
      &sdp->AddMediaSection(mediatype, dir, 0, proto, sdp::kIPv4, "0.0.0.0");

  return EnableOfferMsection(msection);
}

nsresult
JsepSessionImpl::GetFreeMsectionForSend(
    SdpMediaSection::MediaType type,
    Sdp* sdp,
    SdpMediaSection** msectionOut)
{
  for (size_t i = 0; i < sdp->GetMediaSectionCount(); ++i) {
    SdpMediaSection& msection = sdp->GetMediaSection(i);
    
    
    
    if (msection.GetMediaType() != type) {
      continue;
    }

    if (FindTrackByLevel(mLocalTracks, i) != mLocalTracks.end()) {
      
      continue;
    }

    if (mSdpHelper.MsectionIsDisabled(msection)) {
      
      nsresult rv = EnableOfferMsection(&msection);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    *msectionOut = &msection;
    return NS_OK;
  }

  
  nsresult rv = CreateOfferMSection(type,
                                    SdpDirectionAttribute::kInactive,
                                    sdp);
  NS_ENSURE_SUCCESS(rv, rv);

  *msectionOut = &sdp->GetMediaSection(sdp->GetMediaSectionCount() - 1);
  return NS_OK;
}

nsresult
JsepSessionImpl::CreateAnswerMSection(const JsepAnswerOptions& options,
                                      size_t mlineIndex,
                                      const SdpMediaSection& remoteMsection,
                                      Sdp* sdp)
{
  SdpMediaSection& msection =
      sdp->AddMediaSection(remoteMsection.GetMediaType(),
                           SdpDirectionAttribute::kInactive,
                           9,
                           remoteMsection.GetProtocol(),
                           sdp::kIPv4,
                           "0.0.0.0");

  nsresult rv = mSdpHelper.CopyStickyParams(remoteMsection, &msection);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mSdpHelper.MsectionIsDisabled(remoteMsection)) {
    DisableMsection(sdp, &msection);
    return NS_OK;
  }

  SdpSetupAttribute::Role role;
  rv = DetermineAnswererSetupRole(remoteMsection, &role);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddTransportAttributes(&msection, role);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRecvonlySsrc(&msection);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (remoteMsection.IsReceiving()) {
    rv = BindMatchingLocalTrackForAnswer(&msection);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (remoteMsection.IsSending()) {
    msection.SetReceiving(true);
  }

  
  PtrVector<JsepCodecDescription> matchingCodecs(
      GetCommonCodecs(remoteMsection));

  for (JsepCodecDescription* codec : matchingCodecs.values) {
    if (codec->Negotiate(remoteMsection)) {
      codec->AddToMediaSection(msection);
      
      
      break;
    }
  }

  
  AddCommonExtmaps(remoteMsection, &msection);

  if (!msection.IsReceiving() && !msection.IsSending()) {
    DisableMsection(sdp, &msection);
    return NS_OK;
  }

  if (msection.GetFormats().empty()) {
    
    DisableMsection(sdp, &msection);
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::SetRecvonlySsrc(SdpMediaSection* msection)
{
  
  while (mRecvonlySsrcs.size() <= msection->GetLevel()) {
    uint32_t ssrc;
    nsresult rv = CreateSsrc(&ssrc);
    NS_ENSURE_SUCCESS(rv, rv);
    mRecvonlySsrcs.push_back(ssrc);
  }

  std::vector<uint32_t> ssrcs;
  ssrcs.push_back(mRecvonlySsrcs[msection->GetLevel()]);
  mSdpHelper.SetSsrcs(ssrcs, mCNAME, msection);
  return NS_OK;
}

nsresult
JsepSessionImpl::BindMatchingLocalTrackForAnswer(SdpMediaSection* msection)
{
  auto track = FindTrackByLevel(mLocalTracks, msection->GetLevel());

  if (track == mLocalTracks.end()) {
    track = FindUnassignedTrackByType(mLocalTracks, msection->GetMediaType());
  }

  if (track == mLocalTracks.end() &&
      msection->GetMediaType() == SdpMediaSection::kApplication) {
    
    
    std::string streamId;
    std::string trackId;

    if (!mUuidGen->Generate(&streamId) || !mUuidGen->Generate(&trackId)) {
      JSEP_SET_ERROR("Failed to generate UUIDs for datachannel track");
      return NS_ERROR_FAILURE;
    }

    AddTrack(RefPtr<JsepTrack>(
          new JsepTrack(SdpMediaSection::kApplication, streamId, trackId)));
    track = FindUnassignedTrackByType(mLocalTracks, msection->GetMediaType());
    MOZ_ASSERT(track != mLocalTracks.end());
  }

  if (track != mLocalTracks.end()) {
    nsresult rv = BindTrackToMsection(&(*track), msection);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::DetermineAnswererSetupRole(
    const SdpMediaSection& remoteMsection,
    SdpSetupAttribute::Role* rolep)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  SdpSetupAttribute::Role role = SdpSetupAttribute::kActive;

  if (remoteMsection.GetAttributeList().HasAttribute(
          SdpAttribute::kSetupAttribute)) {
    switch (remoteMsection.GetAttributeList().GetSetup().mRole) {
      case SdpSetupAttribute::kActive:
        role = SdpSetupAttribute::kPassive;
        break;
      case SdpSetupAttribute::kPassive:
      case SdpSetupAttribute::kActpass:
        role = SdpSetupAttribute::kActive;
        break;
      case SdpSetupAttribute::kHoldconn:
        
        MOZ_ASSERT(false);
        JSEP_SET_ERROR("The other side used an illegal setup attribute"
                       " (\"holdconn\").");
        return NS_ERROR_INVALID_ARG;
    }
  }

  *rolep = role;
  return NS_OK;
}

nsresult
JsepSessionImpl::SetLocalDescription(JsepSdpType type, const std::string& sdp)
{
  mLastError.clear();

  MOZ_MTLOG(ML_DEBUG, "SetLocalDescription type=" << type << "\nSDP=\n"
                                                  << sdp);

  if (type == kJsepSdpRollback) {
    if (mState != kJsepStateHaveLocalOffer) {
      JSEP_SET_ERROR("Cannot rollback local description in "
                     << GetStateStr(mState));
      return NS_ERROR_UNEXPECTED;
    }

    mPendingLocalDescription.reset();
    SetState(kJsepStateStable);
    mTransports = mOldTransports;
    mOldTransports.clear();
    return NS_OK;
  }

  switch (mState) {
    case kJsepStateStable:
      if (type != kJsepSdpOffer) {
        JSEP_SET_ERROR("Cannot set local answer in state "
                       << GetStateStr(mState));
        return NS_ERROR_UNEXPECTED;
      }
      mIsOfferer = true;
      break;
    case kJsepStateHaveRemoteOffer:
      if (type != kJsepSdpAnswer && type != kJsepSdpPranswer) {
        JSEP_SET_ERROR("Cannot set local offer in state "
                       << GetStateStr(mState));
        return NS_ERROR_UNEXPECTED;
      }
      break;
    default:
      JSEP_SET_ERROR("Cannot set local offer or answer in state "
                     << GetStateStr(mState));
      return NS_ERROR_UNEXPECTED;
  }

  UniquePtr<Sdp> parsed;
  nsresult rv = ParseSdp(sdp, &parsed);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = ValidateLocalDescription(*parsed);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mOldTransports = mTransports;
  for (size_t t = 0; t < parsed->GetMediaSectionCount(); ++t) {
    if (t >= mTransports.size()) {
      mTransports.push_back(RefPtr<JsepTransport>(new JsepTransport));
    }

    UpdateTransport(parsed->GetMediaSection(t), mTransports[t].get());
  }

  switch (type) {
    case kJsepSdpOffer:
      rv = SetLocalDescriptionOffer(Move(parsed));
      break;
    case kJsepSdpAnswer:
    case kJsepSdpPranswer:
      rv = SetLocalDescriptionAnswer(type, Move(parsed));
      break;
    case kJsepSdpRollback:
      MOZ_CRASH(); 
  }

  return rv;
}

nsresult
JsepSessionImpl::SetLocalDescriptionOffer(UniquePtr<Sdp> offer)
{
  MOZ_ASSERT(mState == kJsepStateStable);
  mPendingLocalDescription = Move(offer);
  SetState(kJsepStateHaveLocalOffer);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetLocalDescriptionAnswer(JsepSdpType type,
                                           UniquePtr<Sdp> answer)
{
  MOZ_ASSERT(mState == kJsepStateHaveRemoteOffer);
  mPendingLocalDescription = Move(answer);

  nsresult rv = ValidateAnswer(*mPendingRemoteDescription,
                               *mPendingLocalDescription);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = HandleNegotiatedSession(mPendingLocalDescription,
                               mPendingRemoteDescription);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentRemoteDescription = Move(mPendingRemoteDescription);
  mCurrentLocalDescription = Move(mPendingLocalDescription);
  mWasOffererLastTime = mIsOfferer;

  SetState(kJsepStateStable);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetRemoteDescription(JsepSdpType type, const std::string& sdp)
{
  mLastError.clear();
  mRemoteTracksAdded.clear();
  mRemoteTracksRemoved.clear();

  MOZ_MTLOG(ML_DEBUG, "SetRemoteDescription type=" << type << "\nSDP=\n"
                                                   << sdp);

  if (type == kJsepSdpRollback) {
    if (mState != kJsepStateHaveRemoteOffer) {
      JSEP_SET_ERROR("Cannot rollback remote description in "
                     << GetStateStr(mState));
      return NS_ERROR_UNEXPECTED;
    }

    mPendingRemoteDescription.reset();
    SetState(kJsepStateStable);

    
    return SetRemoteTracksFromDescription(mCurrentRemoteDescription.get());
  }

  switch (mState) {
    case kJsepStateStable:
      if (type != kJsepSdpOffer) {
        JSEP_SET_ERROR("Cannot set remote answer in state "
                       << GetStateStr(mState));
        return NS_ERROR_UNEXPECTED;
      }
      mIsOfferer = false;
      break;
    case kJsepStateHaveLocalOffer:
    case kJsepStateHaveRemotePranswer:
      if (type != kJsepSdpAnswer && type != kJsepSdpPranswer) {
        JSEP_SET_ERROR("Cannot set remote offer in state "
                       << GetStateStr(mState));
        return NS_ERROR_UNEXPECTED;
      }
      break;
    default:
      JSEP_SET_ERROR("Cannot set remote offer or answer in current state "
                     << GetStateStr(mState));
      return NS_ERROR_UNEXPECTED;
  }

  
  UniquePtr<Sdp> parsed;
  nsresult rv = ParseSdp(sdp, &parsed);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ValidateRemoteDescription(*parsed);
  NS_ENSURE_SUCCESS(rv, rv);

  bool iceLite =
      parsed->GetAttributeList().HasAttribute(SdpAttribute::kIceLiteAttribute);

  std::vector<std::string> iceOptions;
  if (parsed->GetAttributeList().HasAttribute(
          SdpAttribute::kIceOptionsAttribute)) {
    iceOptions = parsed->GetAttributeList().GetIceOptions().mValues;
  }

  switch (type) {
    case kJsepSdpOffer:
      rv = SetRemoteDescriptionOffer(Move(parsed));
      break;
    case kJsepSdpAnswer:
    case kJsepSdpPranswer:
      rv = SetRemoteDescriptionAnswer(type, Move(parsed));
      break;
    case kJsepSdpRollback:
      MOZ_CRASH(); 
  }

  if (NS_SUCCEEDED(rv)) {
    mRemoteIsIceLite = iceLite;
    mIceOptions = iceOptions;
  }

  return rv;
}

nsresult
JsepSessionImpl::HandleNegotiatedSession(const UniquePtr<Sdp>& local,
                                         const UniquePtr<Sdp>& remote)
{
  bool remoteIceLite =
      remote->GetAttributeList().HasAttribute(SdpAttribute::kIceLiteAttribute);

  mIceControlling = remoteIceLite || mIsOfferer;

  const Sdp& answer = mIsOfferer ? *remote : *local;

  SdpHelper::BundledMids bundledMids;
  nsresult rv = mSdpHelper.GetBundledMids(answer, &bundledMids);
  NS_ENSURE_SUCCESS(rv, rv);

  std::vector<JsepTrackPair> trackPairs;

  if (mTransports.size() < local->GetMediaSectionCount()) {
    JSEP_SET_ERROR("Fewer transports set up than m-lines");
    MOZ_ASSERT(false);
    return NS_ERROR_FAILURE;
  }

  
  
  for (size_t i = 0; i < local->GetMediaSectionCount(); ++i) {
    
    if (answer.GetMediaSection(i).GetPort() == 0) {
      mTransports[i]->Close();
      continue;
    }

    
    
    size_t transportLevel = i;
    bool usingBundle = false;
    {
      const SdpMediaSection& answerMsection(answer.GetMediaSection(i));
      if (answerMsection.GetAttributeList().HasAttribute(
            SdpAttribute::kMidAttribute)) {
        if (bundledMids.count(answerMsection.GetAttributeList().GetMid())) {
          const SdpMediaSection* masterBundleMsection =
            bundledMids[answerMsection.GetAttributeList().GetMid()];
          transportLevel = masterBundleMsection->GetLevel();
          usingBundle = true;
          if (i != transportLevel) {
            mTransports[i]->Close();
          }
        }
      }
    }

    RefPtr<JsepTransport> transport = mTransports[transportLevel];

    rv = FinalizeTransport(
        remote->GetMediaSection(transportLevel).GetAttributeList(),
        answer.GetMediaSection(transportLevel).GetAttributeList(),
        transport);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!answer.GetMediaSection(i).IsSending() &&
        !answer.GetMediaSection(i).IsReceiving()) {
      MOZ_MTLOG(ML_DEBUG, "Inactive m-section, skipping creation of negotiated "
                          "track pair.");
      continue;
    }

    JsepTrackPair trackPair;
    rv = MakeNegotiatedTrackPair(remote->GetMediaSection(i),
                                 local->GetMediaSection(i),
                                 transport,
                                 usingBundle,
                                 transportLevel,
                                 &trackPair);
    NS_ENSURE_SUCCESS(rv, rv);

    trackPairs.push_back(trackPair);
  }

  rv = SetUniquePayloadTypes();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mNegotiatedTrackPairs = trackPairs;

  
  if (NS_SUCCEEDED(rv)) {
    for (auto i = mLocalTracks.begin(); i != mLocalTracks.end(); ++i) {
      if (i->mAssignedMLine.isSome()) {
        i->mNegotiated = true;
      }
    }
  }

  mGeneratedLocalDescription.reset();
  return NS_OK;
}

nsresult
JsepSessionImpl::MakeNegotiatedTrackPair(const SdpMediaSection& remote,
                                         const SdpMediaSection& local,
                                         const RefPtr<JsepTransport>& transport,
                                         bool usingBundle,
                                         size_t transportLevel,
                                         JsepTrackPair* trackPairOut)
{
  MOZ_ASSERT(transport->mComponents);
  const SdpMediaSection& answer = mIsOfferer ? remote : local;

  bool sending;
  bool receiving;

  if (mIsOfferer) {
    receiving = answer.IsSending();
    sending = answer.IsReceiving();
  } else {
    sending = answer.IsSending();
    receiving = answer.IsReceiving();
  }

  MOZ_MTLOG(ML_DEBUG, "Negotiated m= line"
                          << " index=" << local.GetLevel()
                          << " type=" << local.GetMediaType()
                          << " sending=" << sending
                          << " receiving=" << receiving);

  trackPairOut->mLevel = local.GetLevel();

  MOZ_ASSERT(mRecvonlySsrcs.size() > local.GetLevel(),
             "Failed to set the default ssrc for an active m-section");
  trackPairOut->mRecvonlySsrc = mRecvonlySsrcs[local.GetLevel()];

  if (usingBundle) {
    trackPairOut->mBundleLevel = Some(transportLevel);
  }

  if (sending) {
    auto sendTrack = FindTrackByLevel(mLocalTracks, local.GetLevel());
    if (sendTrack == mLocalTracks.end()) {
      JSEP_SET_ERROR("Failed to find local track for level " <<
                     local.GetLevel()
                     << " in local SDP. This should never happen.");
      NS_ASSERTION(false, "Failed to find local track for level");
      return NS_ERROR_FAILURE;
    }

    nsresult rv = NegotiateTrack(remote,
                                 local,
                                 JsepTrack::kJsepTrackSending,
                                 &sendTrack->mTrack);
    NS_ENSURE_SUCCESS(rv, rv);

    trackPairOut->mSending = sendTrack->mTrack;
  }

  if (receiving) {
    auto recvTrack = FindTrackByLevel(mRemoteTracks, local.GetLevel());
    if (recvTrack == mRemoteTracks.end()) {
      JSEP_SET_ERROR("Failed to find remote track for level "
                     << local.GetLevel()
                     << " in remote SDP. This should never happen.");
      NS_ASSERTION(false, "Failed to find remote track for level");
      return NS_ERROR_FAILURE;
    }

    nsresult rv = NegotiateTrack(remote,
                                 local,
                                 JsepTrack::kJsepTrackReceiving,
                                 &recvTrack->mTrack);
    NS_ENSURE_SUCCESS(rv, rv);

    if (remote.GetAttributeList().HasAttribute(SdpAttribute::kSsrcAttribute)) {
      auto& ssrcs = remote.GetAttributeList().GetSsrc().mSsrcs;
      for (auto i = ssrcs.begin(); i != ssrcs.end(); ++i) {
        recvTrack->mTrack->AddSsrc(i->ssrc);
      }
    }

    if (trackPairOut->mBundleLevel.isSome() &&
        recvTrack->mTrack->GetSsrcs().empty() &&
        recvTrack->mTrack->GetMediaType() != SdpMediaSection::kApplication) {
      MOZ_MTLOG(ML_ERROR, "Bundled m-section has no ssrc attributes. "
                          "This may cause media packets to be dropped.");
    }

    trackPairOut->mReceiving = recvTrack->mTrack;
  }

  trackPairOut->mRtpTransport = transport;

  if (transport->mComponents == 2) {
    
    
    MOZ_MTLOG(ML_DEBUG, "RTCP-MUX is off");
    trackPairOut->mRtcpTransport = transport;
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::NegotiateTrack(const SdpMediaSection& remoteMsection,
                                const SdpMediaSection& localMsection,
                                JsepTrack::Direction direction,
                                RefPtr<JsepTrack>* track)
{
  UniquePtr<JsepTrackNegotiatedDetailsImpl> negotiatedDetails =
      MakeUnique<JsepTrackNegotiatedDetailsImpl>();
  negotiatedDetails->mProtocol = remoteMsection.GetProtocol();

  auto& answerMsection = mIsOfferer ? remoteMsection : localMsection;

  for (auto& format : answerMsection.GetFormats()) {
    JsepCodecDescription* origCodec = FindMatchingCodec(format, answerMsection);
    if (!origCodec) {
      continue;
    }

    
    
    for (auto& remoteFormat : remoteMsection.GetFormats()) {
      if (origCodec->Matches(remoteFormat, remoteMsection)) {
        origCodec->mDefaultPt = remoteFormat;
        break;
      }
    }

    UniquePtr<JsepCodecDescription> codec(origCodec->Clone());

    bool sending = (direction == JsepTrack::kJsepTrackSending);

    
    
    
    
    
    
    
    if (sending) {
      if (!codec->LoadSendParameters(remoteMsection)) {
        continue;
      }
    } else {
      if (!codec->LoadRecvParameters(remoteMsection)) {
        continue;
      }
    }

    if (remoteMsection.GetMediaType() == SdpMediaSection::kAudio ||
        remoteMsection.GetMediaType() == SdpMediaSection::kVideo) {
      
      uint16_t payloadType;
      if (!codec->GetPtAsInt(&payloadType) ||
          payloadType > UINT8_MAX) {
        JSEP_SET_ERROR("audio/video payload type is not an 8 bit unsigned int: "
                       << codec->mDefaultPt);
        return NS_ERROR_INVALID_ARG;
      }
    }

    negotiatedDetails->mCodecs.values.push_back(codec.release());
    break;
  }

  if (negotiatedDetails->mCodecs.values.empty()) {
    JSEP_SET_ERROR("Failed to negotiate codec details for all codecs");
    return NS_ERROR_INVALID_ARG;
  }

  if (answerMsection.GetAttributeList().HasAttribute(
        SdpAttribute::kExtmapAttribute)) {
    auto& extmap = answerMsection.GetAttributeList().GetExtmap().mExtmaps;
    for (auto i = extmap.begin(); i != extmap.end(); ++i) {
      negotiatedDetails->mExtmap[i->extensionname] = *i;
    }
  }

  (*track)->SetNegotiatedDetails(Move(negotiatedDetails));
  return NS_OK;
}

void
JsepSessionImpl::UpdateTransport(const SdpMediaSection& msection,
                                 JsepTransport* transport)
{
  if (mSdpHelper.MsectionIsDisabled(msection)) {
    transport->Close();
    return;
  }

  if (mSdpHelper.HasRtcp(msection.GetProtocol())) {
    transport->mComponents = 2;
  } else {
    transport->mComponents = 1;
  }

  if (msection.GetAttributeList().HasAttribute(SdpAttribute::kMidAttribute)) {
    transport->mTransportId = msection.GetAttributeList().GetMid();
  } else {
    std::ostringstream os;
    os << "level_" << msection.GetLevel() << "(no mid)";
    transport->mTransportId = os.str();
  }
}

nsresult
JsepSessionImpl::FinalizeTransport(const SdpAttributeList& remote,
                                   const SdpAttributeList& answer,
                                   const RefPtr<JsepTransport>& transport)
{
  UniquePtr<JsepIceTransport> ice = MakeUnique<JsepIceTransport>();

  
  ice->mUfrag = remote.GetIceUfrag();
  ice->mPwd = remote.GetIcePwd();
  if (remote.HasAttribute(SdpAttribute::kCandidateAttribute)) {
    ice->mCandidates = remote.GetCandidate();
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  UniquePtr<JsepDtlsTransport> dtls = MakeUnique<JsepDtlsTransport>();
  dtls->mFingerprints = remote.GetFingerprint();
  if (!answer.HasAttribute(mozilla::SdpAttribute::kSetupAttribute)) {
    dtls->mRole = mIsOfferer ? JsepDtlsTransport::kJsepDtlsServer
                             : JsepDtlsTransport::kJsepDtlsClient;
  } else {
    if (mIsOfferer) {
      dtls->mRole = (answer.GetSetup().mRole == SdpSetupAttribute::kActive)
                        ? JsepDtlsTransport::kJsepDtlsServer
                        : JsepDtlsTransport::kJsepDtlsClient;
    } else {
      dtls->mRole = (answer.GetSetup().mRole == SdpSetupAttribute::kActive)
                        ? JsepDtlsTransport::kJsepDtlsClient
                        : JsepDtlsTransport::kJsepDtlsServer;
    }
  }

  transport->mIce = Move(ice);
  transport->mDtls = Move(dtls);

  if (answer.HasAttribute(SdpAttribute::kRtcpMuxAttribute)) {
    transport->mComponents = 1;
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::AddTransportAttributes(SdpMediaSection* msection,
                                        SdpSetupAttribute::Role dtlsRole)
{
  if (mIceUfrag.empty() || mIcePwd.empty()) {
    JSEP_SET_ERROR("Missing ICE ufrag or password");
    return NS_ERROR_FAILURE;
  }

  SdpAttributeList& attrList = msection->GetAttributeList();
  attrList.SetAttribute(
      new SdpStringAttribute(SdpAttribute::kIceUfragAttribute, mIceUfrag));
  attrList.SetAttribute(
      new SdpStringAttribute(SdpAttribute::kIcePwdAttribute, mIcePwd));

  msection->GetAttributeList().SetAttribute(new SdpSetupAttribute(dtlsRole));

  return NS_OK;
}

nsresult
JsepSessionImpl::SetupTransportParams(const Sdp& oldAnswer,
                                      const Sdp& newOffer,
                                      Sdp* newLocal)
{
  for (size_t i = 0; i < oldAnswer.GetMediaSectionCount(); ++i) {
    if (!mSdpHelper.MsectionIsDisabled(newLocal->GetMediaSection(i)) &&
        mSdpHelper.AreOldTransportParamsValid(oldAnswer, newOffer, i)) {
      
      
      
      size_t numComponents = mTransports[i]->mComponents;
      nsresult rv = mSdpHelper.CopyTransportParams(
          numComponents,
          mCurrentLocalDescription->GetMediaSection(i),
          &newLocal->GetMediaSection(i));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::ParseSdp(const std::string& sdp, UniquePtr<Sdp>* parsedp)
{
  UniquePtr<Sdp> parsed = mParser.Parse(sdp);
  if (!parsed) {
    std::string error = "Failed to parse SDP: ";
    mSdpHelper.appendSdpParseErrors(mParser.GetParseErrors(), &error);
    JSEP_SET_ERROR(error);
    return NS_ERROR_INVALID_ARG;
  }

  
  if (!parsed->GetMediaSectionCount()) {
    JSEP_SET_ERROR("Description has no media sections");
    return NS_ERROR_INVALID_ARG;
  }

  std::set<std::string> trackIds;

  for (size_t i = 0; i < parsed->GetMediaSectionCount(); ++i) {
    if (mSdpHelper.MsectionIsDisabled(parsed->GetMediaSection(i))) {
      
      continue;
    }

    auto& mediaAttrs = parsed->GetMediaSection(i).GetAttributeList();

    if (mediaAttrs.GetIceUfrag().empty()) {
      JSEP_SET_ERROR("Invalid description, no ice-ufrag attribute");
      return NS_ERROR_INVALID_ARG;
    }

    if (mediaAttrs.GetIcePwd().empty()) {
      JSEP_SET_ERROR("Invalid description, no ice-pwd attribute");
      return NS_ERROR_INVALID_ARG;
    }

    if (!mediaAttrs.HasAttribute(SdpAttribute::kFingerprintAttribute)) {
      JSEP_SET_ERROR("Invalid description, no fingerprint attribute");
      return NS_ERROR_INVALID_ARG;
    }

    const SdpFingerprintAttributeList& fingerprints(
        mediaAttrs.GetFingerprint());
    if (fingerprints.mFingerprints.empty()) {
      JSEP_SET_ERROR("Invalid description, no supported fingerprint algorithms "
                     "present");
      return NS_ERROR_INVALID_ARG;
    }

    if (mediaAttrs.HasAttribute(SdpAttribute::kSetupAttribute) &&
        mediaAttrs.GetSetup().mRole == SdpSetupAttribute::kHoldconn) {
      JSEP_SET_ERROR("Description has illegal setup attribute "
                     "\"holdconn\" at level "
                     << i);
      return NS_ERROR_INVALID_ARG;
    }

    auto& formats = parsed->GetMediaSection(i).GetFormats();
    for (auto f = formats.begin(); f != formats.end(); ++f) {
      uint16_t pt;
      if (!JsepCodecDescription::GetPtAsInt(*f, &pt)) {
        JSEP_SET_ERROR("Payload type \""
                       << *f << "\" is not a 16-bit unsigned int at level "
                       << i);
        return NS_ERROR_INVALID_ARG;
      }
    }

    std::string streamId;
    std::string trackId;
    nsresult rv = mSdpHelper.GetIdsFromMsid(*parsed,
                                            parsed->GetMediaSection(i),
                                            &streamId,
                                            &trackId);

    if (NS_SUCCEEDED(rv)) {
      if (trackIds.count(trackId)) {
        JSEP_SET_ERROR("track id:" << trackId
                       << " appears in more than one m-section at level " << i);
        return NS_ERROR_INVALID_ARG;
      }

      trackIds.insert(trackId);
    } else if (rv != NS_ERROR_NOT_AVAILABLE) {
      
      return rv;
    }
  }

  *parsedp = Move(parsed);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetRemoteDescriptionOffer(UniquePtr<Sdp> offer)
{
  MOZ_ASSERT(mState == kJsepStateStable);

  
  
  nsresult rv = SetRemoteTracksFromDescription(offer.get());
  NS_ENSURE_SUCCESS(rv, rv);

  mPendingRemoteDescription = Move(offer);

  SetState(kJsepStateHaveRemoteOffer);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetRemoteDescriptionAnswer(JsepSdpType type,
                                            UniquePtr<Sdp> answer)
{
  MOZ_ASSERT(mState == kJsepStateHaveLocalOffer ||
             mState == kJsepStateHaveRemotePranswer);

  mPendingRemoteDescription = Move(answer);

  nsresult rv = ValidateAnswer(*mPendingLocalDescription,
                               *mPendingRemoteDescription);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = SetRemoteTracksFromDescription(mPendingRemoteDescription.get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = HandleNegotiatedSession(mPendingLocalDescription,
                               mPendingRemoteDescription);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentRemoteDescription = Move(mPendingRemoteDescription);
  mCurrentLocalDescription = Move(mPendingLocalDescription);
  mWasOffererLastTime = mIsOfferer;

  SetState(kJsepStateStable);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetRemoteTracksFromDescription(const Sdp* remoteDescription)
{
  
  for (auto i = mRemoteTracks.begin(); i != mRemoteTracks.end(); ++i) {
    i->mAssignedMLine.reset();
  }

  
  if (remoteDescription) {
    size_t numMlines = remoteDescription->GetMediaSectionCount();
    nsresult rv;

    
    for (size_t i = 0; i < numMlines; ++i) {
      const SdpMediaSection& msection = remoteDescription->GetMediaSection(i);

      if (mSdpHelper.MsectionIsDisabled(msection) || !msection.IsSending()) {
        continue;
      }

      std::vector<JsepReceivingTrack>::iterator track;

      if (msection.GetMediaType() == SdpMediaSection::kApplication) {
        
        track = FindUnassignedTrackByType(mRemoteTracks,
                                          msection.GetMediaType());
      } else {
        std::string streamId;
        std::string trackId;
        rv = GetRemoteIds(*remoteDescription, msection, &streamId, &trackId);
        NS_ENSURE_SUCCESS(rv, rv);

        track = FindTrackByIds(mRemoteTracks, streamId, trackId);
      }

      if (track == mRemoteTracks.end()) {
        RefPtr<JsepTrack> track;
        rv = CreateReceivingTrack(i, *remoteDescription, msection, &track);
        NS_ENSURE_SUCCESS(rv, rv);

        JsepReceivingTrack rtrack;
        rtrack.mTrack = track;
        rtrack.mAssignedMLine = Some(i);
        mRemoteTracks.push_back(rtrack);
        mRemoteTracksAdded.push_back(rtrack);
      } else {
        track->mAssignedMLine = Some(i);
      }
    }
  }

  
  for (size_t i = 0; i < mRemoteTracks.size();) {
    if (!mRemoteTracks[i].mAssignedMLine.isSome()) {
      mRemoteTracksRemoved.push_back(mRemoteTracks[i]);
      mRemoteTracks.erase(mRemoteTracks.begin() + i);
    } else {
      ++i;
    }
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::ValidateLocalDescription(const Sdp& description)
{
  
  if (!mGeneratedLocalDescription) {
    JSEP_SET_ERROR("Calling SetLocal without first calling CreateOffer/Answer"
                   " is not supported.");
    return NS_ERROR_UNEXPECTED;
  }

  if (description.GetMediaSectionCount() !=
      mGeneratedLocalDescription->GetMediaSectionCount()) {
    JSEP_SET_ERROR("Changing the number of m-sections is not allowed");
    return NS_ERROR_INVALID_ARG;
  }

  for (size_t i = 0; i < description.GetMediaSectionCount(); ++i) {
    auto& origMsection = mGeneratedLocalDescription->GetMediaSection(i);
    auto& finalMsection = description.GetMediaSection(i);
    if (origMsection.GetMediaType() != finalMsection.GetMediaType()) {
      JSEP_SET_ERROR("Changing the media-type of m-sections is not allowed");
      return NS_ERROR_INVALID_ARG;
    }

    
    if (!mCurrentLocalDescription) {
      if (finalMsection.GetAttributeList().HasAttribute(
              SdpAttribute::kCandidateAttribute)) {
        JSEP_SET_ERROR("Adding your own candidate attributes is not supported");
        return NS_ERROR_INVALID_ARG;
      }

      if (finalMsection.GetAttributeList().HasAttribute(
              SdpAttribute::kEndOfCandidatesAttribute)) {
        JSEP_SET_ERROR("Why are you trying to set a=end-of-candidates?");
        return NS_ERROR_INVALID_ARG;
      }
    }

    
    
    
    
    
    
  }

  if (description.GetAttributeList().HasAttribute(
          SdpAttribute::kIceLiteAttribute)) {
    JSEP_SET_ERROR("Running ICE in lite mode is unsupported");
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::ValidateRemoteDescription(const Sdp& description)
{
  if (!mCurrentRemoteDescription || !mCurrentLocalDescription) {
    
    
    return NS_OK;
  }

  if (mCurrentRemoteDescription->GetMediaSectionCount() >
      description.GetMediaSectionCount()) {
    JSEP_SET_ERROR("New remote description has fewer m-sections than the "
                   "previous remote description.");
    return NS_ERROR_INVALID_ARG;
  }

  
  SdpHelper::BundledMids bundledMids;
  nsresult rv = GetNegotiatedBundledMids(&bundledMids);
  NS_ENSURE_SUCCESS(rv, rv);

  SdpHelper::BundledMids newBundledMids;
  rv = mSdpHelper.GetBundledMids(description, &newBundledMids);
  NS_ENSURE_SUCCESS(rv, rv);

  for (size_t i = 0;
       i < mCurrentRemoteDescription->GetMediaSectionCount();
       ++i) {
    if (mSdpHelper.MsectionIsDisabled(description.GetMediaSection(i)) ||
        mSdpHelper.MsectionIsDisabled(mCurrentRemoteDescription->GetMediaSection(i))) {
      continue;
    }

    const SdpAttributeList& newAttrs(
        description.GetMediaSection(i).GetAttributeList());
    const SdpAttributeList& oldAttrs(
        mCurrentRemoteDescription->GetMediaSection(i).GetAttributeList());

    if ((newAttrs.GetIceUfrag() != oldAttrs.GetIceUfrag()) ||
        (newAttrs.GetIcePwd() != oldAttrs.GetIcePwd())) {
      JSEP_SET_ERROR("ICE restart is unsupported at this time "
                     "(new remote description changes either the ice-ufrag "
                     "or ice-pwd)" <<
                     "ice-ufrag (old): " << oldAttrs.GetIceUfrag() <<
                     "ice-ufrag (new): " << newAttrs.GetIceUfrag() <<
                     "ice-pwd (old): " << oldAttrs.GetIcePwd() <<
                     "ice-pwd (new): " << newAttrs.GetIcePwd());
      return NS_ERROR_INVALID_ARG;
    }
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::ValidateAnswer(const Sdp& offer, const Sdp& answer)
{
  if (offer.GetMediaSectionCount() != answer.GetMediaSectionCount()) {
    JSEP_SET_ERROR("Offer and answer have different number of m-lines "
                   << "(" << offer.GetMediaSectionCount() << " vs "
                   << answer.GetMediaSectionCount() << ")");
    return NS_ERROR_INVALID_ARG;
  }

  for (size_t i = 0; i < offer.GetMediaSectionCount(); ++i) {
    const SdpMediaSection& offerMsection = offer.GetMediaSection(i);
    const SdpMediaSection& answerMsection = answer.GetMediaSection(i);

    if (offerMsection.GetMediaType() != answerMsection.GetMediaType()) {
      JSEP_SET_ERROR(
          "Answer and offer have different media types at m-line " << i);
      return NS_ERROR_INVALID_ARG;
    }

    if (!offerMsection.IsSending() && answerMsection.IsReceiving()) {
      JSEP_SET_ERROR("Answer tried to set recv when offer did not set send");
      return NS_ERROR_INVALID_ARG;
    }

    if (!offerMsection.IsReceiving() && answerMsection.IsSending()) {
      JSEP_SET_ERROR("Answer tried to set send when offer did not set recv");
      return NS_ERROR_INVALID_ARG;
    }

    const SdpAttributeList& answerAttrs(answerMsection.GetAttributeList());
    const SdpAttributeList& offerAttrs(offerMsection.GetAttributeList());
    if (answerAttrs.HasAttribute(SdpAttribute::kMidAttribute) &&
        offerAttrs.HasAttribute(SdpAttribute::kMidAttribute) &&
        offerAttrs.GetMid() != answerAttrs.GetMid()) {
      JSEP_SET_ERROR("Answer changes mid for level, was \'"
                     << offerMsection.GetAttributeList().GetMid()
                     << "\', now \'"
                     << answerMsection.GetAttributeList().GetMid() << "\'");
      return NS_ERROR_INVALID_ARG;
    }
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::CreateReceivingTrack(size_t mline,
                                      const Sdp& sdp,
                                      const SdpMediaSection& msection,
                                      RefPtr<JsepTrack>* track)
{
  std::string streamId;
  std::string trackId;

  nsresult rv = GetRemoteIds(sdp, msection, &streamId, &trackId);
  NS_ENSURE_SUCCESS(rv, rv);

  *track = new JsepTrack(msection.GetMediaType(),
                         streamId,
                         trackId,
                         JsepTrack::kJsepTrackReceiving);

  (*track)->SetCNAME(mSdpHelper.GetCNAME(msection));

  return NS_OK;
}

nsresult
JsepSessionImpl::CreateGenericSDP(UniquePtr<Sdp>* sdpp)
{
  
  
  
  
  
  
  
  
  
  
  
  

  auto origin =
      SdpOrigin("mozilla...THIS_IS_SDPARTA-" MOZ_APP_UA_VERSION,
                mSessionId,
                mSessionVersion,
                sdp::kIPv4,
                "0.0.0.0");

  UniquePtr<Sdp> sdp = MakeUnique<SipccSdp>(origin);

  if (mDtlsFingerprints.empty()) {
    JSEP_SET_ERROR("Missing DTLS fingerprint");
    return NS_ERROR_FAILURE;
  }

  UniquePtr<SdpFingerprintAttributeList> fpl =
      MakeUnique<SdpFingerprintAttributeList>();
  for (auto fp = mDtlsFingerprints.begin(); fp != mDtlsFingerprints.end();
       ++fp) {
    fpl->PushEntry(fp->mAlgorithm, fp->mValue);
  }
  sdp->GetAttributeList().SetAttribute(fpl.release());

  auto* iceOpts = new SdpOptionsAttribute(SdpAttribute::kIceOptionsAttribute);
  iceOpts->PushEntry("trickle");
  sdp->GetAttributeList().SetAttribute(iceOpts);

  
  
  std::vector<std::string> msids;
  msids.push_back("*");
  mSdpHelper.SetupMsidSemantic(msids, sdp.get());

  *sdpp = Move(sdp);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetupIds()
{
  SECStatus rv = PK11_GenerateRandom(
      reinterpret_cast<unsigned char*>(&mSessionId), sizeof(mSessionId));
  
  
  mSessionId = mSessionId >> 1;
  if (rv != SECSuccess) {
    JSEP_SET_ERROR("Failed to generate session id: " << rv);
    return NS_ERROR_FAILURE;
  }

  if (!mUuidGen->Generate(&mDefaultRemoteStreamId)) {
    JSEP_SET_ERROR("Failed to generate default uuid for streams");
    return NS_ERROR_FAILURE;
  }

  if (!mUuidGen->Generate(&mCNAME)) {
    JSEP_SET_ERROR("Failed to generate CNAME");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::CreateSsrc(uint32_t* ssrc)
{
  do {
    SECStatus rv = PK11_GenerateRandom(
        reinterpret_cast<unsigned char*>(ssrc), sizeof(uint32_t));
    if (rv != SECSuccess) {
      JSEP_SET_ERROR("Failed to generate SSRC, error=" << rv);
      return NS_ERROR_FAILURE;
    }
  } while (mSsrcs.count(*ssrc));
  mSsrcs.insert(*ssrc);

  return NS_OK;
}

void
JsepSessionImpl::SetupDefaultCodecs()
{
  
  mCodecs.values.push_back(new JsepAudioCodecDescription(
      "109",
      "opus",
      48000,
      2,
      960,
      16000));

  mCodecs.values.push_back(new JsepAudioCodecDescription(
      "9",
      "G722",
      8000,
      1,
      320,
      64000));

  
  
  mCodecs.values.push_back(
      new JsepAudioCodecDescription("0",
                                    "PCMU",
                                    8000,
                                    1,
                                    8000 / 50,   
                                    8 * 8000 * 1 
                                    ));

  mCodecs.values.push_back(
      new JsepAudioCodecDescription("8",
                                    "PCMA",
                                    8000,
                                    1,
                                    8000 / 50,   
                                    8 * 8000 * 1 
                                    ));

  
  JsepVideoCodecDescription* vp8 = new JsepVideoCodecDescription(
      "120",
      "VP8",
      90000
      );
  
  vp8->mMaxFs = 12288;
  vp8->mMaxFr = 60;
  mCodecs.values.push_back(vp8);

  JsepVideoCodecDescription* vp9 = new JsepVideoCodecDescription(
      "121",
      "VP9",
      90000
      );
  
  vp9->mMaxFs = 12288;
  vp9->mMaxFr = 60;
  mCodecs.values.push_back(vp9);

  JsepVideoCodecDescription* h264_1 = new JsepVideoCodecDescription(
      "126",
      "H264",
      90000
      );
  h264_1->mPacketizationMode = 1;
  
  h264_1->mProfileLevelId = 0x42E00D;
  mCodecs.values.push_back(h264_1);

  JsepVideoCodecDescription* h264_0 = new JsepVideoCodecDescription(
      "97",
      "H264",
      90000
      );
  h264_0->mPacketizationMode = 0;
  
  h264_0->mProfileLevelId = 0x42E00D;
  mCodecs.values.push_back(h264_0);

  mCodecs.values.push_back(new JsepApplicationCodecDescription(
      "5000",
      "webrtc-datachannel",
      WEBRTC_DATACHANNEL_STREAMS_DEFAULT
      ));
}

void
JsepSessionImpl::SetupDefaultRtpExtensions()
{
  AddAudioRtpExtension("urn:ietf:params:rtp-hdrext:ssrc-audio-level");
}

void
JsepSessionImpl::SetState(JsepSignalingState state)
{
  if (state == mState)
    return;

  MOZ_MTLOG(ML_NOTICE, "[" << mName << "]: " <<
            GetStateStr(mState) << " -> " << GetStateStr(state));
  mState = state;
}

nsresult
JsepSessionImpl::AddRemoteIceCandidate(const std::string& candidate,
                                       const std::string& mid,
                                       uint16_t level)
{
  mLastError.clear();

  mozilla::Sdp* sdp = 0;

  if (mPendingRemoteDescription) {
    sdp = mPendingRemoteDescription.get();
  } else if (mCurrentRemoteDescription) {
    sdp = mCurrentRemoteDescription.get();
  } else {
    JSEP_SET_ERROR("Cannot add ICE candidate in state " << GetStateStr(mState));
    return NS_ERROR_UNEXPECTED;
  }

  return mSdpHelper.AddCandidateToSdp(sdp, candidate, mid, level);
}

nsresult
JsepSessionImpl::AddLocalIceCandidate(const std::string& candidate,
                                      const std::string& mid,
                                      uint16_t level,
                                      bool* skipped)
{
  mLastError.clear();

  mozilla::Sdp* sdp = 0;

  if (mPendingLocalDescription) {
    sdp = mPendingLocalDescription.get();
  } else if (mCurrentLocalDescription) {
    sdp = mCurrentLocalDescription.get();
  } else {
    JSEP_SET_ERROR("Cannot add ICE candidate in state " << GetStateStr(mState));
    return NS_ERROR_UNEXPECTED;
  }

  if (sdp->GetMediaSectionCount() <= level) {
    
    *skipped = true;
    return NS_OK;
  }

  if (mState == kJsepStateStable) {
    const Sdp* answer(GetAnswer());
    if (mSdpHelper.IsBundleSlave(*answer, level)) {
      
      
      *skipped = true;
      return NS_OK;
    }
  }

  *skipped = false;

  return mSdpHelper.AddCandidateToSdp(sdp, candidate, mid, level);
}

nsresult
JsepSessionImpl::EndOfLocalCandidates(const std::string& defaultCandidateAddr,
                                      uint16_t defaultCandidatePort,
                                      const std::string& defaultRtcpCandidateAddr,
                                      uint16_t defaultRtcpCandidatePort,
                                      uint16_t level)
{
  mLastError.clear();

  mozilla::Sdp* sdp = 0;

  if (mPendingLocalDescription) {
    sdp = mPendingLocalDescription.get();
  } else if (mCurrentLocalDescription) {
    sdp = mCurrentLocalDescription.get();
  } else {
    JSEP_SET_ERROR("Cannot add ICE candidate in state " << GetStateStr(mState));
    return NS_ERROR_UNEXPECTED;
  }

  if (level >= sdp->GetMediaSectionCount()) {
    return NS_OK;
  }

  SdpHelper::BundledMids bundledMids;
  nsresult rv = GetNegotiatedBundledMids(&bundledMids);
  if (NS_FAILED(rv)) {
    MOZ_ASSERT(false);
    mLastError += " (This should have been caught sooner!)";
    return NS_ERROR_FAILURE;
  }

  std::string defaultRtcpCandidateAddrCopy(defaultRtcpCandidateAddr);
  if (mState == kJsepStateStable && mTransports[level]->mComponents == 1) {
    
    defaultRtcpCandidateAddrCopy = "";
    defaultRtcpCandidatePort = 0;
  }

  SdpMediaSection& msection = sdp->GetMediaSection(level);

  
  if (mState == kJsepStateStable) {
    
    if (msection.GetAttributeList().HasAttribute(SdpAttribute::kMidAttribute)) {
      std::string mid(msection.GetAttributeList().GetMid());
      if (bundledMids.count(mid)) {
        const SdpMediaSection* masterBundleMsection(bundledMids[mid]);
        if (msection.GetLevel() != masterBundleMsection->GetLevel()) {
          
          return NS_OK;
        }

        
        
        for (auto i = bundledMids.begin(); i != bundledMids.end(); ++i) {
          if (i->second != masterBundleMsection) {
            continue;
          }
          SdpMediaSection* bundledMsection =
            mSdpHelper.FindMsectionByMid(*sdp, i->first);
          if (!bundledMsection) {
            MOZ_ASSERT(false);
            continue;
          }
          mSdpHelper.SetDefaultAddresses(defaultCandidateAddr,
                                         defaultCandidatePort,
                                         defaultRtcpCandidateAddrCopy,
                                         defaultRtcpCandidatePort,
                                         bundledMsection);
        }
      }
    }
  }

  mSdpHelper.SetDefaultAddresses(defaultCandidateAddr,
                                 defaultCandidatePort,
                                 defaultRtcpCandidateAddrCopy,
                                 defaultRtcpCandidatePort,
                                 &msection);

  

  SdpAttributeList& attrs = msection.GetAttributeList();
  attrs.SetAttribute(
      new SdpFlagAttribute(SdpAttribute::kEndOfCandidatesAttribute));
  if (!mIsOfferer) {
    attrs.RemoveAttribute(SdpAttribute::kIceOptionsAttribute);
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::GetNegotiatedBundledMids(SdpHelper::BundledMids* bundledMids)
{
  const Sdp* answerSdp = GetAnswer();

  if (!answerSdp) {
    return NS_OK;
  }

  return mSdpHelper.GetBundledMids(*answerSdp, bundledMids);
}

void
JsepSessionImpl::DisableMsection(Sdp* sdp, SdpMediaSection* msection) const
{
  mSdpHelper.DisableMsection(sdp, msection);
  msection->ClearCodecs();

  
  
  
  
  msection->AddCodec("111", "NULL", 0, 0);
}

nsresult
JsepSessionImpl::EnableOfferMsection(SdpMediaSection* msection)
{
  
  
  MOZ_ASSERT(mSdpHelper.MsectionIsDisabled(*msection));

  msection->SetPort(9);

  
  
  if (mSdpHelper.HasRtcp(msection->GetProtocol())) {
    
    msection->GetAttributeList().SetAttribute(
        new SdpFlagAttribute(SdpAttribute::kRtcpMuxAttribute));
  }

  nsresult rv = AddTransportAttributes(msection, SdpSetupAttribute::kActpass);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetRecvonlySsrc(msection);
  NS_ENSURE_SUCCESS(rv, rv);

  AddCodecs(msection);

  AddExtmap(msection);

  std::ostringstream osMid;
  osMid << "sdparta_" << msection->GetLevel();
  AddMid(osMid.str(), msection);

  return NS_OK;
}

nsresult
JsepSessionImpl::GetAllPayloadTypes(
    const JsepTrackNegotiatedDetails& trackDetails,
    std::vector<uint8_t>* payloadTypesOut)
{
  for (size_t j = 0; j < trackDetails.GetCodecCount(); ++j) {
    const JsepCodecDescription* codec;
    nsresult rv = trackDetails.GetCodec(j, &codec);
    if (NS_FAILED(rv)) {
      JSEP_SET_ERROR("GetCodec failed in GetAllPayloadTypes. rv="
                     << static_cast<uint32_t>(rv));
      MOZ_ASSERT(false);
      return NS_ERROR_FAILURE;
    }

    uint16_t payloadType;
    if (!codec->GetPtAsInt(&payloadType) || payloadType > UINT8_MAX) {
      JSEP_SET_ERROR("Non-UINT8 payload type in GetAllPayloadTypes ("
                     << codec->mType
                     << "), this should have been caught sooner.");
      MOZ_ASSERT(false);
      return NS_ERROR_INVALID_ARG;
    }

    payloadTypesOut->push_back(payloadType);
  }

  return NS_OK;
}





nsresult
JsepSessionImpl::SetUniquePayloadTypes()
{
  
  
  std::map<uint8_t, JsepTrackNegotiatedDetails*> payloadTypeToDetailsMap;

  for (size_t i = 0; i < mRemoteTracks.size(); ++i) {
    auto track = mRemoteTracks[i].mTrack;

    if (track->GetMediaType() == SdpMediaSection::kApplication) {
      continue;
    }

    auto* details = track->GetNegotiatedDetails();
    if (!details) {
      
      continue;
    }

    
    details->ClearUniquePayloadTypes();

    std::vector<uint8_t> payloadTypesForTrack;
    nsresult rv = GetAllPayloadTypes(*details, &payloadTypesForTrack);
    NS_ENSURE_SUCCESS(rv, rv);

    for (auto j = payloadTypesForTrack.begin();
         j != payloadTypesForTrack.end();
         ++j) {

      if (payloadTypeToDetailsMap.count(*j)) {
        
        payloadTypeToDetailsMap[*j] = nullptr;
      } else {
        payloadTypeToDetailsMap[*j] = details;
      }
    }
  }

  for (auto i = payloadTypeToDetailsMap.begin();
       i != payloadTypeToDetailsMap.end();
       ++i) {
    uint8_t uniquePt = i->first;
    auto trackDetails = i->second;

    if (!trackDetails) {
      continue;
    }

    trackDetails->AddUniquePayloadType(uniquePt);
  }

  return NS_OK;
}

const Sdp*
JsepSessionImpl::GetAnswer() const
{
  return mWasOffererLastTime ? mCurrentRemoteDescription.get()
                             : mCurrentLocalDescription.get();
}

nsresult
JsepSessionImpl::Close()
{
  mLastError.clear();
  SetState(kJsepStateClosed);
  return NS_OK;
}

const std::string
JsepSessionImpl::GetLastError() const
{
  return mLastError;
}

bool
JsepSessionImpl::AllLocalTracksAreAssigned() const
{
  for (auto i = mLocalTracks.begin(); i != mLocalTracks.end(); ++i) {
    if (!i->mAssignedMLine.isSome()) {
      return false;
    }
  }

  return true;
}

} 
