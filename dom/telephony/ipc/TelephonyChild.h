




#ifndef mozilla_dom_telephony_TelephonyChild_h
#define mozilla_dom_telephony_TelephonyChild_h

#include "mozilla/dom/telephony/TelephonyCommon.h"
#include "mozilla/dom/telephony/PTelephonyChild.h"
#include "mozilla/dom/telephony/PTelephonyRequestChild.h"
#include "nsITelephonyProvider.h"

BEGIN_TELEPHONY_NAMESPACE

class TelephonyChild : public PTelephonyChild
{
public:
  TelephonyChild(nsITelephonyListener* aListener);

protected:
  virtual ~TelephonyChild() {}

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PTelephonyRequestChild*
  AllocPTelephonyRequestChild() MOZ_OVERRIDE;

  virtual bool
  DeallocPTelephonyRequestChild(PTelephonyRequestChild* aActor) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyCallError(const uint32_t& aClientId, const int32_t& aCallIndex,
                      const nsString& aError) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyCallStateChanged(const uint32_t& aClientId,
                             const IPCCallStateData& aData) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyCdmaCallWaiting(const uint32_t& aClientId,
                            const nsString& aNumber) MOZ_OVERRIDE;

  virtual bool
  RecvNotifyConferenceCallStateChanged(const uint16_t& aCallState) MOZ_OVERRIDE;

  virtual bool
  RecvNotifySupplementaryService(const uint32_t& aClientId,
                                 const int32_t& aCallIndex,
                                 const uint16_t& aNotification) MOZ_OVERRIDE;

private:
  nsCOMPtr<nsITelephonyListener> mListener;
};

class TelephonyRequestChild : public PTelephonyRequestChild
{
public:
  TelephonyRequestChild(nsITelephonyListener* aListener);

protected:
  virtual ~TelephonyRequestChild() {}

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__() MOZ_OVERRIDE;

  virtual bool
  RecvNotifyEnumerateCallState(const uint32_t& aClientId,
                               const IPCCallStateData& aData) MOZ_OVERRIDE;

private:
  nsCOMPtr<nsITelephonyListener> mListener;
};

END_TELEPHONY_NAMESPACE

#endif 
