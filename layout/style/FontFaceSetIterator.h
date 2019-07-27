




#ifndef mozilla_dom_FontFaceSetIterator_h
#define mozilla_dom_FontFaceSetIterator_h

#include "mozilla/dom/FontFaceSetBinding.h"
#include "mozilla/dom/NonRefcountedDOMObject.h"

namespace mozilla {
namespace dom {

class FontFaceSetIterator final
{
public:
  FontFaceSetIterator(mozilla::dom::FontFaceSet* aFontFaceSet,
                      bool aIsKeyAndValue);

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(FontFaceSetIterator)
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(FontFaceSetIterator)

  bool WrapObject(JSContext* aCx,
                  JS::Handle<JSObject*> aGivenProto,
                  JS::MutableHandle<JSObject*> aReflector);

  
  void Next(JSContext* aCx, FontFaceSetIteratorResult& aResult,
            mozilla::ErrorResult& aRv);

private:
  ~FontFaceSetIterator();

  nsRefPtr<FontFaceSet> mFontFaceSet;
  uint32_t mNextIndex;
  bool mIsKeyAndValue;
};

} 
} 

#endif 
