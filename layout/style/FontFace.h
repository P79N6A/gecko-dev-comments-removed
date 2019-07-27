




#ifndef mozilla_dom_FontFace_h
#define mozilla_dom_FontFace_h

#include "nsWrapperCache.h"
#include "mozilla/dom/FontFaceBinding.h"

namespace mozilla {
namespace dom {
struct FontFaceDescriptors;
class Promise;
class StringOrArrayBufferOrArrayBufferView;
}
}

namespace mozilla {
namespace dom {

class FontFace MOZ_FINAL : public nsISupports,
                           public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(FontFace)

  nsISupports* GetParentObject() const { return mParent; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
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
  mozilla::dom::Promise* Load();
  mozilla::dom::Promise* Loaded();

private:
  FontFace(nsISupports* aParent);
  ~FontFace();

  nsCOMPtr<nsISupports> mParent;
  nsRefPtr<mozilla::dom::Promise> mLoaded;
};

} 
} 

#endif 
