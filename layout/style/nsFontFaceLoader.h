








































#ifndef nsFontFaceLoader_h_
#define nsFontFaceLoader_h_

#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsIStreamLoader.h"
#include "nsIURI.h"
#include "gfxUserFontSet.h"

class nsIRequest;
class nsISupports;
class nsIPresShell;
class nsPresContext;
class nsIPrincipal;

class nsFontFaceLoader : public nsIStreamLoaderObserver
{
public:

  nsFontFaceLoader(gfxFontEntry *aFontToLoad, nsIURI *aFontURI, 
                   nsIPresShell *aShell);
  virtual ~nsFontFaceLoader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER 

  
  nsresult Init();  

  static nsresult CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                                   nsIURI* aTargetURI,
                                   nsISupports* aContext);
  
private:

  nsRefPtr<gfxFontEntry>  mFontEntry;
  nsCOMPtr<nsIURI>        mFontURI;
  nsCOMPtr<nsIPresShell>  mShell;
};



class nsUserFontSet : public gfxUserFontSet
{
public:
  nsUserFontSet(nsPresContext *aContext);
  ~nsUserFontSet();
  
  
  
  nsresult StartLoad(gfxFontEntry *aFontToLoad, 
                     const gfxFontFaceSrc *aFontFaceSrc);
protected:
  nsPresContext *mPresContext;  
};

#endif 
