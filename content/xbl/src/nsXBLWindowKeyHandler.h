




#ifndef nsXBLWindowKeyHandler_h__
#define nsXBLWindowKeyHandler_h__

#include "nsWeakPtr.h"
#include "nsIDOMEventListener.h"

class nsIAtom;
class nsIDOMElement;
class nsIDOMEventTarget;
class nsIDOMKeyEvent;
class nsIDOMEventTarget;
class nsXBLSpecialDocInfo;
class nsXBLPrototypeHandler;

namespace mozilla {
namespace dom {
class Element;
class EventTarget;
}
}

class nsXBLWindowKeyHandler : public nsIDOMEventListener
{
public:
  nsXBLWindowKeyHandler(nsIDOMElement* aElement, mozilla::dom::EventTarget* aTarget);
  virtual ~nsXBLWindowKeyHandler();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  
  static NS_HIDDEN_(void) ShutDown();

protected:
  nsresult WalkHandlers(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType);

  
  nsresult WalkHandlersInternal(nsIDOMKeyEvent* aKeyEvent,
                                nsIAtom* aEventType, 
                                nsXBLPrototypeHandler* aHandler);

  
  bool WalkHandlersAndExecute(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType,
                                nsXBLPrototypeHandler* aHandler,
                                uint32_t aCharCode, bool aIgnoreShiftKey);

  
  
  nsresult EnsureHandlers(bool *aIsEditor);

  
  bool EventMatched(nsXBLPrototypeHandler* inHandler, nsIAtom* inEventType,
                      nsIDOMKeyEvent* inEvent, uint32_t aCharCode,
                      bool aIgnoreShiftKey);

  
  bool IsEditor() ;

  
  
  already_AddRefed<mozilla::dom::Element> GetElement();
  
  nsWeakPtr              mWeakPtrForElement;
  mozilla::dom::EventTarget* mTarget; 

  
  
  nsXBLPrototypeHandler* mHandler;     
  nsXBLPrototypeHandler* mUserHandler; 

  
  static nsXBLSpecialDocInfo* sXBLSpecialDocInfo;
  static uint32_t sRefCnt;
};

already_AddRefed<nsXBLWindowKeyHandler>
NS_NewXBLWindowKeyHandler(nsIDOMElement* aElement,
                          mozilla::dom::EventTarget* aTarget);

#endif
