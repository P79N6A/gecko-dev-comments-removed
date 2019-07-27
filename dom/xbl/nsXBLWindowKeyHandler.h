





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
struct IgnoreModifierState;
} 
} 

class nsXBLWindowKeyHandler : public nsIDOMEventListener
{
  typedef mozilla::dom::IgnoreModifierState IgnoreModifierState;

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
                            bool aExecute,
                            bool* aOutReservedForChrome = nullptr);

  
  
  bool WalkHandlersAndExecute(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType,
                              nsXBLPrototypeHandler* aHandler,
                              uint32_t aCharCode,
                              const IgnoreModifierState& aIgnoreModifierState,
                              bool aExecute,
                              bool* aOutReservedForChrome = nullptr);

  
  void HandleEventOnCapture(nsIDOMKeyEvent* aEvent);

  
  
  
  bool HasHandlerForEvent(nsIDOMKeyEvent* aEvent,
                          bool* aOutReservedForChrome = nullptr);

  
  
  nsresult EnsureHandlers();

  
  bool EventMatched(nsXBLPrototypeHandler* aHandler, nsIAtom* aEventType,
                    nsIDOMKeyEvent* aEvent, uint32_t aCharCode,
                    const IgnoreModifierState& aIgnoreModifierState);

  
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
