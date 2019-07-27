




#ifndef mozilla_dom_FontFace_h
#define mozilla_dom_FontFace_h

#include "mozilla/dom/FontFaceBinding.h"
#include "gfxUserFontSet.h"
#include "nsCSSProperty.h"
#include "nsCSSValue.h"
#include "nsWrapperCache.h"

class nsCSSFontFaceRule;
class nsPresContext;

namespace mozilla {
struct CSSFontFaceDescriptors;
namespace dom {
struct FontFaceDescriptors;
class FontFaceSet;
class Promise;
class StringOrArrayBufferOrArrayBufferView;
}
}

namespace mozilla {
namespace dom {

class FontFace MOZ_FINAL : public nsISupports,
                           public nsWrapperCache
{
  friend class Entry;

public:
  class Entry MOZ_FINAL : public gfxUserFontEntry {
    friend class FontFace;

  public:
    Entry(gfxUserFontSet* aFontSet,
          const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
          uint32_t aWeight,
          int32_t aStretch,
          uint32_t aItalicStyle,
          const nsTArray<gfxFontFeature>& aFeatureSettings,
          uint32_t aLanguageOverride,
          gfxSparseBitSet* aUnicodeRanges)
      : gfxUserFontEntry(aFontSet, aFontFaceSrcList, aWeight, aStretch,
                         aItalicStyle, aFeatureSettings, aLanguageOverride,
                         aUnicodeRanges) {}

    virtual void SetLoadState(UserFontLoadState aLoadState) MOZ_OVERRIDE;

  protected:
    
    
    
    
    nsAutoTArray<FontFace*,1> mFontFaces;
  };

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(FontFace)

  nsISupports* GetParentObject() const { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static already_AddRefed<FontFace> CreateForRule(
                                              nsISupports* aGlobal,
                                              nsPresContext* aPresContext,
                                              nsCSSFontFaceRule* aRule,
                                              gfxUserFontEntry* aUserFontEntry);

  nsCSSFontFaceRule* GetRule() { return mRule; }

  void GetDesc(nsCSSFontDesc aDescID, nsCSSValue& aResult) const;

  gfxUserFontEntry* GetUserFontEntry() const { return mUserFontEntry; }
  void SetUserFontEntry(gfxUserFontEntry* aEntry);

  bool IsInFontFaceSet() { return mInFontFaceSet; }

  




  bool GetFamilyName(nsString& aResult);

  



  bool IsConnected() const { return mRule; }

  


  void DisconnectFromRule();

  
  static already_AddRefed<FontFace>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aFamily,
              const mozilla::dom::StringOrArrayBufferOrArrayBufferView& aSource,
              const mozilla::dom::FontFaceDescriptors& aDescriptors,
              ErrorResult& aRV);

  void GetFamily(nsString& aResult);
  void SetFamily(const nsAString& aValue, mozilla::ErrorResult& aRv);
  void GetStyle(nsString& aResult);
  void SetStyle(const nsAString& aValue, mozilla::ErrorResult& aRv);
  void GetWeight(nsString& aResult);
  void SetWeight(const nsAString& aValue, mozilla::ErrorResult& aRv);
  void GetStretch(nsString& aResult);
  void SetStretch(const nsAString& aValue, mozilla::ErrorResult& aRv);
  void GetUnicodeRange(nsString& aResult);
  void SetUnicodeRange(const nsAString& aValue, mozilla::ErrorResult& aRv);
  void GetVariant(nsString& aResult);
  void SetVariant(const nsAString& aValue, mozilla::ErrorResult& aRv);
  void GetFeatureSettings(nsString& aResult);
  void SetFeatureSettings(const nsAString& aValue, mozilla::ErrorResult& aRv);

  mozilla::dom::FontFaceLoadStatus Status();
  mozilla::dom::Promise* Load(mozilla::ErrorResult& aRv);
  mozilla::dom::Promise* GetLoaded(mozilla::ErrorResult& aRv);

private:
  FontFace(nsISupports* aParent, nsPresContext* aPresContext);
  ~FontFace();

  



  bool ParseDescriptor(nsCSSFontDesc aDescID, const nsAString& aString,
                       nsCSSValue& aResult);

  
  void SetDescriptor(nsCSSFontDesc aFontDesc,
                     const nsAString& aValue,
                     mozilla::ErrorResult& aRv);

  


  void SetStatus(mozilla::dom::FontFaceLoadStatus aStatus);

  void GetDesc(nsCSSFontDesc aDescID,
               nsCSSProperty aPropID,
               nsString& aResult) const;

  nsCOMPtr<nsISupports> mParent;
  nsPresContext* mPresContext;

  
  
  nsRefPtr<mozilla::dom::Promise> mLoaded;

  
  
  nsRefPtr<nsCSSFontFaceRule> mRule;

  
  
  nsRefPtr<Entry> mUserFontEntry;

  
  
  
  
  mozilla::dom::FontFaceLoadStatus mStatus;

  
  
  
  nsAutoPtr<mozilla::CSSFontFaceDescriptors> mDescriptors;

  
  
  nsRefPtr<FontFaceSet> mFontFaceSet;

  
  bool mInFontFaceSet;
};

} 
} 

#endif 
