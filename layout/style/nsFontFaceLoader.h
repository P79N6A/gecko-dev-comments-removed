








































#ifndef nsFontFaceLoader_h_
#define nsFontFaceLoader_h_

#include "nsIStreamLoader.h"
#include "nsIURI.h"
#include "gfxUserFontSet.h"

class nsIRequest;
class nsISupports;
class nsPresContext;
class nsIPrincipal;

class nsFontFaceLoader : public nsIStreamLoaderObserver
{
public:

  nsFontFaceLoader(gfxFontEntry *aFontToLoad, nsIURI *aFontURI, 
                   gfxUserFontSet::LoaderContext *aContext);
  virtual ~nsFontFaceLoader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER 

  
  nsresult Init();  

  
  static nsresult CreateHandler(gfxFontEntry *aFontToLoad, 
                                const gfxFontFaceSrc *aFontFaceSrc,
                                gfxUserFontSet::LoaderContext *aContext);
                              
private:

  static nsresult CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                                   nsIURI* aTargetURI,
                                   nsISupports* aContext);
  
  nsRefPtr<gfxFontEntry>              mFontEntry;
  nsCOMPtr<nsIURI>                    mFontURI;
  gfxUserFontSet::LoaderContext*      mLoaderContext;
};

class nsFontFaceLoaderContext : public gfxUserFontSet::LoaderContext {
public:
  nsFontFaceLoaderContext(nsPresContext* aContext)
    : gfxUserFontSet::LoaderContext(nsFontFaceLoader::CreateHandler), 
      mPresContext(aContext)
  {

  }

  nsPresContext*    mPresContext;
};


#endif 
