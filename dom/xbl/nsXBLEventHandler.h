





#ifndef nsXBLEventHandler_h__
#define nsXBLEventHandler_h__

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventListener.h"
#include "nsTArray.h"

class nsIAtom;
class nsIDOMKeyEvent;
class nsXBLPrototypeHandler;

namespace mozilla {
namespace dom {
struct IgnoreModifierState;
} 
} 

class nsXBLEventHandler : public nsIDOMEventListener
{
public:
  explicit nsXBLEventHandler(nsXBLPrototypeHandler* aHandler);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMEVENTLISTENER

protected:
  virtual ~nsXBLEventHandler();
  nsXBLPrototypeHandler* mProtoHandler;

private:
  nsXBLEventHandler();
  virtual bool EventMatched(nsIDOMEvent* aEvent)
  {
    return true;
  }
};

class nsXBLMouseEventHandler : public nsXBLEventHandler
{
public:
  explicit nsXBLMouseEventHandler(nsXBLPrototypeHandler* aHandler);
  virtual ~nsXBLMouseEventHandler();

private:
  bool EventMatched(nsIDOMEvent* aEvent) override;
};

class nsXBLKeyEventHandler : public nsIDOMEventListener
{
  typedef mozilla::dom::IgnoreModifierState IgnoreModifierState;

public:
  nsXBLKeyEventHandler(nsIAtom* aEventType, uint8_t aPhase, uint8_t aType);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMEVENTLISTENER

  void AddProtoHandler(nsXBLPrototypeHandler* aProtoHandler)
  {
    mProtoHandlers.AppendElement(aProtoHandler);
  }

  bool Matches(nsIAtom* aEventType, uint8_t aPhase, uint8_t aType) const
  {
    return (mEventType == aEventType && mPhase == aPhase && mType == aType);
  }

  void GetEventName(nsAString& aString) const
  {
    mEventType->ToString(aString);
  }

  uint8_t GetPhase() const
  {
    return mPhase;
  }

  uint8_t GetType() const
  {
    return mType;
  }

  void SetIsBoundToChrome(bool aIsBoundToChrome)
  {
    mIsBoundToChrome = aIsBoundToChrome;
  }

  void SetUsingContentXBLScope(bool aUsingContentXBLScope)
  {
    mUsingContentXBLScope = aUsingContentXBLScope;
  }

private:
  nsXBLKeyEventHandler();
  virtual ~nsXBLKeyEventHandler();

  bool ExecuteMatchedHandlers(nsIDOMKeyEvent* aEvent, uint32_t aCharCode,
                              const IgnoreModifierState& aIgnoreModifierState);

  nsTArray<nsXBLPrototypeHandler*> mProtoHandlers;
  nsCOMPtr<nsIAtom> mEventType;
  uint8_t mPhase;
  uint8_t mType;
  bool mIsBoundToChrome;
  bool mUsingContentXBLScope;
};

already_AddRefed<nsXBLEventHandler>
NS_NewXBLEventHandler(nsXBLPrototypeHandler* aHandler,
                      nsIAtom* aEventType);

#endif
