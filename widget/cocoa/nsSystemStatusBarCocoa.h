




#ifndef nsSystemStatusBarCocoa_h_
#define nsSystemStatusBarCocoa_h_

#include "nsISystemStatusBar.h"
#include "nsClassHashtable.h"
#include "nsAutoPtr.h"

class nsStandaloneNativeMenu;
@class NSStatusItem;

class nsSystemStatusBarCocoa : public nsISystemStatusBar
{
public:
  nsSystemStatusBarCocoa() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSISYSTEMSTATUSBAR

protected:
  virtual ~nsSystemStatusBarCocoa() {}

  struct StatusItem
  {
    explicit StatusItem(nsStandaloneNativeMenu* aMenu);
    ~StatusItem();

  private:
    nsRefPtr<nsStandaloneNativeMenu> mMenu;
    NSStatusItem* mStatusItem;
  };

  nsClassHashtable<nsISupportsHashKey, StatusItem> mItems;
};

#endif 
