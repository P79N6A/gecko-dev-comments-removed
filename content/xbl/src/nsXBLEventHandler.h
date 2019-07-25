





































#ifndef nsXBLEventHandler_h__
#define nsXBLEventHandler_h__

#include "nsCOMPtr.h"
#include "nsIDOMEventListener.h"
#include "nsTArray.h"

class nsIAtom;
class nsIContent;
class nsIDOM3EventTarget;
class nsIDOMKeyEvent;
class nsXBLPrototypeHandler;

class nsXBLEventHandler : public nsIDOMEventListener
{
public:
  nsXBLEventHandler(nsXBLPrototypeHandler* aHandler);
  virtual ~nsXBLEventHandler();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMEVENTLISTENER

protected:
  nsXBLPrototypeHandler* mProtoHandler;

private:
  nsXBLEventHandler();
  virtual PRBool EventMatched(nsIDOMEvent* aEvent)
  {
    return PR_TRUE;
  }
};

class nsXBLMouseEventHandler : public nsXBLEventHandler
{
public:
  nsXBLMouseEventHandler(nsXBLPrototypeHandler* aHandler);
  virtual ~nsXBLMouseEventHandler();

private:
  PRBool EventMatched(nsIDOMEvent* aEvent);
};

class nsXBLKeyEventHandler : public nsIDOMEventListener
{
public:
  nsXBLKeyEventHandler(nsIAtom* aEventType, PRUint8 aPhase, PRUint8 aType);
  virtual ~nsXBLKeyEventHandler();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMEVENTLISTENER

  void AddProtoHandler(nsXBLPrototypeHandler* aProtoHandler)
  {
    mProtoHandlers.AppendElement(aProtoHandler);
  }

  PRBool Matches(nsIAtom* aEventType, PRUint8 aPhase, PRUint8 aType) const
  {
    return (mEventType == aEventType && mPhase == aPhase && mType == aType);
  }

  void GetEventName(nsAString& aString) const
  {
    mEventType->ToString(aString);
  }

  PRUint8 GetPhase() const
  {
    return mPhase;
  }

  PRUint8 GetType() const
  {
    return mType;
  }

  void SetIsBoundToChrome(PRBool aIsBoundToChrome)
  {
    mIsBoundToChrome = aIsBoundToChrome;
  }
private:
  nsXBLKeyEventHandler();
  PRBool ExecuteMatchedHandlers(nsIDOMKeyEvent* aEvent, PRUint32 aCharCode,
                                PRBool aIgnoreShiftKey);

  nsTArray<nsXBLPrototypeHandler*> mProtoHandlers;
  nsCOMPtr<nsIAtom> mEventType;
  PRUint8 mPhase;
  PRUint8 mType;
  PRPackedBool mIsBoundToChrome;
};

nsresult
NS_NewXBLEventHandler(nsXBLPrototypeHandler* aHandler,
                      nsIAtom* aEventType,
                      nsXBLEventHandler** aResult);

nsresult
NS_NewXBLKeyEventHandler(nsIAtom* aEventType, PRUint8 aPhase,
                         PRUint8 aType, nsXBLKeyEventHandler** aResult);

#endif
