




































#ifndef nsDOMParser_h__
#define nsDOMParser_h__

#include "nsIDOMParser.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsWeakReference.h"
#include "nsIJSNativeInitializer.h"

class nsDOMParser : public nsIDOMParser,
                    public nsIDOMParserJS,
                    public nsIJSNativeInitializer,
                    public nsSupportsWeakReference
{
public: 
  nsDOMParser();
  virtual ~nsDOMParser();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMPARSER

  
  NS_DECL_NSIDOMPARSERJS

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* cx, JSObject* obj,
                        PRUint32 argc, jsval *argv);

private:
  class AttemptedInitMarker {
  public:
    AttemptedInitMarker(PRPackedBool* aAttemptedInit) :
      mAttemptedInit(aAttemptedInit)
    {}

    ~AttemptedInitMarker() {
      *mAttemptedInit = PR_TRUE;
    }

  private:
    PRPackedBool* mAttemptedInit;
  };
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIPrincipal> mOriginalPrincipal;
  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mBaseURI;
  nsWeakPtr mScriptHandlingObject;
  
  PRPackedBool mLoopingForSyncLoad;
  PRPackedBool mAttemptedInit;
};

#endif
