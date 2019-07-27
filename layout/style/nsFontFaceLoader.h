







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

class nsPresContext;
class nsIPrincipal;

class nsFontFaceLoader;


class nsUserFontSet : public gfxUserFontSet
{
public:
  explicit nsUserFontSet(nsPresContext* aContext);

  
  void Destroy();

  
  
  nsresult StartLoad(gfxUserFontEntry* aFontToLoad,
                     const gfxFontFaceSrc* aFontFaceSrc) MOZ_OVERRIDE;

  
  
  void RemoveLoader(nsFontFaceLoader* aLoader);

  bool UpdateRules(const nsTArray<nsFontFaceRuleContainer>& aRules);

  nsPresContext* GetPresContext() { return mPresContext; }

  
  nsCSSFontFaceRule* FindRuleForEntry(gfxFontEntry* aFontEntry);

protected:
  
  
  ~nsUserFontSet();

  
  
  
  
  struct FontFaceRuleRecord {
    nsRefPtr<gfxUserFontEntry>   mUserFontEntry;
    nsFontFaceRuleContainer      mContainer;
  };

  void InsertRule(nsCSSFontFaceRule* aRule, uint8_t aSheetType,
                  nsTArray<FontFaceRuleRecord>& oldRules,
                  bool& aFontSetModified);

  already_AddRefed<gfxUserFontEntry> FindOrCreateFontFaceFromRule(
                                                   const nsAString& aFamilyName,
                                                   nsCSSFontFaceRule* aRule,
                                                   uint8_t aSheetType);

  virtual nsresult LogMessage(gfxUserFontEntry* aUserFontEntry,
                              const char* aMessage,
                              uint32_t aFlags = nsIScriptError::errorFlag,
                              nsresult aStatus = NS_OK) MOZ_OVERRIDE;

  virtual nsresult CheckFontLoad(const gfxFontFaceSrc* aFontFaceSrc,
                                 nsIPrincipal** aPrincipal,
                                 bool* aBypassCache) MOZ_OVERRIDE;

  virtual nsresult SyncLoadFontData(gfxUserFontEntry* aFontToLoad,
                                    const gfxFontFaceSrc* aFontFaceSrc,
                                    uint8_t*& aBuffer,
                                    uint32_t& aBufferLength) MOZ_OVERRIDE;

  virtual bool GetPrivateBrowsing() MOZ_OVERRIDE;

  virtual void DoRebuildUserFontSet() MOZ_OVERRIDE;

  
  nsCSSFontFaceRule* FindRuleForUserFontEntry(gfxUserFontEntry* aUserFontEntry);

  nsPresContext* mPresContext;  

  
  
  
  nsTHashtable< nsPtrHashKey<nsFontFaceLoader> > mLoaders;

  nsTArray<FontFaceRuleRecord>   mRules;
};

class nsFontFaceLoader : public nsIStreamLoaderObserver
{
public:
  nsFontFaceLoader(gfxUserFontEntry* aFontToLoad, nsIURI* aFontURI,
                   nsUserFontSet* aFontSet, nsIChannel* aChannel);

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

protected:
  virtual ~nsFontFaceLoader();

private:
  nsRefPtr<gfxUserFontEntry>  mUserFontEntry;
  nsCOMPtr<nsIURI>        mFontURI;
  nsRefPtr<nsUserFontSet> mFontSet;
  nsCOMPtr<nsIChannel>    mChannel;
  nsCOMPtr<nsITimer>      mLoadTimer;

  nsIStreamLoader*        mStreamLoader;
};

#endif 
