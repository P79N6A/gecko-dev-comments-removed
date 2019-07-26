










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
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual bool GetVisible(ErrorResult& aRv) = 0;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) = 0;

protected:
  virtual ~BarProp();

  bool GetVisibleByFlag(uint32_t aChromeFlag, ErrorResult& aRv);
  void SetVisibleByFlag(bool aVisible, uint32_t aChromeFlag, ErrorResult &aRv);

  already_AddRefed<nsIWebBrowserChrome> GetBrowserChrome();

  nsRefPtr<nsGlobalWindow> mDOMWindow;
};


class MenubarProp MOZ_FINAL : public BarProp
{
public:
  explicit MenubarProp(nsGlobalWindow *aWindow);
  virtual ~MenubarProp();

  virtual bool GetVisible(ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) MOZ_OVERRIDE;
};


class ToolbarProp MOZ_FINAL : public BarProp
{
public:
  explicit ToolbarProp(nsGlobalWindow *aWindow);
  virtual ~ToolbarProp();

  virtual bool GetVisible(ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) MOZ_OVERRIDE;
};


class LocationbarProp MOZ_FINAL : public BarProp
{
public:
  explicit LocationbarProp(nsGlobalWindow *aWindow);
  virtual ~LocationbarProp();

  virtual bool GetVisible(ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) MOZ_OVERRIDE;
};


class PersonalbarProp MOZ_FINAL : public BarProp
{
public:
  explicit PersonalbarProp(nsGlobalWindow *aWindow);
  virtual ~PersonalbarProp();

  virtual bool GetVisible(ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) MOZ_OVERRIDE;
};


class StatusbarProp MOZ_FINAL : public BarProp
{
public:
  explicit StatusbarProp(nsGlobalWindow *aWindow);
  virtual ~StatusbarProp();

  virtual bool GetVisible(ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) MOZ_OVERRIDE;
};


class ScrollbarsProp MOZ_FINAL : public BarProp
{
public:
  explicit ScrollbarsProp(nsGlobalWindow *aWindow);
  virtual ~ScrollbarsProp();

  virtual bool GetVisible(ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void SetVisible(bool aVisible, ErrorResult& aRv) MOZ_OVERRIDE;
};

} 
} 

#endif 

