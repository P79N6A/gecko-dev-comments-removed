










#ifndef mozilla_dom_BarProps_h
#define mozilla_dom_BarProps_h

#include "nscore.h"
#include "nsIScriptContext.h"
#include "nsIDOMBarProp.h"
#include "nsIWeakReference.h"

class nsGlobalWindow;
class nsIWebBrowserChrome;

namespace mozilla {
namespace dom {


class BarProp : public nsIDOMBarProp
{
public:
  explicit BarProp(nsGlobalWindow *aWindow);
  virtual ~BarProp();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetVisibleByFlag(bool *aVisible, uint32_t aChromeFlag);
  NS_IMETHOD SetVisibleByFlag(bool aVisible, uint32_t aChromeFlag);

protected:
  already_AddRefed<nsIWebBrowserChrome> GetBrowserChrome();

  nsGlobalWindow             *mDOMWindow;
  nsCOMPtr<nsIWeakReference>  mDOMWindowWeakref;
  







};


class MenubarProp : public BarProp
{
public:
  explicit MenubarProp(nsGlobalWindow *aWindow);
  virtual ~MenubarProp();

  NS_DECL_NSIDOMBARPROP
};


class ToolbarProp : public BarProp
{
public:
  explicit ToolbarProp(nsGlobalWindow *aWindow);
  virtual ~ToolbarProp();

  NS_DECL_NSIDOMBARPROP
};


class LocationbarProp : public BarProp
{
public:
  explicit LocationbarProp(nsGlobalWindow *aWindow);
  virtual ~LocationbarProp();

  NS_DECL_NSIDOMBARPROP
};


class PersonalbarProp : public BarProp
{
public:
  explicit PersonalbarProp(nsGlobalWindow *aWindow);
  virtual ~PersonalbarProp();

  NS_DECL_NSIDOMBARPROP
};


class StatusbarProp : public BarProp
{
public:
  explicit StatusbarProp(nsGlobalWindow *aWindow);
  virtual ~StatusbarProp();

  NS_DECL_NSIDOMBARPROP
};


class ScrollbarsProp : public BarProp
{
public:
  explicit ScrollbarsProp(nsGlobalWindow *aWindow);
  virtual ~ScrollbarsProp();

  NS_DECL_NSIDOMBARPROP
};

} 
} 

#endif

