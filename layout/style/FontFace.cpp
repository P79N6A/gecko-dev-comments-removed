




#include "mozilla/dom/FontFace.h"

#include "mozilla/dom/FontFaceBinding.h"
#include "mozilla/dom/Promise.h"
#include "nsCSSRules.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(FontFace, mParent, mLoaded, mRule)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FontFace)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(FontFace)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FontFace)

FontFace::FontFace(nsISupports* aParent)
  : mParent(aParent)
{
  MOZ_COUNT_CTOR(FontFace);

  SetIsDOMBinding();
}

FontFace::~FontFace()
{
  MOZ_COUNT_DTOR(FontFace);
}

JSObject*
FontFace::WrapObject(JSContext* aCx)
{
  return FontFaceBinding::Wrap(aCx, this);
}

already_AddRefed<FontFace>
FontFace::CreateForRule(nsISupports* aGlobal, nsCSSFontFaceRule* aRule)
{
  nsRefPtr<FontFace> obj = new FontFace(aGlobal);
  obj->mRule = aRule;
  return obj.forget();
}

already_AddRefed<FontFace>
FontFace::Constructor(const GlobalObject& aGlobal,
                      const nsAString& aFamily,
                      const StringOrArrayBufferOrArrayBufferView& aSource,
                      const FontFaceDescriptors& aDescriptors,
                      ErrorResult& aRV)
{
  nsRefPtr<FontFace> obj = new FontFace(aGlobal.GetAsSupports());
  return obj.forget();
}

void
FontFace::GetFamily(nsString& aResult)
{
}

void
FontFace::SetFamily(const nsAString& aValue, ErrorResult& aRv)
{
}

void
FontFace::GetStyle(nsString& aResult)
{
}

void
FontFace::SetStyle(const nsAString& aValue, ErrorResult& aRv)
{
}

void
FontFace::GetWeight(nsString& aResult)
{
}

void
FontFace::SetWeight(const nsAString& aValue, ErrorResult& aRv)
{
}

void
FontFace::GetStretch(nsString& aResult)
{
}

void
FontFace::SetStretch(const nsAString& aValue, ErrorResult& aRv)
{
}

void
FontFace::GetUnicodeRange(nsString& aResult)
{
}

void
FontFace::SetUnicodeRange(const nsAString& aValue, ErrorResult& aRv)
{
}

void
FontFace::GetVariant(nsString& aResult)
{
}

void
FontFace::SetVariant(const nsAString& aValue, ErrorResult& aRv)
{
}

void
FontFace::GetFeatureSettings(nsString& aResult)
{
}

void
FontFace::SetFeatureSettings(const nsAString& aValue, ErrorResult& aRv)
{
}

FontFaceLoadStatus
FontFace::Status()
{
  return FontFaceLoadStatus::Unloaded;
}

Promise*
FontFace::Load()
{
  return Loaded();
}

Promise*
FontFace::Loaded()
{
  return nullptr;
}
