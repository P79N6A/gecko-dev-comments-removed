




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
#include "nsCycleCollectionParticipant.h"

#include "js/RootingAPI.h"

class JSObject;
class nsIDOMEvent;
class nsIContent;
class nsIDOMUIEvent;
class nsIDOMKeyEvent;
class nsIDOMMouseEvent;
class nsIDOMEventTarget;
class nsIObjectInputStream;
class nsIObjectOutputStream;
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
                        uint32_t aLineNumber);

  
  nsXBLPrototypeHandler(nsIContent* aKeyElement);

  
  nsXBLPrototypeHandler(nsXBLPrototypeBinding* aBinding);

  ~nsXBLPrototypeHandler();

  
  bool KeyEventMatched(nsIDOMKeyEvent* aKeyEvent,
                         uint32_t aCharCode = 0,
                         bool aIgnoreShiftKey = false);
  inline bool KeyEventMatched(nsIAtom* aEventType,
                                nsIDOMKeyEvent* aEvent,
                                uint32_t aCharCode = 0,
                                bool aIgnoreShiftKey = false)
  {
    if (aEventType != mEventName)
      return false;

    return KeyEventMatched(aEvent, aCharCode, aIgnoreShiftKey);
  }

  bool MouseEventMatched(nsIDOMMouseEvent* aMouseEvent);
  inline bool MouseEventMatched(nsIAtom* aEventType,
                                  nsIDOMMouseEvent* aEvent)
  {
    if (aEventType != mEventName)
      return false;

    return MouseEventMatched(aEvent);
  }

  already_AddRefed<nsIContent> GetHandlerElement();

  void AppendHandlerText(const nsAString& aText);

  uint8_t GetPhase() { return mPhase; }
  uint8_t GetType() { return mType; }

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

  bool HasAllowUntrustedAttr()
  {
    return (mType & NS_HANDLER_HAS_ALLOW_UNTRUSTED_ATTR) != 0;
  }

  
  
  bool AllowUntrustedEvents()
  {
    return (mType & NS_HANDLER_ALLOW_UNTRUSTED) != 0;
  }

  nsresult Read(nsIScriptContext* aContext, nsIObjectInputStream* aStream);
  nsresult Write(nsIScriptContext* aContext, nsIObjectOutputStream* aStream);

public:
  static uint32_t gRefCnt;
  
protected:
  void Init() {
    ++gRefCnt;
    if (gRefCnt == 1)
      
      InitAccessKeys();
  }

  already_AddRefed<nsIController> GetController(nsIDOMEventTarget* aTarget);
  
  inline int32_t GetMatchingKeyCode(const nsAString& aKeyName);
  void ConstructPrototype(nsIContent* aKeyElement, 
                          const PRUnichar* aEvent=nullptr, const PRUnichar* aPhase=nullptr,
                          const PRUnichar* aAction=nullptr, const PRUnichar* aCommand=nullptr,
                          const PRUnichar* aKeyCode=nullptr, const PRUnichar* aCharCode=nullptr,
                          const PRUnichar* aModifiers=nullptr, const PRUnichar* aButton=nullptr,
                          const PRUnichar* aClickCount=nullptr, const PRUnichar* aGroup=nullptr,
                          const PRUnichar* aPreventDefault=nullptr,
                          const PRUnichar* aAllowUntrusted=nullptr);

  void ReportKeyConflict(const PRUnichar* aKey, const PRUnichar* aModifiers, nsIContent* aElement, const char *aMessageName);
  void GetEventType(nsAString& type);
  bool ModifiersMatchMask(nsIDOMUIEvent* aEvent,
                            bool aIgnoreShiftKey = false);
  nsresult DispatchXBLCommand(nsIDOMEventTarget* aTarget, nsIDOMEvent* aEvent);
  nsresult DispatchXULKeyCommand(nsIDOMEvent* aEvent);
  nsresult EnsureEventHandler(nsIScriptGlobalObject* aGlobal,
                              nsIScriptContext *aBoundContext, nsIAtom *aName,
                              JS::MutableHandle<JSObject*> aHandler);
  static int32_t KeyToMask(int32_t key);
  
  static int32_t kAccelKey;
  static int32_t kMenuAccessKey;
  static void InitAccessKeys();

  static const int32_t cShift;
  static const int32_t cAlt;
  static const int32_t cControl;
  static const int32_t cMeta;
  static const int32_t cOS;

  static const int32_t cShiftMask;
  static const int32_t cAltMask;
  static const int32_t cControlMask;
  static const int32_t cMetaMask;
  static const int32_t cOSMask;

  static const int32_t cAllModifiers;

protected:
  union {
    nsIWeakReference* mHandlerElement;  
    PRUnichar*        mHandlerText;     
                                        
                                        
                                        
  };

  uint32_t mLineNumber;  
  
  
  uint8_t mPhase;            
  uint8_t mType;             
                             
                             
                             
  uint8_t mMisc;             
                             
                             

  int32_t mKeyMask;          
                             
 
  
  int32_t mDetail;           
                             

  
  nsXBLPrototypeHandler* mNextHandler;
  nsCOMPtr<nsIAtom> mEventName; 
  nsRefPtr<nsXBLEventHandler> mHandler;
  nsXBLPrototypeBinding* mPrototypeBinding; 
};

#endif
