




































#include "nsISupports.h"
#include "nsGUIEvent.h"
#include "nsDOMEvent.h"
#include "nsEventListenerManager.h"
#include "nsCaret.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMContextMenuListener.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMFocusListener.h"
#include "nsIDOMFormListener.h"
#include "nsIDOMLoadListener.h"
#include "nsIDOMTextListener.h"
#include "nsIDOMCompositionListener.h"
#include "nsIDOMUIListener.h"
#include "nsITextControlFrame.h"
#ifdef MOZ_SVG
#include "nsGkAtoms.h"
#endif 
#include "nsIEventStateManager.h"
#include "nsPIDOMWindow.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIJSEventListener.h"
#include "prmem.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptRuntime.h"
#include "nsLayoutUtils.h"
#include "nsINameSpaceManager.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMError.h"
#include "nsIJSContextStack.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsMutationEvent.h"
#include "nsIXPConnect.h"
#include "nsDOMCID.h"
#include "nsIScriptObjectOwner.h" 
#include "nsFocusManager.h"
#include "nsIDOMElement.h"
#include "nsIDOMNSDocument.h"
#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "nsIDOMEventGroup.h"
#include "nsContentCID.h"
#include "nsEventDispatcher.h"
#include "nsDOMJSUtils.h"
#include "nsDOMScriptObjectHolder.h"
#include "nsDataHashtable.h"
#include "nsCOMArray.h"
#include "nsEventListenerService.h"
#include "nsDOMEvent.h"

#define EVENT_TYPE_EQUALS( ls, type, userType ) \
  (ls->mEventType && ls->mEventType == type && \
  (ls->mEventType != NS_USER_DEFINED_EVENT || ls->mTypeAtom == userType))

#define EVENT_TYPE_DATA_EQUALS( type1, type2 ) \
  (type1 && type2 && type1->iid && type2->iid && \
   type1->iid->Equals(*(type2->iid)))

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID,
                     NS_DOM_SCRIPT_OBJECT_FACTORY_CID);
static NS_DEFINE_CID(kDOMEventGroupCID, NS_DOMEVENTGROUP_CID);

static const PRUint32 kAllMutationBits =
  NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED |
  NS_EVENT_BITS_MUTATION_NODEINSERTED |
  NS_EVENT_BITS_MUTATION_NODEREMOVED |
  NS_EVENT_BITS_MUTATION_NODEREMOVEDFROMDOCUMENT |
  NS_EVENT_BITS_MUTATION_NODEINSERTEDINTODOCUMENT |
  NS_EVENT_BITS_MUTATION_ATTRMODIFIED |
  NS_EVENT_BITS_MUTATION_CHARACTERDATAMODIFIED;

static PRUint32
MutationBitForEventType(PRUint32 aEventType)
{
  switch (aEventType) {
    case NS_MUTATION_SUBTREEMODIFIED:
      return NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED;
    case NS_MUTATION_NODEINSERTED:
      return NS_EVENT_BITS_MUTATION_NODEINSERTED;
    case NS_MUTATION_NODEREMOVED:
      return NS_EVENT_BITS_MUTATION_NODEREMOVED;
    case NS_MUTATION_NODEREMOVEDFROMDOCUMENT:
      return NS_EVENT_BITS_MUTATION_NODEREMOVEDFROMDOCUMENT;
    case NS_MUTATION_NODEINSERTEDINTODOCUMENT:
      return NS_EVENT_BITS_MUTATION_NODEINSERTEDINTODOCUMENT;
    case NS_MUTATION_ATTRMODIFIED:
      return NS_EVENT_BITS_MUTATION_ATTRMODIFIED;
    case NS_MUTATION_CHARACTERDATAMODIFIED:
      return NS_EVENT_BITS_MUTATION_CHARACTERDATAMODIFIED;
    default:
      break;
  }
  return 0;
}

typedef
NS_STDCALL_FUNCPROTO(nsresult,
                     GenericHandler,
                     nsIDOMEventListener, HandleEvent, 
                     (nsIDOMEvent*));












static nsresult DispatchToInterface(nsIDOMEvent* aEvent,
                                    nsIDOMEventListener* aListener,
                                    GenericHandler aMethod,
                                    const nsIID& aIID)
{
  nsIDOMEventListener* ifaceListener = nsnull;
  nsresult rv = NS_OK;
  aListener->QueryInterface(aIID, (void**) &ifaceListener);
  NS_WARN_IF_FALSE(ifaceListener,
                   "DispatchToInterface couldn't QI to the right interface");
  if (ifaceListener) {
    rv = (ifaceListener->*aMethod)(aEvent);
    NS_RELEASE(ifaceListener);
  }
  return rv;
}

struct EventDispatchData
{
  PRUint32 message;
  GenericHandler method;
};

struct EventTypeData
{
  const EventDispatchData* events;
  int                      numEvents;
  const nsIID*             iid;
};

#define HANDLER(x) reinterpret_cast<GenericHandler>(x)

static const EventDispatchData sMouseEvents[] = {
  { NS_MOUSE_BUTTON_DOWN,        HANDLER(&nsIDOMMouseListener::MouseDown)     },
  { NS_MOUSE_BUTTON_UP,          HANDLER(&nsIDOMMouseListener::MouseUp)       },
  { NS_MOUSE_CLICK,              HANDLER(&nsIDOMMouseListener::MouseClick)    },
  { NS_MOUSE_DOUBLECLICK,        HANDLER(&nsIDOMMouseListener::MouseDblClick) },
  { NS_MOUSE_ENTER_SYNTH,        HANDLER(&nsIDOMMouseListener::MouseOver)     },
  { NS_MOUSE_EXIT_SYNTH,         HANDLER(&nsIDOMMouseListener::MouseOut)      }
};

static const EventDispatchData sMouseMotionEvents[] = {
  { NS_MOUSE_MOVE, HANDLER(&nsIDOMMouseMotionListener::MouseMove) }
};

static const EventDispatchData sContextMenuEvents[] = {
  { NS_CONTEXTMENU, HANDLER(&nsIDOMContextMenuListener::ContextMenu) }
};

