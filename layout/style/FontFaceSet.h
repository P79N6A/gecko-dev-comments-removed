




#ifndef mozilla_dom_FontFaceSet_h
#define mozilla_dom_FontFaceSet_h

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/FontFace.h"
#include "mozilla/dom/FontFaceSetBinding.h"
#include "gfxUserFontSet.h"
#include "nsCSSRules.h"
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

class FontFaceSet MOZ_FINAL : public DOMEventTargetHelper
{
  friend class UserFontSet;

public:
  









  class UserFontSet MOZ_FINAL : public gfxUserFontSet
  {
    friend class FontFaceSet;

  public:
    UserFontSet(FontFaceSet* aFontFaceSet)
      : mFontFaceSet(aFontFaceSet)
    {
    }

    FontFaceSet* GetFontFaceSet() { return mFontFaceSet; }

    virtual nsresult CheckFontLoad(const gfxFontFaceSrc* aFontFaceSrc,
                                   nsIPrincipal** aPrincipal,
                                   bool* aBypassCache) MOZ_OVERRIDE;
    virtual nsresult StartLoad(gfxUserFontEntry* aUserFontEntry,
                               const gfxFontFaceSrc* aFontFaceSrc) MOZ_OVERRIDE;

  protected:
    virtual bool GetPrivateBrowsing() MOZ_OVERRIDE;
    virtual nsresult SyncLoadFontData(gfxUserFontEntry* aFontToLoad,
                                      const gfxFontFaceSrc* aFontFaceSrc,
                                      uint8_t*& aBuffer,
                                      uint32_t& aBufferLength) MOZ_OVERRIDE;
    virtual nsresult LogMessage(gfxUserFontEntry* aUserFontEntry,
                                const char* aMessage,
                                uint32_t aFlags = nsIScriptError::errorFlag,
                                nsresult aStatus = NS_OK) MOZ_OVERRIDE;
    virtual void DoRebuildUserFontSet() MOZ_OVERRIDE;
    virtual already_AddRefed<gfxUserFontEntry> CreateFontFace(
                                   const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                                   uint32_t aWeight,
                                   int32_t aStretch,
                                   uint32_t aItalicStyle,
                                   const nsTArray<gfxFontFeature>& aFeatureSettings,
                                   uint32_t aLanguageOverride,
                                   gfxSparseBitSet* aUnicodeRanges) MOZ_OVERRIDE;

  private:
    nsRefPtr<FontFaceSet> mFontFaceSet;
  };

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(FontFaceSet, DOMEventTargetHelper)

  FontFaceSet(nsPIDOMWindow* aWindow, nsPresContext* aPresContext);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  UserFontSet* EnsureUserFontSet(nsPresContext* aPresContext);
  UserFontSet* GetUserFontSet() { return mUserFontSet; }

  
  void DestroyUserFontSet();

  
  
  void RemoveLoader(nsFontFaceLoader* aLoader);

  bool UpdateRules(const nsTArray<nsFontFaceRuleContainer>& aRules);

  nsPresContext* GetPresContext() { return mPresContext; }

  
  nsCSSFontFaceRule* FindRuleForEntry(gfxFontEntry* aFontEntry);

  void IncrementGeneration(bool aIsRebuild = false);

  

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
  bool Delete(FontFace& aFontFace, mozilla::ErrorResult& aRv);
  bool Has(FontFace& aFontFace);
  FontFace* IndexedGetter(uint32_t aIndex, bool& aFound);
  uint32_t Length();

private:
  ~FontFaceSet();

  
  
  
  struct FontFaceRuleRecord {
    nsRefPtr<gfxUserFontEntry>   mUserFontEntry;
    nsFontFaceRuleContainer      mContainer;
  };

  FontFace* FontFaceForRule(nsCSSFontFaceRule* aRule);
  void InsertRule(nsCSSFontFaceRule* aRule, uint8_t aSheetType,
                  nsTArray<FontFaceRuleRecord>& oldRules,
                  bool& aFontSetModified);

  already_AddRefed<gfxUserFontEntry> FindOrCreateFontFaceFromRule(
                                                   const nsAString& aFamilyName,
                                                   nsCSSFontFaceRule* aRule,
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
  void DoRebuildUserFontSet();

  nsRefPtr<UserFontSet> mUserFontSet;
  nsPresContext* mPresContext;

  nsRefPtr<mozilla::dom::Promise> mReady;

  
  
  
  nsTHashtable< nsPtrHashKey<nsFontFaceLoader> > mLoaders;

  
  nsTArray<FontFaceRuleRecord> mRules;
};

} 
} 

#endif 
