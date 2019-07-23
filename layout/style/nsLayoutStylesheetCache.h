




































#ifndef nsLayoutStylesheetCache_h__
#define nsLayoutStylesheetCache_h__

#include "nsICSSStyleSheet.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"

class nsIFile;

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

  static nsICSSStyleSheet* ScrollbarsSheet();
  static nsICSSStyleSheet* FormsSheet();
  static nsICSSStyleSheet* UserContentSheet();
  static nsICSSStyleSheet* UserChromeSheet();
  static nsICSSStyleSheet* UASheet();
  static nsICSSStyleSheet* QuirkSheet();

  static void Shutdown();

private:
  nsLayoutStylesheetCache();
  ~nsLayoutStylesheetCache();

  static void EnsureGlobal();
  void InitFromProfile();
  static void LoadSheetFile(nsIFile* aFile, nsCOMPtr<nsICSSStyleSheet> &aSheet);
  static void LoadSheet(nsIURI* aURI, nsCOMPtr<nsICSSStyleSheet> &aSheet,
                        PRBool aEnableUnsafeRules);

  static nsLayoutStylesheetCache* gStyleCache;
  static mozilla::css::Loader* gCSSLoader;
  nsCOMPtr<nsICSSStyleSheet> mScrollbarsSheet;
  nsCOMPtr<nsICSSStyleSheet> mFormsSheet;
  nsCOMPtr<nsICSSStyleSheet> mUserContentSheet;
  nsCOMPtr<nsICSSStyleSheet> mUserChromeSheet;
  nsCOMPtr<nsICSSStyleSheet> mUASheet;
  nsCOMPtr<nsICSSStyleSheet> mQuirkSheet;
};

#endif
