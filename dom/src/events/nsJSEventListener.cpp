



































#include "nsJSEventListener.h"
#include "nsJSUtils.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptRuntime.h"
#include "nsIXPConnect.h"
#include "nsIPrivateDOMEvent.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"
#include "nsDOMScriptObjectHolder.h"
#include "nsIMutableArray.h"
#include "nsVariant.h"
#include "nsIDOMBeforeUnloadEvent.h"
#include "nsGkAtoms.h"
#include "nsPIDOMEventTarget.h"
#include "nsIJSContextStack.h"
#ifdef NS_DEBUG
#include "nsDOMJSUtils.h"

#include "nspr.h" 

class EventListenerCounter
{
public:
  ~EventListenerCounter() {
  }
};

static EventListenerCounter sEventListenerCounter;
#endif




nsJSEventListener::nsJSEventListener(nsIScriptContext *aContext,
                                     void *aScopeObject,
                                     nsISupports *aTarget,
                                     nsIAtom* aType)
  : nsIJSEventListener(aContext, aScopeObject, aTarget), mEventName(aType)
{
  
  
  NS_ASSERTION(aScopeObject && aContext,
               "EventListener with no context or scope?");
  nsContentUtils::HoldScriptObject(aContext->GetScriptTypeID(), this,
                                   &NS_CYCLE_COLLECTION_NAME(nsJSEventListener),
                                   aScopeObject, PR_FALSE);
}

nsJSEventListener::~nsJSEventListener() 
{
  if (mContext)
    nsContentUtils::DropScriptObjects(mContext->GetScriptTypeID(), this,
                                &NS_CYCLE_COLLECTION_NAME(nsJSEventListener));
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSEventListener)
NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsJSEventListener)
  if (tmp->mContext &&
      tmp->mContext->GetScriptTypeID() == nsIProgrammingLanguage::JAVASCRIPT) {
    NS_DROP_JS_OBJECTS(tmp, nsJSEventListener);
    tmp->mScopeObject = nsnull;
  }
NS_IMPL_CYCLE_COLLECTION_ROOT_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSEventListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTarget)
  if (tmp->mContext) {
    if (tmp->mScopeObject) {
      nsContentUtils::DropScriptObjects(tmp->mContext->GetScriptTypeID(), tmp,
                                  &NS_CYCLE_COLLECTION_NAME(nsJSEventListener));
      tmp->mScopeObject = nsnull;
    }
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContext)
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsJSEventListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTarget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSEventListener)
  NS_IMPL_CYCLE_COLLECTION_TRACE_MEMBER_CALLBACK(tmp->mContext->GetScriptTypeID(),
                                                 mScopeObject)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsJSEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIJSEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMEventListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsJSEventListener, nsIDOMEventListener)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsJSEventListener, nsIDOMEventListener)

