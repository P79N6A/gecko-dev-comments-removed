








































#ifndef nsFontFaceLoader_h_
#define nsFontFaceLoader_h_

#include "nsIStreamLoader.h"
#include "nsIURI.h"
#include "gfxUserFontSet.h"

class nsIRequest;
class nsISupports;
class nsPresContext;

class nsFontFaceLoader : public nsIStreamLoaderObserver
{
public:

  nsFontFaceLoader(gfxFontEntry *aFontToLoad, nsIURI *aFontURI, 
                   gfxUserFontSet::LoaderContext *aContext);
  virtual ~nsFontFaceLoader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER 

  
  nsresult Init();  

  
  static PRBool CreateHandler(gfxFontEntry *aFontToLoad, nsIURI *aFontURI, 
                              gfxUserFontSet::LoaderContext *aContext);

private:
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
