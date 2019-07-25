




































#ifndef nsJSEventListener_h__
#define nsJSEventListener_h__

#include "nsIDOMKeyEvent.h"
#include "nsIJSEventListener.h"
#include "nsIDOMEventListener.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIScriptContext.h"
#include "nsCycleCollectionParticipant.h"



class nsJSEventListener : public nsIJSEventListener
{
public:
  nsJSEventListener(nsIScriptContext* aContext, JSObject* aScopeObject,
                    nsISupports* aTarget, nsIAtom* aType, JSObject* aHandler);
  virtual ~nsJSEventListener();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMEVENTLISTENER

  
  virtual void SetHandler(JSObject *aHandler);

  virtual PRInt64 SizeOf() const
  {
    return sizeof(*this);
  }

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsJSEventListener)
protected:
  nsCOMPtr<nsIAtom> mEventName;
};

#endif 

