




































#ifndef nsDOMParser_h__
#define nsDOMParser_h__

#include "nsIDOMParser.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIDOMLoadListener.h"
#include "nsWeakReference.h"
#include "nsIJSNativeInitializer.h"

class nsDOMParser : public nsIDOMParser,
                    public nsIDOMParserJS,
                    public nsIDOMLoadListener,
                    public nsIJSNativeInitializer,
                    public nsSupportsWeakReference
{
public: 
  nsDOMParser();
  virtual ~nsDOMParser();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMPARSER

  
  NS_DECL_NSIDOMPARSERJS

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  NS_IMETHOD Load(nsIDOMEvent* aEvent);
  NS_IMETHOD BeforeUnload(nsIDOMEvent* aEvent);
  NS_IMETHOD Unload(nsIDOMEvent* aEvent);
  NS_IMETHOD Abort(nsIDOMEvent* aEvent);
  NS_IMETHOD Error(nsIDOMEvent* aEvent);

  
  NS_IMETHOD Initialize(JSContext *cx, JSObject *obj, 
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
  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mBaseURI;
  
  PRPackedBool mLoopingForSyncLoad;
  PRPackedBool mAttemptedInit;
};

#endif