static const EventDispatchData sCompositionEvents[] = {
  { NS_COMPOSITION_START,
    HANDLER(&nsIDOMCompositionListener::HandleStartComposition)  },
  { NS_COMPOSITION_END,
    HANDLER(&nsIDOMCompositionListener::HandleEndComposition)    },
  { NS_COMPOSITION_QUERY,
    HANDLER(&nsIDOMCompositionListener::HandleQueryComposition)  }
};

static const EventDispatchData sTextEvents[] = {
  { NS_TEXT_TEXT, HANDLER(&nsIDOMTextListener::HandleText) }
};

static const EventDispatchData sKeyEvents[] = {
  { NS_KEY_UP,    HANDLER(&nsIDOMKeyListener::KeyUp)    },
  { NS_KEY_DOWN,  HANDLER(&nsIDOMKeyListener::KeyDown)  },
  { NS_KEY_PRESS, HANDLER(&nsIDOMKeyListener::KeyPress) }
};

static const EventDispatchData sFocusEvents[] = {
  { NS_FOCUS_CONTENT, HANDLER(&nsIDOMFocusListener::Focus) },
  { NS_BLUR_CONTENT,  HANDLER(&nsIDOMFocusListener::Blur)  }
};

static const EventDispatchData sFormEvents[] = {
  { NS_FORM_SUBMIT,   HANDLER(&nsIDOMFormListener::Submit) },
  { NS_FORM_RESET,    HANDLER(&nsIDOMFormListener::Reset)  },
  { NS_FORM_CHANGE,   HANDLER(&nsIDOMFormListener::Change) },
  { NS_FORM_SELECTED, HANDLER(&nsIDOMFormListener::Select) },
  { NS_FORM_INPUT,    HANDLER(&nsIDOMFormListener::Input)  }
};

static const EventDispatchData sLoadEvents[] = {
  { NS_LOAD,               HANDLER(&nsIDOMLoadListener::Load)         },
  { NS_PAGE_UNLOAD,        HANDLER(&nsIDOMLoadListener::Unload)       },
  { NS_LOAD_ERROR,         HANDLER(&nsIDOMLoadListener::Error)        },
  { NS_BEFORE_PAGE_UNLOAD, HANDLER(&nsIDOMLoadListener::BeforeUnload) }
};

static const EventDispatchData sUIEvents[] = {
  { NS_UI_ACTIVATE, HANDLER(&nsIDOMUIListener::Activate) },
  { NS_UI_FOCUSIN,  HANDLER(&nsIDOMUIListener::FocusIn)  },
  { NS_UI_FOCUSOUT, HANDLER(&nsIDOMUIListener::FocusOut) }
};

#define IMPL_EVENTTYPEDATA(type) \
{ \
  s##type##Events, \
  NS_ARRAY_LENGTH(s##type##Events), \
  &NS_GET_IID(nsIDOM##type##Listener) \
}
 


static const EventTypeData sEventTypes[] = {
  IMPL_EVENTTYPEDATA(Mouse),
  IMPL_EVENTTYPEDATA(MouseMotion),
  IMPL_EVENTTYPEDATA(ContextMenu),
  IMPL_EVENTTYPEDATA(Key),
  IMPL_EVENTTYPEDATA(Load),
  IMPL_EVENTTYPEDATA(Focus),
  IMPL_EVENTTYPEDATA(Form),
  IMPL_EVENTTYPEDATA(Text),
  IMPL_EVENTTYPEDATA(Composition),
  IMPL_EVENTTYPEDATA(UI)
};


nsIDOMEventGroup* gSystemEventGroup = nsnull;
nsIDOMEventGroup* gDOM2EventGroup = nsnull;

PRUint32 nsEventListenerManager::mInstanceCount = 0;
PRUint32 nsEventListenerManager::sCreatedCount = 0;

nsEventListenerManager::nsEventListenerManager() :
  mTarget(nsnull)
{
  ++mInstanceCount;
  ++sCreatedCount;
}

nsEventListenerManager::~nsEventListenerManager() 
{
  NS_ASSERTION(!mTarget, "didn't call Disconnect");
  RemoveAllListeners();

  --mInstanceCount;
  if(mInstanceCount == 0) {
    NS_IF_RELEASE(gSystemEventGroup);
    NS_IF_RELEASE(gDOM2EventGroup);
  }
}

nsresult
nsEventListenerManager::RemoveAllListeners()
{
  mListeners.Clear();
  return NS_OK;
}

void
nsEventListenerManager::Shutdown()
{
  sAddListenerID = JSVAL_VOID;
  nsDOMEvent::Shutdown();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsEventListenerManager)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsEventListenerManager)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEventListenerManager)
   NS_INTERFACE_MAP_ENTRY(nsIEventListenerManager)
   NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
   NS_INTERFACE_MAP_ENTRY(nsIDOM3EventTarget)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsEventListenerManager, nsIEventListenerManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsEventListenerManager, nsIEventListenerManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsEventListenerManager)
  PRUint32 count = tmp->mListeners.Length();
  for (PRUint32 i = 0; i < count; i++) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mListeners[i] mListener");
    cb.NoteXPCOMChild(tmp->mListeners.ElementAt(i).mListener.get());
  }  
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsEventListenerManager)
  tmp->Disconnect();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


const EventTypeData*
nsEventListenerManager::GetTypeDataForIID(const nsIID& aIID)
{
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(sEventTypes); ++i) {
    if (aIID.Equals(*(sEventTypes[i].iid))) {
      return &sEventTypes[i];
    }
  }
  return nsnull;
}

const EventTypeData*
nsEventListenerManager::GetTypeDataForEventName(nsIAtom* aName)
{
  PRUint32 event = nsContentUtils::GetEventId(aName);
  if (event != NS_USER_DEFINED_EVENT) {
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(sEventTypes); ++i) {
      for (PRInt32 j = 0; j < sEventTypes[i].numEvents; ++j) {
         if (event == sEventTypes[i].events[j].message) {
           return &sEventTypes[i];
         }
      }
    }
  }
  return nsnull;
}

