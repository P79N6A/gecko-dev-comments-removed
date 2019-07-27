











#ifndef mozilla_dom_BarProps_h
#define mozilla_dom_BarProps_h

#include "mozilla/Attributes.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsPIDOMWindow.h"

class nsGlobalWindow;
class nsIWebBrowserChrome;

namespace mozilla {

class ErrorResult;

namespace dom {


class BarProp : public nsISupports,
                public nsWrapperCache
{
public:
  explicit BarProp(nsGlobalWindow *aWindow);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BarProp)

  nsPIDOMWindow* GetParentObject() const;

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual bool GetVisible(ErrorResult& aRv) = 0;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) = 0;

protected:
  virtual ~BarProp();

  bool GetVisibleByFlag(uint32_t aChromeFlag, ErrorResult& aRv);
  void SetVisibleByFlag(bool aVisible, uint32_t aChromeFlag, ErrorResult &aRv);

  already_AddRefed<nsIWebBrowserChrome> GetBrowserChrome();

  nsRefPtr<nsGlobalWindow> mDOMWindow;
};


class MenubarProp final : public BarProp
{
public:
  explicit MenubarProp(nsGlobalWindow *aWindow);
  virtual ~MenubarProp();

  virtual bool GetVisible(ErrorResult& aRv) override;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) override;
};


class ToolbarProp final : public BarProp
{
public:
  explicit ToolbarProp(nsGlobalWindow *aWindow);
  virtual ~ToolbarProp();

  virtual bool GetVisible(ErrorResult& aRv) override;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) override;
};


class LocationbarProp final : public BarProp
{
public:
  explicit LocationbarProp(nsGlobalWindow *aWindow);
  virtual ~LocationbarProp();

  virtual bool GetVisible(ErrorResult& aRv) override;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) override;
};


class PersonalbarProp final : public BarProp
{
public:
  explicit PersonalbarProp(nsGlobalWindow *aWindow);
  virtual ~PersonalbarProp();

  virtual bool GetVisible(ErrorResult& aRv) override;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) override;
};


class StatusbarProp final : public BarProp
{
public:
  explicit StatusbarProp(nsGlobalWindow *aWindow);
  virtual ~StatusbarProp();

  virtual bool GetVisible(ErrorResult& aRv) override;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) override;
};


class ScrollbarsProp final : public BarProp
{
public:
  explicit ScrollbarsProp(nsGlobalWindow *aWindow);
  virtual ~ScrollbarsProp();

  virtual bool GetVisible(ErrorResult& aRv) override;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) override;
};

} 
} 

#endif 

