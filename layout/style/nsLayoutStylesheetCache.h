




































#ifndef nsLayoutStylesheetCache_h__
#define nsLayoutStylesheetCache_h__

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsAutoPtr.h"

class nsIFile;
class nsCSSStyleSheet;
class nsIURI;

namespace mozilla {
namespace css {
class Loader;
}
}

class nsLayoutStylesheetCache
 : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static nsCSSStyleSheet* ScrollbarsSheet();
  static nsCSSStyleSheet* FormsSheet();
  static nsCSSStyleSheet* UserContentSheet();
  static nsCSSStyleSheet* UserChromeSheet();
  static nsCSSStyleSheet* UASheet();
  static nsCSSStyleSheet* QuirkSheet();

  static void Shutdown();

private:
  nsLayoutStylesheetCache();
  ~nsLayoutStylesheetCache() {}

  static void EnsureGlobal();
  void InitFromProfile();
  static void LoadSheetFile(nsIFile* aFile, nsRefPtr<nsCSSStyleSheet> &aSheet);
  static void LoadSheet(nsIURI* aURI, nsRefPtr<nsCSSStyleSheet> &aSheet,
                        PRBool aEnableUnsafeRules);

  static nsLayoutStylesheetCache* gStyleCache;
  static mozilla::css::Loader* gCSSLoader;
  nsRefPtr<nsCSSStyleSheet> mScrollbarsSheet;
  nsRefPtr<nsCSSStyleSheet> mFormsSheet;
  nsRefPtr<nsCSSStyleSheet> mUserContentSheet;
  nsRefPtr<nsCSSStyleSheet> mUserChromeSheet;
  nsRefPtr<nsCSSStyleSheet> mUASheet;
  nsRefPtr<nsCSSStyleSheet> mQuirkSheet;
};

#endif