nsPIDOMWindow*
nsEventListenerManager::GetInnerWindowForTarget()
{
  nsCOMPtr<nsINode> node = do_QueryInterface(mTarget);
  if (node) {
    
    
    nsIDocument* document = node->GetOwnerDoc();
    if (document)
      return document->GetInnerWindow();
  }

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mTarget);
  if (window) {
    NS_ASSERTION(window->IsInnerWindow(), "Target should not be an outer window");
    return window;
  }

  return nsnull;
}

nsresult
nsEventListenerManager::AddEventListener(nsIDOMEventListener *aListener,
                                         PRUint32 aType,
                                         nsIAtom* aTypeAtom,
                                         const EventTypeData* aTypeData,
                                         PRInt32 aFlags,
                                         nsIDOMEventGroup* aEvtGrp)
{
  NS_ENSURE_TRUE(aListener, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(aType || aTypeData, NS_ERROR_FAILURE);

  nsRefPtr<nsIDOMEventListener> kungFuDeathGrip = aListener;

  PRBool isSame = PR_FALSE;
  PRUint16 group = 0;
  nsCOMPtr<nsIDOMEventGroup> sysGroup;
  GetSystemEventGroupLM(getter_AddRefs(sysGroup));
  if (sysGroup) {
    sysGroup->IsSameEventGroup(aEvtGrp, &isSame);
    if (isSame) {
      group = NS_EVENT_FLAG_SYSTEM_EVENT;
      mMayHaveSystemGroupListeners = PR_TRUE;
    }
  }

  if (!aTypeData) {
    
    
    
    
    
    const EventTypeData* td = GetTypeDataForEventName(aTypeAtom);
    if (td && td->iid) {
      nsIDOMEventListener* ifaceListener = nsnull;
      aListener->QueryInterface(*(td->iid), (void**) &ifaceListener);
      if (ifaceListener) {
        aTypeData = td;
        NS_RELEASE(ifaceListener);
      }
    }
  }

  nsListenerStruct* ls;
  PRUint32 count = mListeners.Length();
  for (PRUint32 i = 0; i < count; i++) {
    ls = &mListeners.ElementAt(i);
    if (ls->mListener == aListener && ls->mFlags == aFlags &&
        ls->mGroupFlags == group &&
        (EVENT_TYPE_EQUALS(ls, aType, aTypeAtom) ||
         EVENT_TYPE_DATA_EQUALS(aTypeData, ls->mTypeData))) {
      return NS_OK;
    }
  }

  mNoListenerForEvent = NS_EVENT_TYPE_NULL;
  mNoListenerForEventAtom = nsnull;

  ls = mListeners.AppendElement();
  NS_ENSURE_TRUE(ls, NS_ERROR_OUT_OF_MEMORY);

  ls->mListener = aListener;
  ls->mEventType = aType;
  ls->mTypeAtom = aTypeAtom;
  ls->mFlags = aFlags;
  ls->mGroupFlags = group;
  ls->mHandlerIsString = PR_FALSE;
  ls->mTypeData = aTypeData;
  if (aFlags & NS_EVENT_FLAG_CAPTURE) {
    mMayHaveCapturingListeners = PR_TRUE;
  }

  if (aType == NS_AFTERPAINT) {
    mMayHavePaintEventListener = PR_TRUE;
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (window) {
      window->SetHasPaintEventListeners();
    }
  } else if (aType >= NS_MUTATION_START && aType <= NS_MUTATION_END) {
    
    
    mMayHaveMutationListeners = PR_TRUE;
    
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (window) {
      
      
      window->SetMutationListeners((aType == NS_MUTATION_SUBTREEMODIFIED) ?
                                   kAllMutationBits :
                                   MutationBitForEventType(aType));
    }
  } else if (aTypeAtom == nsGkAtoms::onMozOrientation) {
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (window)
      window->SetHasOrientationEventListener();
  }

  return NS_OK;
}

nsresult
nsEventListenerManager::RemoveEventListener(nsIDOMEventListener *aListener, 
                                            PRUint32 aType,
                                            nsIAtom* aUserType,
                                            const EventTypeData* aTypeData,
                                            PRInt32 aFlags,
                                            nsIDOMEventGroup* aEvtGrp)
{
  if (!aListener || !(aType || aTypeData)) {
    return NS_OK;
  }

  PRBool isSame = PR_FALSE;
  PRUint16 group = 0;
  nsCOMPtr<nsIDOMEventGroup> sysGroup;
  GetSystemEventGroupLM(getter_AddRefs(sysGroup));
  if (sysGroup) {
    sysGroup->IsSameEventGroup(aEvtGrp, &isSame);
    if (isSame) {
      group = NS_EVENT_FLAG_SYSTEM_EVENT;
    }
  }

  nsListenerStruct* ls;
  aFlags &= ~NS_PRIV_EVENT_UNTRUSTED_PERMITTED;

  PRUint32 count = mListeners.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    ls = &mListeners.ElementAt(i);
    if (ls->mListener == aListener &&
        ls->mGroupFlags == group &&
        ((ls->mFlags & ~NS_PRIV_EVENT_UNTRUSTED_PERMITTED) == aFlags) &&
        (EVENT_TYPE_EQUALS(ls, aType, aUserType) ||
         (!(ls->mEventType) &&
          EVENT_TYPE_DATA_EQUALS(ls->mTypeData, aTypeData)))) {
      nsRefPtr<nsEventListenerManager> kungFuDeathGrip = this;
      mListeners.RemoveElementAt(i);
      mNoListenerForEvent = NS_EVENT_TYPE_NULL;
      mNoListenerForEventAtom = nsnull;
      break;
    }
  }

  return NS_OK;
}

