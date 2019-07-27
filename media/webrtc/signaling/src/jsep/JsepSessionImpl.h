



#ifndef _JSEPSESSIONIMPL_H_
#define _JSEPSESSIONIMPL_H_

#include <set>
#include <string>
#include <vector>

#include "signaling/src/jsep/JsepCodecDescription.h"
#include "signaling/src/jsep/JsepTrack.h"
#include "signaling/src/jsep/JsepSession.h"
#include "signaling/src/jsep/JsepTrack.h"
#include "signaling/src/jsep/JsepTrackImpl.h"
#include "signaling/src/sdp/SipccSdpParser.h"
#include "signaling/src/sdp/SdpHelper.h"
#include "signaling/src/common/PtrVector.h"

namespace mozilla {

class JsepUuidGenerator
{
public:
  virtual ~JsepUuidGenerator() {}
  virtual bool Generate(std::string* id) = 0;
};

class JsepSessionImpl : public JsepSession
{
public:
  JsepSessionImpl(const std::string& name, UniquePtr<JsepUuidGenerator> uuidgen)
      : JsepSession(name),
        mIsOfferer(false),
        mWasOffererLastTime(false),
        mIceControlling(false),
        mRemoteIsIceLite(false),
        mBundlePolicy(kBundleBalanced),
        mSessionId(0),
        mSessionVersion(0),
        mUuidGen(Move(uuidgen)),
        mSdpHelper(&mLastError)
  {
  }

  
  virtual nsresult Init() override;

  virtual nsresult AddTrack(const RefPtr<JsepTrack>& track) override;

  virtual nsresult RemoveTrack(const std::string& streamId,
                               const std::string& trackId) override;

  virtual nsresult SetIceCredentials(const std::string& ufrag,
                                     const std::string& pwd) override;
  nsresult SetBundlePolicy(JsepBundlePolicy policy) override;

  virtual bool
  RemoteIsIceLite() const override
  {
    return mRemoteIsIceLite;
  }

  virtual std::vector<std::string>
  GetIceOptions() const override
  {
    return mIceOptions;
  }

  virtual nsresult AddDtlsFingerprint(const std::string& algorithm,
                                      const std::vector<uint8_t>& value) override;

  virtual nsresult AddAudioRtpExtension(
      const std::string& extensionName) override;

  virtual nsresult AddVideoRtpExtension(
      const std::string& extensionName) override;

  virtual std::vector<JsepCodecDescription*>&
  Codecs() override
  {
    return mCodecs.values;
  }

  virtual nsresult ReplaceTrack(const std::string& oldStreamId,
                                const std::string& oldTrackId,
                                const std::string& newStreamId,
                                const std::string& newTrackId) override;

  virtual std::vector<RefPtr<JsepTrack>> GetLocalTracks() const override;

  virtual std::vector<RefPtr<JsepTrack>> GetRemoteTracks() const override;

  virtual std::vector<RefPtr<JsepTrack>>
    GetRemoteTracksAdded() const override;

  virtual std::vector<RefPtr<JsepTrack>>
    GetRemoteTracksRemoved() const override;

  virtual nsresult CreateOffer(const JsepOfferOptions& options,
                               std::string* offer) override;

  virtual nsresult CreateAnswer(const JsepAnswerOptions& options,
                                std::string* answer) override;

  virtual std::string GetLocalDescription() const override;

  virtual std::string GetRemoteDescription() const override;

  virtual nsresult SetLocalDescription(JsepSdpType type,
                                       const std::string& sdp) override;

  virtual nsresult SetRemoteDescription(JsepSdpType type,
                                        const std::string& sdp) override;

  virtual nsresult AddRemoteIceCandidate(const std::string& candidate,
                                         const std::string& mid,
                                         uint16_t level) override;

  virtual nsresult AddLocalIceCandidate(const std::string& candidate,
                                        const std::string& mid,
                                        uint16_t level,
                                        bool* skipped) override;

  virtual nsresult EndOfLocalCandidates(const std::string& defaultCandidateAddr,
                                        uint16_t defaultCandidatePort,
                                        const std::string& defaultRtcpCandidateAddr,
                                        uint16_t defaultRtcpCandidatePort,
                                        uint16_t level) override;

  virtual nsresult Close() override;

  virtual const std::string GetLastError() const override;

  virtual bool
  IsIceControlling() const override
  {
    return mIceControlling;
  }

