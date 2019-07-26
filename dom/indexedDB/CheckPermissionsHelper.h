





#ifndef mozilla_dom_indexeddb_checkpermissionshelper_h__
#define mozilla_dom_indexeddb_checkpermissionshelper_h__


#include "OpenDatabaseHelper.h"

#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"

class nsIDOMWindow;
class nsIThread;

BEGIN_INDEXEDDB_NAMESPACE

class CheckPermissionsHelper MOZ_FINAL : public nsIRunnable,
                                         public nsIInterfaceRequestor,
                                         public nsIObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIOBSERVER

  CheckPermissionsHelper(OpenDatabaseHelper* aHelper,
                         nsIDOMWindow* aWindow)
  : mHelper(aHelper),
    mWindow(aWindow),
    
    
    mPromptAllowed(!aHelper->mForDeletion),
    mHasPrompted(false),
    mPromptResult(0)
  {
    NS_ASSERTION(aHelper, "Null pointer!");
    NS_ASSERTION(aHelper->mPersistenceType == quota::PERSISTENCE_TYPE_PERSISTENT,
                 "Checking permission for non persistent databases?!");
  }

private:
  ~CheckPermissionsHelper() {}

  nsRefPtr<OpenDatabaseHelper> mHelper;
  nsCOMPtr<nsIDOMWindow> mWindow;
  bool mPromptAllowed;
  bool mHasPrompted;
  uint32_t mPromptResult;
};

END_INDEXEDDB_NAMESPACE

#endif 