nsresult
nsEventListenerManager::AddEventListenerByIID(nsIDOMEventListener *aListener, 
                                              const nsIID& aIID,
                                              PRInt32 aFlags)
{
  AddEventListener(aListener, NS_EVENT_TYPE_NULL, nsnull,
                   GetTypeDataForIID(aIID), aFlags, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::RemoveEventListenerByIID(nsIDOMEventListener *aListener, 
                                                 const nsIID& aIID,
                                                 PRInt32 aFlags)
{
  RemoveEventListener(aListener, NS_EVENT_TYPE_NULL, nsnull,
                      GetTypeDataForIID(aIID), aFlags, nsnull);
  return NS_OK;
}

PRBool
nsEventListenerManager::ListenerCanHandle(nsListenerStruct* aLs,
                                          nsEvent* aEvent)
{
  if (aEvent->message == NS_USER_DEFINED_EVENT) {
    
    return (aEvent->userType && aLs->mTypeAtom == aEvent->userType);
  }
  return (aLs->mEventType == aEvent->message);
}

NS_IMETHODIMP
nsEventListenerManager::AddEventListenerByType(nsIDOMEventListener *aListener, 
                                               const nsAString& aType,
                                               PRInt32 aFlags,
                                               nsIDOMEventGroup* aEvtGrp)
{
  nsCOMPtr<nsIAtom> atom = do_GetAtom(NS_LITERAL_STRING("on") + aType);
  PRUint32 type = nsContentUtils::GetEventId(atom);
  AddEventListener(aListener, type, atom, nsnull, aFlags, aEvtGrp);
  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::RemoveEventListenerByType(nsIDOMEventListener *aListener, 
                                                  const nsAString& aType,
                                                  PRInt32 aFlags,
                                                  nsIDOMEventGroup* aEvtGrp)
{
  nsCOMPtr<nsIAtom> atom = do_GetAtom(NS_LITERAL_STRING("on") + aType);
  PRUint32 type = nsContentUtils::GetEventId(atom);
  RemoveEventListener(aListener, type, atom, nsnull, aFlags, aEvtGrp);
  return NS_OK;
}

nsListenerStruct*
nsEventListenerManager::FindJSEventListener(PRUint32 aEventType,
                                            nsIAtom* aTypeAtom)
{
  
  
  nsListenerStruct *ls;
  PRUint32 count = mListeners.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    ls = &mListeners.ElementAt(i);
    if (EVENT_TYPE_EQUALS(ls, aEventType, aTypeAtom) &&
        ls->mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) {
      return ls;
    }
  }
  return nsnull;
}

nsresult
nsEventListenerManager::SetJSEventListener(nsIScriptContext *aContext,
                                           void *aScopeObject,
                                           nsISupports *aObject,
                                           nsIAtom* aName,
                                           PRBool aIsString,
                                           PRBool aPermitUntrustedEvents)
{
  nsresult rv = NS_OK;
  PRUint32 eventType = nsContentUtils::GetEventId(aName);
  nsListenerStruct* ls = FindJSEventListener(eventType, aName);

  if (!ls) {
    
    
    nsCOMPtr<nsIDOMEventListener> scriptListener;
    rv = NS_NewJSEventListener(aContext, aScopeObject, aObject, aName,
                               getter_AddRefs(scriptListener));
    if (NS_SUCCEEDED(rv)) {
      AddEventListener(scriptListener, eventType, aName, nsnull,
                       NS_EVENT_FLAG_BUBBLE | NS_PRIV_EVENT_FLAG_SCRIPT, nsnull);

      ls = FindJSEventListener(eventType, aName);
    }
  }

  if (NS_SUCCEEDED(rv) && ls) {
    
    ls->mHandlerIsString = aIsString;

    if (aPermitUntrustedEvents) {
      ls->mFlags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
    }
  }

  return rv;
}

NS_IMETHODIMP
nsEventListenerManager::AddScriptEventListener(nsISupports *aObject,
                                               nsIAtom *aName,
                                               const nsAString& aBody,
                                               PRUint32 aLanguage,
                                               PRBool aDeferCompilation,
                                               PRBool aPermitUntrustedEvents)
{
  NS_PRECONDITION(aLanguage != nsIProgrammingLanguage::UNKNOWN,
                  "Must know the language for the script event listener");
  nsIScriptContext *context = nsnull;

  
  
  
  
  
  if (aPermitUntrustedEvents && 
      aLanguage != nsIProgrammingLanguage::JAVASCRIPT) {
    NS_WARNING("Discarding non-JS event listener from untrusted source");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsINode> node(do_QueryInterface(aObject));

  nsCOMPtr<nsIDocument> doc;

  nsISupports *objiSupp = aObject;
  nsCOMPtr<nsIScriptGlobalObject> global;

  if (node) {
    
    
    
    doc = node->GetOwnerDoc();
    if (doc)
      global = doc->GetScriptGlobalObject();
  } else {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aObject));
    if (win) {
      NS_ASSERTION(win->IsInnerWindow(),
                   "Event listener added to outer window!");

      nsCOMPtr<nsIDOMDocument> domdoc;
      win->GetDocument(getter_AddRefs(domdoc));
      doc = do_QueryInterface(domdoc);
      global = do_QueryInterface(win);
    } else {
      global = do_QueryInterface(aObject);
    }
  }

  if (!global) {
    
    
    return NS_OK;
  }
  
  
  
  if (NS_FAILED(global->EnsureScriptEnvironment(aLanguage))) {
    NS_WARNING("Failed to setup script environment for this language");
    
  }

  context = global->GetScriptContext(aLanguage);
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);

  void *scope = global->GetScriptGlobal(aLanguage);
  nsresult rv;

  if (!aDeferCompilation) {
    nsCOMPtr<nsIScriptEventHandlerOwner> handlerOwner =
      do_QueryInterface(aObject);

    nsScriptObjectHolder handler(context);
    PRBool done = PR_FALSE;

    if (handlerOwner) {
      rv = handlerOwner->GetCompiledEventHandler(aName, handler);
      if (NS_SUCCEEDED(rv) && handler) {
        rv = context->BindCompiledEventHandler(aObject, scope, aName, handler);
        if (NS_FAILED(rv))
          return rv;
        done = PR_TRUE;
      }
    }

    if (!done) {
      PRUint32 lineNo = 0;
      nsCAutoString url (NS_LITERAL_CSTRING("-moz-evil:lying-event-listener"));
      if (doc) {
        nsIURI *uri = doc->GetDocumentURI();
        if (uri) {
          uri->GetSpec(url);
          lineNo = 1;
        }
      }

      if (handlerOwner) {
        
        
        rv = handlerOwner->CompileEventHandler(context, aObject, aName,
                                               aBody, url.get(), lineNo, handler);
      }
      else {
        PRInt32 nameSpace = kNameSpaceID_Unknown;
        if (node && node->IsNodeOfType(nsINode::eCONTENT)) {
          nsIContent* content = static_cast<nsIContent*>(node.get());
          nameSpace = content->GetNameSpaceID();
        }
        else if (doc) {
          nsCOMPtr<nsIContent> root = doc->GetRootContent();
          if (root)
            nameSpace = root->GetNameSpaceID();
        }
        PRUint32 argCount;
        const char **argNames;
        nsContentUtils::GetEventArgNames(nameSpace, aName, &argCount,
                                         &argNames);

        nsCxPusher pusher;
        if (!pusher.Push((JSContext*)context->GetNativeContext())) {
          return NS_ERROR_FAILURE;
        }

        rv = context->CompileEventHandler(aName, argCount, argNames,
                                          aBody,
                                          url.get(), lineNo,
                                          SCRIPTVERSION_DEFAULT, 
                                          handler);
        if (rv == NS_ERROR_ILLEGAL_VALUE) {
          NS_WARNING("Probably a syntax error in the event handler!");
          return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
        }
        NS_ENSURE_SUCCESS(rv, rv);
        
        rv = context->BindCompiledEventHandler(aObject, scope,
                                               aName, handler);
      }
      if (NS_FAILED(rv)) return rv;
    }
  }

  return SetJSEventListener(context, scope, objiSupp, aName, aDeferCompilation,
                            aPermitUntrustedEvents);
}