  virtual bool
  IsOfferer() const
  {
    return mIsOfferer;
  }

  
  virtual std::vector<RefPtr<JsepTransport>>
  GetTransports() const override
  {
    return mTransports;
  }

  virtual std::vector<JsepTrackPair>
  GetNegotiatedTrackPairs() const override
  {
    return mNegotiatedTrackPairs;
  }

  virtual bool AllLocalTracksAreAssigned() const override;

private:
  struct JsepDtlsFingerprint {
    std::string mAlgorithm;
    std::vector<uint8_t> mValue;
  };

  struct JsepSendingTrack {
    RefPtr<JsepTrack> mTrack;
    Maybe<size_t> mAssignedMLine;
    bool mNegotiated;
  };

  struct JsepReceivingTrack {
    RefPtr<JsepTrack> mTrack;
    Maybe<size_t> mAssignedMLine;
  };

  
  nsresult CreateGenericSDP(UniquePtr<Sdp>* sdp);
  void AddCodecs(SdpMediaSection* msection) const;
  void AddExtmap(SdpMediaSection* msection) const;
  void AddMid(const std::string& mid, SdpMediaSection* msection) const;
  void AddLocalIds(const JsepTrack& track, SdpMediaSection* msection) const;
  JsepCodecDescription* FindMatchingCodec(
      const std::string& pt,
      const SdpMediaSection& msection) const;
  const std::vector<SdpExtmapAttributeList::Extmap>* GetRtpExtensions(
      SdpMediaSection::MediaType type) const;

  PtrVector<JsepCodecDescription> GetCommonCodecs(
      const SdpMediaSection& offerMsection);
  void AddCommonExtmaps(const SdpMediaSection& remoteMsection,
                        SdpMediaSection* msection);
  nsresult SetupIds();
  nsresult CreateSsrc(uint32_t* ssrc);
  void SetupDefaultCodecs();
  void SetupDefaultRtpExtensions();
  void SetState(JsepSignalingState state);
  
  nsresult ParseSdp(const std::string& sdp, UniquePtr<Sdp>* parsedp);
  nsresult SetLocalDescriptionOffer(UniquePtr<Sdp> offer);
  nsresult SetLocalDescriptionAnswer(JsepSdpType type, UniquePtr<Sdp> answer);
  nsresult SetRemoteDescriptionOffer(UniquePtr<Sdp> offer);
  nsresult SetRemoteDescriptionAnswer(JsepSdpType type, UniquePtr<Sdp> answer);
  nsresult ValidateLocalDescription(const Sdp& description);
  nsresult ValidateRemoteDescription(const Sdp& description);
  nsresult ValidateAnswer(const Sdp& offer, const Sdp& answer);
  nsresult SetRemoteTracksFromDescription(const Sdp* remoteDescription);
  
  nsresult CreateReceivingTrack(size_t mline,
                                const Sdp& sdp,
                                const SdpMediaSection& msection,
                                RefPtr<JsepTrack>* track);
  nsresult HandleNegotiatedSession(const UniquePtr<Sdp>& local,
                                   const UniquePtr<Sdp>& remote);
  nsresult AddTransportAttributes(SdpMediaSection* msection,
                                  SdpSetupAttribute::Role dtlsRole);
  nsresult SetupTransportParams(const Sdp& oldAnswer,
                                const Sdp& newOffer,
                                Sdp* newLocal);
  nsresult AddOfferMSections(const JsepOfferOptions& options, Sdp* sdp);
  
