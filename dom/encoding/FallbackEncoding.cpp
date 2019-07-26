



#include "mozilla/dom/FallbackEncoding.h"

#include "mozilla/dom/EncodingUtils.h"
#include "nsUConvPropertySearch.h"
#include "nsIChromeRegistry.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"

namespace mozilla {
namespace dom {

static const char* localesFallbacks[][3] = {
#include "localesfallbacks.properties.h"
};

FallbackEncoding* FallbackEncoding::sInstance = nullptr;

FallbackEncoding::FallbackEncoding()
{
  MOZ_COUNT_CTOR(FallbackEncoding);
  MOZ_ASSERT(!FallbackEncoding::sInstance,
             "Singleton already exists.");
}

FallbackEncoding::~FallbackEncoding()
{
  MOZ_COUNT_DTOR(FallbackEncoding);
}

void
FallbackEncoding::Get(nsACString& aFallback)
{
  if (!mFallback.IsEmpty()) {
    aFallback = mFallback;
    return;
  }

  const nsAdoptingCString& override =
    Preferences::GetCString("intl.charset.fallback.override");
  
  
  if (!EncodingUtils::FindEncodingForLabel(override, mFallback) ||
      !EncodingUtils::IsAsciiCompatible(mFallback) ||
      mFallback.EqualsLiteral("UTF-8")) {
    mFallback.Truncate();
  }

  if (!mFallback.IsEmpty()) {
    aFallback = mFallback;
    return;
  }

  nsAutoCString locale;
  nsCOMPtr<nsIXULChromeRegistry> registry =
    mozilla::services::GetXULChromeRegistryService();
  if (registry) {
    registry->GetSelectedLocale(NS_LITERAL_CSTRING("global"), locale);
  }

  
  
  ToLowerCase(locale); 

  
  
  
  if (locale.EqualsLiteral("zh-tw") ||
      locale.EqualsLiteral("zh-hk") ||
      locale.EqualsLiteral("zh-mo") ||
      locale.EqualsLiteral("zh-hant")) {
    mFallback.AssignLiteral("Big5");
    aFallback = mFallback;
    return;
  }

  
  
  int32_t index = locale.FindChar('-');
  if (index >= 0) {
    locale.Truncate(index);
  }

  if (NS_FAILED(nsUConvPropertySearch::SearchPropertyValue(
      localesFallbacks, ArrayLength(localesFallbacks), locale, mFallback))) {
    mFallback.AssignLiteral("windows-1252");
  }

  aFallback = mFallback;
}

void
FallbackEncoding::FromLocale(nsACString& aFallback)
{
  MOZ_ASSERT(FallbackEncoding::sInstance,
             "Using uninitialized fallback cache.");
  FallbackEncoding::sInstance->Get(aFallback);
}


int
FallbackEncoding::PrefChanged(const char*, void*)
{
  MOZ_ASSERT(FallbackEncoding::sInstance,
             "Pref callback called with null fallback cache.");
  FallbackEncoding::sInstance->Invalidate();
  return 0;
}

void
FallbackEncoding::Initialize()
{
  MOZ_ASSERT(!FallbackEncoding::sInstance,
             "Initializing pre-existing fallback cache.");
  FallbackEncoding::sInstance = new FallbackEncoding;
  Preferences::RegisterCallback(FallbackEncoding::PrefChanged,
                                "intl.charset.fallback.override",
                                nullptr);
  Preferences::RegisterCallback(FallbackEncoding::PrefChanged,
                                "general.useragent.locale",
                                nullptr);
}

void
FallbackEncoding::Shutdown()
{
  MOZ_ASSERT(FallbackEncoding::sInstance,
             "Releasing non-existent fallback cache.");
  delete FallbackEncoding::sInstance;
  FallbackEncoding::sInstance = nullptr;
}

} 
} 
