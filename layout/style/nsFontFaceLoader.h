








































#ifndef nsFontFaceLoader_h_
#define nsFontFaceLoader_h_

#include "nsCOMPtr.h"
#include "nsIStreamLoader.h"
#include "nsIURI.h"
#include "nsIChannel.h"
#include "nsITimer.h"
#include "gfxUserFontSet.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"

class nsIRequest;
class nsISupports;
class nsPresContext;
class nsIPrincipal;

class nsFontFaceLoader;


class nsUserFontSet : public gfxUserFontSet
{
public:
  nsUserFontSet(nsPresContext *aContext);
  ~nsUserFontSet();

  
  void Destroy();

  
  
  nsresult StartLoad(gfxFontEntry *aFontToLoad, 
                     const gfxFontFaceSrc *aFontFaceSrc);

  
  
  void RemoveLoader(nsFontFaceLoader *aLoader);

  nsPresContext *GetPresContext() { return mPresContext; }

protected:
  nsPresContext *mPresContext;  

  
  
  
  nsTHashtable< nsPtrHashKey<nsFontFaceLoader> > mLoaders;
};

class nsFontFaceLoader : public nsIStreamLoaderObserver
{
public:

  nsFontFaceLoader(gfxFontEntry *aFontToLoad, nsIURI *aFontURI, 
                   nsUserFontSet *aFontSet, nsIChannel *aChannel);
  virtual ~nsFontFaceLoader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER 

  
  nsresult Init();
  
  void Cancel();

  void DropChannel() { mChannel = nsnull; }

  void StartedLoading(nsIStreamLoader *aStreamLoader);

  static void LoadTimerCallback(nsITimer *aTimer, void *aClosure);

  static nsresult CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                                   nsIURI* aTargetURI,
                                   nsISupports* aContext);

private:
  nsRefPtr<gfxFontEntry>  mFontEntry;
  nsCOMPtr<nsIURI>        mFontURI;
  nsRefPtr<nsUserFontSet> mFontSet;
  nsCOMPtr<nsIChannel>    mChannel;
  nsCOMPtr<nsITimer>      mLoadTimer;

  nsIStreamLoader        *mStreamLoader;
};

#endif 
