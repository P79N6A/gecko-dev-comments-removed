




































#ifndef nsJSEventListener_h__
#define nsJSEventListener_h__

#include "nsIDOMKeyEvent.h"
#include "nsIJSEventListener.h"
#include "nsIDOMMouseListener.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIScriptContext.h"
#include "nsCycleCollectionParticipant.h"



class nsJSEventListener : public nsIDOMEventListener,
                          public nsIJSEventListener
{
public:
  nsJSEventListener(nsIScriptContext *aContext, void *aScopeObject,
                    nsISupports* aObject, nsIAtom* aType);
  virtual ~nsJSEventListener();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMEVENTLISTENER

  
  virtual nsresult GetJSVal(const nsAString& aEventName, jsval* aJSVal);

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsJSEventListener,
                                                         nsIDOMEventListener)
protected:
  nsCOMPtr<nsIAtom> mEventName;
};

#endif 