nsresult
nsEventListenerManager::RemoveScriptEventListener(nsIAtom* aName)
{
  PRUint32 eventType = nsContentUtils::GetEventId(aName);
  nsListenerStruct* ls = FindJSEventListener(eventType, aName);

  if (ls) {
    mListeners.RemoveElementAt(PRUint32(ls - &mListeners.ElementAt(0)));
    mNoListenerForEvent = NS_EVENT_TYPE_NULL;
    mNoListenerForEventAtom = nsnull;
  }

  return NS_OK;
}

jsval
nsEventListenerManager::sAddListenerID = JSVAL_VOID;

NS_IMETHODIMP
nsEventListenerManager::RegisterScriptEventListener(nsIScriptContext *aContext,
                                                    void *aScope,
                                                    nsISupports *aObject, 
                                                    nsIAtom *aName)
{
  
  
  
  
  
  
  nsresult rv;
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv))
    return rv;
  JSContext *cx;
  if (NS_FAILED(rv = stack->Peek(&cx)))
    return rv;

  if (cx) {
    if (sAddListenerID == JSVAL_VOID) {
      JSAutoRequest ar(cx);
      sAddListenerID =
        STRING_TO_JSVAL(::JS_InternString(cx, "addEventListener"));
    }

    if (aContext->GetScriptTypeID() == nsIProgrammingLanguage::JAVASCRIPT) {
        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        jsval v;
        rv = nsContentUtils::WrapNative(cx, (JSObject *)aScope, aObject, &v,
                                        getter_AddRefs(holder));
        NS_ENSURE_SUCCESS(rv, rv);
      
        rv = nsContentUtils::GetSecurityManager()->
          CheckPropertyAccess(cx, JSVAL_TO_OBJECT(v),
                              "EventTarget",
                              sAddListenerID,
                              nsIXPCSecurityManager::ACCESS_SET_PROPERTY);
        if (NS_FAILED(rv)) {
          
          return rv;
        }
    } else {
        NS_WARNING("Skipping CheckPropertyAccess for non JS language");
    }
        
  }

  
  
  return SetJSEventListener(aContext, aScope, aObject, aName,
                            PR_FALSE, !nsContentUtils::IsCallerChrome());
}

nsresult
nsEventListenerManager::CompileScriptEventListener(nsIScriptContext *aContext, 
                                                   void *aScope,
                                                   nsISupports *aObject, 
                                                   nsIAtom *aName,
                                                   PRBool *aDidCompile)
{
  nsresult rv = NS_OK;
  *aDidCompile = PR_FALSE;
  PRUint32 eventType = nsContentUtils::GetEventId(aName);
  nsListenerStruct* ls = FindJSEventListener(eventType, aName);

  if (!ls) {
    
    return NS_OK;
  }

  if (ls->mHandlerIsString) {
    rv = CompileEventHandlerInternal(aContext, aScope, aObject, aName,
                                     ls, nsnull, PR_TRUE);
  }

  
  
  
  

  *aDidCompile = PR_TRUE;

  return rv;
}

