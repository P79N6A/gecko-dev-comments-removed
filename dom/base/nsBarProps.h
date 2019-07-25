










































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
  explicit nsBarProp(nsGlobalWindow *aWindow);
  virtual ~nsBarProp();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetVisibleByFlag(PRBool *aVisible, PRUint32 aChromeFlag);
  NS_IMETHOD SetVisibleByFlag(PRBool aVisible, PRUint32 aChromeFlag);

protected:
  already_AddRefed<nsIWebBrowserChrome> GetBrowserChrome();

  nsGlobalWindow             *mDOMWindow;
  nsCOMPtr<nsIWeakReference>  mDOMWindowWeakref;
  







};


class nsMenubarProp : public nsBarProp
{
public:
  explicit nsMenubarProp(nsGlobalWindow *aWindow);
  virtual ~nsMenubarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsToolbarProp : public nsBarProp
{
public:
  explicit nsToolbarProp(nsGlobalWindow *aWindow);
  virtual ~nsToolbarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsLocationbarProp : public nsBarProp
{
public:
  explicit nsLocationbarProp(nsGlobalWindow *aWindow);
  virtual ~nsLocationbarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsPersonalbarProp : public nsBarProp
{
public:
  explicit nsPersonalbarProp(nsGlobalWindow *aWindow);
  virtual ~nsPersonalbarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsStatusbarProp : public nsBarProp
{
public:
  explicit nsStatusbarProp(nsGlobalWindow *aWindow);
  virtual ~nsStatusbarProp();

  NS_DECL_NSIDOMBARPROP
};


class nsScrollbarsProp : public nsBarProp
{
public:
  explicit nsScrollbarsProp(nsGlobalWindow *aWindow);
  virtual ~nsScrollbarsProp();

  NS_DECL_NSIDOMBARPROP
};

#endif

