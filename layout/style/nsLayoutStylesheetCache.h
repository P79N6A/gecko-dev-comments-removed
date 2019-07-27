





#ifndef nsLayoutStylesheetCache_h__
#define nsLayoutStylesheetCache_h__

#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/StaticPtr.h"

class nsIFile;
class nsIURI;

namespace mozilla {
class CSSStyleSheet;
namespace css {
class Loader;
}
}

class nsLayoutStylesheetCache MOZ_FINAL
 : public nsIObserver
 , public nsIMemoryReporter
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIMEMORYREPORTER

  static mozilla::CSSStyleSheet* ScrollbarsSheet();
  static mozilla::CSSStyleSheet* FormsSheet();
  
  
  static mozilla::CSSStyleSheet* NumberControlSheet();
  static mozilla::CSSStyleSheet* UserContentSheet();
  static mozilla::CSSStyleSheet* UserChromeSheet();
  static mozilla::CSSStyleSheet* UASheet();
  static mozilla::CSSStyleSheet* HTMLSheet();
  static mozilla::CSSStyleSheet* MinimalXULSheet();
  static mozilla::CSSStyleSheet* XULSheet();
  static mozilla::CSSStyleSheet* QuirkSheet();
  static mozilla::CSSStyleSheet* FullScreenOverrideSheet();
  static mozilla::CSSStyleSheet* SVGSheet();
  static mozilla::CSSStyleSheet* MathMLSheet();
  static mozilla::CSSStyleSheet* CounterStylesSheet();

  static void Shutdown();

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  nsLayoutStylesheetCache();
  ~nsLayoutStylesheetCache();

  static void EnsureGlobal();
  void InitFromProfile();
  void InitMemoryReporter();
  static void LoadSheetURL(const char* aURL,
                           nsRefPtr<mozilla::CSSStyleSheet>& aSheet,
                           bool aEnableUnsafeRules);
  static void LoadSheetFile(nsIFile* aFile,
                            nsRefPtr<mozilla::CSSStyleSheet>& aSheet);
  static void LoadSheet(nsIURI* aURI, nsRefPtr<mozilla::CSSStyleSheet>& aSheet,
                        bool aEnableUnsafeRules);

  static mozilla::StaticRefPtr<nsLayoutStylesheetCache> gStyleCache;
  static mozilla::css::Loader* gCSSLoader;
  nsRefPtr<mozilla::CSSStyleSheet> mCounterStylesSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mFormsSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mFullScreenOverrideSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mHTMLSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mMathMLSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mMinimalXULSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mNumberControlSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mQuirkSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mSVGSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mScrollbarsSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mUASheet;
  nsRefPtr<mozilla::CSSStyleSheet> mUserChromeSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mUserContentSheet;
  nsRefPtr<mozilla::CSSStyleSheet> mXULSheet;
};

#endif
