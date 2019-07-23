








































#ifndef nsFontFaceLoader_h_
#define nsFontFaceLoader_h_

#include "nsIDownloader.h"
#include "nsIURI.h"
#include "gfxUserFontSet.h"

class nsIRequest;
class nsISupports;
class nsPresContext;

class nsFontFaceLoader : public nsIDownloadObserver
{
public:

  nsFontFaceLoader(gfxFontEntry *aFontToLoad, nsIURI *aFontURI, 
                   gfxUserFontSet::LoaderContext *aContext);
  virtual ~nsFontFaceLoader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOWNLOADOBSERVER 

  
  nsresult Init();  

  
  static PRBool CreateHandler(gfxFontEntry *aFontToLoad, nsIURI *aFontURI, 
                              gfxUserFontSet::LoaderContext *aContext);

private:
  nsRefPtr<gfxFontEntry>              mFontEntry;
  nsCOMPtr<nsIURI>                    mFontURI;
  gfxUserFontSet::LoaderContext*      mLoaderContext;
  gfxDownloadedFontData               mFaceData;
  nsCOMPtr<nsIStreamListener>         mDownloader;
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
