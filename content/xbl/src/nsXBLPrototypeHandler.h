





































#ifndef nsXBLPrototypeHandler_h__
#define nsXBLPrototypeHandler_h__

#include "nsIAtom.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIController.h"
#include "nsAutoPtr.h"
#include "nsXBLEventHandler.h"
#include "nsIWeakReference.h"
#include "nsIScriptGlobalObject.h"
#include "nsDOMScriptObjectHolder.h"
#include "nsCycleCollectionParticipant.h"

class nsIDOMEvent;
class nsIContent;
class nsIDOMUIEvent;
class nsIDOMKeyEvent;
class nsIDOMMouseEvent;
class nsIDOMEventTarget;
class nsXBLPrototypeBinding;

#define NS_HANDLER_TYPE_XBL_JS              (1 << 0)
#define NS_HANDLER_TYPE_XBL_COMMAND         (1 << 1)
#define NS_HANDLER_TYPE_XUL                 (1 << 2)
#define NS_HANDLER_HAS_ALLOW_UNTRUSTED_ATTR (1 << 4)
#define NS_HANDLER_ALLOW_UNTRUSTED          (1 << 5)
#define NS_HANDLER_TYPE_SYSTEM              (1 << 6)
#define NS_HANDLER_TYPE_PREVENTDEFAULT      (1 << 7)


#define NS_PHASE_CAPTURING          1
#define NS_PHASE_TARGET             2
#define NS_PHASE_BUBBLING           3

class nsXBLPrototypeHandler
{
public:
  
  nsXBLPrototypeHandler(const PRUnichar* aEvent, const PRUnichar* aPhase,
                        const PRUnichar* aAction, const PRUnichar* aCommand,
                        const PRUnichar* aKeyCode, const PRUnichar* aCharCode,
                        const PRUnichar* aModifiers, const PRUnichar* aButton,
                        const PRUnichar* aClickCount, const PRUnichar* aGroup,
                        const PRUnichar* aPreventDefault,
                        const PRUnichar* aAllowUntrusted,
                        nsXBLPrototypeBinding* aBinding,
                        PRUint32 aLineNumber);

  
  nsXBLPrototypeHandler(nsIContent* aKeyElement);

  ~nsXBLPrototypeHandler();

  
  PRBool KeyEventMatched(nsIDOMKeyEvent* aKeyEvent,
                         PRUint32 aCharCode = 0,
                         PRBool aIgnoreShiftKey = PR_FALSE);
  inline PRBool KeyEventMatched(nsIAtom* aEventType,
                                nsIDOMKeyEvent* aEvent,
                                PRUint32 aCharCode = 0,
                                PRBool aIgnoreShiftKey = PR_FALSE)
  {
    if (aEventType != mEventName)
      return PR_FALSE;

    return KeyEventMatched(aEvent, aCharCode, aIgnoreShiftKey);
  }

  PRBool MouseEventMatched(nsIDOMMouseEvent* aMouseEvent);
  inline PRBool MouseEventMatched(nsIAtom* aEventType,
                                  nsIDOMMouseEvent* aEvent)
  {
    if (aEventType != mEventName)
      return PR_FALSE;

    return MouseEventMatched(aEvent);
  }

  already_AddRefed<nsIContent> GetHandlerElement();

  void AppendHandlerText(const nsAString& aText);

  PRUint8 GetPhase() { return mPhase; }
  PRUint8 GetType() { return mType; }

  nsXBLPrototypeHandler* GetNextHandler() { return mNextHandler; }
  void SetNextHandler(nsXBLPrototypeHandler* aHandler) { mNextHandler = aHandler; }

  nsresult ExecuteHandler(nsIDOMEventTarget* aTarget, nsIDOMEvent* aEvent);

  already_AddRefed<nsIAtom> GetEventName();
  void SetEventName(nsIAtom* aName) { mEventName = aName; }

  nsXBLEventHandler* GetEventHandler()
  {
    if (!mHandler) {
      NS_NewXBLEventHandler(this, mEventName, getter_AddRefs(mHandler));
      
    }

    return mHandler;
  }

  nsXBLEventHandler* GetCachedEventHandler()
  {
    return mHandler;
  }

  PRBool HasAllowUntrustedAttr()
  {
    return (mType & NS_HANDLER_HAS_ALLOW_UNTRUSTED_ATTR) != 0;
  }

  
  
  PRBool AllowUntrustedEvents()
  {
    return (mType & NS_HANDLER_ALLOW_UNTRUSTED) != 0;
  }

public:
  static PRUint32 gRefCnt;
  
protected:
  already_AddRefed<nsIController> GetController(nsIDOMEventTarget* aTarget);
  
  inline PRInt32 GetMatchingKeyCode(const nsAString& aKeyName);
  void ConstructPrototype(nsIContent* aKeyElement, 
                          const PRUnichar* aEvent=nsnull, const PRUnichar* aPhase=nsnull,
                          const PRUnichar* aAction=nsnull, const PRUnichar* aCommand=nsnull,
                          const PRUnichar* aKeyCode=nsnull, const PRUnichar* aCharCode=nsnull,
                          const PRUnichar* aModifiers=nsnull, const PRUnichar* aButton=nsnull,
                          const PRUnichar* aClickCount=nsnull, const PRUnichar* aGroup=nsnull,
                          const PRUnichar* aPreventDefault=nsnull,
                          const PRUnichar* aAllowUntrusted=nsnull);

  void ReportKeyConflict(const PRUnichar* aKey, const PRUnichar* aModifiers, nsIContent* aElement, const char *aMessageName);
  void GetEventType(nsAString& type);
  PRBool ModifiersMatchMask(nsIDOMUIEvent* aEvent,
                            PRBool aIgnoreShiftKey = PR_FALSE);
  nsresult DispatchXBLCommand(nsIDOMEventTarget* aTarget, nsIDOMEvent* aEvent);
  nsresult DispatchXULKeyCommand(nsIDOMEvent* aEvent);
  nsresult EnsureEventHandler(nsIScriptGlobalObject* aGlobal,
                              nsIScriptContext *aBoundContext, nsIAtom *aName,
                              nsScriptObjectHolder &aHandler);
  static PRInt32 KeyToMask(PRInt32 key);
  
  static PRInt32 kAccelKey;
  static PRInt32 kMenuAccessKey;
  static void InitAccessKeys();

  static const PRInt32 cShift;
  static const PRInt32 cAlt;
  static const PRInt32 cControl;
  static const PRInt32 cMeta;

  static const PRInt32 cShiftMask;
  static const PRInt32 cAltMask;
  static const PRInt32 cControlMask;
  static const PRInt32 cMetaMask;

  static const PRInt32 cAllModifiers;

protected:
  union {
    nsIWeakReference* mHandlerElement;  
    PRUnichar*        mHandlerText;     
                                        
                                        
                                        
  };

  PRUint32 mLineNumber;  
  
  
  PRUint8 mPhase;            
  PRUint8 mKeyMask;          
                             
  PRUint8 mType;             
                             
                             
                             
  PRUint8 mMisc;             
                             
                             

  
  PRInt32 mDetail;           
                             

  
  nsXBLPrototypeHandler* mNextHandler;
  nsCOMPtr<nsIAtom> mEventName; 
  nsRefPtr<nsXBLEventHandler> mHandler;
  nsXBLPrototypeBinding* mPrototypeBinding; 
};

#endif
