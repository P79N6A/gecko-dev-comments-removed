






































#ifndef mozilla_dom_indexeddb_checkpermissionshelper_h__
#define mozilla_dom_indexeddb_checkpermissionshelper_h__


#include "OpenDatabaseHelper.h"

#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsIRunnable.h"

class nsIDOMWindow;
class nsIThread;

BEGIN_INDEXEDDB_NAMESPACE

class CheckPermissionsHelper : public nsIRunnable,
                               public nsIInterfaceRequestor,
                               public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIOBSERVER

  CheckPermissionsHelper(OpenDatabaseHelper* aHelper,
                         nsIDOMWindow* aWindow,
                         const nsACString& aASCIIOrigin,
                         bool aForDeletion)
  : mHelper(aHelper),
    mWindow(aWindow),
    mASCIIOrigin(aASCIIOrigin),
    
    
    mPromptAllowed(!aForDeletion),
    mHasPrompted(false),
    mPromptResult(0)
  {
    NS_ASSERTION(aHelper, "Null pointer!");
    NS_ASSERTION(aWindow, "Null pointer!");
    NS_ASSERTION(!aASCIIOrigin.IsEmpty(), "Empty origin!");
  }

private:
  nsRefPtr<OpenDatabaseHelper> mHelper;
  nsCOMPtr<nsIDOMWindow> mWindow;
  nsCString mASCIIOrigin;
  bool mPromptAllowed;
  bool mHasPrompted;
  PRUint32 mPromptResult;
};

END_INDEXEDDB_NAMESPACE

#endif 