nsresult
nsJSEventListener::GetJSVal(const nsAString& aEventName, jsval* aJSVal)
{
  nsCOMPtr<nsPIDOMEventTarget> target = do_QueryInterface(mTarget);
  if (target && mContext) {
    nsAutoString eventString = NS_LITERAL_STRING("on") + aEventName;
    nsCOMPtr<nsIAtom> atomName = do_GetAtom(eventString);
    nsScriptObjectHolder funcval(mContext);
    nsresult rv = mContext->GetBoundEventHandler(mTarget, mScopeObject,
                                                 atomName, funcval);
    NS_ENSURE_SUCCESS(rv, rv);
    jsval funval =
      OBJECT_TO_JSVAL(static_cast<JSObject*>(static_cast<void*>(funcval)));
    *aJSVal = funval;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsJSEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> iargv;

  nsScriptObjectHolder funcval(mContext);
  rv = mContext->GetBoundEventHandler(mTarget, mScopeObject, mEventName,
                                      funcval);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!funcval)
    return NS_OK;

  PRBool handledScriptError = PR_FALSE;
  if (mEventName == nsGkAtoms::onerror) {
    nsCOMPtr<nsIPrivateDOMEvent> priv(do_QueryInterface(aEvent));
    NS_ENSURE_TRUE(priv, NS_ERROR_UNEXPECTED);

    nsEvent *event = priv->GetInternalNSEvent();
    if (event->message == NS_LOAD_ERROR &&
        event->eventStructType == NS_SCRIPT_ERROR_EVENT) {
      nsScriptErrorEvent *scriptEvent =
        static_cast<nsScriptErrorEvent*>(event);
      
      iargv = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
      if (NS_FAILED(rv)) return rv;
      
      nsCOMPtr<nsIWritableVariant>
          var(do_CreateInstance(NS_VARIANT_CONTRACTID, &rv));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = var->SetAsWString(scriptEvent->errorMsg);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = iargv->AppendElement(var, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
      
      var = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = var->SetAsWString(scriptEvent->fileName);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = iargv->AppendElement(var, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
      
      var = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = var->SetAsUint32(scriptEvent->lineNr);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = iargv->AppendElement(var, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      handledScriptError = PR_TRUE;
    }
  }

  if (!handledScriptError) {
    iargv = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    NS_ENSURE_TRUE(iargv != nsnull, NS_ERROR_OUT_OF_MEMORY);
    rv = iargv->AppendElement(aEvent, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
#ifdef NS_DEBUG
  JSContext* cx = nsnull;
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  NS_ASSERTION(stack && NS_SUCCEEDED(stack->Peek(&cx)) && cx &&
               GetScriptContextFromJSContext(cx) == mContext,
               "JSEventListener has wrong script context?");
#endif
  nsCOMPtr<nsIVariant> vrv;
  rv = mContext->CallEventHandler(mTarget, mScopeObject, funcval, iargv,
                                  getter_AddRefs(vrv));

  if (NS_SUCCEEDED(rv)) {
    PRUint16 dataType = nsIDataType::VTYPE_VOID;
    if (vrv)
      vrv->GetDataType(&dataType);

    if (mEventName == nsGkAtoms::onbeforeunload) {
      nsCOMPtr<nsIDOMBeforeUnloadEvent> beforeUnload = do_QueryInterface(aEvent);
      NS_ENSURE_STATE(beforeUnload);

      if (dataType != nsIDataType::VTYPE_VOID) {
        aEvent->PreventDefault();
        nsAutoString text;
        beforeUnload->GetReturnValue(text);

        
        
        
        if ((dataType == nsIDataType::VTYPE_DOMSTRING ||
             dataType == nsIDataType::VTYPE_CHAR_STR ||
             dataType == nsIDataType::VTYPE_WCHAR_STR ||
             dataType == nsIDataType::VTYPE_STRING_SIZE_IS ||
             dataType == nsIDataType::VTYPE_WSTRING_SIZE_IS ||
             dataType == nsIDataType::VTYPE_CSTRING ||
             dataType == nsIDataType::VTYPE_ASTRING)
            && text.IsEmpty()) {
          vrv->GetAsDOMString(text);
          beforeUnload->SetReturnValue(text);
        }
      }
    } else if (dataType == nsIDataType::VTYPE_BOOL) {
      
      
      
      PRBool brv;
      if (NS_SUCCEEDED(vrv->GetAsBool(&brv)) &&
          brv == (mEventName == nsGkAtoms::onerror ||
                  mEventName == nsGkAtoms::onmouseover)) {
        aEvent->PreventDefault();
      }
    }
  }

  return rv;
}





nsresult
NS_NewJSEventListener(nsIScriptContext *aContext, void *aScopeObject,
                      nsISupports*aTarget, nsIAtom* aEventType,
                      nsIDOMEventListener ** aReturn)
{
  NS_ENSURE_ARG(aEventType);
  nsJSEventListener* it =
    new nsJSEventListener(aContext, aScopeObject, aTarget, aEventType);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aReturn = it);

  return NS_OK;
}