nsresult
nsEventListenerManager::CompileEventHandlerInternal(nsIScriptContext *aContext,
                                                    void *aScope,
                                                    nsISupports *aObject,
                                                    nsIAtom *aName,
                                                    nsListenerStruct *aListenerStruct,
                                                    nsISupports* aCurrentTarget,
                                                    PRBool aNeedsCxPush)
{
  nsresult result = NS_OK;

  nsCOMPtr<nsIScriptEventHandlerOwner> handlerOwner =
    do_QueryInterface(aObject);
  nsScriptObjectHolder handler(aContext);

  if (handlerOwner) {
    result = handlerOwner->GetCompiledEventHandler(aName,
                                                   handler);
    if (NS_SUCCEEDED(result) && handler) {
      
      result = aContext->BindCompiledEventHandler(aObject, aScope, aName, handler);
      aListenerStruct->mHandlerIsString = PR_FALSE;
    }
  }

  if (aListenerStruct->mHandlerIsString) {
    
    
    
    
    nsCOMPtr<nsIContent> content = do_QueryInterface(aObject);
    NS_ASSERTION(content, "only content should have event handler attributes");
    if (content) {
      nsAutoString handlerBody;
      nsIAtom* attrName = aName;
#ifdef MOZ_SVG
      if (aName == nsGkAtoms::onSVGLoad)
        attrName = nsGkAtoms::onload;
      else if (aName == nsGkAtoms::onSVGUnload)
        attrName = nsGkAtoms::onunload;
      else if (aName == nsGkAtoms::onSVGAbort)
        attrName = nsGkAtoms::onabort;
      else if (aName == nsGkAtoms::onSVGError)
        attrName = nsGkAtoms::onerror;
      else if (aName == nsGkAtoms::onSVGResize)
        attrName = nsGkAtoms::onresize;
      else if (aName == nsGkAtoms::onSVGScroll)
        attrName = nsGkAtoms::onscroll;
      else if (aName == nsGkAtoms::onSVGZoom)
        attrName = nsGkAtoms::onzoom;
#endif 

      content->GetAttr(kNameSpaceID_None, attrName, handlerBody);

      PRUint32 lineNo = 0;
      nsCAutoString url (NS_LITERAL_CSTRING("javascript:alert('TODO: FIXME')"));
      nsIDocument* doc = nsnull;
      nsCOMPtr<nsINode> node = do_QueryInterface(aCurrentTarget);
      if (node) {
        doc = node->GetOwnerDoc();
      }
      if (doc) {
        nsIURI *uri = doc->GetDocumentURI();
        if (uri) {
          uri->GetSpec(url);
          lineNo = 1;
        }
      }

      nsCxPusher pusher;
      if (aNeedsCxPush &&
          !pusher.Push((JSContext*)aContext->GetNativeContext())) {
        return NS_ERROR_FAILURE;
      }


      if (handlerOwner) {
        
        
        
        result = handlerOwner->CompileEventHandler(aContext, aObject, aName,
                                                   handlerBody,
                                                   url.get(), lineNo,
                                                   handler);
      }
      else {
        PRUint32 argCount;
        const char **argNames;
        nsContentUtils::GetEventArgNames(content->GetNameSpaceID(), aName,
                                         &argCount, &argNames);

        result = aContext->CompileEventHandler(aName,
                                               argCount, argNames,
                                               handlerBody,
                                               url.get(), lineNo,
                                               SCRIPTVERSION_DEFAULT, 
                                               handler);
        NS_ENSURE_SUCCESS(result, result);
        
        result = aContext->BindCompiledEventHandler(aObject, aScope,
                                                    aName, handler);
        NS_ENSURE_SUCCESS(result, result);
      }

      if (NS_SUCCEEDED(result)) {
        aListenerStruct->mHandlerIsString = PR_FALSE;
      }
    }
  }

  return result;
}

nsresult
nsEventListenerManager::HandleEventSubType(nsListenerStruct* aListenerStruct,
                                           nsIDOMEventListener* aListener,
                                           nsIDOMEvent* aDOMEvent,
                                           nsPIDOMEventTarget* aCurrentTarget,
                                           PRUint32 aPhaseFlags,
                                           nsCxPusher* aPusher)
{
  nsresult result = NS_OK;

  
  
  if ((aListenerStruct->mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) &&
      aListenerStruct->mHandlerIsString) {
    nsCOMPtr<nsIJSEventListener> jslistener = do_QueryInterface(aListener);
    if (jslistener) {
      
      nsCOMPtr<nsIAtom> atom = aListenerStruct->mTypeAtom;
      if (!atom) {
        nsAutoString eventString;
        if (NS_SUCCEEDED(aDOMEvent->GetType(eventString))) {
          atom = do_GetAtom(NS_LITERAL_STRING("on") + eventString);
        }
      }

      if (atom) {
#ifdef DEBUG
        nsAutoString type;
        aDOMEvent->GetType(type);
        nsCOMPtr<nsIAtom> eventAtom = do_GetAtom(NS_LITERAL_STRING("on") + type);
        NS_ASSERTION(eventAtom == atom, "Something wrong with event atoms!");
#endif
        result = CompileEventHandlerInternal(jslistener->GetEventContext(),
                                             jslistener->GetEventScope(),
                                             jslistener->GetEventTarget(),
                                             atom, aListenerStruct,
                                             aCurrentTarget,
                                             !jslistener->GetEventContext() ||
                                             jslistener->GetEventContext() !=
                                             aPusher->GetCurrentScriptContext());
      }
    }
  }

  if (NS_SUCCEEDED(result)) {
    
    result = aListener->HandleEvent(aDOMEvent);
  }

  return result;
}

static PRUint32                 sLatestEventType = 0;
static const EventTypeData*     sLatestEventTypeData = nsnull;
static const EventDispatchData* sLatestEventDispData = nsnull;






nsresult
nsEventListenerManager::HandleEvent(nsPresContext* aPresContext,
                                    nsEvent* aEvent, nsIDOMEvent** aDOMEvent,
                                    nsPIDOMEventTarget* aCurrentTarget,
                                    PRUint32 aFlags,
                                    nsEventStatus* aEventStatus,
                                    nsCxPusher* aPusher)
{
  if (mListeners.IsEmpty() || aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) {
    return NS_OK;
  }

  if (!mMayHaveCapturingListeners &&
      !(aEvent->flags & NS_EVENT_FLAG_BUBBLE)) {
    return NS_OK;
  }
  
  if (!mMayHaveSystemGroupListeners &&
      aFlags & NS_EVENT_FLAG_SYSTEM_EVENT) {
    return NS_OK;
  }

  
  if (mNoListenerForEvent == aEvent->message &&
      (mNoListenerForEvent != NS_USER_DEFINED_EVENT ||
       mNoListenerForEventAtom == aEvent->userType)) {
    return NS_OK;
  }

  
  if (*aEventStatus == nsEventStatus_eConsumeNoDefault) {
    aEvent->flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }
  PRUint16 currentGroup = aFlags & NS_EVENT_FLAG_SYSTEM_EVENT;

  const EventTypeData* typeData = nsnull;
  const EventDispatchData* dispData = nsnull;
  if (aEvent->message != NS_USER_DEFINED_EVENT) {
    
    
    if (aEvent->message == sLatestEventType) {
      typeData = sLatestEventTypeData;
      dispData = sLatestEventDispData;
      goto found;
    }
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(sEventTypes); ++i) {
     typeData = &sEventTypes[i];
     for (PRInt32 j = 0; j < typeData->numEvents; ++j) {
       dispData = &(typeData->events[j]);
       if (aEvent->message == dispData->message) {
         sLatestEventType = aEvent->message;
         sLatestEventTypeData = typeData;
         sLatestEventDispData = dispData;
         goto found;
       }
     }
     typeData = nsnull;
     dispData = nsnull;
    }
  }

