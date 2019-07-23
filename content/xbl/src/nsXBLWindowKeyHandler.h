






































#ifndef nsXBLWindowKeyHandler_h__
#define nsXBLWindowKeyHandler_h__

#include "nsWeakPtr.h"
#include "nsIDOMKeyListener.h"

class nsIAtom;
class nsIDOMElement;
class nsIDOMEventReceiver;
class nsIXBLDocumentInfo;
class nsXBLSpecialDocInfo;
class nsXBLPrototypeHandler;

class nsXBLWindowKeyHandler : public nsIDOMKeyListener
{
public:
  nsXBLWindowKeyHandler(nsIDOMElement* aElement, nsIDOMEventReceiver* aReceiver);
  virtual ~nsXBLWindowKeyHandler();
  
  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent)
  {
    return NS_OK;
  };

  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);
   
  NS_DECL_ISUPPORTS

  
  static NS_HIDDEN_(void) ShutDown();

protected:
  nsresult WalkHandlers(nsIDOMEvent* aKeyEvent, nsIAtom* aEventType);

  
  nsresult WalkHandlersInternal(nsIDOMEvent* aKeyEvent,
                                nsIAtom* aEventType, 
                                nsXBLPrototypeHandler* aHandler);

  
  
  nsresult EnsureHandlers(PRBool *aIsEditor);

  
  PRBool EventMatched(nsXBLPrototypeHandler* inHandler, nsIAtom* inEventType,
                      nsIDOMEvent* inEvent);

  
  PRBool IsEditor() ;

  
  
  already_AddRefed<nsIDOMElement> GetElement();
  
  nsWeakPtr              mWeakPtrForElement;
  nsIDOMEventReceiver*   mReceiver; 

  
  
  nsXBLPrototypeHandler* mHandler;     
  nsXBLPrototypeHandler* mUserHandler; 

  
  static nsXBLSpecialDocInfo* sXBLSpecialDocInfo;
  static PRUint32 sRefCnt;
};

nsresult
NS_NewXBLWindowKeyHandler(nsIDOMElement* aElement,
                          nsIDOMEventReceiver* aReceiver,
                          nsXBLWindowKeyHandler** aResult);

#endif
