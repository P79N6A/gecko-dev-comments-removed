







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
#include "nsCSSRules.h"

class nsIRequest;
class nsISupports;
class nsPresContext;
class nsIPrincipal;

class nsFontFaceLoader;
class nsCSSFontFaceRule;


class nsUserFontSet : public gfxUserFontSet
{
public:
  nsUserFontSet(nsPresContext *aContext);
  ~nsUserFontSet();

  
  void Destroy();

  
  
  nsresult StartLoad(gfxProxyFontEntry *aFontToLoad,
                     const gfxFontFaceSrc *aFontFaceSrc);

  
  
  void RemoveLoader(nsFontFaceLoader *aLoader);

  bool UpdateRules(const nsTArray<nsFontFaceRuleContainer>& aRules);

  nsPresContext *GetPresContext() { return mPresContext; }

  virtual void ReplaceFontEntry(gfxProxyFontEntry *aProxy,
                                gfxFontEntry *aFontEntry);

  nsCSSFontFaceRule *FindRuleForEntry(gfxFontEntry *aFontEntry);

protected:
  
  
  
  
  struct FontFaceRuleRecord {
    nsRefPtr<gfxFontEntry>       mFontEntry;
    nsFontFaceRuleContainer      mContainer;
  };

  void InsertRule(nsCSSFontFaceRule *aRule, PRUint8 aSheetType,
                  nsTArray<FontFaceRuleRecord>& oldRules,
                  bool& aFontSetModified);

  virtual nsresult LogMessage(gfxProxyFontEntry *aProxy,
                              const char *aMessage,
                              PRUint32 aFlags = nsIScriptError::errorFlag,
                              nsresult aStatus = 0);

  nsresult CheckFontLoad(gfxProxyFontEntry *aFontToLoad,
                         const gfxFontFaceSrc *aFontFaceSrc,
                         nsIPrincipal **aPrincipal);

  virtual nsresult SyncLoadFontData(gfxProxyFontEntry *aFontToLoad,
                                    const gfxFontFaceSrc *aFontFaceSrc,
                                    PRUint8* &aBuffer,
                                    PRUint32 &aBufferLength);

  nsPresContext *mPresContext;  

  
  
  
  nsTHashtable< nsPtrHashKey<nsFontFaceLoader> > mLoaders;

  nsTArray<FontFaceRuleRecord>   mRules;
};

class nsFontFaceLoader : public nsIStreamLoaderObserver
{
public:

  nsFontFaceLoader(gfxProxyFontEntry *aFontToLoad, nsIURI *aFontURI, 
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
  nsRefPtr<gfxProxyFontEntry>  mFontEntry;
  nsRefPtr<gfxFontFamily>      mFontFamily;
  nsCOMPtr<nsIURI>        mFontURI;
  nsRefPtr<nsUserFontSet> mFontSet;
  nsCOMPtr<nsIChannel>    mChannel;
  nsCOMPtr<nsITimer>      mLoadTimer;

  nsIStreamLoader        *mStreamLoader;
};

#endif 
