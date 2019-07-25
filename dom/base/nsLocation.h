









































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
  
  
  
  nsresult GetURI(nsIURI** aURL, PRBool aGetInnermostURI = PR_FALSE);
  nsresult GetWritableURI(nsIURI** aURL);
  nsresult SetURI(nsIURI* aURL, PRBool aReplace = PR_FALSE);
  nsresult SetHrefWithBase(const nsAString& aHref, nsIURI* aBase,
                           PRBool aReplace);
  nsresult SetHrefWithContext(JSContext* cx, const nsAString& aHref,
                              PRBool aReplace);

  nsresult GetSourceBaseURL(JSContext* cx, nsIURI** sourceURL);
  nsresult GetSourceDocument(JSContext* cx, nsIDocument** aDocument);

  nsresult CheckURL(nsIURI *url, nsIDocShellLoadInfo** aLoadInfo);

  nsString mCachedHash;
  nsWeakPtr mDocShell;
};

#endif 

