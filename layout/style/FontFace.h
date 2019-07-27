




#ifndef mozilla_dom_FontFace_h
#define mozilla_dom_FontFace_h

#include "mozilla/dom/FontFaceBinding.h"
#include "gfxUserFontSet.h"
#include "nsCSSProperty.h"
#include "nsCSSValue.h"
#include "nsWrapperCache.h"

class gfxFontFaceBufferSource;
class nsCSSFontFaceRule;

namespace mozilla {
struct CSSFontFaceDescriptors;
namespace dom {
class FontFaceBufferSource;
struct FontFaceDescriptors;
class FontFaceSet;
class Promise;
class StringOrArrayBufferOrArrayBufferView;
}
}

namespace mozilla {
namespace dom {

class FontFace final : public nsISupports,
                       public nsWrapperCache
{
  friend class mozilla::dom::FontFaceBufferSource;
  friend class Entry;

public:
  class Entry final : public gfxUserFontEntry {
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

    virtual void SetLoadState(UserFontLoadState aLoadState) override;

  protected:
    
    
    
    
    nsAutoTArray<FontFace*,1> mFontFaces;
  };

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(FontFace)

  nsISupports* GetParentObject() const { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<FontFace>
  CreateForRule(nsISupports* aGlobal, FontFaceSet* aFontFaceSet,
                nsCSSFontFaceRule* aRule);

  nsCSSFontFaceRule* GetRule() { return mRule; }

  void GetDesc(nsCSSFontDesc aDescID, nsCSSValue& aResult) const;

  gfxUserFontEntry* GetUserFontEntry() const { return mUserFontEntry; }
  void SetUserFontEntry(gfxUserFontEntry* aEntry);

  


  bool IsInFontFaceSet() { return mInFontFaceSet; }

  



  void SetIsInFontFaceSet(bool aInFontFaceSet) {
    MOZ_ASSERT(!(!aInFontFaceSet && HasRule()),
               "use DisconnectFromRule instead");
    mInFontFaceSet = aInFontFaceSet;
  }

  FontFaceSet* GetFontFaceSet() const { return mFontFaceSet; }

  




  bool GetFamilyName(nsString& aResult);

  



  bool HasRule() const { return mRule; }

  


  void DisconnectFromRule();

  



  bool HasFontData() const;

  



  already_AddRefed<gfxFontFaceBufferSource> CreateBufferSource();

  



  bool GetData(uint8_t*& aBuffer, uint32_t& aLength);

  
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
  FontFace(nsISupports* aParent, FontFaceSet* aFontFaceSet);
  ~FontFace();

  void InitializeSource(const StringOrArrayBufferOrArrayBufferView& aSource);

  
  void DoLoad();

  



  bool ParseDescriptor(nsCSSFontDesc aDescID, const nsAString& aString,
                       nsCSSValue& aResult);

  
  void SetDescriptor(nsCSSFontDesc aFontDesc,
                     const nsAString& aValue,
                     mozilla::ErrorResult& aRv);

  



  bool SetDescriptors(const nsAString& aFamily,
                      const FontFaceDescriptors& aDescriptors);

  


  void SetStatus(mozilla::dom::FontFaceLoadStatus aStatus);

  void GetDesc(nsCSSFontDesc aDescID,
               nsCSSProperty aPropID,
               nsString& aResult) const;

  


  void TakeBuffer(uint8_t*& aBuffer, uint32_t& aLength);

  nsCOMPtr<nsISupports> mParent;

  
  
  nsRefPtr<mozilla::dom::Promise> mLoaded;

  
  
  nsRefPtr<nsCSSFontFaceRule> mRule;

  
  
  nsRefPtr<Entry> mUserFontEntry;

  
  
  
  
  mozilla::dom::FontFaceLoadStatus mStatus;

  
  enum SourceType {
    eSourceType_FontFaceRule = 1,
    eSourceType_URLs,
    eSourceType_Buffer
  };

  
  SourceType mSourceType;

  
  
  uint8_t* mSourceBuffer;
  uint32_t mSourceBufferLength;

  
  
  
  nsAutoPtr<mozilla::CSSFontFaceDescriptors> mDescriptors;

  
  
  nsRefPtr<FontFaceSet> mFontFaceSet;

  
  bool mInFontFaceSet;
};

} 
} 

#endif 
