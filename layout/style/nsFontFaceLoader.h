







#ifndef nsFontFaceLoader_h_
#define nsFontFaceLoader_h_

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIStreamLoader.h"
#include "nsIChannel.h"
#include "gfxUserFontSet.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"
#include "nsCSSRules.h"

class nsIPrincipal;

class nsFontFaceLoader : public nsIStreamLoaderObserver
{
public:
  nsFontFaceLoader(gfxUserFontEntry* aFontToLoad, nsIURI* aFontURI,
                   mozilla::dom::FontFaceSet* aFontFaceSet,
                   nsIChannel* aChannel);

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER 

  
  nsresult Init();
  
  void Cancel();

  void DropChannel() { mChannel = nullptr; }

  void StartedLoading(nsIStreamLoader* aStreamLoader);

  static void LoadTimerCallback(nsITimer* aTimer, void* aClosure);

  static nsresult CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                                   nsIURI* aTargetURI,
                                   nsISupports* aContext);
  gfxUserFontEntry* GetUserFontEntry() const { return mUserFontEntry; }

protected:
  virtual ~nsFontFaceLoader();

private:
  nsRefPtr<gfxUserFontEntry>  mUserFontEntry;
  nsCOMPtr<nsIURI>        mFontURI;
  nsRefPtr<mozilla::dom::FontFaceSet> mFontFaceSet;
  nsCOMPtr<nsIChannel>    mChannel;
  nsCOMPtr<nsITimer>      mLoadTimer;

  nsIStreamLoader*        mStreamLoader;
};

#endif 