found:

  nsAutoTObserverArray<nsListenerStruct, 2>::EndLimitedIterator iter(mListeners);
  nsAutoPopupStatePusher popupStatePusher(nsDOMEvent::GetEventPopupControlState(aEvent));
  PRBool hasListener = PR_FALSE;
  while (iter.HasMore()) {
    nsListenerStruct* ls = &iter.GetNext();
    PRBool useTypeInterface =
      EVENT_TYPE_DATA_EQUALS(ls->mTypeData, typeData);
    PRBool useGenericInterface =
      (!useTypeInterface && ListenerCanHandle(ls, aEvent));
    
    
    
    if (useTypeInterface || useGenericInterface) {
      if (ls->mListener) {
        hasListener = PR_TRUE;
        if (ls->mFlags & aFlags &&
            ls->mGroupFlags == currentGroup &&
            (NS_IS_TRUSTED_EVENT(aEvent) ||
             ls->mFlags & NS_PRIV_EVENT_UNTRUSTED_PERMITTED)) {
          if (!*aDOMEvent) {
            nsEventDispatcher::CreateEvent(aPresContext, aEvent,
                                           EmptyString(), aDOMEvent);
          }
          if (*aDOMEvent) {
            if (!aEvent->currentTarget) {
              aEvent->currentTarget = aCurrentTarget->GetTargetForDOMEvent();
              if (!aEvent->currentTarget) {
                break;
              }
            }
            nsRefPtr<nsIDOMEventListener> kungFuDeathGrip = ls->mListener;
            if (useTypeInterface) {
              aPusher->Pop();
              DispatchToInterface(*aDOMEvent, ls->mListener,
                                  dispData->method, *typeData->iid);
            } else if (useGenericInterface &&
                       aPusher->RePush(aCurrentTarget)) {
              HandleEventSubType(ls, ls->mListener, *aDOMEvent,
                                 aCurrentTarget, aFlags, aPusher);
            }
          }
        }
      }
    }
  }

  aEvent->currentTarget = nsnull;

  if (!hasListener) {
    mNoListenerForEvent = aEvent->message;
    mNoListenerForEventAtom = aEvent->userType;
  }

  if (aEvent->flags & NS_EVENT_FLAG_NO_DEFAULT) {
    *aEventStatus = nsEventStatus_eConsumeNoDefault;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::Disconnect()
{
  mTarget = nsnull;
  return RemoveAllListeners();
}

NS_IMETHODIMP
nsEventListenerManager::SetListenerTarget(nsISupports* aTarget)
{
  NS_PRECONDITION(aTarget, "unexpected null pointer");

  
  mTarget = aTarget;
  return NS_OK;
}

NS_IMETHODIMP
nsEventListenerManager::GetSystemEventGroupLM(nsIDOMEventGroup **aGroup)
{
  if (!gSystemEventGroup) {
    nsresult result;
    nsCOMPtr<nsIDOMEventGroup> group(do_CreateInstance(kDOMEventGroupCID,&result));
    if (NS_FAILED(result))
      return result;

    gSystemEventGroup = group;
    NS_ADDREF(gSystemEventGroup);
  }

  *aGroup = gSystemEventGroup;
  NS_ADDREF(*aGroup);
  return NS_OK;
}

nsresult
nsEventListenerManager::GetDOM2EventGroup(nsIDOMEventGroup **aGroup)
{
  if (!gDOM2EventGroup) {
    nsresult result;
    nsCOMPtr<nsIDOMEventGroup> group(do_CreateInstance(kDOMEventGroupCID,&result));
    if (NS_FAILED(result))
      return result;

    gDOM2EventGroup = group;
    NS_ADDREF(gDOM2EventGroup);
  }

  *aGroup = gDOM2EventGroup;
  NS_ADDREF(*aGroup);
  return NS_OK;
}


NS_IMETHODIMP 
nsEventListenerManager::AddEventListener(const nsAString& aType, 
                                         nsIDOMEventListener* aListener, 
                                         PRBool aUseCapture)
{
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  nsresult rv = AddEventListenerByType(aListener, aType, flags, nsnull);
  NS_ASSERTION(NS_FAILED(rv) || HasListenersFor(aType), 
               "Adding event listener didn't work!");
  return rv;
}

NS_IMETHODIMP 
nsEventListenerManager::RemoveEventListener(const nsAString& aType, 
                                            nsIDOMEventListener* aListener, 
                                            PRBool aUseCapture)
{
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
  
  return RemoveEventListenerByType(aListener, aType, flags, nsnull);
}

NS_IMETHODIMP
nsEventListenerManager::DispatchEvent(nsIDOMEvent* aEvent, PRBool *_retval)
{
  nsCOMPtr<nsINode> targetNode(do_QueryInterface(mTarget));
  if (!targetNode) {
    
    return NS_ERROR_FAILURE;
  }
  
  
  
  nsCOMPtr<nsIDocument> document = targetNode->GetOwnerDoc();

  
  if (!document) {
    return NS_OK;
  }

  
  nsIPresShell *shell = document->GetPrimaryShell();
  nsCOMPtr<nsPresContext> context;
  if (shell) {
    context = shell->GetPresContext();
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv =
    nsEventDispatcher::DispatchDOMEvent(targetNode, nsnull, aEvent,
                                        context, &status);
  *_retval = (status != nsEventStatus_eConsumeNoDefault);
  return rv;
}


NS_IMETHODIMP 
nsEventListenerManager::AddGroupedEventListener(const nsAString& aType, 
                                                nsIDOMEventListener* aListener, 
                                                PRBool aUseCapture,
                                                nsIDOMEventGroup* aEvtGrp)
{
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  return AddEventListenerByType(aListener, aType, flags, aEvtGrp);
}

NS_IMETHODIMP 
nsEventListenerManager::RemoveGroupedEventListener(const nsAString& aType, 
                                            nsIDOMEventListener* aListener, 
                                            PRBool aUseCapture,
                                            nsIDOMEventGroup* aEvtGrp)
{
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
  
  return RemoveEventListenerByType(aListener, aType, flags, aEvtGrp);
}

NS_IMETHODIMP
nsEventListenerManager::CanTrigger(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEventListenerManager::IsRegisteredHere(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsEventListenerManager::HasMutationListeners(PRBool* aListener)
{
  *aListener = PR_FALSE;
  if (mMayHaveMutationListeners) {
    PRUint32 count = mListeners.Length();
    for (PRUint32 i = 0; i < count; ++i) {
      nsListenerStruct* ls = &mListeners.ElementAt(i);
      if (ls->mEventType >= NS_MUTATION_START &&
          ls->mEventType <= NS_MUTATION_END) {
        *aListener = PR_TRUE;
        break;
      }
    }
  }

  return NS_OK;
}

PRUint32
nsEventListenerManager::MutationListenerBits()
{
  PRUint32 bits = 0;
  if (mMayHaveMutationListeners) {
    PRUint32 count = mListeners.Length();
    for (PRUint32 i = 0; i < count; ++i) {
      nsListenerStruct* ls = &mListeners.ElementAt(i);
      if (ls->mEventType >= NS_MUTATION_START &&
          ls->mEventType <= NS_MUTATION_END) {
        if (ls->mEventType == NS_MUTATION_SUBTREEMODIFIED) {
          return kAllMutationBits;
        }
        bits |= MutationBitForEventType(ls->mEventType);
      }
    }
  }
  return bits;
}

PRBool
nsEventListenerManager::HasListenersFor(const nsAString& aEventName)
{
  nsCOMPtr<nsIAtom> atom = do_GetAtom(NS_LITERAL_STRING("on") + aEventName);
  PRUint32 type = nsContentUtils::GetEventId(atom);

  const EventTypeData* typeData = nsnull;
  const EventDispatchData* dispData = nsnull;
  if (type != NS_USER_DEFINED_EVENT) {
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(sEventTypes); ++i) {
     typeData = &sEventTypes[i];
     for (PRInt32 j = 0; j < typeData->numEvents; ++j) {
       dispData = &(typeData->events[j]);
       if (type == dispData->message) {
         goto found;
       }
     }
     typeData = nsnull;
     dispData = nsnull;
    }
  }
found:

  PRUint32 count = mListeners.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    nsListenerStruct* ls = &mListeners.ElementAt(i);
    if (ls->mTypeAtom == atom ||
        EVENT_TYPE_DATA_EQUALS(ls->mTypeData, typeData)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

PRBool
nsEventListenerManager::HasListeners()
{
  return !mListeners.IsEmpty();
}

nsresult
nsEventListenerManager::GetListenerInfo(nsCOMArray<nsIEventListenerInfo>* aList)
{
  nsCOMPtr<nsPIDOMEventTarget> target = do_QueryInterface(mTarget);
  NS_ENSURE_STATE(target);
  aList->Clear();
  PRUint32 count = mListeners.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    const nsListenerStruct& ls = mListeners.ElementAt(i);
    PRBool capturing = !!(ls.mFlags & NS_EVENT_FLAG_CAPTURE);
    PRBool systemGroup = !!(ls.mGroupFlags & NS_EVENT_FLAG_SYSTEM_EVENT);
    PRBool allowsUntrusted = !!(ls.mFlags & NS_PRIV_EVENT_UNTRUSTED_PERMITTED);
    
    
    if ((ls.mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) && ls.mHandlerIsString) {
      nsCOMPtr<nsIJSEventListener> jslistener = do_QueryInterface(ls.mListener);
      if (jslistener) {
        CompileEventHandlerInternal(jslistener->GetEventContext(),
                                    jslistener->GetEventScope(),
                                    jslistener->GetEventTarget(),
                                    ls.mTypeAtom,
                                    const_cast<nsListenerStruct*>(&ls),
                                    mTarget);
      }
    }
    if (ls.mTypeData) {
      
      for (PRInt32 j = 0; j < ls.mTypeData->numEvents; ++j) {
        const EventDispatchData* dispData = &(ls.mTypeData->events[j]);
        const char* eventName = nsDOMEvent::GetEventName(dispData->message);
        if (eventName) {
          NS_ConvertASCIItoUTF16 eventType(eventName);
          nsRefPtr<nsEventListenerInfo> info =
            new nsEventListenerInfo(eventType, ls.mListener, capturing,
                                    allowsUntrusted, systemGroup);
          NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);
          aList->AppendObject(info);
        }
      }
    } else if (ls.mEventType == NS_USER_DEFINED_EVENT) {
      
      if (ls.mTypeAtom) {
        nsAutoString atomName;
        ls.mTypeAtom->ToString(atomName);
        const nsDependentSubstring& eventType =
          Substring(atomName, 2, atomName.Length() - 2);
        nsRefPtr<nsEventListenerInfo> info =
          new nsEventListenerInfo(eventType, ls.mListener, capturing,
                                  allowsUntrusted, systemGroup);
        NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);
        aList->AppendObject(info);
      }
    } else {
      
      const char* eventName = nsDOMEvent::GetEventName(ls.mEventType);
      if (eventName) {
        NS_ConvertASCIItoUTF16 eventType(eventName);
        nsRefPtr<nsEventListenerInfo> info =
          new nsEventListenerInfo(eventType, ls.mListener, capturing,
                                  allowsUntrusted, systemGroup);
        NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);
        aList->AppendObject(info);
      }
    }
  }
  return NS_OK;
}

PRBool
nsEventListenerManager::HasUnloadListeners()
{
  PRUint32 count = mListeners.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    nsListenerStruct* ls = &mListeners.ElementAt(i);
    if (ls->mEventType == NS_PAGE_UNLOAD ||
        ls->mEventType == NS_BEFORE_PAGE_UNLOAD ||
        (ls->mTypeData && ls->mTypeData->iid &&
         ls->mTypeData->iid->Equals(NS_GET_IID(nsIDOMLoadListener)))) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

nsresult
NS_NewEventListenerManager(nsIEventListenerManager** aInstancePtrResult) 
{
  nsIEventListenerManager* l = new nsEventListenerManager();

  if (!l) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return CallQueryInterface(l, aInstancePtrResult);
}
