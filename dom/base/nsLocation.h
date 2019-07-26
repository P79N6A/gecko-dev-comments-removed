





#ifndef nsLocation_h__
#define nsLocation_h__

#include "nsIDOMLocation.h"
#include "nsString.h"
#include "nsIWeakReferenceUtils.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "js/TypeDecls.h"

class nsIURI;
class nsIDocShell;
class nsIDocShellLoadInfo;





class nsLocation : public nsIDOMLocation
                 , public nsWrapperCache
{
public:
  nsLocation(nsIDocShell *aDocShell);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsLocation)

  void SetDocShell(nsIDocShell *aDocShell);
  nsIDocShell *GetDocShell();

  
  NS_DECL_NSIDOMLOCATION

protected:
  virtual ~nsLocation();

  
  
  
  nsresult GetURI(nsIURI** aURL, bool aGetInnermostURI = false);
  nsresult GetWritableURI(nsIURI** aURL);
  nsresult SetURI(nsIURI* aURL, bool aReplace = false);
  nsresult SetHrefWithBase(const nsAString& aHref, nsIURI* aBase,
                           bool aReplace);
  nsresult SetHrefWithContext(JSContext* cx, const nsAString& aHref,
                              bool aReplace);

  nsresult GetSourceBaseURL(JSContext* cx, nsIURI** sourceURL);
  nsresult CheckURL(nsIURI *url, nsIDocShellLoadInfo** aLoadInfo);
  bool CallerSubsumes();

  nsString mCachedHash;
  nsWeakPtr mDocShell;
  nsWeakPtr mOuter;
};

#endif 

