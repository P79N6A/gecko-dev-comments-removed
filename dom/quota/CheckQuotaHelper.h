





#ifndef mozilla_dom_quota_checkquotahelper_h__
#define mozilla_dom_quota_checkquotahelper_h__

#include "QuotaCommon.h"

#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"

#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"

class nsIPrincipal;
class nsPIDOMWindow;

BEGIN_QUOTA_NAMESPACE

class CheckQuotaHelper MOZ_FINAL : public nsIRunnable,
                                   public nsIInterfaceRequestor,
                                   public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIOBSERVER

  CheckQuotaHelper(nsPIDOMWindow* aWindow,
                   mozilla::Mutex& aMutex);

  bool PromptAndReturnQuotaIsDisabled();

  void Cancel();

  static uint32_t GetQuotaPermission(nsIPrincipal* aPrincipal);

private:
  nsPIDOMWindow* mWindow;

  mozilla::Mutex& mMutex;
  mozilla::CondVar mCondVar;
  uint32_t mPromptResult;
  bool mWaiting;
  bool mHasPrompted;
};

END_QUOTA_NAMESPACE

#endif 
