






































#ifndef nsXBLWindowKeyHandler_h__
#define nsXBLWindowKeyHandler_h__

#include "nsWeakPtr.h"
#include "nsIDOMKeyListener.h"

class nsIAtom;
class nsIDOMElement;
class nsIDOMEventTarget;
class nsIDOMKeyEvent;
class nsIDOMEventTarget;
class nsIXBLDocumentInfo;
class nsXBLSpecialDocInfo;
class nsXBLPrototypeHandler;

class nsXBLWindowKeyHandler : public nsIDOMKeyListener
{
public:
  nsXBLWindowKeyHandler(nsIDOMElement* aElement, nsIDOMEventTarget* aTarget);
  virtual ~nsXBLWindowKeyHandler();
  
  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent)
  {
    return NS_OK;
  }

  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);
   
  NS_DECL_ISUPPORTS

  
  static NS_HIDDEN_(void) ShutDown();

protected:
  nsresult WalkHandlers(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType);

  
  nsresult WalkHandlersInternal(nsIDOMKeyEvent* aKeyEvent,
                                nsIAtom* aEventType, 
                                nsXBLPrototypeHandler* aHandler);

  
  PRBool WalkHandlersAndExecute(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType,
                                nsXBLPrototypeHandler* aHandler,
                                PRUint32 aCharCode, PRBool aIgnoreShiftKey);

  
  
  nsresult EnsureHandlers(PRBool *aIsEditor);

  
  PRBool EventMatched(nsXBLPrototypeHandler* inHandler, nsIAtom* inEventType,
                      nsIDOMKeyEvent* inEvent, PRUint32 aCharCode,
                      PRBool aIgnoreShiftKey);

  
  PRBool IsEditor() ;

  
  
  already_AddRefed<nsIDOMElement> GetElement();
  
  nsWeakPtr              mWeakPtrForElement;
  nsIDOMEventTarget*    mTarget; 

  
  
  nsXBLPrototypeHandler* mHandler;     
  nsXBLPrototypeHandler* mUserHandler; 

  
  static nsXBLSpecialDocInfo* sXBLSpecialDocInfo;
  static PRUint32 sRefCnt;
};

nsresult
NS_NewXBLWindowKeyHandler(nsIDOMElement* aElement,
                          nsIDOMEventTarget* aTarget,
                          nsXBLWindowKeyHandler** aResult);

#endif