  nsresult AddOfferMSectionsByType(SdpMediaSection::MediaType type,
                                   Maybe<size_t> offerToReceive,
                                   Sdp* sdp);
  nsresult BindLocalTracks(SdpMediaSection::MediaType mediatype,
                           Sdp* sdp);
  nsresult BindTrackToMsection(JsepSendingTrack* track,
                               SdpMediaSection* msection);
  nsresult EnsureRecvForRemoteTracks(SdpMediaSection::MediaType mediatype,
                                     Sdp* sdp,
                                     size_t* offerToReceive);
  nsresult SetRecvAsNeededOrDisable(SdpMediaSection::MediaType mediatype,
                                    Sdp* sdp,
                                    size_t* offerToRecv);
  nsresult AddRecvonlyMsections(SdpMediaSection::MediaType mediatype,
                                size_t count,
                                Sdp* sdp);
  nsresult CreateReoffer(const Sdp& oldLocalSdp,
                         const Sdp& oldAnswer,
                         Sdp* newSdp);
  void SetupBundle(Sdp* sdp) const;
  nsresult GetRemoteIds(const Sdp& sdp,
                        const SdpMediaSection& msection,
                        std::string* streamId,
                        std::string* trackId);
  nsresult CreateOfferMSection(SdpMediaSection::MediaType type,
                               SdpDirectionAttribute::Direction direction,
                               Sdp* sdp);
  nsresult GetFreeMsectionForSend(SdpMediaSection::MediaType type,
                                  Sdp* sdp,
                                  SdpMediaSection** msection);
  nsresult CreateAnswerMSection(const JsepAnswerOptions& options,
                                size_t mlineIndex,
                                const SdpMediaSection& remoteMsection,
                                Sdp* sdp);
  nsresult SetRecvonlySsrc(SdpMediaSection* msection);
  nsresult BindMatchingLocalTrackForAnswer(SdpMediaSection* msection);
  nsresult DetermineAnswererSetupRole(const SdpMediaSection& remoteMsection,
                                      SdpSetupAttribute::Role* rolep);
  nsresult MakeNegotiatedTrackPair(const SdpMediaSection& remote,
                                   const SdpMediaSection& local,
                                   const RefPtr<JsepTransport>& transport,
                                   bool usingBundle,
                                   size_t transportLevel,
                                   JsepTrackPair* trackPairOut);
  nsresult NegotiateTrack(const SdpMediaSection& remoteMsection,
                          const SdpMediaSection& localMsection,
                          JsepTrack::Direction,
                          RefPtr<JsepTrack>* track);

  void UpdateTransport(const SdpMediaSection& msection,
                       JsepTransport* transport);

  nsresult FinalizeTransport(const SdpAttributeList& remote,
                             const SdpAttributeList& answer,
                             const RefPtr<JsepTransport>& transport);

  nsresult GetNegotiatedBundledMids(SdpHelper::BundledMids* bundledMids);

  void DisableMsection(Sdp* sdp, SdpMediaSection* msection) const;
  nsresult EnableOfferMsection(SdpMediaSection* msection);

  nsresult SetUniquePayloadTypes();
  nsresult GetAllPayloadTypes(const JsepTrackNegotiatedDetails& trackDetails,
                              std::vector<uint8_t>* payloadTypesOut);
  const Sdp* GetAnswer() const;

  std::vector<JsepSendingTrack> mLocalTracks;
  std::vector<JsepReceivingTrack> mRemoteTracks;
  
  std::vector<JsepReceivingTrack> mRemoteTracksAdded;
  std::vector<JsepReceivingTrack> mRemoteTracksRemoved;
  std::vector<RefPtr<JsepTransport> > mTransports;
  
  std::vector<RefPtr<JsepTransport> > mOldTransports;
  std::vector<JsepTrackPair> mNegotiatedTrackPairs;

  bool mIsOfferer;
  bool mWasOffererLastTime;
  bool mIceControlling;
  std::string mIceUfrag;
  std::string mIcePwd;
  bool mRemoteIsIceLite;
  std::vector<std::string> mIceOptions;
  JsepBundlePolicy mBundlePolicy;
  std::vector<JsepDtlsFingerprint> mDtlsFingerprints;
  uint64_t mSessionId;
  uint64_t mSessionVersion;
  std::vector<SdpExtmapAttributeList::Extmap> mAudioRtpExtensions;
  std::vector<SdpExtmapAttributeList::Extmap> mVideoRtpExtensions;
  UniquePtr<JsepUuidGenerator> mUuidGen;
  std::string mDefaultRemoteStreamId;
  std::map<size_t, std::string> mDefaultRemoteTrackIdsByLevel;
  std::string mCNAME;
  
  
  std::set<uint32_t> mSsrcs;
  
  
  std::vector<uint32_t> mRecvonlySsrcs;
  UniquePtr<Sdp> mGeneratedLocalDescription; 
  UniquePtr<Sdp> mCurrentLocalDescription;
  UniquePtr<Sdp> mCurrentRemoteDescription;
  UniquePtr<Sdp> mPendingLocalDescription;
  UniquePtr<Sdp> mPendingRemoteDescription;
  PtrVector<JsepCodecDescription> mCodecs;
  std::string mLastError;
  SipccSdpParser mParser;
  SdpHelper mSdpHelper;
};

} 

#endif
