




#ifndef nsJSEventListener_h__
#define nsJSEventListener_h__

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
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
                    nsISupports* aTarget, nsIAtom* aType,
                    const nsEventHandler& aHandler);
  virtual ~nsJSEventListener();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMEVENTLISTENER

  

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsJSEventListener)

protected:
  virtual void UpdateScopeObject(JS::Handle<JSObject*> aScopeObject);

  bool IsBlackForCC();
};

#endif 

