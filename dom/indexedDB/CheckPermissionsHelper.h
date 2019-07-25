






































#ifndef mozilla_dom_indexeddb_checkpermissionshelper_h__
#define mozilla_dom_indexeddb_checkpermissionshelper_h__


#include "AsyncConnectionHelper.h"

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

  CheckPermissionsHelper(AsyncConnectionHelper* aHelper,
                         nsIDOMWindow* aWindow,
                         const nsAString& aName,
                         const nsACString& aASCIIOrigin)
  : mHelper(aHelper),
    mWindow(aWindow),
    mName(aName),
    mASCIIOrigin(aASCIIOrigin),
    mHasPrompted(PR_FALSE),
    mPromptResult(0)
  {
    NS_ASSERTION(aHelper, "Null pointer!");
    NS_ASSERTION(aWindow, "Null pointer!");
    NS_ASSERTION(!aName.IsEmpty(), "Empty name!");
    NS_ASSERTION(!aASCIIOrigin.IsEmpty(), "Empty origin!");
  }

private:
  nsRefPtr<AsyncConnectionHelper> mHelper;
  nsCOMPtr<nsIDOMWindow> mWindow;
  nsString mName;
  nsCString mASCIIOrigin;
  PRBool mHasPrompted;
  PRUint32 mPromptResult;
};

END_INDEXEDDB_NAMESPACE

#endif 
