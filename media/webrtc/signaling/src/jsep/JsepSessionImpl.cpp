



#include "logging.h"

#include "signaling/src/jsep/JsepSessionImpl.h"
#include <string>
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

namespace mozilla {

MOZ_MTLOG_MODULE("jsep")

#define JSEP_SET_ERROR(error)                                                  \
  do {                                                                         \
    std::ostringstream os;                                                     \
    os << error;                                                               \
    mLastError = os.str();                                                     \
    MOZ_MTLOG(ML_ERROR, mLastError);                                           \
  } while (0);

JsepSessionImpl::~JsepSessionImpl()
{
  for (auto i = mCodecs.begin(); i != mCodecs.end(); ++i) {
    delete *i;
  }
}

nsresult
JsepSessionImpl::Init()
{
  mLastError.clear();

  MOZ_ASSERT(!mSessionId, "Init called more than once");

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

  SetupDefaultCodecs();
  SetupDefaultRtpExtensions();

  return NS_OK;
}

nsresult
JsepSessionImpl::AddTrack(const RefPtr<JsepTrack>& track)
{
  mLastError.clear();
  MOZ_ASSERT(track->GetDirection() == JsepTrack::kJsepTrackSending);

  JsepSendingTrack strack;
  strack.mTrack = track;

  mLocalTracks.push_back(strack);

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

nsresult
JsepSessionImpl::GetLocalTrack(size_t index, RefPtr<JsepTrack>* track) const
{
  if (index >= mLocalTracks.size()) {
    return NS_ERROR_INVALID_ARG;
  }

  *track = mLocalTracks[index].mTrack;

  return NS_OK;
}

nsresult
JsepSessionImpl::GetRemoteTrack(size_t index, RefPtr<JsepTrack>* track) const
{
  if (index >= mRemoteTracks.size()) {
    return NS_ERROR_INVALID_ARG;
  }

  *track = mRemoteTracks[index].mTrack;

  return NS_OK;
}

nsresult
JsepSessionImpl::AddOfferMSectionsByType(SdpMediaSection::MediaType mediatype,
                                         Maybe<size_t> offerToReceive,
                                         Sdp* sdp)
{

  
  SdpMediaSection::Protocol proto = SdpMediaSection::kRtpSavpf;

  if (mediatype == SdpMediaSection::kApplication) {
    proto = SdpMediaSection::kDtlsSctp;
  }

  size_t added = 0;

  for (auto track = mLocalTracks.begin(); track != mLocalTracks.end();
       ++track) {
    if (mediatype != track->mTrack->GetMediaType()) {
      continue;
    }

    SdpDirectionAttribute::Direction dir = SdpDirectionAttribute::kSendrecv;

    ++added;

    
    
    if (offerToReceive.isSome() && added > *offerToReceive) {
      dir = SdpDirectionAttribute::kSendonly;
    }

    nsresult rv = CreateOfferMSection(mediatype, dir, proto, sdp);

    NS_ENSURE_SUCCESS(rv, rv);

    track->mAssignedMLine = Some(sdp->GetMediaSectionCount() - 1);
  }

  while (offerToReceive.isSome() && added < *offerToReceive) {
    nsresult rv = CreateOfferMSection(
        mediatype, SdpDirectionAttribute::kRecvonly, proto, sdp);

    NS_ENSURE_SUCCESS(rv, rv);
    ++added;
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::CreateOffer(const JsepOfferOptions& options,
                             std::string* offer)
{
  mLastError.clear();

  switch (mState) {
    case kJsepStateStable:
      break;
    default:
      JSEP_SET_ERROR("Cannot create offer in state " << GetStateStr(mState));
      return NS_ERROR_UNEXPECTED;
  }

  UniquePtr<Sdp> sdp;

  
  nsresult rv = CreateGenericSDP(&sdp);
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  
  
  rv = AddOfferMSectionsByType(
      SdpMediaSection::kAudio, options.mOfferToReceiveAudio, sdp.get());

  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddOfferMSectionsByType(
      SdpMediaSection::kVideo, options.mOfferToReceiveVideo, sdp.get());

  NS_ENSURE_SUCCESS(rv, rv);

  if (!(options.mDontOfferDataChannel.isSome() &&
        *options.mDontOfferDataChannel)) {
    rv = AddOfferMSectionsByType(
        SdpMediaSection::kApplication, Maybe<size_t>(), sdp.get());

    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!sdp->GetMediaSectionCount()) {
    JSEP_SET_ERROR("Cannot create an offer with no local tracks, "
                   "no offerToReceiveAudio/Video, and no DataChannel.");
    return NS_ERROR_INVALID_ARG;
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
  for (auto c = mCodecs.begin(); c != mCodecs.end(); ++c) {
    (*c)->AddToMediaSection(*msection);
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

JsepCodecDescription*
JsepSessionImpl::FindMatchingCodec(const std::string& fmt,
                                   const SdpMediaSection& msection) const
{
  for (auto c = mCodecs.begin(); c != mCodecs.end(); ++c) {
    auto codec = *c;
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

void
JsepSessionImpl::AddCommonCodecs(const SdpMediaSection& remoteMsection,
                                 SdpMediaSection* msection)
{
  const std::vector<std::string>& formats = remoteMsection.GetFormats();

  for (auto fmt = formats.begin(); fmt != formats.end(); ++fmt) {
    JsepCodecDescription* codec = FindMatchingCodec(*fmt, remoteMsection);
    if (codec) {
      codec->mDefaultPt = *fmt; 
      codec->AddToMediaSection(*msection);
      
      
      break;
    }
  }
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

  switch (mState) {
    case kJsepStateHaveRemoteOffer:
      break;
    default:
      JSEP_SET_ERROR("Cannot create answer in state " << GetStateStr(mState));
      return NS_ERROR_UNEXPECTED;
  }

  
  
  
  
  
  
  
  
  
  
  UniquePtr<Sdp> sdp;

  
  nsresult rv = CreateGenericSDP(&sdp);
  NS_ENSURE_SUCCESS(rv, rv);

  const Sdp& offer = *mPendingRemoteDescription;

  size_t numMsections = offer.GetMediaSectionCount();

  for (size_t i = 0; i < numMsections; ++i) {
    const SdpMediaSection& remoteMsection = offer.GetMediaSection(i);
    SdpMediaSection& msection =
        sdp->AddMediaSection(remoteMsection.GetMediaType(),
                             SdpDirectionAttribute::kSendrecv,
                             9,
                             remoteMsection.GetProtocol(),
                             sdp::kIPv4,
                             "0.0.0.0");

    rv = CreateAnswerMSection(options, i, remoteMsection, &msection, sdp.get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *answer = sdp->ToString();
  mGeneratedLocalDescription = Move(sdp);

  return NS_OK;
}

static bool
HasRtcp(SdpMediaSection::Protocol proto)
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

nsresult
JsepSessionImpl::CreateOfferMSection(SdpMediaSection::MediaType mediatype,
                                     SdpDirectionAttribute::Direction dir,
                                     SdpMediaSection::Protocol proto,
                                     Sdp* sdp)
{

  nsresult rv;

  SdpMediaSection* msection =
      &sdp->AddMediaSection(mediatype, dir, 9, proto, sdp::kIPv4, "0.0.0.0");

  
  
  if (HasRtcp(proto)) {
    
    msection->GetAttributeList().SetAttribute(
        new SdpFlagAttribute(SdpAttribute::kRtcpMuxAttribute));
  }

  rv = AddTransportAttributes(msection, SdpSetupAttribute::kActpass);
  NS_ENSURE_SUCCESS(rv, rv);

  AddCodecs(msection);

  AddExtmap(msection);

  return NS_OK;
}

nsresult
JsepSessionImpl::CreateAnswerMSection(const JsepAnswerOptions& options,
                                      size_t mlineIndex,
                                      const SdpMediaSection& remoteMsection,
                                      SdpMediaSection* msection,
                                      Sdp* sdp)
{
  SdpSetupAttribute::Role role;
  nsresult rv = DetermineAnswererSetupRole(remoteMsection, &role);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddTransportAttributes(msection, role);
  NS_ENSURE_SUCCESS(rv, rv);

  SdpDirectionAttribute::Direction remoteDirection =
      remoteMsection.GetDirectionAttribute().mValue;
  SdpDirectionAttribute::Direction localDirection =
      SdpDirectionAttribute::kInactive;

  
  
  if (remoteDirection & SdpDirectionAttribute::kRecvFlag) {
    bool found = false;
    for (auto track = mLocalTracks.begin(); track != mLocalTracks.end();
         ++track) {
      if (track->mAssignedMLine.isSome())
        continue;

      
      if (track->mTrack->GetMediaType() != remoteMsection.GetMediaType())
        continue;

      localDirection = SdpDirectionAttribute::kSendonly;
      track->mAssignedMLine = Some(mlineIndex);
      found = true;
      break;
    }

    if (!found &&
        remoteMsection.GetMediaType() == SdpMediaSection::kApplication) {
      
      
      std::string streamId;
      std::string trackId;

      if (!mUuidGen->Generate(&streamId) || !mUuidGen->Generate(&trackId)) {
        JSEP_SET_ERROR("Failed to generate UUIDs for datachannel track");
        return NS_ERROR_FAILURE;
      }

      AddTrack(RefPtr<JsepTrack>(
          new JsepTrack(SdpMediaSection::kApplication, streamId, trackId)));
      localDirection = SdpDirectionAttribute::kSendonly;
      mLocalTracks.back().mAssignedMLine = Some(mlineIndex);
      found = true;
    }
  }

  if (remoteDirection & SdpDirectionAttribute::kSendFlag) {
    localDirection = static_cast<SdpDirectionAttribute::Direction>(
        localDirection | SdpDirectionAttribute::kRecvFlag);
  }

  msection->GetAttributeList().SetAttribute(
      new SdpDirectionAttribute(localDirection));

  if (remoteMsection.GetAttributeList().HasAttribute(
          SdpAttribute::kRtcpMuxAttribute)) {
    
    msection->GetAttributeList().SetAttribute(
        new SdpFlagAttribute(SdpAttribute::kRtcpMuxAttribute));
  }

  
  AddCommonCodecs(remoteMsection, msection);

  
  AddCommonExtmaps(remoteMsection, msection);

  if (msection->GetFormats().empty()) {
    

    
    msection->GetAttributeList().Clear();

    
    
    
    msection->AddCodec("111", "NULL", 0, 0);

    auto* direction =
        new SdpDirectionAttribute(SdpDirectionAttribute::kInactive);
    msection->GetAttributeList().SetAttribute(direction);
    msection->SetPort(0);
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

static void
appendSdpParseErrors(
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

nsresult
JsepSessionImpl::SetLocalDescription(JsepSdpType type, const std::string& sdp)
{
  mLastError.clear();

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

  
  size_t numMsections = parsed->GetMediaSectionCount();
  for (size_t t = 0; t < numMsections; ++t) {
    if (t < mTransports.size())
      continue; 

    
    RefPtr<JsepTransport> transport;
    nsresult rv = CreateTransport(parsed->GetMediaSection(t), &transport);
    NS_ENSURE_SUCCESS(rv, rv);

    mTransports.push_back(transport);
  }

  switch (type) {
    case kJsepSdpOffer:
      rv = SetLocalDescriptionOffer(Move(parsed));
      break;
    case kJsepSdpAnswer:
    case kJsepSdpPranswer:
      rv = SetLocalDescriptionAnswer(type, Move(parsed));
      break;
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

  nsresult rv = HandleNegotiatedSession(mPendingLocalDescription,
                                        mPendingRemoteDescription);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentRemoteDescription = Move(mPendingRemoteDescription);
  mCurrentLocalDescription = Move(mPendingLocalDescription);

  SetState(kJsepStateStable);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetRemoteDescription(JsepSdpType type, const std::string& sdp)
{
  mLastError.clear();

  MOZ_MTLOG(ML_DEBUG, "SetRemoteDescription type=" << type << "\nSDP=\n"
                                                   << sdp);
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
  }

  if (NS_SUCCEEDED(rv)) {
    mRemoteIsIceLite = iceLite;
    mIceOptions = iceOptions;
  }

  return rv;
}


template <class T>
nsresult
FindTrackForMSection(const SdpMediaSection& msection,
                     const std::vector<T>& tracks,
                     size_t mLine,
                     RefPtr<JsepTrack>* mst)
{
  for (auto t = tracks.begin(); t != tracks.end(); ++t) {
    if (t->mAssignedMLine.isSome() &&
        (*t->mAssignedMLine == msection.GetLevel())) {
      *mst = t->mTrack;
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}

nsresult
JsepSessionImpl::HandleNegotiatedSession(const UniquePtr<Sdp>& local,
                                         const UniquePtr<Sdp>& remote)
{
  bool remoteIceLite =
      remote->GetAttributeList().HasAttribute(SdpAttribute::kIceLiteAttribute);

  mIceControlling = remoteIceLite || mIsOfferer;

  if (local->GetMediaSectionCount() != remote->GetMediaSectionCount()) {
    JSEP_SET_ERROR("Local and remote SDP have different number of m-lines "
                   << "(" << local->GetMediaSectionCount() << " vs "
                   << remote->GetMediaSectionCount() << ")");
    return NS_ERROR_INVALID_ARG;
  }

  std::vector<JsepTrackPair> trackPairs;

  
  
  for (size_t i = 0; i < local->GetMediaSectionCount(); ++i) {
    const SdpMediaSection& lm = local->GetMediaSection(i);
    const SdpMediaSection& rm = remote->GetMediaSection(i);
    const SdpMediaSection& offer = mIsOfferer ? lm : rm;
    const SdpMediaSection& answer = mIsOfferer ? rm : lm;

    if (lm.GetMediaType() != rm.GetMediaType()) {
      JSEP_SET_ERROR("Answer and offer have different media types at m-line "
                     << i);
      return NS_ERROR_INVALID_ARG;
    }

    RefPtr<JsepTransport> transport;

    
    
    MOZ_ASSERT(mTransports.size() > i);
    if (mTransports.size() < i) {
      JSEP_SET_ERROR("Fewer transports set up than m-lines");
      return NS_ERROR_FAILURE;
    }
    transport = mTransports[i];

    
    
    
    
    if (answer.GetDirectionAttribute().mValue ==
            SdpDirectionAttribute::kInactive &&
        answer.GetPort() == 0) {
      transport->mState = JsepTransport::kJsepTransportClosed;
      continue;
    }

    bool sending;
    bool receiving;

    nsresult rv = DetermineSendingDirection(
        offer.GetDirectionAttribute().mValue,
        answer.GetDirectionAttribute().mValue, &sending, &receiving);
    NS_ENSURE_SUCCESS(rv, rv);

    MOZ_MTLOG(ML_DEBUG, "Negotiated m= line"
                            << " index=" << i << " type=" << lm.GetMediaType()
                            << " sending=" << sending
                            << " receiving=" << receiving);

    JsepTrackPair jpair;

    
    jpair.mLevel = i;

    RefPtr<JsepTrack> track;
    if (sending) {
      rv = FindTrackForMSection(lm, mLocalTracks, i, &track);
      if (NS_FAILED(rv)) {
        JSEP_SET_ERROR("Failed to find local track for level " << i
                       << " in local SDP. This should never happen.");
        NS_ASSERTION(false, "Failed to find local track for level");
        return NS_ERROR_FAILURE;
      }

      rv = NegotiateTrack(rm, lm, JsepTrack::kJsepTrackSending, &track);
      NS_ENSURE_SUCCESS(rv, rv);

      jpair.mSending = track;
    }

    if (receiving) {
      rv = FindTrackForMSection(lm, mRemoteTracks, i, &track);
      if (NS_FAILED(rv)) {
        JSEP_SET_ERROR("Failed to find remote track for level " << i
                       << " in remote SDP. This should never happen.");
        NS_ASSERTION(false, "Failed to find remote track for level");
        return NS_ERROR_FAILURE;
      }

      rv = NegotiateTrack(rm, lm, JsepTrack::kJsepTrackReceiving, &track);
      NS_ENSURE_SUCCESS(rv, rv);

      jpair.mReceiving = track;
    }

    rv = SetupTransport(
        rm.GetAttributeList(), answer.GetAttributeList(), transport);
    NS_ENSURE_SUCCESS(rv, rv);
    jpair.mRtpTransport = transport;

    if (HasRtcp(lm.GetProtocol())) {
      
      
      if (offer.GetAttributeList().HasAttribute(
              SdpAttribute::kRtcpMuxAttribute) &&
          answer.GetAttributeList().HasAttribute(
              SdpAttribute::kRtcpMuxAttribute)) {
        jpair.mRtcpTransport = nullptr; 
        MOZ_MTLOG(ML_DEBUG, "RTCP-MUX is on");
      } else {
        MOZ_MTLOG(ML_DEBUG, "RTCP-MUX is off");
        rv = SetupTransport(
            rm.GetAttributeList(), answer.GetAttributeList(), transport);
        NS_ENSURE_SUCCESS(rv, rv);

        jpair.mRtcpTransport = transport;
      }
    }

    trackPairs.push_back(jpair);
  }

  
  
  mNegotiatedTrackPairs = trackPairs;
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

  
  const std::vector<std::string>& formats = remoteMsection.GetFormats();

  for (auto fmt = formats.begin(); fmt != formats.end(); ++fmt) {
    JsepCodecDescription* codec = FindMatchingCodec(*fmt, remoteMsection);

    if (!codec) {
      continue;
    }

    bool sending = (direction == JsepTrack::kJsepTrackSending);

    
    
    
    
    JsepCodecDescription* negotiated =
        codec->MakeNegotiatedCodec(remoteMsection, *fmt, sending);

    if (!negotiated) {
      continue;
    }

    negotiatedDetails->mCodecs.push_back(negotiated);
    break;
  }

  if (negotiatedDetails->mCodecs.empty()) {
    JSEP_SET_ERROR("Failed to negotiate codec details for all codecs");
    return NS_ERROR_INVALID_ARG;
  }

  auto& answerMsection = mIsOfferer ? remoteMsection : localMsection;

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

nsresult
JsepSessionImpl::CreateTransport(const SdpMediaSection& msection,
                                 RefPtr<JsepTransport>* transport)
{
  size_t components;

  if (HasRtcp(msection.GetProtocol())) {
    components = 2;
  } else {
    components = 1;
  }

  *transport = new JsepTransport("transport-id", components);

  return NS_OK;
}

nsresult
JsepSessionImpl::SetupTransport(const SdpAttributeList& remote,
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

  transport->mState = JsepTransport::kJsepTransportAccepted;

  return NS_OK;
}

nsresult
JsepSessionImpl::DetermineSendingDirection(
    SdpDirectionAttribute::Direction offer,
    SdpDirectionAttribute::Direction answer, bool* sending, bool* receiving)
{

  if (!(offer & SdpDirectionAttribute::kSendFlag) &&
      answer & SdpDirectionAttribute::kRecvFlag) {
    JSEP_SET_ERROR("Answer tried to set recv when offer did not set send");
    return NS_ERROR_INVALID_ARG;
  }

  if (!(offer & SdpDirectionAttribute::kRecvFlag) &&
      answer & SdpDirectionAttribute::kSendFlag) {
    JSEP_SET_ERROR("Answer tried to set send when offer did not set recv");
    return NS_ERROR_INVALID_ARG;
  }

  if (mIsOfferer) {
    *receiving = answer & SdpDirectionAttribute::kSendFlag;
    *sending = answer & SdpDirectionAttribute::kRecvFlag;
  } else {
    *sending = answer & SdpDirectionAttribute::kSendFlag;
    *receiving = answer & SdpDirectionAttribute::kRecvFlag;
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
JsepSessionImpl::ParseSdp(const std::string& sdp, UniquePtr<Sdp>* parsedp)
{
  UniquePtr<Sdp> parsed = mParser.Parse(sdp);
  if (!parsed) {
    std::string error = "Failed to parse SDP: ";
    appendSdpParseErrors(mParser.GetParseErrors(), &error);
    JSEP_SET_ERROR(error);
    return NS_ERROR_INVALID_ARG;
  }

  
  if (!parsed->GetMediaSectionCount()) {
    JSEP_SET_ERROR("Description has no media sections");
    return NS_ERROR_INVALID_ARG;
  }

  for (size_t i = 0; i < parsed->GetMediaSectionCount(); ++i) {
    if (parsed->GetMediaSection(i).GetPort() == 0) {
      
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
  }

  *parsedp = Move(parsed);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetRemoteDescriptionOffer(UniquePtr<Sdp> offer)
{
  MOZ_ASSERT(mState == kJsepStateStable);

  
  
  nsresult rv = SetRemoteTracksFromDescription(*offer);
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

  
  
  nsresult rv = SetRemoteTracksFromDescription(*mPendingRemoteDescription);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = HandleNegotiatedSession(mPendingLocalDescription,
                               mPendingRemoteDescription);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentRemoteDescription = Move(mPendingRemoteDescription);
  mCurrentLocalDescription = Move(mPendingLocalDescription);

  SetState(kJsepStateStable);
  return NS_OK;
}

nsresult
JsepSessionImpl::SetRemoteTracksFromDescription(const Sdp& remoteDescription)
{
  size_t numMlines = remoteDescription.GetMediaSectionCount();

  for (size_t i = 0; i < numMlines; ++i) {
    const SdpMediaSection& msection = remoteDescription.GetMediaSection(i);
    auto direction = msection.GetDirectionAttribute().mValue;

    
    
    if (direction & SdpDirectionAttribute::kSendFlag) {
      nsresult rv = CreateReceivingTrack(i, msection);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::ValidateLocalDescription(const Sdp& description)
{
  
  

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

  if (description.GetAttributeList().HasAttribute(
          SdpAttribute::kIceLiteAttribute)) {
    JSEP_SET_ERROR("Running ICE in lite mode is unsupported");
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

nsresult
JsepSessionImpl::CreateReceivingTrack(size_t mline,
                                      const SdpMediaSection& msection)
{
  std::string streamId;
  std::string trackId;

  
  
  if (!mUuidGen->Generate(&trackId)) {
    JSEP_SET_ERROR("Failed to generate UUID for JsepTrack");
    return NS_ERROR_FAILURE;
  }

  JsepTrack* remote = new JsepTrack(msection.GetMediaType(),
                                    mDefaultRemoteStreamId,
                                    trackId,
                                    JsepTrack::kJsepTrackReceiving);
  JsepReceivingTrack rtrack;
  rtrack.mTrack = remote;
  rtrack.mAssignedMLine = Some(mline);
  mRemoteTracks.push_back(rtrack);

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

  *sdpp = Move(sdp);
  return NS_OK;
}

void
JsepSessionImpl::SetupDefaultCodecs()
{
  
  mCodecs.push_back(new JsepAudioCodecDescription(
      "109",
      "opus",
      48000,
      2,
      960,
      16000));

  mCodecs.push_back(new JsepAudioCodecDescription(
      "9",
      "G722",
      8000,
      1,
      320,
      64000));

  
  
  mCodecs.push_back(
      new JsepAudioCodecDescription("0",
                                    "PCMU",
                                    8000,
                                    1,
                                    8000 / 50,   
                                    8 * 8000 * 1 
                                    ));

  mCodecs.push_back(
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
  mCodecs.push_back(vp8);

  JsepVideoCodecDescription* h264_1 = new JsepVideoCodecDescription(
      "126",
      "H264",
      90000
      );
  h264_1->mPacketizationMode = 1;
  
  h264_1->mProfileLevelId = 0x42E00D;
  mCodecs.push_back(h264_1);

  JsepVideoCodecDescription* h264_0 = new JsepVideoCodecDescription(
      "97",
      "H264",
      90000
      );
  h264_0->mPacketizationMode = 0;
  
  h264_0->mProfileLevelId = 0x42E00D;
  mCodecs.push_back(h264_0);

  mCodecs.push_back(new JsepApplicationCodecDescription(
      "5000",
      "webrtc-datachannel",
      16
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
JsepSessionImpl::AddCandidateToSdp(Sdp* sdp,
                                   const std::string& candidateUntrimmed,
                                   const std::string& mid,
                                   uint16_t level)
{

  if (level >= sdp->GetMediaSectionCount()) {
    
    return NS_OK;
  }

  
  size_t begin = candidateUntrimmed.find(':');
  if (begin == std::string::npos) {
    JSEP_SET_ERROR("Invalid candidate, no ':' (" << candidateUntrimmed << ")");
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

  return AddCandidateToSdp(sdp, candidate, mid, level);
}

nsresult
JsepSessionImpl::AddLocalIceCandidate(const std::string& candidate,
                                      const std::string& mid,
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

  return AddCandidateToSdp(sdp, candidate, mid, level);
}

nsresult
JsepSessionImpl::EndOfLocalCandidates(const std::string& defaultCandidateAddr,
                                      uint16_t defaultCandidatePort,
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

  if (level < sdp->GetMediaSectionCount()) {
    SdpMediaSection& msection = sdp->GetMediaSection(level);
    msection.GetConnection().SetAddress(defaultCandidateAddr);
    msection.SetPort(defaultCandidatePort);

    

    SdpAttributeList& attrs = msection.GetAttributeList();
    
    MOZ_ASSERT(!attrs.HasAttribute(SdpAttribute::kEndOfCandidatesAttribute));
    attrs.SetAttribute(
        new SdpFlagAttribute(SdpAttribute::kEndOfCandidatesAttribute));
    if (!mIsOfferer) {
      attrs.RemoveAttribute(SdpAttribute::kIceOptionsAttribute);
    }
  }

  return NS_OK;
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

} 
