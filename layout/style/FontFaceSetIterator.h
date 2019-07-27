




#ifndef mozilla_dom_FontFaceSetIterator_h
#define mozilla_dom_FontFaceSetIterator_h

#include "mozilla/dom/FontFaceSetBinding.h"
#include "mozilla/dom/NonRefcountedDOMObject.h"

namespace mozilla {
namespace dom {

class FontFaceSetIterator final : public NonRefcountedDOMObject
{
public:
  FontFaceSetIterator(mozilla::dom::FontFaceSet* aFontFaceSet,
                      bool aIsKeyAndValue);
  ~FontFaceSetIterator();

  bool WrapObject(JSContext* aCx,
                  JS::Handle<JSObject*> aGivenProto,
                  JS::MutableHandle<JSObject*> aReflector);

  
  void Next(JSContext* aCx, FontFaceSetIteratorResult& aResult,
            mozilla::ErrorResult& aRv);

private:
  nsRefPtr<FontFaceSet> mFontFaceSet;
  uint32_t mNextIndex;
  bool mIsKeyAndValue;
};

} 
} 

#endif 
