





#ifndef mozilla_dom_domerror_h__
#define mozilla_dom_domerror_h__

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsCycleCollectionParticipant.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class GlobalObject;

class DOMError : public nsISupports,
                 public nsWrapperCache
{
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsString mName;
  nsString mMessage;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMError)

  
  

  DOMError(nsPIDOMWindow* aWindow);

  DOMError(nsPIDOMWindow* aWindow, nsresult aValue);

  DOMError(nsPIDOMWindow* aWindow, const nsAString& aName);

  DOMError(nsPIDOMWindow* aWindow, const nsAString& aName,
           const nsAString& aMessage);

  virtual ~DOMError();

  nsPIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  static already_AddRefed<DOMError>
  Constructor(const GlobalObject& global, const nsAString& name,
              const nsAString& message, ErrorResult& aRv);

  void GetName(nsString& aRetval) const
  {
    aRetval = mName;
  }

  void GetMessage(nsString& aRetval) const
  {
    aRetval = mMessage;
  }

  void Init(const nsAString& aName, const nsAString& aMessage)
  {
    mName = aName;
    mMessage = aMessage;
  }
};

} 
} 

#endif 
