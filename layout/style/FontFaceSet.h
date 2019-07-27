




#ifndef mozilla_dom_FontFaceSet_h
#define mozilla_dom_FontFaceSet_h

#include "mozilla/dom/FontFace.h"
#include "mozilla/dom/FontFaceSetBinding.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "gfxUserFontSet.h"
#include "nsCSSRules.h"
#include "nsICSSLoaderObserver.h"
#include "nsPIDOMWindow.h"

struct gfxFontFaceSrc;
class gfxUserFontEntry;
class nsFontFaceLoader;
class nsIPrincipal;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {
class FontFace;
class Promise;
}
}

namespace mozilla {
namespace dom {

class FontFaceSet final : public DOMEventTargetHelper
                        , public nsIDOMEventListener
                        , public nsICSSLoaderObserver
{
  friend class UserFontSet;

public:
  









  class UserFontSet final : public gfxUserFontSet
  {
    friend class FontFaceSet;

  public:
    explicit UserFontSet(FontFaceSet* aFontFaceSet)
      : mFontFaceSet(aFontFaceSet)
    {
    }

    FontFaceSet* GetFontFaceSet() { return mFontFaceSet; }

    virtual nsresult CheckFontLoad(const gfxFontFaceSrc* aFontFaceSrc,
                                   nsIPrincipal** aPrincipal,
                                   bool* aBypassCache) override;
    virtual nsresult StartLoad(gfxUserFontEntry* aUserFontEntry,
                               const gfxFontFaceSrc* aFontFaceSrc) override;

  protected:
    virtual bool GetPrivateBrowsing() override;
    virtual nsresult SyncLoadFontData(gfxUserFontEntry* aFontToLoad,
                                      const gfxFontFaceSrc* aFontFaceSrc,
                                      uint8_t*& aBuffer,
                                      uint32_t& aBufferLength) override;
    virtual nsresult LogMessage(gfxUserFontEntry* aUserFontEntry,
                                const char* aMessage,
                                uint32_t aFlags = nsIScriptError::errorFlag,
                                nsresult aStatus = NS_OK) override;
    virtual void DoRebuildUserFontSet() override;
    virtual already_AddRefed<gfxUserFontEntry> CreateUserFontEntry(
                                   const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                                   uint32_t aWeight,
                                   int32_t aStretch,
                                   uint32_t aItalicStyle,
                                   const nsTArray<gfxFontFeature>& aFeatureSettings,
                                   uint32_t aLanguageOverride,
                                   gfxSparseBitSet* aUnicodeRanges) override;

  private:
    nsRefPtr<FontFaceSet> mFontFaceSet;
  };

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(FontFaceSet, DOMEventTargetHelper)
  NS_DECL_NSIDOMEVENTLISTENER

  FontFaceSet(nsPIDOMWindow* aWindow, nsIDocument* aDocument);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  UserFontSet* GetUserFontSet() { return mUserFontSet; }

  
  
  void RemoveLoader(nsFontFaceLoader* aLoader);

  bool UpdateRules(const nsTArray<nsFontFaceRuleContainer>& aRules);

  nsPresContext* GetPresContext();

  
  nsCSSFontFaceRule* FindRuleForEntry(gfxFontEntry* aFontEntry);

  void IncrementGeneration(bool aIsRebuild = false);

  



  already_AddRefed<gfxUserFontEntry>
    FindOrCreateUserFontEntryFromFontFace(FontFace* aFontFace);

  



  void OnFontFaceStatusChanged(FontFace* aFontFace);

  




  void DidRefresh();

  


  static bool PrefEnabled();

  
  NS_IMETHOD StyleSheetLoaded(mozilla::CSSStyleSheet* aSheet,
                              bool aWasAlternate,
                              nsresult aStatus) override;

  FontFace* GetFontFaceAt(uint32_t aIndex);

  void FlushUserFontSet();

  

  IMPL_EVENT_HANDLER(loading)
  IMPL_EVENT_HANDLER(loadingdone)
  IMPL_EVENT_HANDLER(loadingerror)
  already_AddRefed<mozilla::dom::Promise> Load(const nsAString& aFont,
                                               const nsAString& aText,
                                               mozilla::ErrorResult& aRv);
  bool Check(const nsAString& aFont,
             const nsAString& aText,
             mozilla::ErrorResult& aRv);
  mozilla::dom::Promise* GetReady(mozilla::ErrorResult& aRv);
  mozilla::dom::FontFaceSetLoadStatus Status();

  FontFaceSet* Add(FontFace& aFontFace, mozilla::ErrorResult& aRv);
  void Clear();
  bool Delete(FontFace& aFontFace);
  bool Has(FontFace& aFontFace);
  uint32_t Size();
  mozilla::dom::FontFaceSetIterator* Entries();
  mozilla::dom::FontFaceSetIterator* Values();
  void ForEach(JSContext* aCx, FontFaceSetForEachCallback& aCallback,
               JS::Handle<JS::Value> aThisArg,
               mozilla::ErrorResult& aRv);

private:
  ~FontFaceSet();

  


  bool HasAvailableFontFace(FontFace* aFontFace);

  


  void Disconnect();

  void RemoveDOMContentLoadedListener();

  



  bool MightHavePendingFontLoads();

  



  void CheckLoadingStarted();

  



  void CheckLoadingFinished();

  



  void CheckLoadingFinishedAfterDelay();

  


  void DispatchLoadingFinishedEvent(
                                const nsAString& aType,
                                const nsTArray<FontFace*>& aFontFaces);

  
  
  
  struct FontFaceRecord {
    nsRefPtr<FontFace> mFontFace;
    uint8_t mSheetType;  

    
    
    
    bool mLoadEventShouldFire;
  };

  already_AddRefed<gfxUserFontEntry> FindOrCreateUserFontEntryFromFontFace(
                                                   const nsAString& aFamilyName,
                                                   FontFace* aFontFace,
                                                   uint8_t aSheetType);

  
  nsCSSFontFaceRule* FindRuleForUserFontEntry(gfxUserFontEntry* aUserFontEntry);

  nsresult StartLoad(gfxUserFontEntry* aUserFontEntry,
                     const gfxFontFaceSrc* aFontFaceSrc);
  nsresult CheckFontLoad(const gfxFontFaceSrc* aFontFaceSrc,
                         nsIPrincipal** aPrincipal,
                         bool* aBypassCache);
  bool GetPrivateBrowsing();
  nsresult SyncLoadFontData(gfxUserFontEntry* aFontToLoad,
                            const gfxFontFaceSrc* aFontFaceSrc,
                            uint8_t*& aBuffer,
                            uint32_t& aBufferLength);
  nsresult LogMessage(gfxUserFontEntry* aUserFontEntry,
                      const char* aMessage,
                      uint32_t aFlags,
                      nsresult aStatus);
  void RebuildUserFontSet();

  void InsertRuleFontFace(FontFace* aFontFace, uint8_t aSheetType,
                          nsTArray<FontFaceRecord>& aOldRecords,
                          bool& aFontSetModified);
  void InsertNonRuleFontFace(FontFace* aFontFace, bool& aFontSetModified);

#ifdef DEBUG
  bool HasRuleFontFace(FontFace* aFontFace);
#endif

  


  bool HasLoadingFontFaces();

  
  void UpdateHasLoadingFontFaces();

  nsRefPtr<UserFontSet> mUserFontSet;

  
  nsCOMPtr<nsIDocument> mDocument;

  
  
  
  
  
  
  nsRefPtr<mozilla::dom::Promise> mReady;

  
  
  
  nsTHashtable< nsPtrHashKey<nsFontFaceLoader> > mLoaders;

  
  nsTArray<FontFaceRecord> mRuleFaces;

  
  
  nsTArray<FontFaceRecord> mNonRuleFaces;

  
  mozilla::dom::FontFaceSetLoadStatus mStatus;

  
  bool mNonRuleFacesDirty;

  
  
  
  bool mHasLoadingFontFaces;

  
  bool mHasLoadingFontFacesIsDirty;

  
  
  bool mDelayedLoadCheck;
};

} 
} 

#endif 
