




#ifndef nsXBLWindowKeyHandler_h__
#define nsXBLWindowKeyHandler_h__

#include "nsWeakPtr.h"
#include "nsIDOMEventListener.h"

class nsIAtom;
class nsIDOMElement;
class nsIDOMKeyEvent;
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

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

protected:
  virtual ~nsXBLWindowKeyHandler();

  nsresult WalkHandlers(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType);

  
  bool WalkHandlersInternal(nsIDOMKeyEvent* aKeyEvent,
                            nsIAtom* aEventType,
                            nsXBLPrototypeHandler* aHandler,
                            bool aExecute);

  
  
  bool WalkHandlersAndExecute(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType,
                              nsXBLPrototypeHandler* aHandler,
                              uint32_t aCharCode, bool aIgnoreShiftKey,
                              bool aExecute);

  
  void HandleEventOnCapture(nsIDOMKeyEvent* aEvent);

  
  bool HasHandlerForEvent(nsIDOMKeyEvent* aEvent);

  
  
  nsresult EnsureHandlers();

  
  bool EventMatched(nsXBLPrototypeHandler* inHandler, nsIAtom* inEventType,
                      nsIDOMKeyEvent* inEvent, uint32_t aCharCode,
                      bool aIgnoreShiftKey);

  
  bool IsHTMLEditableFieldFocused();

  
  
  
  
  already_AddRefed<mozilla::dom::Element> GetElement(bool* aIsDisabled = nullptr);
  
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
