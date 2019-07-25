




#ifndef nsDOMParser_h__
#define nsDOMParser_h__

#include "nsIDOMParser.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsWeakReference.h"
#include "nsIJSNativeInitializer.h"
#include "nsIDocument.h"

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
                        uint32_t argc, jsval *argv);

private:
  nsresult SetUpDocument(DocumentFlavor aFlavor, nsIDOMDocument** aResult);

  class AttemptedInitMarker {
  public:
    AttemptedInitMarker(bool* aAttemptedInit) :
      mAttemptedInit(aAttemptedInit)
    {}

    ~AttemptedInitMarker() {
      *mAttemptedInit = true;
    }

  private:
    bool* mAttemptedInit;
  };
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIPrincipal> mOriginalPrincipal;
  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mBaseURI;
  nsWeakPtr mScriptHandlingObject;
  
  bool mAttemptedInit;
};

#endif
