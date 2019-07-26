





#ifndef nsLocation_h__
#define nsLocation_h__

#include "nsIDOMLocation.h"
#include "nsString.h"
#include "nsWeakReference.h"


class nsIURI;
class nsIDocShell;
struct JSContext;
class nsIDocument;
class nsIDocShellLoadInfo;





class nsLocation : public nsIDOMLocation
{
public:
  nsLocation(nsIDocShell *aDocShell);
  virtual ~nsLocation();

  NS_DECL_ISUPPORTS

  void SetDocShell(nsIDocShell *aDocShell);
  nsIDocShell *GetDocShell();

  
  NS_DECL_NSIDOMLOCATION

protected:
  
  
  
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

