






































#ifndef mozilla_dom_indexeddb_checkquotahelper_h__
#define mozilla_dom_indexeddb_checkquotahelper_h__


#include "IndexedDatabase.h"
#include "IDBDatabase.h"

#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"

#include "mozilla/Mutex.h"
#include "mozilla/CondVar.h"

class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class CheckQuotaHelper : public nsIRunnable,
                         public nsIInterfaceRequestor,
                         public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIOBSERVER

  CheckQuotaHelper(IDBDatabase* aDatabase,
                   mozilla::Mutex& aMutex);

  bool PromptAndReturnQuotaIsDisabled();

  PRUint32 WindowSerial()
  {
    return mWindowSerial;
  }

private:
  nsPIDOMWindow* mWindow;
  PRUint32 mWindowSerial;
  nsCString mOrigin;

  mozilla::Mutex& mMutex;
  mozilla::CondVar mCondVar;
  PRUint32 mPromptResult;
  bool mWaiting;
  bool mHasPrompted;
};

END_INDEXEDDB_NAMESPACE

#endif 
