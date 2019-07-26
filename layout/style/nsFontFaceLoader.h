







#ifndef nsFontFaceLoader_h_
#define nsFontFaceLoader_h_

#include "nsCOMPtr.h"
#include "nsIStreamLoader.h"
#include "nsIChannel.h"
#include "gfxUserFontSet.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"
#include "nsCSSRules.h"

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

  
  
  nsresult StartLoad(gfxMixedFontFamily *aFamily,
                     gfxProxyFontEntry *aFontToLoad,
                     const gfxFontFaceSrc *aFontFaceSrc);

  
  
  void RemoveLoader(nsFontFaceLoader *aLoader);

  bool UpdateRules(const nsTArray<nsFontFaceRuleContainer>& aRules);

  nsPresContext *GetPresContext() { return mPresContext; }

  virtual void ReplaceFontEntry(gfxMixedFontFamily *aFamily,
                                gfxProxyFontEntry *aProxy,
                                gfxFontEntry *aFontEntry);

  nsCSSFontFaceRule *FindRuleForEntry(gfxFontEntry *aFontEntry);

protected:
  
  
  
  
  struct FontFaceRuleRecord {
    nsRefPtr<gfxFontEntry>       mFontEntry;
    nsFontFaceRuleContainer      mContainer;
  };

  void InsertRule(nsCSSFontFaceRule *aRule, uint8_t aSheetType,
                  nsTArray<FontFaceRuleRecord>& oldRules,
                  bool& aFontSetModified);

  virtual nsresult LogMessage(gfxMixedFontFamily *aFamily,
                              gfxProxyFontEntry *aProxy,
                              const char *aMessage,
                              uint32_t aFlags = nsIScriptError::errorFlag,
                              nsresult aStatus = NS_OK);

  virtual nsresult CheckFontLoad(const gfxFontFaceSrc *aFontFaceSrc,
                                 nsIPrincipal **aPrincipal);

  virtual nsresult SyncLoadFontData(gfxProxyFontEntry *aFontToLoad,
                                    const gfxFontFaceSrc *aFontFaceSrc,
                                    uint8_t* &aBuffer,
                                    uint32_t &aBufferLength);

  nsPresContext *mPresContext;  

  
  
  
  nsTHashtable< nsPtrHashKey<nsFontFaceLoader> > mLoaders;

  nsTArray<FontFaceRuleRecord>   mRules;
};

class nsFontFaceLoader : public nsIStreamLoaderObserver
{
public:
  nsFontFaceLoader(gfxMixedFontFamily *aFontFamily,
                   gfxProxyFontEntry *aFontToLoad, nsIURI *aFontURI, 
                   nsUserFontSet *aFontSet, nsIChannel *aChannel);

  virtual ~nsFontFaceLoader();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER 

  
  nsresult Init();
  
  void Cancel();

  void DropChannel() { mChannel = nullptr; }

  void StartedLoading(nsIStreamLoader *aStreamLoader);

  static void LoadTimerCallback(nsITimer *aTimer, void *aClosure);

  static nsresult CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                                   nsIURI* aTargetURI,
                                   nsISupports* aContext);

private:
  nsRefPtr<gfxMixedFontFamily> mFontFamily;
  nsRefPtr<gfxProxyFontEntry>  mFontEntry;
  nsCOMPtr<nsIURI>        mFontURI;
  nsRefPtr<nsUserFontSet> mFontSet;
  nsCOMPtr<nsIChannel>    mChannel;
  nsCOMPtr<nsITimer>      mLoadTimer;

  nsIStreamLoader        *mStreamLoader;
};

#endif 
