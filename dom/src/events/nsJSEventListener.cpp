



































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


#ifdef NS_DEBUG

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
                                     nsISupports *aTarget)
  : nsIJSEventListener(aContext, aScopeObject, aTarget),
    mReturnResult(nsReturnResult_eNotSet)
{
  
  
  NS_ASSERTION(aScopeObject && aContext,
               "EventListener with no context or scope?");
  aContext->HoldScriptObject(aScopeObject);
}

nsJSEventListener::~nsJSEventListener() 
{
  mContext->DropScriptObject(mScopeObject);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSEventListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSEventListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTarget)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsJSEventListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTarget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)
  cb.NoteScriptChild(nsIProgrammingLanguage::JAVASCRIPT, tmp->mScopeObject);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN(nsJSEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIJSEventListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsJSEventListener)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsJSEventListener, nsIDOMEventListener)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsJSEventListener, nsIDOMEventListener)



void
nsJSEventListener::SetEventName(nsIAtom* aName)
{
  mEventName = aName;
}

nsresult
nsJSEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsresult rv;
  nsCOMPtr<nsIArray> iargv;
  nsAutoString eventString;
  nsCOMPtr<nsIAtom> atomName;

  if (!mEventName) {
    if (NS_OK != aEvent->GetType(eventString)) {
      
      return NS_OK;
    }
    
      if (eventString.EqualsLiteral("error") ||
          eventString.EqualsLiteral("mouseover")) {
        mReturnResult = nsReturnResult_eReverseReturnResult;
      }
      else {
        mReturnResult = nsReturnResult_eDoNotReverseReturnResult;
      }
    
    eventString.Assign(NS_LITERAL_STRING("on") + eventString);
	atomName = do_GetAtom(eventString);
  }
  else {
    mEventName->ToString(eventString);
	atomName = mEventName;
  }


  nsScriptObjectHolder funcval(mContext);
  rv = mContext->GetBoundEventHandler(mTarget, mScopeObject, atomName,
                                      funcval);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!funcval)
    return NS_OK;

  PRBool handledScriptError = PR_FALSE;
  if (eventString.EqualsLiteral("onerror")) {
    nsCOMPtr<nsIPrivateDOMEvent> priv(do_QueryInterface(aEvent));
    NS_ENSURE_TRUE(priv, NS_ERROR_UNEXPECTED);

    nsEvent* event;
    priv->GetInternalNSEvent(&event);
    if (event->message == NS_LOAD_ERROR &&
        event->eventStructType == NS_SCRIPT_ERROR_EVENT) {
      nsScriptErrorEvent *scriptEvent =
        NS_STATIC_CAST(nsScriptErrorEvent*, event);
      
      nsCOMPtr<nsIMutableArray> tempargv = 
        do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
      if (NS_FAILED(rv)) return rv;
      
      nsCOMPtr<nsIWritableVariant>
          var(do_CreateInstance(NS_VARIANT_CONTRACTID, &rv));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = var->SetAsWString(scriptEvent->errorMsg);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = tempargv->AppendElement(var, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
      
      var = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = var->SetAsWString(scriptEvent->fileName);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = tempargv->AppendElement(var, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
      
      var = do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = var->SetAsUint32(scriptEvent->lineNr);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = tempargv->AppendElement(var, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      
      iargv = do_QueryInterface(tempargv);

      handledScriptError = PR_TRUE;
    }
  }

  if (!handledScriptError) {
    nsCOMPtr<nsIMutableArray> tempargv = 
      do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    NS_ENSURE_TRUE(tempargv != nsnull, NS_ERROR_OUT_OF_MEMORY);
    rv = tempargv->AppendElement(aEvent, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    iargv = do_QueryInterface(tempargv);
  }

  
  
  
  nsCOMPtr<nsIVariant> vrv;
  rv = mContext->CallEventHandler(mTarget, mScopeObject, funcval, iargv,
                                  getter_AddRefs(vrv));

  if (NS_SUCCEEDED(rv)) {
    PRUint16 dataType = nsIDataType::VTYPE_VOID;
    if (vrv)
      vrv->GetDataType(&dataType);
    if (eventString.EqualsLiteral("onbeforeunload")) {
      nsCOMPtr<nsIPrivateDOMEvent> priv(do_QueryInterface(aEvent));
      NS_ENSURE_TRUE(priv, NS_ERROR_UNEXPECTED);

      nsEvent* event;
      priv->GetInternalNSEvent(&event);
      NS_ENSURE_TRUE(event && event->message == NS_BEFORE_PAGE_UNLOAD,
                     NS_ERROR_UNEXPECTED);

      nsBeforePageUnloadEvent *beforeUnload =
        NS_STATIC_CAST(nsBeforePageUnloadEvent *, event);

      if (dataType != nsIDataType::VTYPE_VOID) {
        aEvent->PreventDefault();

        
        
        
        if ((dataType == nsIDataType::VTYPE_DOMSTRING ||
             dataType == nsIDataType::VTYPE_CHAR_STR ||
             dataType == nsIDataType::VTYPE_WCHAR_STR ||
             dataType == nsIDataType::VTYPE_STRING_SIZE_IS ||
             dataType == nsIDataType::VTYPE_WSTRING_SIZE_IS ||
             dataType == nsIDataType::VTYPE_CSTRING ||
             dataType == nsIDataType::VTYPE_ASTRING)
            && beforeUnload->text.IsEmpty()) {
          vrv->GetAsDOMString(beforeUnload->text);
        }
      }
    } else if (dataType == nsIDataType::VTYPE_BOOL) {
      
      
      
      PRBool brv;
      if (NS_SUCCEEDED(vrv->GetAsBool(&brv)) &&
          brv == (mReturnResult == nsReturnResult_eReverseReturnResult)) {
        aEvent->PreventDefault();
      }
    }
  }

  return rv;
}





nsresult
NS_NewJSEventListener(nsIScriptContext *aContext, void *aScopeObject,
                      nsISupports*aTarget, nsIDOMEventListener ** aReturn)
{
  nsJSEventListener* it =
    new nsJSEventListener(aContext, aScopeObject, aTarget);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aReturn = it);

  return NS_OK;
}
