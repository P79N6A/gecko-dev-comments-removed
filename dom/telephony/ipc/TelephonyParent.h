




#ifndef mozilla_dom_telephony_TelephonyParent_h
#define mozilla_dom_telephony_TelephonyParent_h

#include "mozilla/dom/telephony/TelephonyCommon.h"
#include "mozilla/dom/telephony/PTelephonyParent.h"
#include "mozilla/dom/telephony/PTelephonyRequestParent.h"
#include "nsITelephonyService.h"

BEGIN_TELEPHONY_NAMESPACE

class TelephonyParent : public PTelephonyParent
                      , public nsITelephonyListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONYLISTENER

  TelephonyParent();

protected:
  virtual ~TelephonyParent() {}

  virtual void
  ActorDestroy(ActorDestroyReason why);

  virtual bool
  RecvPTelephonyRequestConstructor(PTelephonyRequestParent* aActor, const IPCTelephonyRequest& aRequest) MOZ_OVERRIDE;

  virtual PTelephonyRequestParent*
  AllocPTelephonyRequestParent(const IPCTelephonyRequest& aRequest) MOZ_OVERRIDE;

  virtual bool
  DeallocPTelephonyRequestParent(PTelephonyRequestParent* aActor) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__() MOZ_OVERRIDE;

  virtual bool
  RecvRegisterListener() MOZ_OVERRIDE;

  virtual bool
  RecvUnregisterListener() MOZ_OVERRIDE;

  virtual bool
  RecvHangUpCall(const uint32_t& aClientId, const uint32_t& aCallIndex) MOZ_OVERRIDE;

  virtual bool
  RecvAnswerCall(const uint32_t& aClientId, const uint32_t& aCallIndex) MOZ_OVERRIDE;

  virtual bool
  RecvRejectCall(const uint32_t& aClientId, const uint32_t& aCallIndex) MOZ_OVERRIDE;

  virtual bool
  RecvHoldCall(const uint32_t& aClientId, const uint32_t& aCallIndex) MOZ_OVERRIDE;

  virtual bool
  RecvResumeCall(const uint32_t& aClientId, const uint32_t& aCallIndex) MOZ_OVERRIDE;

  virtual bool
  RecvConferenceCall(const uint32_t& aClientId) MOZ_OVERRIDE;

  virtual bool
  RecvSeparateCall(const uint32_t& aClientId, const uint32_t& callIndex) MOZ_OVERRIDE;

  virtual bool
  RecvHoldConference(const uint32_t& aClientId) MOZ_OVERRIDE;

  virtual bool
  RecvResumeConference(const uint32_t& aClientId) MOZ_OVERRIDE;

  virtual bool
  RecvStartTone(const uint32_t& aClientId, const nsString& aTone) MOZ_OVERRIDE;

  virtual bool
  RecvStopTone(const uint32_t& aClientId) MOZ_OVERRIDE;

  virtual bool
  RecvGetMicrophoneMuted(bool* aMuted) MOZ_OVERRIDE;

  virtual bool
  RecvSetMicrophoneMuted(const bool& aMuted) MOZ_OVERRIDE;

  virtual bool
  RecvGetSpeakerEnabled(bool* aEnabled) MOZ_OVERRIDE;

  virtual bool
  RecvSetSpeakerEnabled(const bool& aEnabled) MOZ_OVERRIDE;

private:
  bool mActorDestroyed;
  bool mRegistered;
};

class TelephonyRequestParent : public PTelephonyRequestParent
                             , public nsITelephonyListener
                             , public nsITelephonyDialCallback
{
  friend class TelephonyParent;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITELEPHONYLISTENER
  NS_DECL_NSITELEPHONYCALLBACK
  NS_DECL_NSITELEPHONYDIALCALLBACK

protected:
  TelephonyRequestParent();
  virtual ~TelephonyRequestParent() {}

  virtual void
  ActorDestroy(ActorDestroyReason why);

  nsresult
  SendResponse(const IPCTelephonyResponse& aResponse);

private:
  bool mActorDestroyed;

  bool
  DoRequest(const EnumerateCallsRequest& aRequest);

  bool
  DoRequest(const DialRequest& aRequest);
};

END_TELEPHONY_NAMESPACE

#endif 
