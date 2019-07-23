










































#ifndef nsBarProps_h___
#define nsBarProps_h___

#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsIDOMBarProp.h"
#include "nsIWeakReference.h"

class nsGlobalWindow;
class nsIWebBrowserChrome;


class nsBarProp : public nsIDOMBarProp
{
public:
  nsBarProp();
  virtual ~nsBarProp();

  NS_DECL_ISUPPORTS

  NS_IMETHOD SetWebBrowserChrome(nsIWebBrowserChrome* aBrowserChrome);

  NS_IMETHOD GetVisibleByFlag(PRBool *aVisible, PRUint32 aChromeFlag);
  NS_IMETHOD SetVisibleByFlag(PRBool aVisible, PRUint32 aChromeFlag);

protected:
  
  nsIWebBrowserChrome* mBrowserChrome;
};


class nsMenubarProp : public nsBarProp
{
public:
  nsMenubarProp();
  virtual ~nsMenubarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsToolbarProp : public nsBarProp
{
public:
  nsToolbarProp();
  virtual ~nsToolbarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsLocationbarProp : public nsBarProp
{
public:
  nsLocationbarProp();
  virtual ~nsLocationbarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsPersonalbarProp : public nsBarProp
{
public:
  nsPersonalbarProp();
  virtual ~nsPersonalbarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsStatusbarProp : public nsBarProp
{
public:
  nsStatusbarProp();
  virtual ~nsStatusbarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsScrollbarsProp : public nsBarProp {
public:
  nsScrollbarsProp(nsGlobalWindow *aWindow);
  virtual ~nsScrollbarsProp();

  NS_DECL_NSIDOMBARPROP

private:
  nsGlobalWindow           *mDOMWindow;
  nsCOMPtr<nsIWeakReference>  mDOMWindowWeakref;
  







};

#endif

