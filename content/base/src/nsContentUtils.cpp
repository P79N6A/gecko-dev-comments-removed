











































#include "jscntxt.h"

#include "nsJSUtils.h"
#include "nsCOMPtr.h"
#include "nsAString.h"
#include "nsPrintfCString.h"
#include "nsUnicharUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefLocalizedString.h"
#include "nsServiceManagerUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsDOMCID.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsINodeInfo.h"
#include "nsReadableUtils.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNode.h"
#include "nsIDOM3Node.h"
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMError.h"
#include "nsPIDOMWindow.h"
#include "nsIJSContextStack.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsParserCIID.h"
#include "nsIParser.h"
#include "nsIFragmentContentSink.h"
#include "nsIContentSink.h"
#include "nsIHTMLContentSink.h"
#include "nsIXMLContentSink.h"
#include "nsHTMLParts.h"
#include "nsIParserService.h"
#include "nsIServiceManager.h"
#include "nsIAttribute.h"
#include "nsContentList.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsGkAtoms.h"
#include "nsISupportsPrimitives.h"
#include "imgIDecoderObserver.h"
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "imgILoader.h"
#include "nsDocShellCID.h"
#include "nsIImageLoadingContent.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsILoadGroup.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsContentPolicyUtils.h"
#include "nsNodeInfoManager.h"
#include "nsIXBLService.h"
#include "nsCRT.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIPrivateDOMEvent.h"
#ifdef MOZ_XTF
#include "nsIXTFService.h"
static NS_DEFINE_CID(kXTFServiceCID, NS_XTFSERVICE_CID);
#endif
#include "nsIMIMEService.h"
#include "nsLWBrkCIID.h"
#include "nsILineBreaker.h"
#include "nsIWordBreaker.h"
#include "jsdbgapi.h"
#include "nsIJSRuntimeService.h"
#include "nsIDOMDocumentXBL.h"
#include "nsBindingManager.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsEscape.h"
#include "nsICharsetConverterManager.h"
#include "nsIEventListenerManager.h"
#include "nsAttrName.h"
#include "nsIDOMUserDataHandler.h"
#include "nsContentCreatorFunctions.h"
#include "nsTPtrArray.h"
#include "nsGUIEvent.h"
#include "nsMutationEvent.h"
#include "nsIMEStateManager.h"
#include "nsContentErrors.h"
#include "nsUnicharUtilCIID.h"
#include "nsCompressedCharMap.h"
#include "nsINativeKeyBindings.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsXULPopupManager.h"
#include "nsIPermissionManager.h"
#include "nsIContentPrefService.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIRunnable.h"
#include "nsDOMJSUtils.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsReferencedElement.h"
#include "nsIUGenCategory.h"
#include "nsIDragService.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIInterfaceRequestor.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsCPrefetchService.h"
#include "nsIChromeRegistry.h"
#include "nsEventDispatcher.h"
#include "nsIMIMEHeaderParam.h"
#include "nsIDOMXULCommandEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsDOMDataTransfer.h"
#include "nsHtml5Module.h"
#include "nsPresContext.h"
#include "nsLayoutStatics.h"
#include "nsLayoutUtils.h"
#include "nsFrameManager.h"
#include "BasicLayers.h"
#include "nsFocusManager.h"
#include "nsTextEditorState.h"
#include "nsIPluginHost.h"
#include "nsICategoryManager.h"
#include "nsAHtml5FragmentParser.h"
#include "nsIViewManager.h"

#ifdef IBMBIDI
#include "nsIBidiKeyboard.h"
#endif
#include "nsCycleCollectionParticipant.h"


#include "nsIStringBundle.h"
#include "nsIScriptError.h"
#include "nsIConsoleService.h"

#include "mozAutoDocUpdate.h"
#include "imgICache.h"
#include "jsinterp.h"
#include "jsarray.h"
#include "jsdate.h"
#include "jsregexp.h"
#include "jstypedarray.h"
#include "xpcprivate.h"
#include "nsScriptSecurityManager.h"
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsContentDLF.h"
#ifdef MOZ_MEDIA
#include "nsHTMLMediaElement.h"
#endif
#include "nsDOMTouchEvent.h"

#include "mozilla/Preferences.h"

using namespace mozilla::dom;
using namespace mozilla::layers;
using namespace mozilla;

const char kLoadAsData[] = "loadAsData";

static const char kJSStackContractID[] = "@mozilla.org/js/xpc/ContextStack;1";
static NS_DEFINE_CID(kParserServiceCID, NS_PARSERSERVICE_CID);
static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);

nsIDOMScriptObjectFactory *nsContentUtils::sDOMScriptObjectFactory = nsnull;
nsIXPConnect *nsContentUtils::sXPConnect;
nsIScriptSecurityManager *nsContentUtils::sSecurityManager;
nsIThreadJSContextStack *nsContentUtils::sThreadJSContextStack;
nsIParserService *nsContentUtils::sParserService = nsnull;
nsINameSpaceManager *nsContentUtils::sNameSpaceManager;
nsIIOService *nsContentUtils::sIOService;
#ifdef MOZ_XTF
nsIXTFService *nsContentUtils::sXTFService = nsnull;
#endif
nsIPrefBranch2 *nsContentUtils::sPrefBranch = nsnull;
imgILoader *nsContentUtils::sImgLoader;
imgICache *nsContentUtils::sImgCache;
nsIConsoleService *nsContentUtils::sConsoleService;
nsDataHashtable<nsISupportsHashKey, EventNameMapping>* nsContentUtils::sAtomEventTable = nsnull;
nsDataHashtable<nsStringHashKey, EventNameMapping>* nsContentUtils::sStringEventTable = nsnull;
nsCOMArray<nsIAtom>* nsContentUtils::sUserDefinedEvents = nsnull;
nsIStringBundleService *nsContentUtils::sStringBundleService;
nsIStringBundle *nsContentUtils::sStringBundles[PropertiesFile_COUNT];
nsIContentPolicy *nsContentUtils::sContentPolicyService;
PRBool nsContentUtils::sTriedToGetContentPolicy = PR_FALSE;
nsILineBreaker *nsContentUtils::sLineBreaker;
nsIWordBreaker *nsContentUtils::sWordBreaker;
nsIUGenCategory *nsContentUtils::sGenCat;
nsIScriptRuntime *nsContentUtils::sScriptRuntimes[NS_STID_ARRAY_UBOUND];
PRInt32 nsContentUtils::sScriptRootCount[NS_STID_ARRAY_UBOUND];
PRUint32 nsContentUtils::sJSGCThingRootCount;
#ifdef IBMBIDI
nsIBidiKeyboard *nsContentUtils::sBidiKeyboard = nsnull;
#endif
PRUint32 nsContentUtils::sScriptBlockerCount = 0;
#ifdef DEBUG
PRUint32 nsContentUtils::sDOMNodeRemovedSuppressCount = 0;
#endif
nsCOMArray<nsIRunnable>* nsContentUtils::sBlockedScriptRunners = nsnull;
PRUint32 nsContentUtils::sRunnersCountAtFirstBlocker = 0;
PRUint32 nsContentUtils::sScriptBlockerCountWhereRunnersPrevented = 0;
nsIInterfaceRequestor* nsContentUtils::sSameOriginChecker = nsnull;

PRBool nsContentUtils::sIsHandlingKeyBoardEvent = PR_FALSE;
PRBool nsContentUtils::sAllowXULXBL_for_file = PR_FALSE;

PRBool nsContentUtils::sInitialized = PR_FALSE;

nsRefPtrHashtable<nsPrefObserverHashKey, nsPrefOldCallback>
  *nsContentUtils::sPrefCallbackTable = nsnull;

static PLDHashTable sEventListenerManagersHash;

class EventListenerManagerMapEntry : public PLDHashEntryHdr
{
public:
  EventListenerManagerMapEntry(const void *aKey)
    : mKey(aKey)
  {
  }

  ~EventListenerManagerMapEntry()
  {
    NS_ASSERTION(!mListenerManager, "caller must release and disconnect ELM");
  }

private:
  const void *mKey; 

public:
  nsCOMPtr<nsIEventListenerManager> mListenerManager;
};

static PRBool
EventListenerManagerHashInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                                  const void *key)
{
  
  new (entry) EventListenerManagerMapEntry(key);
  return PR_TRUE;
}

static void
EventListenerManagerHashClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  EventListenerManagerMapEntry *lm =
    static_cast<EventListenerManagerMapEntry *>(entry);

  
  lm->~EventListenerManagerMapEntry();
}

class nsSameOriginChecker : public nsIChannelEventSink,
                            public nsIInterfaceRequestor
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
};

class nsPrefObserverHashKey : public PLDHashEntryHdr {
public:
  typedef nsPrefObserverHashKey* KeyType;
  typedef const nsPrefObserverHashKey* KeyTypePointer;

  static const nsPrefObserverHashKey* KeyToPointer(nsPrefObserverHashKey *aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(const nsPrefObserverHashKey *aKey)
  {
    PRUint32 strHash = nsCRT::HashCode(aKey->mPref.BeginReading(),
                                       aKey->mPref.Length());
    return PR_ROTATE_LEFT32(strHash, 4) ^
           NS_PTR_TO_UINT32(aKey->mCallback);
  }

  nsPrefObserverHashKey(const char *aPref, PrefChangedFunc aCallback) :
    mPref(aPref), mCallback(aCallback) { }

  nsPrefObserverHashKey(const nsPrefObserverHashKey *aOther) :
    mPref(aOther->mPref), mCallback(aOther->mCallback)
  { }

  PRBool KeyEquals(const nsPrefObserverHashKey *aOther) const
  {
    return mCallback == aOther->mCallback &&
           mPref.Equals(aOther->mPref);
  }

  nsPrefObserverHashKey *GetKey() const
  {
    return const_cast<nsPrefObserverHashKey*>(this);
  }

  enum { ALLOW_MEMMOVE = PR_TRUE };

public:
  nsCString mPref;
  PrefChangedFunc mCallback;
};


class nsPrefOldCallback : public nsIObserver,
                          public nsPrefObserverHashKey
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

public:
  nsPrefOldCallback(const char *aPref, PrefChangedFunc aCallback)
    : nsPrefObserverHashKey(aPref, aCallback) { }

  ~nsPrefOldCallback() {
    nsIPrefBranch2 *prefBranch = nsContentUtils::GetPrefBranch();
    if(prefBranch)
      prefBranch->RemoveObserver(mPref.get(), this);
  }

  void AppendClosure(void *aClosure) {
    mClosures.AppendElement(aClosure);
  }

  void RemoveClosure(void *aClosure) {
    mClosures.RemoveElement(aClosure);
  }

  PRBool HasNoClosures() {
    return mClosures.Length() == 0;
  }

public:
  nsTArray<void *>  mClosures;
};

NS_IMPL_ISUPPORTS1(nsPrefOldCallback, nsIObserver)

NS_IMETHODIMP
nsPrefOldCallback::Observe(nsISupports     *aSubject,
                           const char      *aTopic,
                           const PRUnichar *aData)
{
  NS_ASSERTION(!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID),
               "invalid topic");
  NS_LossyConvertUTF16toASCII data(aData);
  for (PRUint32 i = 0; i < mClosures.Length(); i++) {
    mCallback(data.get(), mClosures.ElementAt(i));
  }

  return NS_OK;
}

struct PrefCacheData {
  void* cacheLocation;
  union {
    PRBool defaultValueBool;
    PRInt32 defaultValueInt;
  };
};

nsTArray<nsAutoPtr<PrefCacheData> >* sPrefCacheData = nsnull;


nsresult
nsContentUtils::Init()
{
  if (sInitialized) {
    NS_WARNING("Init() called twice");

    return NS_OK;
  }

  sPrefCacheData = new nsTArray<nsAutoPtr<PrefCacheData> >();

  
  CallGetService(NS_PREFSERVICE_CONTRACTID, &sPrefBranch);

  nsresult rv = NS_GetNameSpaceManager(&sNameSpaceManager);
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPConnect* xpconnect = nsXPConnect::GetXPConnect();
  NS_ENSURE_TRUE(xpconnect, NS_ERROR_FAILURE);

  sXPConnect = xpconnect;
  sThreadJSContextStack = xpconnect;

  sSecurityManager = nsScriptSecurityManager::GetScriptSecurityManager();
  if(!sSecurityManager)
    return NS_ERROR_FAILURE;
  NS_ADDREF(sSecurityManager);

  rv = CallGetService(NS_IOSERVICE_CONTRACTID, &sIOService);
  if (NS_FAILED(rv)) {
    

    sIOService = nsnull;
  }

  rv = CallGetService(NS_LBRK_CONTRACTID, &sLineBreaker);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = CallGetService(NS_WBRK_CONTRACTID, &sWordBreaker);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallGetService(NS_UNICHARCATEGORY_CONTRACTID, &sGenCat);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!InitializeEventTable())
    return NS_ERROR_FAILURE;

  if (!sEventListenerManagersHash.ops) {
    static PLDHashTableOps hash_table_ops =
    {
      PL_DHashAllocTable,
      PL_DHashFreeTable,
      PL_DHashVoidPtrKeyStub,
      PL_DHashMatchEntryStub,
      PL_DHashMoveEntryStub,
      EventListenerManagerHashClearEntry,
      PL_DHashFinalizeStub,
      EventListenerManagerHashInitEntry
    };

    if (!PL_DHashTableInit(&sEventListenerManagersHash, &hash_table_ops,
                           nsnull, sizeof(EventListenerManagerMapEntry), 16)) {
      sEventListenerManagersHash.ops = nsnull;

      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  sBlockedScriptRunners = new nsCOMArray<nsIRunnable>;
  NS_ENSURE_TRUE(sBlockedScriptRunners, NS_ERROR_OUT_OF_MEMORY);

  nsContentUtils::AddBoolPrefVarCache("dom.allow_XUL_XBL_for_file",
                                      &sAllowXULXBL_for_file);

  sInitialized = PR_TRUE;

  return NS_OK;
}

bool nsContentUtils::sImgLoaderInitialized;

void
nsContentUtils::InitImgLoader()
{
  sImgLoaderInitialized = true;

  
  nsresult rv = CallGetService("@mozilla.org/image/loader;1", &sImgLoader);
  if (NS_FAILED(rv)) {
    
    sImgLoader = nsnull;
    sImgCache = nsnull;
  } else {
    if (NS_FAILED(CallGetService("@mozilla.org/image/cache;1", &sImgCache )))
      sImgCache = nsnull;
  }
}

PRBool
nsContentUtils::InitializeEventTable() {
  NS_ASSERTION(!sAtomEventTable, "EventTable already initialized!");
  NS_ASSERTION(!sStringEventTable, "EventTable already initialized!");

  static const EventNameMapping eventArray[] = {
    { nsGkAtoms::onmousedown,                   NS_MOUSE_BUTTON_DOWN, EventNameType_All, NS_MOUSE_EVENT },
    { nsGkAtoms::onmouseup,                     NS_MOUSE_BUTTON_UP, EventNameType_All, NS_MOUSE_EVENT },
    { nsGkAtoms::onclick,                       NS_MOUSE_CLICK, EventNameType_All, NS_MOUSE_EVENT },
    { nsGkAtoms::ondblclick,                    NS_MOUSE_DOUBLECLICK, EventNameType_HTMLXUL, NS_MOUSE_EVENT },
    { nsGkAtoms::onmouseover,                   NS_MOUSE_ENTER_SYNTH, EventNameType_All, NS_MOUSE_EVENT },
    { nsGkAtoms::onmouseout,                    NS_MOUSE_EXIT_SYNTH, EventNameType_All, NS_MOUSE_EVENT },
    { nsGkAtoms::onMozMouseHittest,             NS_MOUSE_MOZHITTEST, EventNameType_None, NS_MOUSE_EVENT },
    { nsGkAtoms::onmousemove,                   NS_MOUSE_MOVE, EventNameType_All, NS_MOUSE_EVENT },
    { nsGkAtoms::oncontextmenu,                 NS_CONTEXTMENU, EventNameType_HTMLXUL, NS_MOUSE_EVENT },

    { nsGkAtoms::onkeydown,                     NS_KEY_DOWN, EventNameType_HTMLXUL, NS_KEY_EVENT },
    { nsGkAtoms::onkeyup,                       NS_KEY_UP, EventNameType_HTMLXUL, NS_KEY_EVENT },
    { nsGkAtoms::onkeypress,                    NS_KEY_PRESS, EventNameType_HTMLXUL, NS_KEY_EVENT },
                                                
    { nsGkAtoms::onfocus,                       NS_FOCUS_CONTENT, EventNameType_HTMLXUL, NS_FOCUS_EVENT },
    { nsGkAtoms::onblur,                        NS_BLUR_CONTENT, EventNameType_HTMLXUL, NS_FOCUS_EVENT },

    { nsGkAtoms::onoffline,                     NS_OFFLINE, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::ononline,                      NS_ONLINE, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onsubmit,                      NS_FORM_SUBMIT, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onreset,                       NS_FORM_RESET, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onchange,                      NS_FORM_CHANGE, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onselect,                      NS_FORM_SELECTED, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::oninvalid,                     NS_FORM_INVALID, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onload,                        NS_LOAD, EventNameType_All, NS_EVENT },
    { nsGkAtoms::onpopstate,                    NS_POPSTATE, EventNameType_HTMLXUL, NS_EVENT_NULL },
    { nsGkAtoms::onunload,                      NS_PAGE_UNLOAD,
                                                (EventNameType_HTMLXUL | EventNameType_SVGSVG), NS_EVENT },
    { nsGkAtoms::onhashchange,                  NS_HASHCHANGE, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onreadystatechange,            NS_READYSTATECHANGE, EventNameType_HTMLXUL },
    { nsGkAtoms::onbeforeunload,                NS_BEFORE_PAGE_UNLOAD, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onabort,                       NS_IMAGE_ABORT,
                                                (EventNameType_HTMLXUL | EventNameType_SVGSVG), NS_EVENT },
    { nsGkAtoms::onerror,                       NS_LOAD_ERROR,
                                                (EventNameType_HTMLXUL | EventNameType_SVGSVG), NS_EVENT },
    { nsGkAtoms::onbeforescriptexecute,         NS_BEFORE_SCRIPT_EXECUTE, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onafterscriptexecute,          NS_AFTER_SCRIPT_EXECUTE, EventNameType_HTMLXUL, NS_EVENT },

    { nsGkAtoms::onDOMAttrModified,             NS_MUTATION_ATTRMODIFIED, EventNameType_HTMLXUL, NS_MUTATION_EVENT },
    { nsGkAtoms::onDOMCharacterDataModified,    NS_MUTATION_CHARACTERDATAMODIFIED, EventNameType_HTMLXUL, NS_MUTATION_EVENT },
    { nsGkAtoms::onDOMNodeInserted,             NS_MUTATION_NODEINSERTED, EventNameType_HTMLXUL, NS_MUTATION_EVENT },
    { nsGkAtoms::onDOMNodeRemoved,              NS_MUTATION_NODEREMOVED, EventNameType_HTMLXUL, NS_MUTATION_EVENT },
    { nsGkAtoms::onDOMNodeInsertedIntoDocument, NS_MUTATION_NODEINSERTEDINTODOCUMENT, EventNameType_HTMLXUL, NS_MUTATION_EVENT },
    { nsGkAtoms::onDOMNodeRemovedFromDocument,  NS_MUTATION_NODEREMOVEDFROMDOCUMENT, EventNameType_HTMLXUL, NS_MUTATION_EVENT },
    { nsGkAtoms::onDOMSubtreeModified,          NS_MUTATION_SUBTREEMODIFIED, EventNameType_HTMLXUL, NS_MUTATION_EVENT },

    { nsGkAtoms::onDOMActivate,                 NS_UI_ACTIVATE, EventNameType_HTMLXUL, NS_UI_EVENT },
    { nsGkAtoms::onDOMFocusIn,                  NS_UI_FOCUSIN, EventNameType_HTMLXUL, NS_UI_EVENT },
    { nsGkAtoms::onDOMFocusOut,                 NS_UI_FOCUSOUT, EventNameType_HTMLXUL, NS_UI_EVENT },
    { nsGkAtoms::oninput,                       NS_FORM_INPUT, EventNameType_HTMLXUL, NS_UI_EVENT },
                                                
    { nsGkAtoms::onDOMMouseScroll,              NS_MOUSE_SCROLL, EventNameType_HTMLXUL, NS_MOUSE_SCROLL_EVENT },
    { nsGkAtoms::onMozMousePixelScroll,         NS_MOUSE_PIXEL_SCROLL, EventNameType_HTMLXUL, NS_MOUSE_SCROLL_EVENT },
                                                
    { nsGkAtoms::onpageshow,                    NS_PAGE_SHOW, EventNameType_HTML, NS_EVENT },
    { nsGkAtoms::onpagehide,                    NS_PAGE_HIDE, EventNameType_HTML, NS_EVENT },
    { nsGkAtoms::onMozBeforeResize,             NS_BEFORERESIZE_EVENT, EventNameType_None, NS_EVENT },
    { nsGkAtoms::onresize,                      NS_RESIZE_EVENT,
                                                (EventNameType_HTMLXUL | EventNameType_SVGSVG), NS_EVENT },
    { nsGkAtoms::onscroll,                      NS_SCROLL_EVENT,
                                                (EventNameType_HTMLXUL | EventNameType_SVGSVG), NS_EVENT_NULL },
    { nsGkAtoms::oncopy,                        NS_COPY, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::oncut,                         NS_CUT, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onpaste,                       NS_PASTE, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onopen,                        NS_OPEN, EventNameType_None, NS_EVENT },
    { nsGkAtoms::onmessage,                     NS_MESSAGE, EventNameType_None, NS_EVENT },
    
    { nsGkAtoms::ontext,                        NS_TEXT_TEXT, EventNameType_XUL, NS_EVENT_NULL },

    { nsGkAtoms::oncompositionstart,            NS_COMPOSITION_START, EventNameType_XUL, NS_COMPOSITION_EVENT },
    { nsGkAtoms::oncompositionend,              NS_COMPOSITION_END, EventNameType_XUL, NS_COMPOSITION_EVENT },

    { nsGkAtoms::oncommand,                     NS_XUL_COMMAND, EventNameType_XUL, NS_INPUT_EVENT },

    { nsGkAtoms::onclose,                       NS_XUL_CLOSE, EventNameType_XUL, NS_EVENT_NULL},
    { nsGkAtoms::onpopupshowing,                NS_XUL_POPUP_SHOWING, EventNameType_XUL, NS_EVENT_NULL},
    { nsGkAtoms::onpopupshown,                  NS_XUL_POPUP_SHOWN, EventNameType_XUL, NS_EVENT_NULL},
    { nsGkAtoms::onpopuphiding,                 NS_XUL_POPUP_HIDING, EventNameType_XUL, NS_EVENT_NULL},
    { nsGkAtoms::onpopuphidden,                 NS_XUL_POPUP_HIDDEN, EventNameType_XUL, NS_EVENT_NULL},
    { nsGkAtoms::onbroadcast,                   NS_XUL_BROADCAST, EventNameType_XUL, NS_EVENT_NULL},
    { nsGkAtoms::oncommandupdate,               NS_XUL_COMMAND_UPDATE, EventNameType_XUL, NS_EVENT_NULL},

    { nsGkAtoms::ondragenter,                   NS_DRAGDROP_ENTER, EventNameType_HTMLXUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondragover,                    NS_DRAGDROP_OVER_SYNTH, EventNameType_HTMLXUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondragexit,                    NS_DRAGDROP_EXIT_SYNTH, EventNameType_XUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondragdrop,                    NS_DRAGDROP_DRAGDROP, EventNameType_XUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondraggesture,                 NS_DRAGDROP_GESTURE, EventNameType_XUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondrag,                        NS_DRAGDROP_DRAG, EventNameType_HTMLXUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondragend,                     NS_DRAGDROP_END, EventNameType_HTMLXUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondragstart,                   NS_DRAGDROP_START, EventNameType_HTMLXUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondragleave,                   NS_DRAGDROP_LEAVE_SYNTH, EventNameType_HTMLXUL, NS_DRAG_EVENT },
    { nsGkAtoms::ondrop,                        NS_DRAGDROP_DROP, EventNameType_HTMLXUL, NS_DRAG_EVENT },

    { nsGkAtoms::onoverflow,                    NS_SCROLLPORT_OVERFLOW, EventNameType_XUL, NS_EVENT_NULL},
    { nsGkAtoms::onunderflow,                   NS_SCROLLPORT_UNDERFLOW, EventNameType_XUL, NS_EVENT_NULL},
#ifdef MOZ_SVG
    { nsGkAtoms::onSVGLoad,                     NS_SVG_LOAD, EventNameType_None, NS_SVG_EVENT },
    { nsGkAtoms::onSVGUnload,                   NS_SVG_UNLOAD, EventNameType_None, NS_SVG_EVENT },
    { nsGkAtoms::onSVGAbort,                    NS_SVG_ABORT, EventNameType_None, NS_SVG_EVENT },
    { nsGkAtoms::onSVGError,                    NS_SVG_ERROR, EventNameType_None, NS_SVG_EVENT },
    { nsGkAtoms::onSVGResize,                   NS_SVG_RESIZE, EventNameType_None, NS_SVG_EVENT },
    { nsGkAtoms::onSVGScroll,                   NS_SVG_SCROLL, EventNameType_None, NS_SVG_EVENT },

    { nsGkAtoms::onSVGZoom,                     NS_SVG_ZOOM, EventNameType_None, NS_SVGZOOM_EVENT },

    
    { nsGkAtoms::onzoom,                        NS_SVG_ZOOM, EventNameType_SVGSVG, NS_EVENT_NULL },
#endif 
#ifdef MOZ_SMIL
    { nsGkAtoms::onbegin,                       NS_SMIL_BEGIN, EventNameType_SMIL, NS_EVENT_NULL },
    { nsGkAtoms::onbeginEvent,                  NS_SMIL_BEGIN, EventNameType_None, NS_SMIL_TIME_EVENT },
    { nsGkAtoms::onend,                         NS_SMIL_END, EventNameType_SMIL, NS_EVENT_NULL },
    { nsGkAtoms::onendEvent,                    NS_SMIL_END, EventNameType_None, NS_SMIL_TIME_EVENT },
    { nsGkAtoms::onrepeat,                      NS_SMIL_REPEAT, EventNameType_SMIL, NS_EVENT_NULL },
    { nsGkAtoms::onrepeatEvent,                 NS_SMIL_REPEAT, EventNameType_None, NS_SMIL_TIME_EVENT },
#endif 
#ifdef MOZ_MEDIA
    { nsGkAtoms::onloadstart,                   NS_LOADSTART, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onprogress,                    NS_PROGRESS, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onsuspend,                     NS_SUSPEND, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onemptied,                     NS_EMPTIED, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onstalled,                     NS_STALLED, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onplay,                        NS_PLAY, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onpause,                       NS_PAUSE, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onloadedmetadata,              NS_LOADEDMETADATA, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onloadeddata,                  NS_LOADEDDATA, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onwaiting,                     NS_WAITING, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onplaying,                     NS_PLAYING, EventNameType_HTML,  NS_EVENT_NULL },
    { nsGkAtoms::oncanplay,                     NS_CANPLAY, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::oncanplaythrough,              NS_CANPLAYTHROUGH, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onseeking,                     NS_SEEKING, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onseeked,                      NS_SEEKED, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::ontimeupdate,                  NS_TIMEUPDATE, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onended,                       NS_ENDED, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onratechange,                  NS_RATECHANGE, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::ondurationchange,              NS_DURATIONCHANGE, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onvolumechange,                NS_VOLUMECHANGE, EventNameType_HTML, NS_EVENT_NULL },
    { nsGkAtoms::onMozAudioAvailable,           NS_MOZAUDIOAVAILABLE, EventNameType_None, NS_EVENT_NULL },
#endif 
    { nsGkAtoms::onMozAfterPaint,               NS_AFTERPAINT, EventNameType_None, NS_EVENT },
    { nsGkAtoms::onMozBeforePaint,              NS_BEFOREPAINT, EventNameType_None, NS_EVENT_NULL },

    { nsGkAtoms::onMozScrolledAreaChanged,      NS_SCROLLEDAREACHANGED, EventNameType_None, NS_SCROLLAREA_EVENT },

    
    { nsGkAtoms::onMozSwipeGesture,             NS_SIMPLE_GESTURE_SWIPE, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },
    { nsGkAtoms::onMozMagnifyGestureStart,      NS_SIMPLE_GESTURE_MAGNIFY_START, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },
    { nsGkAtoms::onMozMagnifyGestureUpdate,     NS_SIMPLE_GESTURE_MAGNIFY_UPDATE, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },
    { nsGkAtoms::onMozMagnifyGesture,           NS_SIMPLE_GESTURE_MAGNIFY, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },
    { nsGkAtoms::onMozRotateGestureStart,       NS_SIMPLE_GESTURE_ROTATE_START, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },
    { nsGkAtoms::onMozRotateGestureUpdate,      NS_SIMPLE_GESTURE_ROTATE_UPDATE, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },
    { nsGkAtoms::onMozRotateGesture,            NS_SIMPLE_GESTURE_ROTATE, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },
    { nsGkAtoms::onMozTapGesture,               NS_SIMPLE_GESTURE_TAP, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },
    { nsGkAtoms::onMozPressTapGesture,          NS_SIMPLE_GESTURE_PRESSTAP, EventNameType_None, NS_SIMPLE_GESTURE_EVENT },

    { nsGkAtoms::onMozTouchDown,                NS_MOZTOUCH_DOWN, EventNameType_None, NS_MOZTOUCH_EVENT },
    { nsGkAtoms::onMozTouchMove,                NS_MOZTOUCH_MOVE, EventNameType_None, NS_MOZTOUCH_EVENT },
    { nsGkAtoms::onMozTouchUp,                  NS_MOZTOUCH_UP, EventNameType_None, NS_MOZTOUCH_EVENT },

    { nsGkAtoms::ontransitionend,               NS_TRANSITION_END, EventNameType_None, NS_TRANSITION_EVENT }
#ifdef MOZ_CSS_ANIMATIONS
    ,
    { nsGkAtoms::onanimationstart,              NS_ANIMATION_START, EventNameType_None, NS_ANIMATION_EVENT },
    { nsGkAtoms::onanimationend,                NS_ANIMATION_END, EventNameType_None, NS_ANIMATION_EVENT },
    { nsGkAtoms::onanimationiteration,          NS_ANIMATION_ITERATION, EventNameType_None, NS_ANIMATION_EVENT },
#endif
    { nsGkAtoms::onbeforeprint,                 NS_BEFOREPRINT, EventNameType_HTMLXUL, NS_EVENT },
    { nsGkAtoms::onafterprint,                  NS_AFTERPRINT, EventNameType_HTMLXUL, NS_EVENT }
  };

  sAtomEventTable = new nsDataHashtable<nsISupportsHashKey, EventNameMapping>;
  sStringEventTable = new nsDataHashtable<nsStringHashKey, EventNameMapping>;
  sUserDefinedEvents = new nsCOMArray<nsIAtom>(64);

  if (!sAtomEventTable || !sStringEventTable || !sUserDefinedEvents ||
      !sAtomEventTable->Init(int(NS_ARRAY_LENGTH(eventArray) / 0.75) + 1) ||
      !sStringEventTable->Init(int(NS_ARRAY_LENGTH(eventArray) / 0.75) + 1)) {
    delete sAtomEventTable;
    sAtomEventTable = nsnull;
    delete sStringEventTable;
    sStringEventTable = nsnull;
    delete sUserDefinedEvents;
    sUserDefinedEvents = nsnull;
    return PR_FALSE;
  }

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(eventArray); ++i) {
    if (!sAtomEventTable->Put(eventArray[i].mAtom, eventArray[i]) ||
        !sStringEventTable->Put(Substring(nsDependentAtomString(eventArray[i].mAtom), 2),
                                eventArray[i])) {
      delete sAtomEventTable;
      sAtomEventTable = nsnull;
      delete sStringEventTable;
      sStringEventTable = nsnull;
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

void
nsContentUtils::InitializeTouchEventTable()
{
  static PRBool sEventTableInitialized = PR_FALSE;
  if (!sEventTableInitialized && sAtomEventTable && sStringEventTable) {
    sEventTableInitialized = PR_TRUE;
    static const EventNameMapping touchEventArray[] = {
      { nsGkAtoms::ontouchstart, NS_USER_DEFINED_EVENT, EventNameType_All, NS_INPUT_EVENT },
      { nsGkAtoms::ontouchend, NS_USER_DEFINED_EVENT, EventNameType_All, NS_INPUT_EVENT },
      { nsGkAtoms::ontouchmove, NS_USER_DEFINED_EVENT, EventNameType_All, NS_INPUT_EVENT },
      { nsGkAtoms::ontouchenter, NS_USER_DEFINED_EVENT, EventNameType_All, NS_INPUT_EVENT },
      { nsGkAtoms::ontouchleave, NS_USER_DEFINED_EVENT, EventNameType_All, NS_INPUT_EVENT },
      { nsGkAtoms::ontouchcancel, NS_USER_DEFINED_EVENT, EventNameType_All, NS_INPUT_EVENT }
    };
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(touchEventArray); ++i) {
      if (!sAtomEventTable->Put(touchEventArray[i].mAtom, touchEventArray[i]) ||
          !sStringEventTable->Put(Substring(nsDependentAtomString(touchEventArray[i].mAtom), 2),
                                  touchEventArray[i])) {
        delete sAtomEventTable;
        sAtomEventTable = nsnull;
        delete sStringEventTable;
        sStringEventTable = nsnull;
        return;
      }
    }
  }
}







nsIParserService*
nsContentUtils::GetParserService()
{
  
  if (!sParserService) {
    
    
    nsresult rv = CallGetService(kParserServiceCID, &sParserService);
    if (NS_FAILED(rv)) {
      sParserService = nsnull;
    }
  }

  return sParserService;
}

#ifdef MOZ_XTF
nsIXTFService*
nsContentUtils::GetXTFService()
{
  if (!sXTFService) {
    nsresult rv = CallGetService(kXTFServiceCID, &sXTFService);
    if (NS_FAILED(rv)) {
      sXTFService = nsnull;
    }
  }

  return sXTFService;
}
#endif

#ifdef IBMBIDI
nsIBidiKeyboard*
nsContentUtils::GetBidiKeyboard()
{
  if (!sBidiKeyboard) {
    nsresult rv = CallGetService("@mozilla.org/widget/bidikeyboard;1", &sBidiKeyboard);
    if (NS_FAILED(rv)) {
      sBidiKeyboard = nsnull;
    }
  }
  return sBidiKeyboard;
}
#endif

template <class OutputIterator>
struct NormalizeNewlinesCharTraits {
  public:
    typedef typename OutputIterator::value_type value_type;

  public:
    NormalizeNewlinesCharTraits(OutputIterator& aIterator) : mIterator(aIterator) { }
    void writechar(typename OutputIterator::value_type aChar) {
      *mIterator++ = aChar;
    }

  private:
    OutputIterator mIterator;
};

#ifdef HAVE_CPP_PARTIAL_SPECIALIZATION

template <class CharT>
struct NormalizeNewlinesCharTraits<CharT*> {
  public:
    typedef CharT value_type;

  public:
    NormalizeNewlinesCharTraits(CharT* aCharPtr) : mCharPtr(aCharPtr) { }
    void writechar(CharT aChar) {
      *mCharPtr++ = aChar;
    }

  private:
    CharT* mCharPtr;
};

#else

NS_SPECIALIZE_TEMPLATE
struct NormalizeNewlinesCharTraits<char*> {
  public:
    typedef char value_type;

  public:
    NormalizeNewlinesCharTraits(char* aCharPtr) : mCharPtr(aCharPtr) { }
    void writechar(char aChar) {
      *mCharPtr++ = aChar;
    }

  private:
    char* mCharPtr;
};

NS_SPECIALIZE_TEMPLATE
struct NormalizeNewlinesCharTraits<PRUnichar*> {
  public:
    typedef PRUnichar value_type;

  public:
    NormalizeNewlinesCharTraits(PRUnichar* aCharPtr) : mCharPtr(aCharPtr) { }
    void writechar(PRUnichar aChar) {
      *mCharPtr++ = aChar;
    }

  private:
    PRUnichar* mCharPtr;
};

#endif

template <class OutputIterator>
class CopyNormalizeNewlines
{
  public:
    typedef typename OutputIterator::value_type value_type;

  public:
    CopyNormalizeNewlines(OutputIterator* aDestination,
                          PRBool aLastCharCR=PR_FALSE) :
      mLastCharCR(aLastCharCR),
      mDestination(aDestination),
      mWritten(0)
    { }

    PRUint32 GetCharsWritten() {
      return mWritten;
    }

    PRBool IsLastCharCR() {
      return mLastCharCR;
    }

    void write(const typename OutputIterator::value_type* aSource, PRUint32 aSourceLength) {

      const typename OutputIterator::value_type* done_writing = aSource + aSourceLength;

      
      if (mLastCharCR) {
        
        
        if (aSourceLength && (*aSource == value_type('\n'))) {
          ++aSource;
        }
        mLastCharCR = PR_FALSE;
      }

      PRUint32 num_written = 0;
      while ( aSource < done_writing ) {
        if (*aSource == value_type('\r')) {
          mDestination->writechar('\n');
          ++aSource;
          
          
          if (aSource == done_writing) {
            mLastCharCR = PR_TRUE;
          }
          
          else if (*aSource == value_type('\n')) {
            ++aSource;
          }
        }
        else {
          mDestination->writechar(*aSource++);
        }
        ++num_written;
      }

      mWritten += num_written;
    }

  private:
    PRBool mLastCharCR;
    OutputIterator* mDestination;
    PRUint32 mWritten;
};


PRUint32
nsContentUtils::CopyNewlineNormalizedUnicodeTo(const nsAString& aSource,
                                               PRUint32 aSrcOffset,
                                               PRUnichar* aDest,
                                               PRUint32 aLength,
                                               PRBool& aLastCharCR)
{
  typedef NormalizeNewlinesCharTraits<PRUnichar*> sink_traits;

  sink_traits dest_traits(aDest);
  CopyNormalizeNewlines<sink_traits> normalizer(&dest_traits,aLastCharCR);
  nsReadingIterator<PRUnichar> fromBegin, fromEnd;
  copy_string(aSource.BeginReading(fromBegin).advance( PRInt32(aSrcOffset) ),
              aSource.BeginReading(fromEnd).advance( PRInt32(aSrcOffset+aLength) ),
              normalizer);
  aLastCharCR = normalizer.IsLastCharCR();
  return normalizer.GetCharsWritten();
}


PRUint32
nsContentUtils::CopyNewlineNormalizedUnicodeTo(nsReadingIterator<PRUnichar>& aSrcStart, const nsReadingIterator<PRUnichar>& aSrcEnd, nsAString& aDest)
{
  typedef nsWritingIterator<PRUnichar> WritingIterator;
  typedef NormalizeNewlinesCharTraits<WritingIterator> sink_traits;

  WritingIterator iter;
  aDest.BeginWriting(iter);
  sink_traits dest_traits(iter);
  CopyNormalizeNewlines<sink_traits> normalizer(&dest_traits);
  copy_string(aSrcStart, aSrcEnd, normalizer);
  return normalizer.GetCharsWritten();
}







#include "punct_marks.x-ccmap"
DEFINE_X_CCMAP(gPuncCharsCCMapExt, const);


PRBool
nsContentUtils::IsPunctuationMark(PRUint32 aChar)
{
  return CCMAP_HAS_CHAR_EXT(gPuncCharsCCMapExt, aChar);
}


PRBool
nsContentUtils::IsPunctuationMarkAt(const nsTextFragment* aFrag, PRUint32 aOffset)
{
  PRUnichar h = aFrag->CharAt(aOffset);
  if (!IS_SURROGATE(h)) {
    return IsPunctuationMark(h);
  }
  if (NS_IS_HIGH_SURROGATE(h) && aOffset + 1 < aFrag->GetLength()) {
    PRUnichar l = aFrag->CharAt(aOffset + 1);
    if (NS_IS_LOW_SURROGATE(l)) {
      return IsPunctuationMark(SURROGATE_TO_UCS4(h, l));
    }
  }
  return PR_FALSE;
}


PRBool nsContentUtils::IsAlphanumeric(PRUint32 aChar)
{
  nsIUGenCategory::nsUGenCategory cat = sGenCat->Get(aChar);

  return (cat == nsIUGenCategory::kLetter || cat == nsIUGenCategory::kNumber);
}
 

PRBool nsContentUtils::IsAlphanumericAt(const nsTextFragment* aFrag, PRUint32 aOffset)
{
  PRUnichar h = aFrag->CharAt(aOffset);
  if (!IS_SURROGATE(h)) {
    return IsAlphanumeric(h);
  }
  if (NS_IS_HIGH_SURROGATE(h) && aOffset + 1 < aFrag->GetLength()) {
    PRUnichar l = aFrag->CharAt(aOffset + 1);
    if (NS_IS_LOW_SURROGATE(l)) {
      return IsAlphanumeric(SURROGATE_TO_UCS4(h, l));
    }
  }
  return PR_FALSE;
}


PRBool
nsContentUtils::IsHTMLWhitespace(PRUnichar aChar)
{
  return aChar == PRUnichar(0x0009) ||
         aChar == PRUnichar(0x000A) ||
         aChar == PRUnichar(0x000C) ||
         aChar == PRUnichar(0x000D) ||
         aChar == PRUnichar(0x0020);
}


PRBool
nsContentUtils::ParseIntMarginValue(const nsAString& aString, nsIntMargin& result)
{
  nsAutoString marginStr(aString);
  marginStr.CompressWhitespace(PR_TRUE, PR_TRUE);
  if (marginStr.IsEmpty()) {
    return PR_FALSE;
  }

  PRInt32 start = 0, end = 0;
  for (int count = 0; count < 4; count++) {
    if ((PRUint32)end >= marginStr.Length())
      return PR_FALSE;

    
    if (count < 3)
      end = Substring(marginStr, start).FindChar(',');
    else
      end = Substring(marginStr, start).Length();

    if (end <= 0)
      return PR_FALSE;

    PRInt32 ec, val = 
      nsString(Substring(marginStr, start, end)).ToInteger(&ec);
    if (NS_FAILED(ec))
      return PR_FALSE;

    switch(count) {
      case 0:
        result.top = val;
      break;
      case 1:
        result.right = val;
      break;
      case 2:
        result.bottom = val;
      break;
      case 3:
        result.left = val;
      break;
    }
    start += end + 1;
  }
  return PR_TRUE;
}


void
nsContentUtils::GetOfflineAppManifest(nsIDocument *aDocument, nsIURI **aURI)
{
  Element* docElement = aDocument->GetRootElement();
  if (!docElement) {
    return;
  }

  nsAutoString manifestSpec;
  docElement->GetAttr(kNameSpaceID_None, nsGkAtoms::manifest, manifestSpec);

  
  if (manifestSpec.IsEmpty() ||
      manifestSpec.FindChar('#') != kNotFound) {
    return;
  }

  nsContentUtils::NewURIWithDocumentCharset(aURI, manifestSpec,
                                            aDocument,
                                            aDocument->GetDocBaseURI());
}


PRBool
nsContentUtils::OfflineAppAllowed(nsIURI *aURI)
{
  nsCOMPtr<nsIOfflineCacheUpdateService> updateService =
    do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID);
  if (!updateService) {
    return PR_FALSE;
  }

  PRBool allowed;
  nsresult rv = updateService->OfflineAppAllowedForURI(aURI,
                                                       sPrefBranch,
                                                       &allowed);
  return NS_SUCCEEDED(rv) && allowed;
}


PRBool
nsContentUtils::OfflineAppAllowed(nsIPrincipal *aPrincipal)
{
  nsCOMPtr<nsIOfflineCacheUpdateService> updateService =
    do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID);
  if (!updateService) {
    return PR_FALSE;
  }

  PRBool allowed;
  nsresult rv = updateService->OfflineAppAllowed(aPrincipal,
                                                 sPrefBranch,
                                                 &allowed);
  return NS_SUCCEEDED(rv) && allowed;
}


void
nsContentUtils::Shutdown()
{
  sInitialized = PR_FALSE;

  NS_HTMLParanoidFragmentSinkShutdown();
  NS_XHTMLParanoidFragmentSinkShutdown();

  NS_IF_RELEASE(sContentPolicyService);
  sTriedToGetContentPolicy = PR_FALSE;
  PRUint32 i;
  for (i = 0; i < PropertiesFile_COUNT; ++i)
    NS_IF_RELEASE(sStringBundles[i]);

  
  if (sPrefCallbackTable) {
    delete sPrefCallbackTable;
    sPrefCallbackTable = nsnull;
  }

  delete sPrefCacheData;
  sPrefCacheData = nsnull;

  NS_IF_RELEASE(sStringBundleService);
  NS_IF_RELEASE(sConsoleService);
  NS_IF_RELEASE(sDOMScriptObjectFactory);
  sXPConnect = nsnull;
  sThreadJSContextStack = nsnull;
  NS_IF_RELEASE(sSecurityManager);
  NS_IF_RELEASE(sNameSpaceManager);
  NS_IF_RELEASE(sParserService);
  NS_IF_RELEASE(sIOService);
  NS_IF_RELEASE(sLineBreaker);
  NS_IF_RELEASE(sWordBreaker);
  NS_IF_RELEASE(sGenCat);
#ifdef MOZ_XTF
  NS_IF_RELEASE(sXTFService);
#endif
  NS_IF_RELEASE(sImgLoader);
  NS_IF_RELEASE(sImgCache);
  NS_IF_RELEASE(sPrefBranch);
#ifdef IBMBIDI
  NS_IF_RELEASE(sBidiKeyboard);
#endif

  delete sAtomEventTable;
  sAtomEventTable = nsnull;
  delete sStringEventTable;
  sStringEventTable = nsnull;
  delete sUserDefinedEvents;
  sUserDefinedEvents = nsnull;

  if (sEventListenerManagersHash.ops) {
    NS_ASSERTION(sEventListenerManagersHash.entryCount == 0,
                 "Event listener manager hash not empty at shutdown!");

    

    
    
    
    
    
    

    if (sEventListenerManagersHash.entryCount == 0) {
      PL_DHashTableFinish(&sEventListenerManagersHash);
      sEventListenerManagersHash.ops = nsnull;
    }
  }

  NS_ASSERTION(!sBlockedScriptRunners ||
               sBlockedScriptRunners->Count() == 0,
               "How'd this happen?");
  delete sBlockedScriptRunners;
  sBlockedScriptRunners = nsnull;

  NS_IF_RELEASE(sSameOriginChecker);
  
  nsTextEditorState::ShutDown();
}


PRBool
nsContentUtils::IsCallerTrustedForCapability(const char* aCapability)
{
  
  
  PRBool hasCap;
  if (NS_FAILED(sSecurityManager->IsCapabilityEnabled(aCapability, &hasCap)))
    return PR_FALSE;
  if (hasCap)
    return PR_TRUE;
    
  if (NS_FAILED(sSecurityManager->IsCapabilityEnabled("UniversalXPConnect",
                                                      &hasCap)))
    return PR_FALSE;
  return hasCap;
}









nsresult
nsContentUtils::CheckSameOrigin(nsINode *aTrustedNode,
                                nsIDOMNode *aUnTrustedNode)
{
  NS_PRECONDITION(aTrustedNode, "There must be a trusted node");

  PRBool isSystem = PR_FALSE;
  sSecurityManager->SubjectPrincipalIsSystem(&isSystem);
  if (isSystem) {
    

    return NS_OK;
  }

  


  nsCOMPtr<nsINode> unTrustedNode = do_QueryInterface(aUnTrustedNode);

  
  NS_ENSURE_TRUE(aTrustedNode && unTrustedNode, NS_ERROR_UNEXPECTED);

  nsIPrincipal* trustedPrincipal = aTrustedNode->NodePrincipal();
  nsIPrincipal* unTrustedPrincipal = unTrustedNode->NodePrincipal();

  if (trustedPrincipal == unTrustedPrincipal) {
    return NS_OK;
  }

  PRBool equal;
  
  
  if (NS_FAILED(trustedPrincipal->Equals(unTrustedPrincipal, &equal)) ||
      !equal) {
    return NS_ERROR_DOM_PROP_ACCESS_DENIED;
  }

  return NS_OK;
}


PRBool
nsContentUtils::CanCallerAccess(nsIPrincipal* aSubjectPrincipal,
                                nsIPrincipal* aPrincipal)
{
  PRBool subsumes;
  nsresult rv = aSubjectPrincipal->Subsumes(aPrincipal, &subsumes);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  if (subsumes) {
    return PR_TRUE;
  }

  
  
  
  PRBool isSystem;
  rv = sSecurityManager->IsSystemPrincipal(aPrincipal, &isSystem);
  isSystem = NS_FAILED(rv) || isSystem;
  const char* capability =
    NS_FAILED(rv) || isSystem ? "UniversalXPConnect" : "UniversalBrowserRead";

  return IsCallerTrustedForCapability(capability);
}


PRBool
nsContentUtils::CanCallerAccess(nsIDOMNode *aNode)
{
  
  
  
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  sSecurityManager->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));

  if (!subjectPrincipal) {
    

    return PR_TRUE;
  }

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, PR_FALSE);

  return CanCallerAccess(subjectPrincipal, node->NodePrincipal());
}


PRBool
nsContentUtils::CanCallerAccess(nsPIDOMWindow* aWindow)
{
  
  
  
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  sSecurityManager->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));

  if (!subjectPrincipal) {
    

    return PR_TRUE;
  }

  nsCOMPtr<nsIScriptObjectPrincipal> scriptObject =
    do_QueryInterface(aWindow->IsOuterWindow() ?
                      aWindow->GetCurrentInnerWindow() : aWindow);
  NS_ENSURE_TRUE(scriptObject, PR_FALSE);

  return CanCallerAccess(subjectPrincipal, scriptObject->GetPrincipal());
}


PRBool
nsContentUtils::InProlog(nsINode *aNode)
{
  NS_PRECONDITION(aNode, "missing node to nsContentUtils::InProlog");

  nsINode* parent = aNode->GetNodeParent();
  if (!parent || !parent->IsNodeOfType(nsINode::eDOCUMENT)) {
    return PR_FALSE;
  }

  nsIDocument* doc = static_cast<nsIDocument*>(parent);
  nsIContent* root = doc->GetRootElement();

  return !root || doc->IndexOf(aNode) < doc->IndexOf(root);
}

JSContext *
nsContentUtils::GetContextFromDocument(nsIDocument *aDocument)
{
  nsIScriptGlobalObject *sgo = aDocument->GetScopeObject();
  if (!sgo) {
    
    return nsnull;
  }

  nsIScriptContext *scx = sgo->GetContext();
  if (!scx) {
    

    return nsnull;
  }

  return (JSContext *)scx->GetNativeContext();
}


nsresult
nsContentUtils::GetContextAndScope(nsIDocument *aOldDocument,
                                   nsIDocument *aNewDocument, JSContext **aCx,
                                   JSObject **aNewScope)
{
  *aCx = nsnull;
  *aNewScope = nsnull;

  JSObject *newScope = aNewDocument->GetWrapper();
  JSObject *global;
  if (!newScope) {
    nsIScriptGlobalObject *newSGO = aNewDocument->GetScopeObject();
    if (!newSGO || !(global = newSGO->GetGlobalJSObject())) {
      return NS_OK;
    }
  }

  NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_INITIALIZED);

  JSContext *cx = aOldDocument ? GetContextFromDocument(aOldDocument) : nsnull;
  if (!cx) {
    cx = GetContextFromDocument(aNewDocument);

    if (!cx) {
      
      
      

      sThreadJSContextStack->Peek(&cx);

      if (!cx) {
        sThreadJSContextStack->GetSafeJSContext(&cx);

        if (!cx) {
          
          NS_WARNING("No context reachable in GetContextAndScopes()!");

          return NS_ERROR_NOT_AVAILABLE;
        }
      }
    }
  }

  if (!newScope && cx) {
    jsval v;
    nsresult rv = WrapNative(cx, global, aNewDocument, aNewDocument, &v);
    NS_ENSURE_SUCCESS(rv, rv);

    newScope = JSVAL_TO_OBJECT(v);
  }

  *aCx = cx;
  *aNewScope = newScope;

  return NS_OK;
}

nsresult
nsContentUtils::ReparentContentWrappersInScope(nsIScriptGlobalObject *aOldScope,
                                               nsIScriptGlobalObject *aNewScope)
{
  JSContext *cx = nsnull;

  
  nsIScriptContext *context = aOldScope->GetContext();
  if (context) {
    cx = static_cast<JSContext *>(context->GetNativeContext());
  }

  if (!cx) {
    context = aNewScope->GetContext();
    if (context) {
      cx = static_cast<JSContext *>(context->GetNativeContext());
    }

    if (!cx) {
      sThreadJSContextStack->Peek(&cx);

      if (!cx) {
        sThreadJSContextStack->GetSafeJSContext(&cx);

        if (!cx) {
          
          NS_WARNING("No context reachable in ReparentContentWrappers()!");

          return NS_ERROR_NOT_AVAILABLE;
        }
      }
    }
  }

  
  

  JSObject *oldScopeObj = aOldScope->GetGlobalJSObject();
  JSObject *newScopeObj = aNewScope->GetGlobalJSObject();

  if (!newScopeObj || !oldScopeObj) {
    

    return NS_ERROR_NOT_AVAILABLE;
  }

  return sXPConnect->MoveWrappers(cx, oldScopeObj, newScopeObj);
}

nsPIDOMWindow *
nsContentUtils::GetWindowFromCaller()
{
  JSContext *cx = nsnull;
  sThreadJSContextStack->Peek(&cx);

  if (cx) {
    nsCOMPtr<nsPIDOMWindow> win =
      do_QueryInterface(nsJSUtils::GetDynamicScriptGlobal(cx));
    return win;
  }

  return nsnull;
}

nsIDOMDocument *
nsContentUtils::GetDocumentFromCaller()
{
  JSContext *cx = nsnull;
  JSObject *obj = nsnull;
  sXPConnect->GetCaller(&cx, &obj);
  NS_ASSERTION(cx && obj, "Caller ensures something is running");

  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, obj)) {
    return nsnull;
  }

  nsCOMPtr<nsPIDOMWindow> win =
    do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(cx, obj));
  if (!win) {
    return nsnull;
  }

  return win->GetExtantDocument();
}

nsIDOMDocument *
nsContentUtils::GetDocumentFromContext()
{
  JSContext *cx = nsnull;
  sThreadJSContextStack->Peek(&cx);

  if (cx) {
    nsIScriptGlobalObject *sgo = nsJSUtils::GetDynamicScriptGlobal(cx);

    if (sgo) {
      nsCOMPtr<nsPIDOMWindow> pwin = do_QueryInterface(sgo);
      if (pwin) {
        return pwin->GetExtantDocument();
      }
    }
  }

  return nsnull;
}

PRBool
nsContentUtils::IsCallerChrome()
{
  PRBool is_caller_chrome = PR_FALSE;
  nsresult rv = sSecurityManager->SubjectPrincipalIsSystem(&is_caller_chrome);
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }

  return is_caller_chrome;
}

PRBool
nsContentUtils::IsCallerTrustedForRead()
{
  return IsCallerTrustedForCapability("UniversalBrowserRead");
}

PRBool
nsContentUtils::IsCallerTrustedForWrite()
{
  return IsCallerTrustedForCapability("UniversalBrowserWrite");
}


nsINode*
nsContentUtils::GetCrossDocParentNode(nsINode* aChild)
{
  NS_PRECONDITION(aChild, "The child is null!");

  nsINode* parent = aChild->GetNodeParent();
  if (parent || !aChild->IsNodeOfType(nsINode::eDOCUMENT))
    return parent;

  nsIDocument* doc = static_cast<nsIDocument*>(aChild);
  nsIDocument* parentDoc = doc->GetParentDocument();
  return parentDoc ? parentDoc->FindContentForSubDocument(doc) : nsnull;
}


PRBool
nsContentUtils::ContentIsDescendantOf(const nsINode* aPossibleDescendant,
                                      const nsINode* aPossibleAncestor)
{
  NS_PRECONDITION(aPossibleDescendant, "The possible descendant is null!");
  NS_PRECONDITION(aPossibleAncestor, "The possible ancestor is null!");

  do {
    if (aPossibleDescendant == aPossibleAncestor)
      return PR_TRUE;
    aPossibleDescendant = aPossibleDescendant->GetNodeParent();
  } while (aPossibleDescendant);

  return PR_FALSE;
}


PRBool
nsContentUtils::ContentIsCrossDocDescendantOf(nsINode* aPossibleDescendant,
                                              nsINode* aPossibleAncestor)
{
  NS_PRECONDITION(aPossibleDescendant, "The possible descendant is null!");
  NS_PRECONDITION(aPossibleAncestor, "The possible ancestor is null!");

  do {
    if (aPossibleDescendant == aPossibleAncestor)
      return PR_TRUE;
    aPossibleDescendant = GetCrossDocParentNode(aPossibleDescendant);
  } while (aPossibleDescendant);

  return PR_FALSE;
}



nsresult
nsContentUtils::GetAncestors(nsINode* aNode,
                             nsTArray<nsINode*>& aArray)
{
  while (aNode) {
    aArray.AppendElement(aNode);
    aNode = aNode->GetNodeParent();
  }
  return NS_OK;
}


nsresult
nsContentUtils::GetAncestorsAndOffsets(nsIDOMNode* aNode,
                                       PRInt32 aOffset,
                                       nsTArray<nsIContent*>* aAncestorNodes,
                                       nsTArray<PRInt32>* aAncestorOffsets)
{
  NS_ENSURE_ARG_POINTER(aNode);

  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));

  if (!content) {
    return NS_ERROR_FAILURE;
  }

  if (!aAncestorNodes->IsEmpty()) {
    NS_WARNING("aAncestorNodes is not empty");
    aAncestorNodes->Clear();
  }

  if (!aAncestorOffsets->IsEmpty()) {
    NS_WARNING("aAncestorOffsets is not empty");
    aAncestorOffsets->Clear();
  }

  
  aAncestorNodes->AppendElement(content.get());
  aAncestorOffsets->AppendElement(aOffset);

  
  nsIContent* child = content;
  nsIContent* parent = child->GetParent();
  while (parent) {
    aAncestorNodes->AppendElement(parent);
    aAncestorOffsets->AppendElement(parent->IndexOf(child));
    child = parent;
    parent = parent->GetParent();
  }

  return NS_OK;
}


nsresult
nsContentUtils::GetCommonAncestor(nsIDOMNode *aNode,
                                  nsIDOMNode *aOther,
                                  nsIDOMNode** aCommonAncestor)
{
  *aCommonAncestor = nsnull;

  nsCOMPtr<nsINode> node1 = do_QueryInterface(aNode);
  nsCOMPtr<nsINode> node2 = do_QueryInterface(aOther);

  NS_ENSURE_TRUE(node1 && node2, NS_ERROR_UNEXPECTED);

  nsINode* common = GetCommonAncestor(node1, node2);
  NS_ENSURE_TRUE(common, NS_ERROR_NOT_AVAILABLE);

  return CallQueryInterface(common, aCommonAncestor);
}


nsINode*
nsContentUtils::GetCommonAncestor(nsINode* aNode1,
                                  nsINode* aNode2)
{
  if (aNode1 == aNode2) {
    return aNode1;
  }

  
  nsAutoTPtrArray<nsINode, 30> parents1, parents2;
  do {
    parents1.AppendElement(aNode1);
    aNode1 = aNode1->GetNodeParent();
  } while (aNode1);
  do {
    parents2.AppendElement(aNode2);
    aNode2 = aNode2->GetNodeParent();
  } while (aNode2);

  
  PRUint32 pos1 = parents1.Length();
  PRUint32 pos2 = parents2.Length();
  nsINode* parent = nsnull;
  PRUint32 len;
  for (len = NS_MIN(pos1, pos2); len > 0; --len) {
    nsINode* child1 = parents1.ElementAt(--pos1);
    nsINode* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      break;
    }
    parent = child1;
  }

  return parent;
}


PRInt32
nsContentUtils::ComparePoints(nsINode* aParent1, PRInt32 aOffset1,
                              nsINode* aParent2, PRInt32 aOffset2,
                              PRBool* aDisconnected)
{
  if (aParent1 == aParent2) {
    return aOffset1 < aOffset2 ? -1 :
           aOffset1 > aOffset2 ? 1 :
           0;
  }

  nsAutoTArray<nsINode*, 32> parents1, parents2;
  nsINode* node1 = aParent1;
  nsINode* node2 = aParent2;
  do {
    parents1.AppendElement(node1);
    node1 = node1->GetNodeParent();
  } while (node1);
  do {
    parents2.AppendElement(node2);
    node2 = node2->GetNodeParent();
  } while (node2);

  PRUint32 pos1 = parents1.Length() - 1;
  PRUint32 pos2 = parents2.Length() - 1;
  
  PRBool disconnected = parents1.ElementAt(pos1) != parents2.ElementAt(pos2);
  if (aDisconnected) {
    *aDisconnected = disconnected;
  }
  if (disconnected) {
    NS_ASSERTION(aDisconnected, "unexpected disconnected nodes");
    return 1;
  }

  
  nsINode* parent = parents1.ElementAt(pos1);
  PRUint32 len;
  for (len = NS_MIN(pos1, pos2); len > 0; --len) {
    nsINode* child1 = parents1.ElementAt(--pos1);
    nsINode* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      return parent->IndexOf(child1) < parent->IndexOf(child2) ? -1 : 1;
    }
    parent = child1;
  }

  
  
  

  NS_ASSERTION(!pos1 || !pos2,
               "should have run out of parent chain for one of the nodes");

  if (!pos1) {
    nsINode* child2 = parents2.ElementAt(--pos2);
    return aOffset1 <= parent->IndexOf(child2) ? -1 : 1;
  }

  nsINode* child1 = parents1.ElementAt(--pos1);
  return parent->IndexOf(child1) < aOffset2 ? -1 : 1;
}

inline PRBool
IsCharInSet(const char* aSet,
            const PRUnichar aChar)
{
  PRUnichar ch;
  while ((ch = *aSet)) {
    if (aChar == PRUnichar(ch)) {
      return PR_TRUE;
    }
    ++aSet;
  }
  return PR_FALSE;
}






const nsDependentSubstring
nsContentUtils::TrimCharsInSet(const char* aSet,
                               const nsAString& aValue)
{
  nsAString::const_iterator valueCurrent, valueEnd;

  aValue.BeginReading(valueCurrent);
  aValue.EndReading(valueEnd);

  
  while (valueCurrent != valueEnd) {
    if (!IsCharInSet(aSet, *valueCurrent)) {
      break;
    }
    ++valueCurrent;
  }

  if (valueCurrent != valueEnd) {
    for (;;) {
      --valueEnd;
      if (!IsCharInSet(aSet, *valueEnd)) {
        break;
      }
    }
    ++valueEnd; 
  }

  
  return Substring(valueCurrent, valueEnd);
}






template<PRBool IsWhitespace(PRUnichar)>
const nsDependentSubstring
nsContentUtils::TrimWhitespace(const nsAString& aStr, PRBool aTrimTrailing)
{
  nsAString::const_iterator start, end;

  aStr.BeginReading(start);
  aStr.EndReading(end);

  
  while (start != end && IsWhitespace(*start)) {
    ++start;
  }

  if (aTrimTrailing) {
    
    while (end != start) {
      --end;

      if (!IsWhitespace(*end)) {
        
        ++end;

        break;
      }
    }
  }

  
  

  return Substring(start, end);
}




template
const nsDependentSubstring
nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(const nsAString&, PRBool);
template
const nsDependentSubstring
nsContentUtils::TrimWhitespace<nsContentUtils::IsHTMLWhitespace>(const nsAString&, PRBool);

static inline void KeyAppendSep(nsACString& aKey)
{
  if (!aKey.IsEmpty()) {
    aKey.Append('>');
  }
}

static inline void KeyAppendString(const nsAString& aString, nsACString& aKey)
{
  KeyAppendSep(aKey);

  
  

  AppendUTF16toUTF8(aString, aKey);
}

static inline void KeyAppendString(const nsACString& aString, nsACString& aKey)
{
  KeyAppendSep(aKey);

  
  

  aKey.Append(aString);
}

static inline void KeyAppendInt(PRInt32 aInt, nsACString& aKey)
{
  KeyAppendSep(aKey);

  aKey.Append(nsPrintfCString("%d", aInt));
}

static inline void KeyAppendAtom(nsIAtom* aAtom, nsACString& aKey)
{
  NS_PRECONDITION(aAtom, "KeyAppendAtom: aAtom can not be null!\n");

  KeyAppendString(nsAtomCString(aAtom), aKey);
}

static inline PRBool IsAutocompleteOff(const nsIContent* aElement)
{
  return aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::autocomplete,
                               NS_LITERAL_STRING("off"), eIgnoreCase);
}

 nsresult
nsContentUtils::GenerateStateKey(nsIContent* aContent,
                                 const nsIDocument* aDocument,
                                 nsIStatefulFrame::SpecialStateID aID,
                                 nsACString& aKey)
{
  aKey.Truncate();

  PRUint32 partID = aDocument ? aDocument->GetPartID() : 0;

  
  
  if (nsIStatefulFrame::eNoID != aID) {
    KeyAppendInt(partID, aKey);  
    KeyAppendInt(aID, aKey);
    return NS_OK;
  }

  
  NS_ENSURE_TRUE(aContent, NS_ERROR_FAILURE);

  
  if (aContent->IsInAnonymousSubtree()) {
    return NS_OK;
  }

  if (IsAutocompleteOff(aContent)) {
    return NS_OK;
  }

  nsCOMPtr<nsIHTMLDocument> htmlDocument(do_QueryInterface(aContent->GetCurrentDoc()));

  KeyAppendInt(partID, aKey);  
  
  
  KeyAppendInt(nsIStatefulFrame::eNoID, aKey);
  PRBool generatedUniqueKey = PR_FALSE;

  if (htmlDocument) {
    
    
    
    
    aContent->GetCurrentDoc()->FlushPendingNotifications(Flush_Content);

    nsContentList *htmlForms = htmlDocument->GetForms();
    nsContentList *htmlFormControls = htmlDocument->GetFormControls();

    NS_ENSURE_TRUE(htmlForms && htmlFormControls, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIFormControl> control(do_QueryInterface(aContent));
    if (control && htmlFormControls && htmlForms) {

      
      KeyAppendInt(control->GetType(), aKey);

      
      PRInt32 index = -1;
      Element *formElement = control->GetFormElement();
      if (formElement) {
        if (IsAutocompleteOff(formElement)) {
          aKey.Truncate();
          return NS_OK;
        }

        KeyAppendString(NS_LITERAL_CSTRING("f"), aKey);

        
        index = htmlForms->IndexOf(formElement, PR_FALSE);
        if (index <= -1) {
          
          
          
          
          
          
          
          index = htmlDocument->GetNumFormsSynchronous() - 1;
        }
        if (index > -1) {
          KeyAppendInt(index, aKey);

          
          nsCOMPtr<nsIForm> form(do_QueryInterface(formElement));
          index = form->IndexOfControl(control);

          if (index > -1) {
            KeyAppendInt(index, aKey);
            generatedUniqueKey = PR_TRUE;
          }
        }

        
        nsAutoString formName;
        formElement->GetAttr(kNameSpaceID_None, nsGkAtoms::name, formName);
        KeyAppendString(formName, aKey);

      } else {

        KeyAppendString(NS_LITERAL_CSTRING("d"), aKey);

        
        

        
        

        
        
        index = htmlFormControls->IndexOf(aContent, PR_TRUE);
        if (index > -1) {
          KeyAppendInt(index, aKey);
          generatedUniqueKey = PR_TRUE;
        }
      }

      
      nsAutoString name;
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
      KeyAppendString(name, aKey);
    }
  }

  if (!generatedUniqueKey) {
    
    
    
    if (aContent->IsElement()) {
      KeyAppendString(nsDependentAtomString(aContent->Tag()), aKey);
    }
    else {
      
      
      KeyAppendString(NS_LITERAL_CSTRING("o"), aKey);
    }

    
    
    
    nsINode* parent = aContent->GetNodeParent();
    nsINode* content = aContent;
    while (parent) {
      KeyAppendInt(parent->IndexOf(content), aKey);
      content = parent;
      parent = content->GetNodeParent();
    }
  }

  return NS_OK;
}


nsresult
nsContentUtils::NewURIWithDocumentCharset(nsIURI** aResult,
                                          const nsAString& aSpec,
                                          nsIDocument* aDocument,
                                          nsIURI* aBaseURI)
{
  return NS_NewURI(aResult, aSpec,
                   aDocument ? aDocument->GetDocumentCharacterSet().get() : nsnull,
                   aBaseURI, sIOService);
}


PRBool
nsContentUtils::BelongsInForm(nsIContent *aForm,
                              nsIContent *aContent)
{
  NS_PRECONDITION(aForm, "Must have a form");
  NS_PRECONDITION(aContent, "Must have a content node");

  if (aForm == aContent) {
    

    return PR_FALSE;
  }

  nsIContent* content = aContent->GetParent();

  while (content) {
    if (content == aForm) {
      

      return PR_TRUE;
    }

    if (content->Tag() == nsGkAtoms::form &&
        content->IsHTML()) {
      
      

      return PR_FALSE;
    }

    content = content->GetParent();
  }

  if (aForm->GetChildCount() > 0) {
    
    

    return PR_FALSE;
  }

  
  
  
  
  if (PositionIsBefore(aForm, aContent)) {
    
    
    
    
    return PR_TRUE;
  }

  return PR_FALSE;
}


nsresult
nsContentUtils::CheckQName(const nsAString& aQualifiedName,
                           PRBool aNamespaceAware)
{
  nsIParserService *parserService = GetParserService();
  NS_ENSURE_TRUE(parserService, NS_ERROR_FAILURE);

  const PRUnichar *colon;
  return parserService->CheckQName(PromiseFlatString(aQualifiedName),
                                   aNamespaceAware, &colon);
}


nsresult
nsContentUtils::SplitQName(const nsIContent* aNamespaceResolver,
                           const nsAFlatString& aQName,
                           PRInt32 *aNamespace, nsIAtom **aLocalName)
{
  nsIParserService* parserService = GetParserService();
  NS_ENSURE_TRUE(parserService, NS_ERROR_FAILURE);

  const PRUnichar* colon;
  nsresult rv = parserService->CheckQName(aQName, PR_TRUE, &colon);
  NS_ENSURE_SUCCESS(rv, rv);

  if (colon) {
    const PRUnichar* end;
    aQName.EndReading(end);
    nsAutoString nameSpace;
    rv = aNamespaceResolver->LookupNamespaceURI(Substring(aQName.get(), colon),
                                                nameSpace);
    NS_ENSURE_SUCCESS(rv, rv);

    *aNamespace = NameSpaceManager()->GetNameSpaceID(nameSpace);
    if (*aNamespace == kNameSpaceID_Unknown)
      return NS_ERROR_FAILURE;

    *aLocalName = NS_NewAtom(Substring(colon + 1, end));
  }
  else {
    *aNamespace = kNameSpaceID_None;
    *aLocalName = NS_NewAtom(aQName);
  }
  NS_ENSURE_TRUE(aLocalName, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}


nsresult
nsContentUtils::GetNodeInfoFromQName(const nsAString& aNamespaceURI,
                                     const nsAString& aQualifiedName,
                                     nsNodeInfoManager* aNodeInfoManager,
                                     nsINodeInfo** aNodeInfo)
{
  nsIParserService* parserService = GetParserService();
  NS_ENSURE_TRUE(parserService, NS_ERROR_FAILURE);

  const nsAFlatString& qName = PromiseFlatString(aQualifiedName);
  const PRUnichar* colon;
  nsresult rv = parserService->CheckQName(qName, PR_TRUE, &colon);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 nsID;
  sNameSpaceManager->RegisterNameSpace(aNamespaceURI, nsID);
  if (colon) {
    const PRUnichar* end;
    qName.EndReading(end);

    nsCOMPtr<nsIAtom> prefix = do_GetAtom(Substring(qName.get(), colon));

    rv = aNodeInfoManager->GetNodeInfo(Substring(colon + 1, end), prefix,
                                       nsID, aNodeInfo);
  }
  else {
    rv = aNodeInfoManager->GetNodeInfo(aQualifiedName, nsnull, nsID,
                                       aNodeInfo);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return nsContentUtils::IsValidNodeName((*aNodeInfo)->NameAtom(),
                                         (*aNodeInfo)->GetPrefixAtom(),
                                         (*aNodeInfo)->NamespaceID()) ?
         NS_OK : NS_ERROR_DOM_NAMESPACE_ERR;
}


void
nsContentUtils::SplitExpatName(const PRUnichar *aExpatName, nsIAtom **aPrefix,
                               nsIAtom **aLocalName, PRInt32* aNameSpaceID)
{
  









  const PRUnichar *uriEnd = nsnull;
  const PRUnichar *nameEnd = nsnull;
  const PRUnichar *pos;
  for (pos = aExpatName; *pos; ++pos) {
    if (*pos == 0xFFFF) {
      if (uriEnd) {
        nameEnd = pos;
      }
      else {
        uriEnd = pos;
      }
    }
  }

  const PRUnichar *nameStart;
  if (uriEnd) {
    if (sNameSpaceManager) {
      sNameSpaceManager->RegisterNameSpace(nsDependentSubstring(aExpatName,
                                                                uriEnd),
                                           *aNameSpaceID);
    }
    else {
      *aNameSpaceID = kNameSpaceID_Unknown;
    }

    nameStart = (uriEnd + 1);
    if (nameEnd)  {
      const PRUnichar *prefixStart = nameEnd + 1;
      *aPrefix = NS_NewAtom(Substring(prefixStart, pos));
    }
    else {
      nameEnd = pos;
      *aPrefix = nsnull;
    }
  }
  else {
    *aNameSpaceID = kNameSpaceID_None;
    nameStart = aExpatName;
    nameEnd = pos;
    *aPrefix = nsnull;
  }
  *aLocalName = NS_NewAtom(Substring(nameStart, nameEnd));
}


nsPresContext*
nsContentUtils::GetContextForContent(const nsIContent* aContent)
{
  nsIDocument* doc = aContent->GetCurrentDoc();
  if (doc) {
    nsIPresShell *presShell = doc->GetShell();
    if (presShell) {
      return presShell->GetPresContext();
    }
  }
  return nsnull;
}


PRBool
nsContentUtils::CanLoadImage(nsIURI* aURI, nsISupports* aContext,
                             nsIDocument* aLoadingDocument,
                             nsIPrincipal* aLoadingPrincipal,
                             PRInt16* aImageBlockingStatus)
{
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aLoadingDocument, "Must have a document");
  NS_PRECONDITION(aLoadingPrincipal, "Must have a loading principal");

  nsresult rv;

  PRUint32 appType = nsIDocShell::APP_TYPE_UNKNOWN;

  {
    nsCOMPtr<nsISupports> container = aLoadingDocument->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem =
      do_QueryInterface(container);

    if (docShellTreeItem) {
      nsCOMPtr<nsIDocShellTreeItem> root;
      docShellTreeItem->GetRootTreeItem(getter_AddRefs(root));

      nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(root));

      if (!docShell || NS_FAILED(docShell->GetAppType(&appType))) {
        appType = nsIDocShell::APP_TYPE_UNKNOWN;
      }
    }
  }

  if (appType != nsIDocShell::APP_TYPE_EDITOR) {
    
    
    
    rv = sSecurityManager->
      CheckLoadURIWithPrincipal(aLoadingPrincipal, aURI,
                                nsIScriptSecurityManager::ALLOW_CHROME);
    if (NS_FAILED(rv)) {
      if (aImageBlockingStatus) {
        
        
        *aImageBlockingStatus = nsIContentPolicy::REJECT_REQUEST;
      }
      return PR_FALSE;
    }
  }

  PRInt16 decision = nsIContentPolicy::ACCEPT;

  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_IMAGE,
                                 aURI,
                                 aLoadingPrincipal,
                                 aContext,
                                 EmptyCString(), 
                                 nsnull,         
                                 &decision,
                                 GetContentPolicy(),
                                 sSecurityManager);

  if (aImageBlockingStatus) {
    *aImageBlockingStatus =
      NS_FAILED(rv) ? nsIContentPolicy::REJECT_REQUEST : decision;
  }
  return NS_FAILED(rv) ? PR_FALSE : NS_CP_ACCEPTED(decision);
}


PRBool
nsContentUtils::IsImageInCache(nsIURI* aURI)
{
    if (!sImgLoaderInitialized)
        InitImgLoader();

    if (!sImgCache) return PR_FALSE;

    
    
    nsCOMPtr<nsIProperties> props;
    nsresult rv = sImgCache->FindEntryProperties(aURI, getter_AddRefs(props));
    return (NS_SUCCEEDED(rv) && props);
}


nsresult
nsContentUtils::LoadImage(nsIURI* aURI, nsIDocument* aLoadingDocument,
                          nsIPrincipal* aLoadingPrincipal, nsIURI* aReferrer,
                          imgIDecoderObserver* aObserver, PRInt32 aLoadFlags,
                          imgIRequest** aRequest)
{
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aLoadingDocument, "Must have a document");
  NS_PRECONDITION(aLoadingPrincipal, "Must have a principal");
  NS_PRECONDITION(aRequest, "Null out param");

  imgILoader* imgLoader = GetImgLoader();
  if (!imgLoader) {
    
    return NS_OK;
  }

  nsCOMPtr<nsILoadGroup> loadGroup = aLoadingDocument->GetDocumentLoadGroup();
  NS_ASSERTION(loadGroup, "Could not get loadgroup; onload may fire too early");

  nsIURI *documentURI = aLoadingDocument->GetDocumentURI();

  
  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  if (aLoadingPrincipal) {
    nsresult rv = aLoadingPrincipal->GetCsp(getter_AddRefs(csp));
    NS_ENSURE_SUCCESS(rv, rv);
    if (csp) {
      channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
      channelPolicy->SetContentSecurityPolicy(csp);
      channelPolicy->SetLoadType(nsIContentPolicy::TYPE_IMAGE);
    }
  }
    
  
  NS_TryToSetImmutable(aURI);

  
  
  return imgLoader->LoadImage(aURI,                 
                              documentURI,          
                              aReferrer,            
                              loadGroup,            
                              aObserver,            
                              aLoadingDocument,     
                              aLoadFlags,           
                              nsnull,               
                              nsnull,               
                              channelPolicy,        
                              aRequest);
}


already_AddRefed<imgIContainer>
nsContentUtils::GetImageFromContent(nsIImageLoadingContent* aContent,
                                    imgIRequest **aRequest)
{
  if (aRequest) {
    *aRequest = nsnull;
  }

  NS_ENSURE_TRUE(aContent, nsnull);

  nsCOMPtr<imgIRequest> imgRequest;
  aContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                      getter_AddRefs(imgRequest));
  if (!imgRequest) {
    return nsnull;
  }

  nsCOMPtr<imgIContainer> imgContainer;
  imgRequest->GetImage(getter_AddRefs(imgContainer));

  if (!imgContainer) {
    return nsnull;
  }

  if (aRequest) {
    imgRequest.swap(*aRequest);
  }

  return imgContainer.forget();
}


already_AddRefed<imgIRequest>
nsContentUtils::GetStaticRequest(imgIRequest* aRequest)
{
  NS_ENSURE_TRUE(aRequest, nsnull);
  nsCOMPtr<imgIRequest> retval;
  aRequest->GetStaticRequest(getter_AddRefs(retval));
  return retval.forget();
}


PRBool
nsContentUtils::ContentIsDraggable(nsIContent* aContent)
{
  nsCOMPtr<nsIDOMNSHTMLElement> htmlElement = do_QueryInterface(aContent);
  if (htmlElement) {
    PRBool draggable = PR_FALSE;
    htmlElement->GetDraggable(&draggable);
    if (draggable)
      return PR_TRUE;

    if (aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::draggable,
                              nsGkAtoms::_false, eIgnoreCase))
      return PR_FALSE;
  }

  
  return IsDraggableImage(aContent) || IsDraggableLink(aContent);
}


PRBool
nsContentUtils::IsDraggableImage(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "Must have content node to test");

  nsCOMPtr<nsIImageLoadingContent> imageContent(do_QueryInterface(aContent));
  if (!imageContent) {
    return PR_FALSE;
  }

  nsCOMPtr<imgIRequest> imgRequest;
  imageContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                           getter_AddRefs(imgRequest));

  
  
  return imgRequest != nsnull;
}


PRBool
nsContentUtils::IsDraggableLink(const nsIContent* aContent) {
  nsCOMPtr<nsIURI> absURI;
  return aContent->IsLink(getter_AddRefs(absURI));
}


nsAdoptingCString
nsContentUtils::GetCharPref(const char *aPref)
{
  nsAdoptingCString result;

  if (sPrefBranch) {
    sPrefBranch->GetCharPref(aPref, getter_Copies(result));
  }

  return result;
}


nsAdoptingString
nsContentUtils::GetLocalizedStringPref(const char *aPref)
{
  nsAdoptingString result;

  if (sPrefBranch) {
    nsCOMPtr<nsIPrefLocalizedString> prefLocalString;
    sPrefBranch->GetComplexValue(aPref, NS_GET_IID(nsIPrefLocalizedString),
                                 getter_AddRefs(prefLocalString));
    if (prefLocalString) {
      prefLocalString->GetData(getter_Copies(result));
    }
  }

  return result;
}


nsAdoptingString
nsContentUtils::GetStringPref(const char *aPref)
{
  nsAdoptingString result;

  if (sPrefBranch) {
    nsCOMPtr<nsISupportsString> theString;
    sPrefBranch->GetComplexValue(aPref, NS_GET_IID(nsISupportsString),
                                 getter_AddRefs(theString));
    if (theString) {
      theString->ToString(getter_Copies(result));
    }
  }

  return result;
}





void
nsContentUtils::RegisterPrefCallback(const char *aPref,
                                     PrefChangedFunc aCallback,
                                     void * aClosure)
{
  if (sPrefBranch) {
    if (!sPrefCallbackTable) {
      sPrefCallbackTable = 
        new nsRefPtrHashtable<nsPrefObserverHashKey, nsPrefOldCallback>();
      sPrefCallbackTable->Init();
    }

    nsPrefObserverHashKey hashKey(aPref, aCallback);
    nsRefPtr<nsPrefOldCallback> callback;
    sPrefCallbackTable->Get(&hashKey, getter_AddRefs(callback));
    if (callback) {
      callback->AppendClosure(aClosure);
      return;
    }

    callback = new nsPrefOldCallback(aPref, aCallback);
    callback->AppendClosure(aClosure);
    if (NS_SUCCEEDED(sPrefBranch->AddObserver(aPref, callback, PR_FALSE))) {
      sPrefCallbackTable->Put(callback, callback);
    }
  }
}


void
nsContentUtils::UnregisterPrefCallback(const char *aPref,
                                       PrefChangedFunc aCallback,
                                       void * aClosure)
{
  if (sPrefBranch) {
    if (!sPrefCallbackTable) {
      return;
    }

    nsPrefObserverHashKey hashKey(aPref, aCallback);
    nsRefPtr<nsPrefOldCallback> callback;
    sPrefCallbackTable->Get(&hashKey, getter_AddRefs(callback));

    if (callback) {
      callback->RemoveClosure(aClosure);
      if (callback->HasNoClosures()) {
        
        sPrefCallbackTable->Remove(callback);
      }
    }
  }
}

static int
BoolVarChanged(const char *aPref, void *aClosure)
{
  PrefCacheData* cache = static_cast<PrefCacheData*>(aClosure);
  *((PRBool*)cache->cacheLocation) =
    Preferences::GetBool(aPref, cache->defaultValueBool);

  return 0;
}

void
nsContentUtils::AddBoolPrefVarCache(const char *aPref,
                                    PRBool* aCache,
                                    PRBool aDefault)
{
  *aCache = Preferences::GetBool(aPref, aDefault);
  PrefCacheData* data = new PrefCacheData;
  data->cacheLocation = aCache;
  data->defaultValueBool = aDefault;
  sPrefCacheData->AppendElement(data);
  RegisterPrefCallback(aPref, BoolVarChanged, data);
}

static int
IntVarChanged(const char *aPref, void *aClosure)
{
  PrefCacheData* cache = static_cast<PrefCacheData*>(aClosure);
  *((PRInt32*)cache->cacheLocation) =
    Preferences::GetInt(aPref, cache->defaultValueInt);
  
  return 0;
}

void
nsContentUtils::AddIntPrefVarCache(const char *aPref,
                                   PRInt32* aCache,
                                   PRInt32 aDefault)
{
  *aCache = Preferences::GetInt(aPref, aDefault);
  PrefCacheData* data = new PrefCacheData;
  data->cacheLocation = aCache;
  data->defaultValueInt = aDefault;
  sPrefCacheData->AppendElement(data);
  RegisterPrefCallback(aPref, IntVarChanged, data);
}

PRBool
nsContentUtils::IsSitePermAllow(nsIURI* aURI, const char* aType)
{
  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService("@mozilla.org/permissionmanager;1");
  NS_ENSURE_TRUE(permMgr, PR_FALSE);

  PRUint32 perm;
  nsresult rv = permMgr->TestPermission(aURI, aType, &perm);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
  return perm == nsIPermissionManager::ALLOW_ACTION;
}

static const char *gEventNames[] = {"event"};
static const char *gSVGEventNames[] = {"evt"};



static const char *gOnErrorNames[] = {"event", "source", "lineno"};


void
nsContentUtils::GetEventArgNames(PRInt32 aNameSpaceID,
                                 nsIAtom *aEventName,
                                 PRUint32 *aArgCount,
                                 const char*** aArgArray)
{
#define SET_EVENT_ARG_NAMES(names) \
    *aArgCount = sizeof(names)/sizeof(names[0]); \
    *aArgArray = names;

  
  
  
  if (aEventName == nsGkAtoms::onerror) {
    SET_EVENT_ARG_NAMES(gOnErrorNames);
  } else if (aNameSpaceID == kNameSpaceID_SVG) {
    SET_EVENT_ARG_NAMES(gSVGEventNames);
  } else {
    SET_EVENT_ARG_NAMES(gEventNames);
  }
}

nsCxPusher::nsCxPusher()
    : mScriptIsRunning(PR_FALSE),
      mPushedSomething(PR_FALSE)
{
}

nsCxPusher::~nsCxPusher()
{
  Pop();
}

static PRBool
IsContextOnStack(nsIJSContextStack *aStack, JSContext *aContext)
{
  JSContext *ctx = nsnull;
  aStack->Peek(&ctx);
  if (!ctx)
    return PR_FALSE;
  if (ctx == aContext)
    return PR_TRUE;

  nsCOMPtr<nsIJSContextStackIterator>
    iterator(do_CreateInstance("@mozilla.org/js/xpc/ContextStackIterator;1"));
  NS_ENSURE_TRUE(iterator, PR_FALSE);

  nsresult rv = iterator->Reset(aStack);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool done;
  while (NS_SUCCEEDED(iterator->Done(&done)) && !done) {
    rv = iterator->Prev(&ctx);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Broken iterator implementation");

    if (!ctx) {
      continue;
    }

    if (nsJSUtils::GetDynamicScriptContext(ctx) && ctx == aContext)
      return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
nsCxPusher::Push(nsPIDOMEventTarget *aCurrentTarget)
{
  if (mPushedSomething) {
    NS_ERROR("Whaaa! No double pushing with nsCxPusher::Push()!");

    return PR_FALSE;
  }

  NS_ENSURE_TRUE(aCurrentTarget, PR_FALSE);
  nsresult rv;
  nsIScriptContext* scx =
    aCurrentTarget->GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  if (!scx) {
    
    JSContext* cx = aCurrentTarget->GetJSContextForEventHandlers();
    if (cx) {
      DoPush(cx);
    }

    
    
    return PR_TRUE;
  }

  JSContext* cx = nsnull;

  if (scx) {
    cx = static_cast<JSContext*>(scx->GetNativeContext());
    
    NS_ENSURE_TRUE(cx, PR_FALSE);
  }

  
  
  
  
  return Push(cx);
}

PRBool
nsCxPusher::RePush(nsPIDOMEventTarget *aCurrentTarget)
{
  if (!mPushedSomething) {
    return Push(aCurrentTarget);
  }

  if (aCurrentTarget) {
    nsresult rv;
    nsIScriptContext* scx =
      aCurrentTarget->GetContextForEventHandlers(&rv);
    if (NS_FAILED(rv)) {
      Pop();
      return PR_FALSE;
    }

    
    
    if (scx && scx == mScx &&
        scx->GetNativeContext()) {
      return PR_TRUE;
    }
  }

  Pop();
  return Push(aCurrentTarget);
}

PRBool
nsCxPusher::Push(JSContext *cx, PRBool aRequiresScriptContext)
{
  if (mPushedSomething) {
    NS_ERROR("Whaaa! No double pushing with nsCxPusher::Push()!");

    return PR_FALSE;
  }

  if (!cx) {
    return PR_FALSE;
  }

  
  
  
  mScx = GetScriptContextFromJSContext(cx);
  if (!mScx && aRequiresScriptContext) {
    
    return PR_TRUE;
  }

  return DoPush(cx);
}

PRBool
nsCxPusher::DoPush(JSContext* cx)
{
  nsIThreadJSContextStack* stack = nsContentUtils::ThreadJSContextStack();
  if (!stack) {
    return PR_TRUE;
  }

  if (cx && IsContextOnStack(stack, cx)) {
    
    
    mScriptIsRunning = PR_TRUE;
  }

  if (NS_FAILED(stack->Push(cx))) {
    mScriptIsRunning = PR_FALSE;
    mScx = nsnull;
    return PR_FALSE;
  }

  mPushedSomething = PR_TRUE;
#ifdef DEBUG
  mPushedContext = cx;
#endif
  return PR_TRUE;
}

PRBool
nsCxPusher::PushNull()
{
  return DoPush(nsnull);
}

void
nsCxPusher::Pop()
{
  nsIThreadJSContextStack* stack = nsContentUtils::ThreadJSContextStack();
  if (!mPushedSomething || !stack) {
    mScx = nsnull;
    mPushedSomething = PR_FALSE;

    NS_ASSERTION(!mScriptIsRunning, "Huh, this can't be happening, "
                 "mScriptIsRunning can't be set here!");

    return;
  }

  JSContext *unused;
  stack->Pop(&unused);

  NS_ASSERTION(unused == mPushedContext, "Unexpected context popped");

  if (!mScriptIsRunning && mScx) {
    
    

    mScx->ScriptEvaluated(PR_TRUE);
  }

  mScx = nsnull;
  mScriptIsRunning = PR_FALSE;
  mPushedSomething = PR_FALSE;
}

static const char gPropertiesFiles[nsContentUtils::PropertiesFile_COUNT][56] = {
  
  "chrome://global/locale/css.properties",
  "chrome://global/locale/xbl.properties",
  "chrome://global/locale/xul.properties",
  "chrome://global/locale/layout_errors.properties",
  "chrome://global/locale/layout/HtmlForm.properties",
  "chrome://global/locale/printing.properties",
  "chrome://global/locale/dom/dom.properties",
#ifdef MOZ_SVG
  "chrome://global/locale/svg/svg.properties",
#endif
  "chrome://branding/locale/brand.properties",
  "chrome://global/locale/commonDialogs.properties"
};

 nsresult
nsContentUtils::EnsureStringBundle(PropertiesFile aFile)
{
  if (!sStringBundles[aFile]) {
    if (!sStringBundleService) {
      nsresult rv =
        CallGetService(NS_STRINGBUNDLE_CONTRACTID, &sStringBundleService);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    nsIStringBundle *bundle;
    nsresult rv =
      sStringBundleService->CreateBundle(gPropertiesFiles[aFile], &bundle);
    NS_ENSURE_SUCCESS(rv, rv);
    sStringBundles[aFile] = bundle; 
  }
  return NS_OK;
}


nsresult nsContentUtils::GetLocalizedString(PropertiesFile aFile,
                                            const char* aKey,
                                            nsXPIDLString& aResult)
{
  nsresult rv = EnsureStringBundle(aFile);
  NS_ENSURE_SUCCESS(rv, rv);
  nsIStringBundle *bundle = sStringBundles[aFile];

  return bundle->GetStringFromName(NS_ConvertASCIItoUTF16(aKey).get(),
                                   getter_Copies(aResult));
}


nsresult nsContentUtils::FormatLocalizedString(PropertiesFile aFile,
                                               const char* aKey,
                                               const PRUnichar **aParams,
                                               PRUint32 aParamsLength,
                                               nsXPIDLString& aResult)
{
  nsresult rv = EnsureStringBundle(aFile);
  NS_ENSURE_SUCCESS(rv, rv);
  nsIStringBundle *bundle = sStringBundles[aFile];

  return bundle->FormatStringFromName(NS_ConvertASCIItoUTF16(aKey).get(),
                                      aParams, aParamsLength,
                                      getter_Copies(aResult));
}

 nsresult
nsContentUtils::ReportToConsole(PropertiesFile aFile,
                                const char *aMessageName,
                                const PRUnichar **aParams,
                                PRUint32 aParamsLength,
                                nsIURI* aURI,
                                const nsAFlatString& aSourceLine,
                                PRUint32 aLineNumber,
                                PRUint32 aColumnNumber,
                                PRUint32 aErrorFlags,
                                const char *aCategory,
                                PRUint64 aWindowId)
{
  NS_ASSERTION((aParams && aParamsLength) || (!aParams && !aParamsLength),
               "Supply either both parameters and their number or no"
               "parameters and 0.");

  nsresult rv;
  if (!sConsoleService) { 
    rv = CallGetService(NS_CONSOLESERVICE_CONTRACTID, &sConsoleService);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsXPIDLString errorText;
  if (aParams) {
    rv = FormatLocalizedString(aFile, aMessageName, aParams, aParamsLength,
                               errorText);
  }
  else {
    rv = GetLocalizedString(aFile, aMessageName, errorText);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  if (aURI)
    aURI->GetSpec(spec);

  nsCOMPtr<nsIScriptError2> errorObject =
      do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = errorObject->InitWithWindowID(errorText.get(),
                                     NS_ConvertUTF8toUTF16(spec).get(), 
                                     aSourceLine.get(),
                                     aLineNumber, aColumnNumber,
                                     aErrorFlags, aCategory, aWindowId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIScriptError> logError = do_QueryInterface(errorObject);
  return sConsoleService->LogMessage(logError);
}

 nsresult
nsContentUtils::ReportToConsole(PropertiesFile aFile,
                                const char *aMessageName,
                                const PRUnichar **aParams,
                                PRUint32 aParamsLength,
                                nsIURI* aURI,
                                const nsAFlatString& aSourceLine,
                                PRUint32 aLineNumber,
                                PRUint32 aColumnNumber,
                                PRUint32 aErrorFlags,
                                const char *aCategory,
                                nsIDocument* aDocument)
{
  nsIURI* uri = aURI;
  PRUint64 windowID = 0;
  if (aDocument) {
    if (!uri) {
      uri = aDocument->GetDocumentURI();
    }
    windowID = aDocument->OuterWindowID();
  }

  return ReportToConsole(aFile, aMessageName, aParams, aParamsLength, uri,
                         aSourceLine, aLineNumber, aColumnNumber, aErrorFlags,
                         aCategory, windowID);
}

PRBool
nsContentUtils::IsChromeDoc(nsIDocument *aDocument)
{
  if (!aDocument) {
    return PR_FALSE;
  }
  
  nsCOMPtr<nsIPrincipal> systemPrincipal;
  sSecurityManager->GetSystemPrincipal(getter_AddRefs(systemPrincipal));

  return aDocument->NodePrincipal() == systemPrincipal;
}

PRBool
nsContentUtils::IsChildOfSameType(nsIDocument* aDoc)
{
  nsCOMPtr<nsISupports> container = aDoc->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(container));
  nsCOMPtr<nsIDocShellTreeItem> sameTypeParent;
  if (docShellAsItem) {
    docShellAsItem->GetSameTypeParent(getter_AddRefs(sameTypeParent));
  }
  return sameTypeParent != nsnull;
}

PRBool
nsContentUtils::GetWrapperSafeScriptFilename(nsIDocument *aDocument,
                                             nsIURI *aURI,
                                             nsACString& aScriptURI)
{
  PRBool scriptFileNameModified = PR_FALSE;
  aURI->GetSpec(aScriptURI);

  if (IsChromeDoc(aDocument)) {
    nsCOMPtr<nsIChromeRegistry> chromeReg =
      mozilla::services::GetChromeRegistryService();

    if (!chromeReg) {
      
      

      return scriptFileNameModified;
    }

    PRBool docWrappersEnabled =
      chromeReg->WrappersEnabled(aDocument->GetDocumentURI());

    PRBool uriWrappersEnabled = chromeReg->WrappersEnabled(aURI);

    nsIURI *docURI = aDocument->GetDocumentURI();

    if (docURI && docWrappersEnabled && !uriWrappersEnabled) {
      
      
      
      
      
      
      nsCAutoString spec;
      docURI->GetSpec(spec);
      spec.AppendASCII(" -> ");
      spec.Append(aScriptURI);

      aScriptURI = spec;

      scriptFileNameModified = PR_TRUE;
    }
  }

  return scriptFileNameModified;
}


PRBool
nsContentUtils::IsInChromeDocshell(nsIDocument *aDocument)
{
  if (!aDocument) {
    return PR_FALSE;
  }

  if (aDocument->GetDisplayDocument()) {
    return IsInChromeDocshell(aDocument->GetDisplayDocument());
  }

  nsCOMPtr<nsISupports> docContainer = aDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryInterface(docContainer));
  PRInt32 itemType = nsIDocShellTreeItem::typeContent;
  if (docShell) {
    docShell->GetItemType(&itemType);
  }

  return itemType == nsIDocShellTreeItem::typeChrome;
}


nsIContentPolicy*
nsContentUtils::GetContentPolicy()
{
  if (!sTriedToGetContentPolicy) {
    CallGetService(NS_CONTENTPOLICY_CONTRACTID, &sContentPolicyService);
    
    sTriedToGetContentPolicy = PR_TRUE;
  }

  return sContentPolicyService;
}


PRBool
nsContentUtils::IsEventAttributeName(nsIAtom* aName, PRInt32 aType)
{
  const PRUnichar* name = aName->GetUTF16String();
  if (name[0] != 'o' || name[1] != 'n')
    return PR_FALSE;

  EventNameMapping mapping;
  return (sAtomEventTable->Get(aName, &mapping) && mapping.mType & aType);
}


PRUint32
nsContentUtils::GetEventId(nsIAtom* aName)
{
  EventNameMapping mapping;
  if (sAtomEventTable->Get(aName, &mapping))
    return mapping.mId;

  return NS_USER_DEFINED_EVENT;
}


PRUint32
nsContentUtils::GetEventCategory(const nsAString& aName)
{
  EventNameMapping mapping;
  if (sStringEventTable->Get(aName, &mapping))
    return mapping.mStructType;

  return NS_EVENT;
}

nsIAtom*
nsContentUtils::GetEventIdAndAtom(const nsAString& aName,
                                  PRUint32 aEventStruct,
                                  PRUint32* aEventID)
{
  EventNameMapping mapping;
  if (sStringEventTable->Get(aName, &mapping)) {
    *aEventID =
      mapping.mStructType == aEventStruct ? mapping.mId : NS_USER_DEFINED_EVENT;
    return mapping.mAtom;
  }

  
  if (sUserDefinedEvents->Count() > 127) {
    while (sUserDefinedEvents->Count() > 64) {
      nsIAtom* first = sUserDefinedEvents->ObjectAt(0);
      sStringEventTable->Remove(Substring(nsDependentAtomString(first), 2));
      sUserDefinedEvents->RemoveObjectAt(0);
    }
  }

  *aEventID = NS_USER_DEFINED_EVENT;
  nsCOMPtr<nsIAtom> atom = do_GetAtom(NS_LITERAL_STRING("on") + aName);
  sUserDefinedEvents->AppendObject(atom);
  mapping.mAtom = atom;
  mapping.mId = NS_USER_DEFINED_EVENT;
  mapping.mType = EventNameType_None;
  mapping.mStructType = NS_EVENT_NULL;
  sStringEventTable->Put(aName, mapping);
  return mapping.mAtom;
}

static
nsresult GetEventAndTarget(nsIDocument* aDoc, nsISupports* aTarget,
                           const nsAString& aEventName,
                           PRBool aCanBubble, PRBool aCancelable,
                           nsIDOMEvent** aEvent,
                           nsIDOMEventTarget** aTargetOut)
{
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aDoc);
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(aTarget));
  NS_ENSURE_TRUE(domDoc && target, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv =
    domDoc->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
  NS_ENSURE_TRUE(privateEvent, NS_ERROR_FAILURE);

  rv = event->InitEvent(aEventName, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = privateEvent->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = privateEvent->SetTarget(target);
  NS_ENSURE_SUCCESS(rv, rv);

  event.forget(aEvent);
  target.forget(aTargetOut);
  return NS_OK;
}


nsresult
nsContentUtils::DispatchTrustedEvent(nsIDocument* aDoc, nsISupports* aTarget,
                                     const nsAString& aEventName,
                                     PRBool aCanBubble, PRBool aCancelable,
                                     PRBool *aDefaultAction)
{
  nsCOMPtr<nsIDOMEvent> event;
  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult rv = GetEventAndTarget(aDoc, aTarget, aEventName, aCanBubble,
                                  aCancelable, getter_AddRefs(event),
                                  getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dummy;
  return target->DispatchEvent(event, aDefaultAction ? aDefaultAction : &dummy);
}

nsresult
nsContentUtils::DispatchChromeEvent(nsIDocument *aDoc,
                                    nsISupports *aTarget,
                                    const nsAString& aEventName,
                                    PRBool aCanBubble, PRBool aCancelable,
                                    PRBool *aDefaultAction)
{

  nsCOMPtr<nsIDOMEvent> event;
  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult rv = GetEventAndTarget(aDoc, aTarget, aEventName, aCanBubble,
                                  aCancelable, getter_AddRefs(event),
                                  getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(aDoc, "GetEventAndTarget lied?");
  if (!aDoc->GetWindow())
    return NS_ERROR_INVALID_ARG;

  nsPIDOMEventTarget* piTarget = aDoc->GetWindow()->GetChromeEventHandler();
  if (!piTarget)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIFrameLoaderOwner> flo = do_QueryInterface(piTarget);
  if (flo) {
    nsRefPtr<nsFrameLoader> fl = flo->GetFrameLoader();
    if (fl) {
      nsPIDOMEventTarget* t = fl->GetTabChildGlobalAsEventTarget();
      piTarget = t ? t : piTarget;
    }
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  rv = piTarget->DispatchDOMEvent(nsnull, event, nsnull, &status);
  if (aDefaultAction) {
    *aDefaultAction = (status != nsEventStatus_eConsumeNoDefault);
  }
  return rv;
}


Element*
nsContentUtils::MatchElementId(nsIContent *aContent, const nsIAtom* aId)
{
  for (nsIContent* cur = aContent;
       cur;
       cur = cur->GetNextNode(aContent)) {
    if (aId == cur->GetID()) {
      return cur->AsElement();
    }
  }

  return nsnull;
}


Element *
nsContentUtils::MatchElementId(nsIContent *aContent, const nsAString& aId)
{
  NS_PRECONDITION(!aId.IsEmpty(), "Will match random elements");
  
  
  nsCOMPtr<nsIAtom> id(do_GetAtom(aId));
  if (!id) {
    
    return nsnull;
  }

  return MatchElementId(aContent, id);
}



nsresult
nsContentUtils::ConvertStringFromCharset(const nsACString& aCharset,
                                         const nsACString& aInput,
                                         nsAString& aOutput)
{
  if (aCharset.IsEmpty()) {
    
    CopyUTF8toUTF16(aInput, aOutput);
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIUnicodeDecoder> decoder;
  rv = ccm->GetUnicodeDecoder(PromiseFlatCString(aCharset).get(),
                              getter_AddRefs(decoder));
  if (NS_FAILED(rv))
    return rv;

  nsPromiseFlatCString flatInput(aInput);
  PRInt32 srcLen = flatInput.Length();
  PRInt32 dstLen;
  rv = decoder->GetMaxLength(flatInput.get(), srcLen, &dstLen);
  if (NS_FAILED(rv))
    return rv;

  PRUnichar *ustr = (PRUnichar *)nsMemory::Alloc((dstLen + 1) *
                                                 sizeof(PRUnichar));
  if (!ustr)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = decoder->Convert(flatInput.get(), &srcLen, ustr, &dstLen);
  if (NS_SUCCEEDED(rv)) {
    ustr[dstLen] = 0;
    aOutput.Assign(ustr, dstLen);
  }

  nsMemory::Free(ustr);
  return rv;
}


PRBool
nsContentUtils::CheckForBOM(const unsigned char* aBuffer, PRUint32 aLength,
                            nsACString& aCharset, PRBool *bigEndian)
{
  PRBool found = PR_TRUE;
  aCharset.Truncate();
  if (aLength >= 3 &&
      aBuffer[0] == 0xEF &&
      aBuffer[1] == 0xBB &&
      aBuffer[2] == 0xBF) {
    aCharset = "UTF-8";
  }
  else if (aLength >= 2 &&
           aBuffer[0] == 0xFE && aBuffer[1] == 0xFF) {
    aCharset = "UTF-16";
    if (bigEndian)
      *bigEndian = PR_TRUE;
  }
  else if (aLength >= 2 &&
           aBuffer[0] == 0xFF && aBuffer[1] == 0xFE) {
    aCharset = "UTF-16";
    if (bigEndian)
      *bigEndian = PR_FALSE;
  } else {
    found = PR_FALSE;
  }

  return found;
}


void
nsContentUtils::RegisterShutdownObserver(nsIObserver* aObserver)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->AddObserver(aObserver, 
                                 NS_XPCOM_SHUTDOWN_OBSERVER_ID, 
                                 PR_FALSE);
  }
}


void
nsContentUtils::UnregisterShutdownObserver(nsIObserver* aObserver)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->RemoveObserver(aObserver, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  }
}


PRBool
nsContentUtils::HasNonEmptyAttr(const nsIContent* aContent, PRInt32 aNameSpaceID,
                                nsIAtom* aName)
{
  static nsIContent::AttrValuesArray strings[] = {&nsGkAtoms::_empty, nsnull};
  return aContent->FindAttrValueIn(aNameSpaceID, aName, strings, eCaseMatters)
    == nsIContent::ATTR_VALUE_NO_MATCH;
}


PRBool
nsContentUtils::HasMutationListeners(nsINode* aNode,
                                     PRUint32 aType,
                                     nsINode* aTargetForSubtreeModified)
{
  nsIDocument* doc = aNode->GetOwnerDoc();
  if (!doc) {
    return PR_FALSE;
  }

  
  nsPIDOMWindow* window = doc->GetInnerWindow();
  
  
  if (window && !window->HasMutationListeners(aType)) {
    return PR_FALSE;
  }

  if (aNode->IsNodeOfType(nsINode::eCONTENT) &&
      static_cast<nsIContent*>(aNode)->IsInNativeAnonymousSubtree()) {
    return PR_FALSE;
  }

  doc->MayDispatchMutationEvent(aTargetForSubtreeModified);

  
  if (aNode->IsInDoc()) {
    nsCOMPtr<nsPIDOMEventTarget> piTarget(do_QueryInterface(window));
    if (piTarget) {
      nsIEventListenerManager* manager = piTarget->GetListenerManager(PR_FALSE);
      if (manager) {
        PRBool hasListeners = PR_FALSE;
        manager->HasMutationListeners(&hasListeners);
        if (hasListeners) {
          return PR_TRUE;
        }
      }
    }
  }

  
  
  
  while (aNode) {
    nsIEventListenerManager* manager = aNode->GetListenerManager(PR_FALSE);
    if (manager) {
      PRBool hasListeners = PR_FALSE;
      manager->HasMutationListeners(&hasListeners);
      if (hasListeners) {
        return PR_TRUE;
      }
    }

    if (aNode->IsNodeOfType(nsINode::eCONTENT)) {
      nsIContent* content = static_cast<nsIContent*>(aNode);
      nsIContent* insertionParent =
        doc->BindingManager()->GetInsertionParent(content);
      if (insertionParent) {
        aNode = insertionParent;
        continue;
      }
    }
    aNode = aNode->GetNodeParent();
  }

  return PR_FALSE;
}


PRBool
nsContentUtils::HasMutationListeners(nsIDocument* aDocument,
                                     PRUint32 aType)
{
  nsPIDOMWindow* window = aDocument ?
    aDocument->GetInnerWindow() : nsnull;

  
  
  return !window || window->HasMutationListeners(aType);
}

void
nsContentUtils::MaybeFireNodeRemoved(nsINode* aChild, nsINode* aParent,
                                     nsIDocument* aOwnerDoc)
{
  NS_PRECONDITION(aChild, "Missing child");
  NS_PRECONDITION(aChild->GetNodeParent() == aParent, "Wrong parent");
  NS_PRECONDITION(aChild->GetOwnerDoc() == aOwnerDoc, "Wrong owner-doc");

  
  
  
  
  
  
  
  
  
  
  
  NS_ASSERTION(aChild->IsNodeOfType(nsINode::eCONTENT) &&
               static_cast<nsIContent*>(aChild)->
                 IsInNativeAnonymousSubtree() ||
               IsSafeToRunScript() ||
               sDOMNodeRemovedSuppressCount,
               "Want to fire DOMNodeRemoved event, but it's not safe");

  
  
  
  
  if (!IsSafeToRunScript()) {
    return;
  }

  if (HasMutationListeners(aChild,
        NS_EVENT_BITS_MUTATION_NODEREMOVED, aParent)) {
    nsMutationEvent mutation(PR_TRUE, NS_MUTATION_NODEREMOVED);
    mutation.mRelatedNode = do_QueryInterface(aParent);

    mozAutoSubtreeModified subtree(aOwnerDoc, aParent);
    nsEventDispatcher::Dispatch(aChild, nsnull, &mutation);
  }
}


void
nsContentUtils::TraverseListenerManager(nsINode *aNode,
                                        nsCycleCollectionTraversalCallback &cb)
{
  if (!sEventListenerManagersHash.ops) {
    
    return;
  }

  EventListenerManagerMapEntry *entry =
    static_cast<EventListenerManagerMapEntry *>
               (PL_DHashTableOperate(&sEventListenerManagersHash, aNode,
                                        PL_DHASH_LOOKUP));
  if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via hash] mListenerManager");
    cb.NoteXPCOMChild(entry->mListenerManager);
  }
}

nsIEventListenerManager*
nsContentUtils::GetListenerManager(nsINode *aNode,
                                   PRBool aCreateIfNotFound)
{
  if (!aCreateIfNotFound && !aNode->HasFlag(NODE_HAS_LISTENERMANAGER)) {
    return nsnull;
  }
  
  if (!sEventListenerManagersHash.ops) {
    
    

    return nsnull;
  }

  if (!aCreateIfNotFound) {
    EventListenerManagerMapEntry *entry =
      static_cast<EventListenerManagerMapEntry *>
                 (PL_DHashTableOperate(&sEventListenerManagersHash, aNode,
                                          PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      return entry->mListenerManager;
    }
    return nsnull;
  }

  EventListenerManagerMapEntry *entry =
    static_cast<EventListenerManagerMapEntry *>
               (PL_DHashTableOperate(&sEventListenerManagersHash, aNode,
                                        PL_DHASH_ADD));

  if (!entry) {
    return nsnull;
  }

  if (!entry->mListenerManager) {
    nsresult rv =
      NS_NewEventListenerManager(getter_AddRefs(entry->mListenerManager));

    if (NS_FAILED(rv)) {
      PL_DHashTableRawRemove(&sEventListenerManagersHash, entry);

      return nsnull;
    }

    entry->mListenerManager->SetListenerTarget(aNode);

    aNode->SetFlags(NODE_HAS_LISTENERMANAGER);
  }

  return entry->mListenerManager;
}


void
nsContentUtils::RemoveListenerManager(nsINode *aNode)
{
  if (sEventListenerManagersHash.ops) {
    EventListenerManagerMapEntry *entry =
      static_cast<EventListenerManagerMapEntry *>
                 (PL_DHashTableOperate(&sEventListenerManagersHash, aNode,
                                          PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      nsCOMPtr<nsIEventListenerManager> listenerManager;
      listenerManager.swap(entry->mListenerManager);
      
      
      PL_DHashTableRawRemove(&sEventListenerManagersHash, entry);
      if (listenerManager) {
        listenerManager->Disconnect();
      }
    }
  }
}


PRBool
nsContentUtils::IsValidNodeName(nsIAtom *aLocalName, nsIAtom *aPrefix,
                                PRInt32 aNamespaceID)
{
  if (aNamespaceID == kNameSpaceID_Unknown) {
    return PR_FALSE;
  }

  if (!aPrefix) {
    
    
    return (aLocalName == nsGkAtoms::xmlns) ==
           (aNamespaceID == kNameSpaceID_XMLNS);
  }

  
  if (aNamespaceID == kNameSpaceID_None) {
    return PR_FALSE;
  }

  
  
  if (aNamespaceID == kNameSpaceID_XMLNS) {
    return aPrefix == nsGkAtoms::xmlns && aLocalName != nsGkAtoms::xmlns;
  }

  
  
  
  
  return aPrefix != nsGkAtoms::xmlns &&
         (aNamespaceID == kNameSpaceID_XML || aPrefix != nsGkAtoms::xml);
}


nsresult
nsContentUtils::CreateContextualFragment(nsINode* aContextNode,
                                         const nsAString& aFragment,
                                         PRBool aWillOwnFragment,
                                         nsIDOMDocumentFragment** aReturn)
{
  *aReturn = nsnull;
  NS_ENSURE_ARG(aContextNode);

  nsresult rv;

  
  
  nsCOMPtr<nsIDocument> document = aContextNode->GetOwnerDoc();
  NS_ENSURE_TRUE(document, NS_ERROR_NOT_AVAILABLE);

  PRBool isHTML = document->IsHTML();
#ifdef DEBUG
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(document);
  NS_ASSERTION(!isHTML || htmlDoc, "Should have HTMLDocument here!");
#endif

  if (isHTML && nsHtml5Module::sEnabled) {
    
    
    nsCOMPtr<nsIParser> parser = document->GetFragmentParser();
    if (parser) {
      
      parser->Reset();
    }
    else {
      
      parser = nsHtml5Module::NewHtml5Parser();
      if (!parser) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    nsCOMPtr<nsIDOMDocumentFragment> frag;
    rv = NS_NewDocumentFragment(getter_AddRefs(frag), document->NodeInfoManager());
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIContent> contextAsContent = do_QueryInterface(aContextNode);
    if (contextAsContent && !contextAsContent->IsElement()) {
      contextAsContent = contextAsContent->GetParent();
      if (contextAsContent && !contextAsContent->IsElement()) {
        
        contextAsContent = nsnull;
      }
    }
    
    nsAHtml5FragmentParser* asFragmentParser =
        static_cast<nsAHtml5FragmentParser*> (parser.get());
    nsCOMPtr<nsIContent> fragment = do_QueryInterface(frag);
    if (contextAsContent &&
        !(nsGkAtoms::html == contextAsContent->Tag() &&
          contextAsContent->IsHTML())) {
      asFragmentParser->ParseHtml5Fragment(aFragment,
                                           fragment,
                                           contextAsContent->Tag(),
                                           contextAsContent->GetNameSpaceID(),
                                           (document->GetCompatibilityMode() ==
                                               eCompatibility_NavQuirks),
                                           PR_FALSE);
    } else {
      asFragmentParser->ParseHtml5Fragment(aFragment,
                                           fragment,
                                           nsGkAtoms::body,
                                           kNameSpaceID_XHTML,
                                           (document->GetCompatibilityMode() ==
                                               eCompatibility_NavQuirks),
                                           PR_FALSE);
    }
  
    frag.swap(*aReturn);
    document->SetFragmentParser(parser);
    return NS_OK;
  }

  nsAutoTArray<nsString, 32> tagStack;
  nsAutoString uriStr, nameStr;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aContextNode);
  
  if (content && !content->IsElement())
    content = content->GetParent();

  while (content && content->IsElement()) {
    nsString& tagName = *tagStack.AppendElement();
    NS_ENSURE_TRUE(&tagName, NS_ERROR_OUT_OF_MEMORY);

    tagName = content->NodeInfo()->QualifiedName();

    
    PRUint32 count = content->GetAttrCount();
    PRBool setDefaultNamespace = PR_FALSE;
    if (count > 0) {
      PRUint32 index;

      for (index = 0; index < count; index++) {
        const nsAttrName* name = content->GetAttrNameAt(index);
        if (name->NamespaceEquals(kNameSpaceID_XMLNS)) {
          content->GetAttr(kNameSpaceID_XMLNS, name->LocalName(), uriStr);

          
          tagName.Append(NS_LITERAL_STRING(" xmlns")); 
          if (name->GetPrefix()) {
            tagName.Append(PRUnichar(':'));
            name->LocalName()->ToString(nameStr);
            tagName.Append(nameStr);
          } else {
            setDefaultNamespace = PR_TRUE;
          }
          tagName.Append(NS_LITERAL_STRING("=\"") + uriStr +
            NS_LITERAL_STRING("\""));
        }
      }
    }

    if (!setDefaultNamespace) {
      nsINodeInfo* info = content->NodeInfo();
      if (!info->GetPrefixAtom() &&
          info->NamespaceID() != kNameSpaceID_None) {
        
        
        
        info->GetNamespaceURI(uriStr);
        tagName.Append(NS_LITERAL_STRING(" xmlns=\"") + uriStr +
                       NS_LITERAL_STRING("\""));
      }
    }

    content = content->GetParent();
  }

  nsCAutoString contentType;
  nsAutoString buf;
  document->GetContentType(buf);
  LossyCopyUTF16toASCII(buf, contentType);

  
  
  nsCOMPtr<nsIParser> parser = document->GetFragmentParser();
  if (parser) {
    
    parser->Reset();
  }
  else {
    
    parser = do_CreateInstance(kCParserCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIFragmentContentSink> sink;
  nsCOMPtr<nsIContentSink> contentsink = parser->GetContentSink();
  if (contentsink) {
    
    if (isHTML) {
      nsCOMPtr<nsIHTMLContentSink> htmlsink = do_QueryInterface(contentsink);
      sink = do_QueryInterface(htmlsink);
    }
    else {
      nsCOMPtr<nsIXMLContentSink> xmlsink = do_QueryInterface(contentsink);
      sink = do_QueryInterface(xmlsink);
    }
  }

  if (!sink) {
    
    
    if (isHTML) {
      rv = NS_NewHTMLFragmentContentSink(getter_AddRefs(sink));
    } else {
      rv = NS_NewXMLFragmentContentSink(getter_AddRefs(sink));
    }
    NS_ENSURE_SUCCESS(rv, rv);

    contentsink = do_QueryInterface(sink);
    NS_ASSERTION(contentsink, "Sink doesn't QI to nsIContentSink!");

    parser->SetContentSink(contentsink);
  }

  sink->SetTargetDocument(document);

  nsDTDMode mode = eDTDMode_autodetect;
  switch (document->GetCompatibilityMode()) {
    case eCompatibility_NavQuirks:
      mode = eDTDMode_quirks;
      break;
    case eCompatibility_AlmostStandards:
      mode = eDTDMode_almost_standards;
      break;
    case eCompatibility_FullStandards:
      mode = eDTDMode_full_standards;
      break;
    default:
      NS_NOTREACHED("unknown mode");
      break;
  }

  rv = parser->ParseFragment(aFragment, nsnull, tagStack,
                             !isHTML, contentType, mode);
  if (NS_SUCCEEDED(rv)) {
    rv = sink->GetFragment(aWillOwnFragment, aReturn);
  }

  document->SetFragmentParser(parser);

  return rv;
}


nsresult
nsContentUtils::CreateDocument(const nsAString& aNamespaceURI, 
                               const nsAString& aQualifiedName, 
                               nsIDOMDocumentType* aDoctype,
                               nsIURI* aDocumentURI, nsIURI* aBaseURI,
                               nsIPrincipal* aPrincipal,
                               nsIScriptGlobalObject* aEventObject,
                               nsIDOMDocument** aResult)
{
  nsresult rv = NS_NewDOMDocument(aResult, aNamespaceURI, aQualifiedName,
                                  aDoctype, aDocumentURI, aBaseURI, aPrincipal,
                                  PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> document = do_QueryInterface(*aResult);
  document->SetScriptHandlingObject(aEventObject);
  
  
  document->SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);
  return NS_OK;
}


nsresult
nsContentUtils::SetNodeTextContent(nsIContent* aContent,
                                   const nsAString& aValue,
                                   PRBool aTryReuse)
{
  
  nsCOMPtr<nsIContent> owningContent;

  
  mozAutoSubtreeModified subtree(nsnull, nsnull);

  
  
  {
    
    
    nsIDocument* doc = aContent->GetOwnerDoc();

    
    if (HasMutationListeners(doc, NS_EVENT_BITS_MUTATION_NODEREMOVED)) {
      subtree.UpdateTarget(doc, nsnull);
      owningContent = aContent;
      nsCOMPtr<nsINode> child;
      bool skipFirst = aTryReuse;
      for (child = aContent->GetFirstChild();
           child && child->GetNodeParent() == aContent;
           child = child->GetNextSibling()) {
        if (skipFirst && child->IsNodeOfType(nsINode::eTEXT)) {
          skipFirst = false;
          continue;
        }
        nsContentUtils::MaybeFireNodeRemoved(child, aContent, doc);
      }
    }
  }

  
  
  mozAutoDocUpdate updateBatch(aContent->GetCurrentDoc(),
    UPDATE_CONTENT_MODEL, PR_TRUE);

  PRUint32 childCount = aContent->GetChildCount();

  if (aTryReuse && !aValue.IsEmpty()) {
    PRUint32 removeIndex = 0;

    
    for (PRUint32 i = 0; i < childCount; ++i) {
      nsIContent* child = aContent->GetChildAt(removeIndex);
      if (removeIndex == 0 && child && child->IsNodeOfType(nsINode::eTEXT)) {
        nsresult rv = child->SetText(aValue, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);

        removeIndex = 1;
      }
      else {
        aContent->RemoveChildAt(removeIndex, PR_TRUE);
      }
    }

    if (removeIndex == 1) {
      return NS_OK;
    }
  }
  else {
    
    for (PRUint32 i = childCount; i-- != 0; ) {
      aContent->RemoveChildAt(i, PR_TRUE);
    }
  }

  if (aValue.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> textContent;
  nsresult rv = NS_NewTextNode(getter_AddRefs(textContent),
                               aContent->NodeInfo()->NodeInfoManager());
  NS_ENSURE_SUCCESS(rv, rv);

  textContent->SetText(aValue, PR_TRUE);

  return aContent->AppendChildTo(textContent, PR_TRUE);
}

static void AppendNodeTextContentsRecurse(nsINode* aNode, nsAString& aResult)
{
  nsIContent* child;
  PRUint32 i;
  for (i = 0; (child = aNode->GetChildAt(i)); ++i) {
    if (child->IsElement()) {
      AppendNodeTextContentsRecurse(child, aResult);
    }
    else if (child->IsNodeOfType(nsINode::eTEXT)) {
      child->AppendTextTo(aResult);
    }
  }
}


void
nsContentUtils::AppendNodeTextContent(nsINode* aNode, PRBool aDeep,
                                      nsAString& aResult)
{
  if (aNode->IsNodeOfType(nsINode::eTEXT)) {
    static_cast<nsIContent*>(aNode)->AppendTextTo(aResult);
  }
  else if (aDeep) {
    AppendNodeTextContentsRecurse(aNode, aResult);
  }
  else {
    nsIContent* child;
    PRUint32 i;
    for (i = 0; (child = aNode->GetChildAt(i)); ++i) {
      if (child->IsNodeOfType(nsINode::eTEXT)) {
        child->AppendTextTo(aResult);
      }
    }
  }
}

PRBool
nsContentUtils::HasNonEmptyTextContent(nsINode* aNode)
{
  nsIContent* child;
  PRUint32 i;
  for (i = 0; (child = aNode->GetChildAt(i)); ++i) {
    if (child->IsNodeOfType(nsINode::eTEXT) &&
        child->TextLength() > 0) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}


PRBool
nsContentUtils::IsInSameAnonymousTree(const nsINode* aNode,
                                      const nsIContent* aContent)
{
  NS_PRECONDITION(aNode,
                  "Must have a node to work with");
  NS_PRECONDITION(aContent,
                  "Must have a content to work with");
  
  if (!aNode->IsNodeOfType(nsINode::eCONTENT)) {
    






    return aContent->GetBindingParent() == nsnull;
  }

  return static_cast<const nsIContent*>(aNode)->GetBindingParent() ==
         aContent->GetBindingParent();
 
}

class AnonymousContentDestroyer : public nsRunnable {
public:
  AnonymousContentDestroyer(nsCOMPtr<nsIContent>* aContent) {
    mContent.swap(*aContent);
    mParent = mContent->GetParent();
    mDoc = mContent->GetOwnerDoc();
  }
  NS_IMETHOD Run() {
    mContent->UnbindFromTree();
    return NS_OK;
  }
private:
  nsCOMPtr<nsIContent> mContent;
  
  
  nsCOMPtr<nsIDocument> mDoc;
  nsCOMPtr<nsIContent> mParent;
};


void
nsContentUtils::DestroyAnonymousContent(nsCOMPtr<nsIContent>* aContent)
{
  if (*aContent) {
    AddScriptRunner(new AnonymousContentDestroyer(aContent));
  }
}


nsIDOMScriptObjectFactory*
nsContentUtils::GetDOMScriptObjectFactory()
{
  if (!sDOMScriptObjectFactory) {
    static NS_DEFINE_CID(kDOMScriptObjectFactoryCID,
                         NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

    CallGetService(kDOMScriptObjectFactoryCID, &sDOMScriptObjectFactory);
  }

  return sDOMScriptObjectFactory;
}


nsresult
nsContentUtils::HoldScriptObject(PRUint32 aLangID, void *aObject)
{
  NS_ASSERTION(aObject, "unexpected null object");
  NS_ASSERTION(aLangID != nsIProgrammingLanguage::JAVASCRIPT,
               "Should use HoldJSObjects.");
  nsresult rv;

  PRUint32 langIndex = NS_STID_INDEX(aLangID);
  nsIScriptRuntime *runtime = sScriptRuntimes[langIndex];
  if (!runtime) {
    nsIDOMScriptObjectFactory *factory = GetDOMScriptObjectFactory();
    NS_ENSURE_TRUE(factory, NS_ERROR_FAILURE);

    rv = factory->GetScriptRuntimeByID(aLangID, &runtime);
    NS_ENSURE_SUCCESS(rv, rv);

    
    sScriptRuntimes[langIndex] = runtime;
  }

  rv = runtime->HoldScriptObject(aObject);
  NS_ENSURE_SUCCESS(rv, rv);

  ++sScriptRootCount[langIndex];
  NS_LOG_ADDREF(sScriptRuntimes[langIndex], sScriptRootCount[langIndex],
                "HoldScriptObject", sizeof(void*));

  return NS_OK;
}


void
nsContentUtils::DropScriptObject(PRUint32 aLangID, void *aObject,
                                 void *aClosure)
{
  NS_ASSERTION(aObject, "unexpected null object");
  NS_ASSERTION(aLangID != nsIProgrammingLanguage::JAVASCRIPT,
               "Should use DropJSObjects.");
  PRUint32 langIndex = NS_STID_INDEX(aLangID);
  NS_LOG_RELEASE(sScriptRuntimes[langIndex], sScriptRootCount[langIndex] - 1,
                 "HoldScriptObject");
  sScriptRuntimes[langIndex]->DropScriptObject(aObject);
  if (--sScriptRootCount[langIndex] == 0) {
    NS_RELEASE(sScriptRuntimes[langIndex]);
  }
}


nsresult
nsContentUtils::HoldJSObjects(void* aScriptObjectHolder,
                              nsScriptObjectTracer* aTracer)
{
  NS_ENSURE_TRUE(sXPConnect, NS_ERROR_UNEXPECTED);

  nsresult rv = sXPConnect->AddJSHolder(aScriptObjectHolder, aTracer);
  NS_ENSURE_SUCCESS(rv, rv);

  if (sJSGCThingRootCount++ == 0) {
    nsLayoutStatics::AddRef();
  }
  NS_LOG_ADDREF(sXPConnect, sJSGCThingRootCount, "HoldJSObjects",
                sizeof(void*));

  return NS_OK;
}


nsresult
nsContentUtils::DropJSObjects(void* aScriptObjectHolder)
{
  NS_LOG_RELEASE(sXPConnect, sJSGCThingRootCount - 1, "HoldJSObjects");
  nsresult rv = sXPConnect->RemoveJSHolder(aScriptObjectHolder);
  if (--sJSGCThingRootCount == 0) {
    nsLayoutStatics::Release();
  }
  return rv;
}


PRUint32
nsContentUtils::GetWidgetStatusFromIMEStatus(PRUint32 aState)
{
  switch (aState & nsIContent::IME_STATUS_MASK_ENABLED) {
    case nsIContent::IME_STATUS_DISABLE:
      return nsIWidget::IME_STATUS_DISABLED;
    case nsIContent::IME_STATUS_ENABLE:
      return nsIWidget::IME_STATUS_ENABLED;
    case nsIContent::IME_STATUS_PASSWORD:
      return nsIWidget::IME_STATUS_PASSWORD;
    case nsIContent::IME_STATUS_PLUGIN:
      return nsIWidget::IME_STATUS_PLUGIN;
    default:
      NS_ERROR("The given state doesn't have valid enable state");
      return nsIWidget::IME_STATUS_ENABLED;
  }
}


void
nsContentUtils::NotifyInstalledMenuKeyboardListener(PRBool aInstalling)
{
  nsIMEStateManager::OnInstalledMenuKeyboardListener(aInstalling);
}

static PRBool SchemeIs(nsIURI* aURI, const char* aScheme)
{
  nsCOMPtr<nsIURI> baseURI = NS_GetInnermostURI(aURI);
  NS_ENSURE_TRUE(baseURI, PR_FALSE);

  PRBool isScheme = PR_FALSE;
  return NS_SUCCEEDED(baseURI->SchemeIs(aScheme, &isScheme)) && isScheme;
}


nsresult
nsContentUtils::CheckSecurityBeforeLoad(nsIURI* aURIToLoad,
                                        nsIPrincipal* aLoadingPrincipal,
                                        PRUint32 aCheckLoadFlags,
                                        PRBool aAllowData,
                                        PRUint32 aContentPolicyType,
                                        nsISupports* aContext,
                                        const nsACString& aMimeGuess,
                                        nsISupports* aExtra)
{
  NS_PRECONDITION(aLoadingPrincipal, "Must have a loading principal here");

  PRBool isSystemPrin = PR_FALSE;
  if (NS_SUCCEEDED(sSecurityManager->IsSystemPrincipal(aLoadingPrincipal,
                                                       &isSystemPrin)) &&
      isSystemPrin) {
    return NS_OK;
  }
  
  
  
  nsresult rv = sSecurityManager->
    CheckLoadURIWithPrincipal(aLoadingPrincipal, aURIToLoad, aCheckLoadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(aContentPolicyType,
                                 aURIToLoad,
                                 aLoadingPrincipal,
                                 aContext,
                                 aMimeGuess,
                                 aExtra,
                                 &shouldLoad,
                                 GetContentPolicy(),
                                 sSecurityManager);
  NS_ENSURE_SUCCESS(rv, rv);
  if (NS_CP_REJECTED(shouldLoad)) {
    return NS_ERROR_CONTENT_BLOCKED;
  }

  
  if ((aAllowData && SchemeIs(aURIToLoad, "data")) ||
      ((aCheckLoadFlags & nsIScriptSecurityManager::ALLOW_CHROME) &&
       SchemeIs(aURIToLoad, "chrome"))) {
    return NS_OK;
  }

  return aLoadingPrincipal->CheckMayLoad(aURIToLoad, PR_TRUE);
}

PRBool
nsContentUtils::IsSystemPrincipal(nsIPrincipal* aPrincipal)
{
  PRBool isSystem;
  nsresult rv = sSecurityManager->IsSystemPrincipal(aPrincipal, &isSystem);
  return NS_SUCCEEDED(rv) && isSystem;
}


void
nsContentUtils::TriggerLink(nsIContent *aContent, nsPresContext *aPresContext,
                            nsIURI *aLinkURI, const nsString &aTargetSpec,
                            PRBool aClick, PRBool aIsUserTriggered)
{
  NS_ASSERTION(aPresContext, "Need a nsPresContext");
  NS_PRECONDITION(aLinkURI, "No link URI");

  if (aContent->IsEditable()) {
    return;
  }

  nsILinkHandler *handler = aPresContext->GetLinkHandler();
  if (!handler) {
    return;
  }

  if (!aClick) {
    handler->OnOverLink(aContent, aLinkURI, aTargetSpec.get());

    return;
  }

  
  nsresult proceed = NS_OK;

  if (sSecurityManager) {
    PRUint32 flag =
      aIsUserTriggered ?
      (PRUint32)nsIScriptSecurityManager::STANDARD :
      (PRUint32)nsIScriptSecurityManager::LOAD_IS_AUTOMATIC_DOCUMENT_REPLACEMENT;
    proceed =
      sSecurityManager->CheckLoadURIWithPrincipal(aContent->NodePrincipal(),
                                                  aLinkURI, flag);
  }

  
  if (NS_SUCCEEDED(proceed)) {
    handler->OnLinkClick(aContent, aLinkURI, aTargetSpec.get());
  }
}


nsIWidget*
nsContentUtils::GetTopLevelWidget(nsIWidget* aWidget)
{
  if (!aWidget)
    return nsnull;

  return aWidget->GetTopLevelWidget();
}


const nsDependentString
nsContentUtils::GetLocalizedEllipsis()
{
  static PRUnichar sBuf[4] = { 0, 0, 0, 0 };
  if (!sBuf[0]) {
    nsAutoString tmp(GetLocalizedStringPref("intl.ellipsis"));
    PRUint32 len = NS_MIN(PRUint32(tmp.Length()),
                          PRUint32(NS_ARRAY_LENGTH(sBuf) - 1));
    CopyUnicodeTo(tmp, 0, sBuf, len);
    if (!sBuf[0])
      sBuf[0] = PRUnichar(0x2026);
  }
  return nsDependentString(sBuf);
}


nsEvent*
nsContentUtils::GetNativeEvent(nsIDOMEvent* aDOMEvent)
{
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aDOMEvent));
  if (!privateEvent)
    return nsnull;
  return privateEvent->GetInternalNSEvent();
}


PRBool
nsContentUtils::DOMEventToNativeKeyEvent(nsIDOMKeyEvent* aKeyEvent,
                                         nsNativeKeyEvent* aNativeEvent,
                                         PRBool aGetCharCode)
{
  nsCOMPtr<nsIDOMNSUIEvent> uievent = do_QueryInterface(aKeyEvent);
  PRBool defaultPrevented;
  uievent->GetPreventDefault(&defaultPrevented);
  if (defaultPrevented)
    return PR_FALSE;

  nsCOMPtr<nsIDOMNSEvent> nsevent = do_QueryInterface(aKeyEvent);
  PRBool trusted = PR_FALSE;
  nsevent->GetIsTrusted(&trusted);
  if (!trusted)
    return PR_FALSE;

  if (aGetCharCode) {
    aKeyEvent->GetCharCode(&aNativeEvent->charCode);
  } else {
    aNativeEvent->charCode = 0;
  }
  aKeyEvent->GetKeyCode(&aNativeEvent->keyCode);
  aKeyEvent->GetAltKey(&aNativeEvent->altKey);
  aKeyEvent->GetCtrlKey(&aNativeEvent->ctrlKey);
  aKeyEvent->GetShiftKey(&aNativeEvent->shiftKey);
  aKeyEvent->GetMetaKey(&aNativeEvent->metaKey);

  aNativeEvent->nativeEvent = GetNativeEvent(aKeyEvent);

  return PR_TRUE;
}

static PRBool
HasASCIIDigit(const nsTArray<nsShortcutCandidate>& aCandidates)
{
  for (PRUint32 i = 0; i < aCandidates.Length(); ++i) {
    PRUint32 ch = aCandidates[i].mCharCode;
    if (ch >= '0' && ch <= '9')
      return PR_TRUE;
  }
  return PR_FALSE;
}

static PRBool
CharsCaseInsensitiveEqual(PRUint32 aChar1, PRUint32 aChar2)
{
  return aChar1 == aChar2 ||
         (IS_IN_BMP(aChar1) && IS_IN_BMP(aChar2) &&
          ToLowerCase(PRUnichar(aChar1)) == ToLowerCase(PRUnichar(aChar2)));
}

static PRBool
IsCaseChangeableChar(PRUint32 aChar)
{
  return IS_IN_BMP(aChar) &&
         ToLowerCase(PRUnichar(aChar)) != ToUpperCase(PRUnichar(aChar));
}


void
nsContentUtils::GetAccelKeyCandidates(nsIDOMKeyEvent* aDOMKeyEvent,
                  nsTArray<nsShortcutCandidate>& aCandidates)
{
  NS_PRECONDITION(aCandidates.IsEmpty(), "aCandidates must be empty");

  nsAutoString eventType;
  aDOMKeyEvent->GetType(eventType);
  
  if (!eventType.EqualsLiteral("keypress"))
    return;

  nsKeyEvent* nativeKeyEvent =
    static_cast<nsKeyEvent*>(GetNativeEvent(aDOMKeyEvent));
  if (nativeKeyEvent) {
    NS_ASSERTION(nativeKeyEvent->eventStructType == NS_KEY_EVENT,
                 "wrong type of native event");
    
    
    
    
    
    
    
    
    
    
    
    if (nativeKeyEvent->charCode) {
      nsShortcutCandidate key(nativeKeyEvent->charCode, PR_FALSE);
      aCandidates.AppendElement(key);
    }

    PRUint32 len = nativeKeyEvent->alternativeCharCodes.Length();
    if (!nativeKeyEvent->isShift) {
      for (PRUint32 i = 0; i < len; ++i) {
        PRUint32 ch =
          nativeKeyEvent->alternativeCharCodes[i].mUnshiftedCharCode;
        if (!ch || ch == nativeKeyEvent->charCode)
          continue;

        nsShortcutCandidate key(ch, PR_FALSE);
        aCandidates.AppendElement(key);
      }
      
      
      
      
      if (!HasASCIIDigit(aCandidates)) {
        for (PRUint32 i = 0; i < len; ++i) {
          PRUint32 ch =
            nativeKeyEvent->alternativeCharCodes[i].mShiftedCharCode;
          if (ch >= '0' && ch <= '9') {
            nsShortcutCandidate key(ch, PR_FALSE);
            aCandidates.AppendElement(key);
            break;
          }
        }
      }
    } else {
      for (PRUint32 i = 0; i < len; ++i) {
        PRUint32 ch = nativeKeyEvent->alternativeCharCodes[i].mShiftedCharCode;
        if (!ch)
          continue;

        if (ch != nativeKeyEvent->charCode) {
          nsShortcutCandidate key(ch, PR_FALSE);
          aCandidates.AppendElement(key);
        }

        
        

        
        
        PRUint32 unshiftCh =
          nativeKeyEvent->alternativeCharCodes[i].mUnshiftedCharCode;
        if (CharsCaseInsensitiveEqual(ch, unshiftCh))
          continue;

        
        
        
        if (IsCaseChangeableChar(ch))
          continue;

        
        
        nsShortcutCandidate key(ch, PR_TRUE);
        aCandidates.AppendElement(key);
      }
    }
  } else {
    PRUint32 charCode;
    aDOMKeyEvent->GetCharCode(&charCode);
    if (charCode) {
      nsShortcutCandidate key(charCode, PR_FALSE);
      aCandidates.AppendElement(key);
    }
  }
}


void
nsContentUtils::GetAccessKeyCandidates(nsKeyEvent* aNativeKeyEvent,
                                       nsTArray<PRUint32>& aCandidates)
{
  NS_PRECONDITION(aCandidates.IsEmpty(), "aCandidates must be empty");

  
  
  
  
  if (aNativeKeyEvent->charCode) {
    PRUint32 ch = aNativeKeyEvent->charCode;
    if (IS_IN_BMP(ch))
      ch = ToLowerCase(PRUnichar(ch));
    aCandidates.AppendElement(ch);
  }
  for (PRUint32 i = 0;
       i < aNativeKeyEvent->alternativeCharCodes.Length(); ++i) {
    PRUint32 ch[2] =
      { aNativeKeyEvent->alternativeCharCodes[i].mUnshiftedCharCode,
        aNativeKeyEvent->alternativeCharCodes[i].mShiftedCharCode };
    for (PRUint32 j = 0; j < 2; ++j) {
      if (!ch[j])
        continue;
      if (IS_IN_BMP(ch[j]))
        ch[j] = ToLowerCase(PRUnichar(ch[j]));
      
      if (aCandidates.IndexOf(ch[j]) == aCandidates.NoIndex)
        aCandidates.AppendElement(ch[j]);
    }
  }
  return;
}


void
nsContentUtils::AddScriptBlocker()
{
  if (!sScriptBlockerCount) {
    NS_ASSERTION(sRunnersCountAtFirstBlocker == 0,
                 "Should not already have a count");
    sRunnersCountAtFirstBlocker = sBlockedScriptRunners->Count();
  }
  ++sScriptBlockerCount;
}


void
nsContentUtils::AddScriptBlockerAndPreventAddingRunners()
{
  AddScriptBlocker();
  if (sScriptBlockerCountWhereRunnersPrevented == 0) {
    sScriptBlockerCountWhereRunnersPrevented = sScriptBlockerCount;
  }
}


void
nsContentUtils::RemoveScriptBlocker()
{
  NS_ASSERTION(sScriptBlockerCount != 0, "Negative script blockers");
  --sScriptBlockerCount;
  if (sScriptBlockerCount < sScriptBlockerCountWhereRunnersPrevented) {
    sScriptBlockerCountWhereRunnersPrevented = 0;
  }
  if (sScriptBlockerCount) {
    return;
  }

  PRUint32 firstBlocker = sRunnersCountAtFirstBlocker;
  PRUint32 lastBlocker = (PRUint32)sBlockedScriptRunners->Count();
  PRUint32 originalFirstBlocker = firstBlocker;
  PRUint32 blockersCount = lastBlocker - firstBlocker;
  sRunnersCountAtFirstBlocker = 0;
  NS_ASSERTION(firstBlocker <= lastBlocker,
               "bad sRunnersCountAtFirstBlocker");

  while (firstBlocker < lastBlocker) {
    nsCOMPtr<nsIRunnable> runnable = (*sBlockedScriptRunners)[firstBlocker];
    ++firstBlocker;

    runnable->Run();
    NS_ASSERTION(sRunnersCountAtFirstBlocker == 0,
                 "Bad count");
    NS_ASSERTION(!sScriptBlockerCount, "This is really bad");
  }
  sBlockedScriptRunners->RemoveObjectsAt(originalFirstBlocker, blockersCount);
}


PRBool
nsContentUtils::AddScriptRunner(nsIRunnable* aRunnable)
{
  if (!aRunnable) {
    return PR_FALSE;
  }

  if (sScriptBlockerCount) {
    if (sScriptBlockerCountWhereRunnersPrevented > 0) {
      NS_ERROR("Adding a script runner when that is prevented!");
      return PR_FALSE;
    }
    return sBlockedScriptRunners->AppendObject(aRunnable);
  }
  
  nsCOMPtr<nsIRunnable> run = aRunnable;
  run->Run();

  return PR_TRUE;
}








static void ProcessViewportToken(nsIDocument *aDocument, 
                                 const nsAString &token) {

  
  nsAString::const_iterator tip, tail, end;
  token.BeginReading(tip);
  tail = tip;
  token.EndReading(end);

  
  while ((tip != end) && (*tip != '='))
    ++tip;

  
  if (tip == end)
    return;

  
  const nsAString &key =
    nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(Substring(tail, tip),
                                                        PR_TRUE);
  const nsAString &value =
    nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(Substring(++tip, end),
                                                        PR_TRUE);

  

  nsCOMPtr<nsIAtom> key_atom = do_GetAtom(key);
  if (key_atom == nsGkAtoms::height)
    aDocument->SetHeaderData(nsGkAtoms::viewport_height, value);
  else if (key_atom == nsGkAtoms::width)
    aDocument->SetHeaderData(nsGkAtoms::viewport_width, value);
  else if (key_atom == nsGkAtoms::initial_scale)
    aDocument->SetHeaderData(nsGkAtoms::viewport_initial_scale, value);
  else if (key_atom == nsGkAtoms::minimum_scale)
    aDocument->SetHeaderData(nsGkAtoms::viewport_minimum_scale, value);
  else if (key_atom == nsGkAtoms::maximum_scale)
    aDocument->SetHeaderData(nsGkAtoms::viewport_maximum_scale, value);
  else if (key_atom == nsGkAtoms::user_scalable)
    aDocument->SetHeaderData(nsGkAtoms::viewport_user_scalable, value);
}

#define IS_SEPARATOR(c) ((c == '=') || (c == ',') || (c == ';') || \
                         (c == '\t') || (c == '\n') || (c == '\r'))


nsresult
nsContentUtils::ProcessViewportInfo(nsIDocument *aDocument,
                                    const nsAString &viewportInfo) {

  
  nsresult rv = NS_OK;

  
  nsAString::const_iterator tip, tail, end;
  viewportInfo.BeginReading(tip);
  tail = tip;
  viewportInfo.EndReading(end);

  
  while ((tip != end) && (IS_SEPARATOR(*tip) || nsCRT::IsAsciiSpace(*tip)))
    ++tip;

  
  while (tip != end) {

    
    tail = tip;

    
    while ((tip != end) && !IS_SEPARATOR(*tip))
      ++tip;

    
    if ((tip != end) && (*tip == '=')) {
      ++tip;

      while ((tip != end) && nsCRT::IsAsciiSpace(*tip))
        ++tip;

      while ((tip != end) && !(IS_SEPARATOR(*tip) || nsCRT::IsAsciiSpace(*tip)))
        ++tip;
    }

    
    ProcessViewportToken(aDocument, Substring(tail, tip));

    
    while ((tip != end) && (IS_SEPARATOR(*tip) || nsCRT::IsAsciiSpace(*tip)))
      ++tip;
  }

  return rv;

}

#undef IS_SEPARATOR


void
nsContentUtils::HidePopupsInDocument(nsIDocument* aDocument)
{
#ifdef MOZ_XUL
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && aDocument) {
    nsCOMPtr<nsISupports> container = aDocument->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> docShellToHide = do_QueryInterface(container);
    if (docShellToHide)
      pm->HidePopupsInDocShell(docShellToHide);
  }
#endif
}


already_AddRefed<nsIDragSession>
nsContentUtils::GetDragSession()
{
  nsIDragSession* dragSession = nsnull;
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  if (dragService)
    dragService->GetCurrentSession(&dragSession);
  return dragSession;
}


nsresult
nsContentUtils::SetDataTransferInEvent(nsDragEvent* aDragEvent)
{
  if (aDragEvent->dataTransfer || !NS_IS_TRUSTED_EVENT(aDragEvent))
    return NS_OK;

  
  
  
  NS_ASSERTION(aDragEvent->message != NS_DRAGDROP_GESTURE &&
               aDragEvent->message != NS_DRAGDROP_START,
               "draggesture event created without a dataTransfer");

  nsCOMPtr<nsIDragSession> dragSession = GetDragSession();
  NS_ENSURE_TRUE(dragSession, NS_OK); 

  nsCOMPtr<nsIDOMDataTransfer> initialDataTransfer;
  dragSession->GetDataTransfer(getter_AddRefs(initialDataTransfer));
  if (!initialDataTransfer) {
    
    
    
    
    initialDataTransfer =
      new nsDOMDataTransfer(aDragEvent->message);
    NS_ENSURE_TRUE(initialDataTransfer, NS_ERROR_OUT_OF_MEMORY);

    
    dragSession->SetDataTransfer(initialDataTransfer);
  }

  
  nsCOMPtr<nsIDOMNSDataTransfer> initialDataTransferNS =
    do_QueryInterface(initialDataTransfer);
  NS_ENSURE_TRUE(initialDataTransferNS, NS_ERROR_FAILURE);
  initialDataTransferNS->Clone(aDragEvent->message, aDragEvent->userCancelled,
                               getter_AddRefs(aDragEvent->dataTransfer));
  NS_ENSURE_TRUE(aDragEvent->dataTransfer, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  if (aDragEvent->message == NS_DRAGDROP_ENTER ||
      aDragEvent->message == NS_DRAGDROP_OVER) {
    nsCOMPtr<nsIDOMNSDataTransfer> newDataTransfer =
      do_QueryInterface(aDragEvent->dataTransfer);
    NS_ENSURE_TRUE(newDataTransfer, NS_ERROR_FAILURE);

    PRUint32 action, effectAllowed;
    dragSession->GetDragAction(&action);
    newDataTransfer->GetEffectAllowedInt(&effectAllowed);
    newDataTransfer->SetDropEffectInt(FilterDropEffect(action, effectAllowed));
  }
  else if (aDragEvent->message == NS_DRAGDROP_DROP ||
           aDragEvent->message == NS_DRAGDROP_DRAGDROP ||
           aDragEvent->message == NS_DRAGDROP_END) {
    
    
    
    
    nsCOMPtr<nsIDOMNSDataTransfer> newDataTransfer =
      do_QueryInterface(aDragEvent->dataTransfer);
    NS_ENSURE_TRUE(newDataTransfer, NS_ERROR_FAILURE);

    PRUint32 dropEffect;
    initialDataTransferNS->GetDropEffectInt(&dropEffect);
    newDataTransfer->SetDropEffectInt(dropEffect);
  }

  return NS_OK;
}


PRUint32
nsContentUtils::FilterDropEffect(PRUint32 aAction, PRUint32 aEffectAllowed)
{
  
  
  
  
  
  if (aAction & nsIDragService::DRAGDROP_ACTION_COPY)
    aAction = nsIDragService::DRAGDROP_ACTION_COPY;
  else if (aAction & nsIDragService::DRAGDROP_ACTION_LINK)
    aAction = nsIDragService::DRAGDROP_ACTION_LINK;
  else if (aAction & nsIDragService::DRAGDROP_ACTION_MOVE)
    aAction = nsIDragService::DRAGDROP_ACTION_MOVE;

  
  
  
  
  
  if (aAction & aEffectAllowed ||
      aEffectAllowed == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED)
    return aAction;
  if (aEffectAllowed & nsIDragService::DRAGDROP_ACTION_MOVE)
    return nsIDragService::DRAGDROP_ACTION_MOVE;
  if (aEffectAllowed & nsIDragService::DRAGDROP_ACTION_COPY)
    return nsIDragService::DRAGDROP_ACTION_COPY;
  if (aEffectAllowed & nsIDragService::DRAGDROP_ACTION_LINK)
    return nsIDragService::DRAGDROP_ACTION_LINK;
  return nsIDragService::DRAGDROP_ACTION_NONE;
}


PRBool
nsContentUtils::URIIsLocalFile(nsIURI *aURI)
{
  PRBool isFile;
  nsCOMPtr<nsINetUtil> util = do_QueryInterface(sIOService);

  return util && NS_SUCCEEDED(util->ProtocolHasFlags(aURI,
                                nsIProtocolHandler::URI_IS_LOCAL_FILE,
                                &isFile)) &&
         isFile;
}

nsresult
nsContentUtils::SplitURIAtHash(nsIURI *aURI,
                               nsACString &aBeforeHash,
                               nsACString &aAfterHash)
{
  

  aBeforeHash.Truncate();
  aAfterHash.Truncate();

  NS_ENSURE_ARG_POINTER(aURI);

  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 index = spec.FindChar('#');
  if (index == -1) {
    index = spec.Length();
  }

  aBeforeHash.Assign(Substring(spec, 0, index));
  aAfterHash.Assign(Substring(spec, index));
  return NS_OK;
}


nsIScriptContext*
nsContentUtils::GetContextForEventHandlers(nsINode* aNode,
                                           nsresult* aRv)
{
  *aRv = NS_OK;
  nsIDocument* ownerDoc = aNode->GetOwnerDoc();
  if (!ownerDoc) {
    *aRv = NS_ERROR_UNEXPECTED;
    return nsnull;
  }

  PRBool hasHadScriptObject = PR_TRUE;
  nsIScriptGlobalObject* sgo =
    ownerDoc->GetScriptHandlingObject(hasHadScriptObject);
  
  
  if (!sgo && hasHadScriptObject) {
    *aRv = NS_ERROR_UNEXPECTED;
    return nsnull;
  }

  if (sgo) {
    nsIScriptContext* scx = sgo->GetContext();
    
    if (!scx) {
      *aRv = NS_ERROR_UNEXPECTED;
      return nsnull;
    }
    return scx;
  }

  return nsnull;
}


JSContext *
nsContentUtils::GetCurrentJSContext()
{
  JSContext *cx = nsnull;

  sThreadJSContextStack->Peek(&cx);

  return cx;
}


void
nsContentUtils::ASCIIToLower(nsAString& aStr)
{
  PRUnichar* iter = aStr.BeginWriting();
  PRUnichar* end = aStr.EndWriting();
  while (iter != end) {
    PRUnichar c = *iter;
    if (c >= 'A' && c <= 'Z') {
      *iter = c + ('a' - 'A');
    }
    ++iter;
  }
}


void
nsContentUtils::ASCIIToLower(const nsAString& aSource, nsAString& aDest)
{
  PRUint32 len = aSource.Length();
  aDest.SetLength(len);
  if (aDest.Length() == len) {
    PRUnichar* dest = aDest.BeginWriting();
    const PRUnichar* iter = aSource.BeginReading();
    const PRUnichar* end = aSource.EndReading();
    while (iter != end) {
      PRUnichar c = *iter;
      *dest = (c >= 'A' && c <= 'Z') ?
         c + ('a' - 'A') : c;
      ++iter;
      ++dest;
    }
  }
}


void
nsContentUtils::ASCIIToUpper(nsAString& aStr)
{
  PRUnichar* iter = aStr.BeginWriting();
  PRUnichar* end = aStr.EndWriting();
  while (iter != end) {
    PRUnichar c = *iter;
    if (c >= 'a' && c <= 'z') {
      *iter = c + ('A' - 'a');
    }
    ++iter;
  }
}


void
nsContentUtils::ASCIIToUpper(const nsAString& aSource, nsAString& aDest)
{
  PRUint32 len = aSource.Length();
  aDest.SetLength(len);
  if (aDest.Length() == len) {
    PRUnichar* dest = aDest.BeginWriting();
    const PRUnichar* iter = aSource.BeginReading();
    const PRUnichar* end = aSource.EndReading();
    while (iter != end) {
      PRUnichar c = *iter;
      *dest = (c >= 'a' && c <= 'z') ?
         c + ('A' - 'a') : c;
      ++iter;
      ++dest;
    }
  }
}

PRBool
nsContentUtils::EqualsIgnoreASCIICase(const nsAString& aStr1,
                                      const nsAString& aStr2)
{
  PRUint32 len = aStr1.Length();
  if (len != aStr2.Length()) {
    return PR_FALSE;
  }

  const PRUnichar* str1 = aStr1.BeginReading();
  const PRUnichar* str2 = aStr2.BeginReading();
  const PRUnichar* end = str1 + len;

  while (str1 < end) {
    PRUnichar c1 = *str1++;
    PRUnichar c2 = *str2++;

    
    if ((c1 ^ c2) & 0xffdf) {
      return PR_FALSE;
    }

    
    
    if (c1 != c2) {
      
      
      PRUnichar c1Upper = c1 & 0xffdf;
      if (!('A' <= c1Upper && c1Upper <= 'Z')) {
        return PR_FALSE;
      }
    }
  }

  return PR_TRUE;
}


nsIInterfaceRequestor*
nsContentUtils::GetSameOriginChecker()
{
  if (!sSameOriginChecker) {
    sSameOriginChecker = new nsSameOriginChecker();
    NS_IF_ADDREF(sSameOriginChecker);
  }
  return sSameOriginChecker;
}


nsresult
nsContentUtils::CheckSameOrigin(nsIChannel *aOldChannel, nsIChannel *aNewChannel)
{
  if (!nsContentUtils::GetSecurityManager())
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIPrincipal> oldPrincipal;
  nsContentUtils::GetSecurityManager()->
    GetChannelPrincipal(aOldChannel, getter_AddRefs(oldPrincipal));

  nsCOMPtr<nsIURI> newURI;
  aNewChannel->GetURI(getter_AddRefs(newURI));
  nsCOMPtr<nsIURI> newOriginalURI;
  aNewChannel->GetOriginalURI(getter_AddRefs(newOriginalURI));

  NS_ENSURE_STATE(oldPrincipal && newURI && newOriginalURI);

  nsresult rv = oldPrincipal->CheckMayLoad(newURI, PR_FALSE);
  if (NS_SUCCEEDED(rv) && newOriginalURI != newURI) {
    rv = oldPrincipal->CheckMayLoad(newOriginalURI, PR_FALSE);
  }

  return rv;
}

NS_IMPL_ISUPPORTS2(nsSameOriginChecker,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor)

NS_IMETHODIMP
nsSameOriginChecker::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                            nsIChannel *aNewChannel,
                                            PRUint32 aFlags,
                                            nsIAsyncVerifyRedirectCallback *cb)
{
  NS_PRECONDITION(aNewChannel, "Redirecting to null channel?");

  nsresult rv = nsContentUtils::CheckSameOrigin(aOldChannel, aNewChannel);
  if (NS_SUCCEEDED(rv)) {
    cb->OnRedirectVerifyCallback(NS_OK);
  }

  return rv;
}

NS_IMETHODIMP
nsSameOriginChecker::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}


nsresult
nsContentUtils::GetASCIIOrigin(nsIPrincipal* aPrincipal, nsCString& aOrigin)
{
  NS_PRECONDITION(aPrincipal, "missing principal");

  aOrigin.Truncate();

  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  if (uri) {
    return GetASCIIOrigin(uri, aOrigin);
  }

  aOrigin.AssignLiteral("null");

  return NS_OK;
}


nsresult
nsContentUtils::GetASCIIOrigin(nsIURI* aURI, nsCString& aOrigin)
{
  NS_PRECONDITION(aURI, "missing uri");

  aOrigin.Truncate();

  nsCOMPtr<nsIURI> uri = NS_GetInnermostURI(aURI);
  NS_ENSURE_TRUE(uri, NS_ERROR_UNEXPECTED);

  nsCString host;
  nsresult rv = uri->GetAsciiHost(host);

  if (NS_SUCCEEDED(rv) && !host.IsEmpty()) {
    nsCString scheme;
    rv = uri->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);

    aOrigin = scheme + NS_LITERAL_CSTRING("://") + host;

    
    PRInt32 port;
    uri->GetPort(&port);
    if (port != -1) {
      PRInt32 defaultPort = NS_GetDefaultPort(scheme.get());
      if (port != defaultPort) {
        aOrigin.Append(':');
        aOrigin.AppendInt(port);
      }
    }
  }
  else {
    aOrigin.AssignLiteral("null");
  }

  return NS_OK;
}


nsresult
nsContentUtils::GetUTFOrigin(nsIPrincipal* aPrincipal, nsString& aOrigin)
{
  NS_PRECONDITION(aPrincipal, "missing principal");

  aOrigin.Truncate();

  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  if (uri) {
    return GetUTFOrigin(uri, aOrigin);
  }

  aOrigin.AssignLiteral("null");

  return NS_OK;
}


nsresult
nsContentUtils::GetUTFOrigin(nsIURI* aURI, nsString& aOrigin)
{
  NS_PRECONDITION(aURI, "missing uri");

  aOrigin.Truncate();

  nsCOMPtr<nsIURI> uri = NS_GetInnermostURI(aURI);
  NS_ENSURE_TRUE(uri, NS_ERROR_UNEXPECTED);

  nsCString host;
  nsresult rv = uri->GetHost(host);

  if (NS_SUCCEEDED(rv) && !host.IsEmpty()) {
    nsCString scheme;
    rv = uri->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);

    aOrigin = NS_ConvertUTF8toUTF16(scheme + NS_LITERAL_CSTRING("://") + host);

    
    PRInt32 port;
    uri->GetPort(&port);
    if (port != -1) {
      PRInt32 defaultPort = NS_GetDefaultPort(scheme.get());
      if (port != defaultPort) {
        aOrigin.Append(':');
        aOrigin.AppendInt(port);
      }
    }
  }
  else {
    aOrigin.AssignLiteral("null");
  }
  
  return NS_OK;
}


already_AddRefed<nsIDocument>
nsContentUtils::GetDocumentFromScriptContext(nsIScriptContext *aScriptContext)
{
  if (!aScriptContext)
    return nsnull;

  nsCOMPtr<nsIDOMWindow> window =
    do_QueryInterface(aScriptContext->GetGlobalObject());
  nsIDocument *doc = nsnull;
  if (window) {
    nsCOMPtr<nsIDOMDocument> domdoc;
    window->GetDocument(getter_AddRefs(domdoc));
    if (domdoc) {
      CallQueryInterface(domdoc, &doc);
    }
  }
  return doc;
}


PRBool
nsContentUtils::CheckMayLoad(nsIPrincipal* aPrincipal, nsIChannel* aChannel)
{
  nsCOMPtr<nsIURI> channelURI;
  nsresult rv = NS_GetFinalChannelURI(aChannel, getter_AddRefs(channelURI));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return NS_SUCCEEDED(aPrincipal->CheckMayLoad(channelURI, PR_FALSE));
}

nsContentTypeParser::nsContentTypeParser(const nsAString& aString)
  : mString(aString), mService(nsnull)
{
  CallGetService("@mozilla.org/network/mime-hdrparam;1", &mService);
}

nsContentTypeParser::~nsContentTypeParser()
{
  NS_IF_RELEASE(mService);
}

nsresult
nsContentTypeParser::GetParameter(const char* aParameterName, nsAString& aResult)
{
  NS_ENSURE_TRUE(mService, NS_ERROR_FAILURE);
  return mService->GetParameter(mString, aParameterName,
                                EmptyCString(), PR_FALSE, nsnull,
                                aResult);
}





PRBool
nsContentUtils::CanAccessNativeAnon()
{
  JSContext* cx = nsnull;
  sThreadJSContextStack->Peek(&cx);
  if (!cx) {
    return PR_TRUE;
  }
  JSStackFrame* fp;
  nsIPrincipal* principal =
    sSecurityManager->GetCxSubjectPrincipalAndFrame(cx, &fp);
  NS_ENSURE_TRUE(principal, PR_FALSE);

  if (!fp) {
    if (!JS_FrameIterator(cx, &fp)) {
      
      
      return PR_TRUE;
    }

    
    
    fp = nsnull;
  } else if (!JS_IsScriptFrame(cx, fp)) {
    fp = nsnull;
  }

  PRBool privileged;
  if (NS_SUCCEEDED(sSecurityManager->IsSystemPrincipal(principal, &privileged)) &&
      privileged) {
    
    return PR_TRUE;
  }

  
  
  static const char prefix[] = "chrome://global/";
  const char *filename;
  if (fp && JS_IsScriptFrame(cx, fp) &&
      (filename = JS_GetFrameScript(cx, fp)->filename) &&
      !strncmp(filename, prefix, NS_ARRAY_LENGTH(prefix) - 1)) {
    return PR_TRUE;
  }

  
  nsresult rv = sSecurityManager->IsCapabilityEnabled("UniversalXPConnect", &privileged);
  if (NS_SUCCEEDED(rv) && privileged) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

 nsresult
nsContentUtils::DispatchXULCommand(nsIContent* aTarget,
                                   PRBool aTrusted,
                                   nsIDOMEvent* aSourceEvent,
                                   nsIPresShell* aShell,
                                   PRBool aCtrl,
                                   PRBool aAlt,
                                   PRBool aShift,
                                   PRBool aMeta)
{
  NS_ENSURE_STATE(aTarget);
  nsIDocument* doc = aTarget->GetOwnerDoc();
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
  NS_ENSURE_STATE(domDoc);
  nsCOMPtr<nsIDOMEvent> event;
  domDoc->CreateEvent(NS_LITERAL_STRING("xulcommandevent"),
                      getter_AddRefs(event));
  nsCOMPtr<nsIDOMXULCommandEvent> xulCommand = do_QueryInterface(event);
  nsCOMPtr<nsIPrivateDOMEvent> pEvent = do_QueryInterface(xulCommand);
  NS_ENSURE_STATE(pEvent);
  nsresult rv = xulCommand->InitCommandEvent(NS_LITERAL_STRING("command"),
                                             PR_TRUE, PR_TRUE, doc->GetWindow(),
                                             0, aCtrl, aAlt, aShift, aMeta,
                                             aSourceEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aShell) {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsCOMPtr<nsIPresShell> kungFuDeathGrip = aShell;
    return aShell->HandleDOMEventWithTarget(aTarget, event, &status);
  }

  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(aTarget);
  NS_ENSURE_STATE(target);
  PRBool dummy;
  return target->DispatchEvent(event, &dummy);
}


nsresult
nsContentUtils::WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
                           nsWrapperCache *cache, const nsIID* aIID, jsval *vp,
                           nsIXPConnectJSObjectHolder **aHolder,
                           PRBool aAllowWrapping)
{
  if (!native) {
    NS_ASSERTION(!aHolder || !*aHolder, "*aHolder should be null!");

    *vp = JSVAL_NULL;

    return NS_OK;
  }

  JSObject *wrapper = xpc_FastGetCachedWrapper(cache, scope, vp);
  if (wrapper) {
    return NS_OK;
  }

  NS_ENSURE_TRUE(sXPConnect && sThreadJSContextStack, NS_ERROR_UNEXPECTED);

  
  
  
  
  
  
  PRBool isMainThread = NS_IsMainThread();

  if (isMainThread) {
    nsLayoutStatics::AddRef();
  }
  else {
    sXPConnect->AddRef();
  }

  JSContext *topJSContext;
  nsresult rv = sThreadJSContextStack->Peek(&topJSContext);
  if (NS_SUCCEEDED(rv)) {
    PRBool push = topJSContext != cx;
    if (push) {
      rv = sThreadJSContextStack->Push(cx);
    }
    if (NS_SUCCEEDED(rv)) {
      rv = sXPConnect->WrapNativeToJSVal(cx, scope, native, cache, aIID,
                                         aAllowWrapping, vp, aHolder);
      if (push) {
        sThreadJSContextStack->Pop(nsnull);
      }
    }
  }

  if (isMainThread) {
    nsLayoutStatics::Release();
  }
  else {
    sXPConnect->Release();
  }

  return rv;
}

void
nsContentUtils::StripNullChars(const nsAString& aInStr, nsAString& aOutStr)
{
  
  
  PRInt32 firstNullPos = aInStr.FindChar('\0');
  if (firstNullPos == kNotFound) {
    aOutStr.Assign(aInStr);
    return;
  }

  aOutStr.SetCapacity(aInStr.Length() - 1);
  nsAString::const_iterator start, end;
  aInStr.BeginReading(start);
  aInStr.EndReading(end);
  while (start != end) {
    if (*start != '\0')
      aOutStr.Append(*start);
    ++start;
  }
}

namespace {

const unsigned int kCloneStackFrameStackSize = 20;

class CloneStackFrame
{
  friend class CloneStack;

public:
  
  
  jsval source;
  jsval clone;
  jsval temp;
  js::AutoIdArray ids;
  jsuint index;

private:
  
  CloneStackFrame(JSContext* aCx, jsval aSource, jsval aClone, JSIdArray* aIds)
  : source(aSource), clone(aClone), temp(JSVAL_NULL), ids(aCx, aIds), index(0),
    prevFrame(nsnull),  tvrVals(aCx, 3, &source)
  {
    MOZ_COUNT_CTOR(CloneStackFrame);
  }

  ~CloneStackFrame()
  {
    MOZ_COUNT_DTOR(CloneStackFrame);
  }

  CloneStackFrame* prevFrame;
  js::AutoArrayRooter tvrVals;
};

class CloneStack
{
public:
  CloneStack(JSContext* cx)
  : mCx(cx), mLastFrame(nsnull) {
    mObjectSet.Init();
  }

  ~CloneStack() {
    while (!IsEmpty()) {
      Pop();
    }
  }

  PRBool
  Push(jsval source, jsval clone, JSIdArray* ids) {
    NS_ASSERTION(!JSVAL_IS_PRIMITIVE(source) && !JSVAL_IS_PRIMITIVE(clone),
                 "Must be an object!");
    if (!ids) {
      return PR_FALSE;
    }

    CloneStackFrame* newFrame;
    if (mObjectSet.Count() < kCloneStackFrameStackSize) {
      
      CloneStackFrame* buf = reinterpret_cast<CloneStackFrame*>(mStackFrames);
      newFrame = new (buf + mObjectSet.Count())
                     CloneStackFrame(mCx, source, clone, ids);
    }
    else {
      
      newFrame = new CloneStackFrame(mCx, source, clone, ids);
    }

    mObjectSet.PutEntry(JSVAL_TO_OBJECT(source));

    newFrame->prevFrame = mLastFrame;
    mLastFrame = newFrame;

    return PR_TRUE;
  }

  CloneStackFrame*
  Peek() {
    return mLastFrame;
  }

  void
  Pop() {
    if (IsEmpty()) {
      NS_ERROR("Empty stack!");
      return;
    }

    CloneStackFrame* lastFrame = mLastFrame;

    mObjectSet.RemoveEntry(JSVAL_TO_OBJECT(lastFrame->source));
    mLastFrame = lastFrame->prevFrame;

    if (mObjectSet.Count() >= kCloneStackFrameStackSize) {
      
      delete lastFrame;
    }
    else {
      
      lastFrame->~CloneStackFrame();
    }
  }

  PRBool
  IsEmpty() {
    NS_ASSERTION((!mLastFrame && !mObjectSet.Count()) ||
                 (mLastFrame && mObjectSet.Count()),
                 "Hashset is out of sync!");
    return mObjectSet.Count() == 0;
  }

  PRBool
  Search(JSObject* obj) {
    return !!mObjectSet.GetEntry(obj);
  }

private:
  JSContext* mCx;
  CloneStackFrame* mLastFrame;
  nsTHashtable<nsVoidPtrHashKey> mObjectSet;

  
  
  char mStackFrames[kCloneStackFrameStackSize * sizeof(CloneStackFrame)];
};

struct ReparentObjectData {
  ReparentObjectData(JSContext* cx, JSObject* obj)
  : cx(cx), obj(obj), ids(nsnull), index(0) { }

  ~ReparentObjectData() {
    if (ids) {
      JS_DestroyIdArray(cx, ids);
    }
  }

  JSContext* cx;
  JSObject* obj;
  JSIdArray* ids;
  jsint index;
};

inline nsresult
SetPropertyOnValueOrObject(JSContext* cx,
                           jsval val,
                           jsval* rval,
                           JSObject* obj,
                           jsid id)
{
  NS_ASSERTION((rval && !obj) || (!rval && obj), "Can only clone to one dest!");
  if (rval) {
    *rval = val;
    return NS_OK;
  }
  if (!JS_DefinePropertyById(cx, obj, id, val, nsnull, nsnull,
                             JSPROP_ENUMERATE)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

inline JSObject*
CreateEmptyObjectOrArray(JSContext* cx,
                         JSObject* obj)
{
  if (JS_IsArrayObject(cx, obj)) {
    jsuint length;
    if (!JS_GetArrayLength(cx, obj, &length)) {
      NS_ERROR("Failed to get array length?!");
      return nsnull;
    }
    return JS_NewArrayObject(cx, length, NULL);
  }
  return JS_NewObject(cx, NULL, NULL, NULL);
}

nsresult
CloneSimpleValues(JSContext* cx,
                  jsval val,
                  jsval* rval,
                  PRBool* wasCloned,
                  JSObject* robj = nsnull,
                  jsid rid = INT_TO_JSID(0))
{
  *wasCloned = PR_TRUE;

  
  if (!JSVAL_IS_GCTHING(val) || JSVAL_IS_NULL(val)) {
    return SetPropertyOnValueOrObject(cx, val, rval, robj, rid);
  }

  
  if (JSVAL_IS_STRING(val)) {
    if (!JS_MakeStringImmutable(cx, JSVAL_TO_STRING(val))) {
      return NS_ERROR_FAILURE;
    }
    return SetPropertyOnValueOrObject(cx, val, rval, robj, rid);
  }

  NS_ASSERTION(!JSVAL_IS_PRIMITIVE(val), "Not an object!");
  JSObject* obj = JSVAL_TO_OBJECT(val);

  
  JSObject* newArray;
  if (!js_CloneDensePrimitiveArray(cx, obj, &newArray)) {
    return NS_ERROR_FAILURE;
  }
  if (newArray) {
    return SetPropertyOnValueOrObject(cx, OBJECT_TO_JSVAL(newArray), rval, robj,
                                      rid);
  }

  
  if (js_DateIsValid(cx, obj)) {
    jsdouble msec = js_DateGetMsecSinceEpoch(cx, obj);
    JSObject* newDate;
    if (!(msec  && (newDate = js_NewDateObjectMsec(cx, msec)))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    return SetPropertyOnValueOrObject(cx, OBJECT_TO_JSVAL(newDate), rval, robj,
                                      rid);
  }

  
  if (js_ObjectIsRegExp(obj)) {
    JSObject* proto;
    if (!js_GetClassPrototype(cx, JS_GetScopeChain(cx), JSProto_RegExp,
                              &proto)) {
      return NS_ERROR_FAILURE;
    }
    JSObject* newRegExp = js_CloneRegExpObject(cx, obj, proto);
    if (!newRegExp) {
      return NS_ERROR_FAILURE;
    }
    return SetPropertyOnValueOrObject(cx, OBJECT_TO_JSVAL(newRegExp), rval,
                                      robj, rid);
  }

  
  if (js_IsTypedArray(obj)) {
    js::TypedArray* src = js::TypedArray::fromJSObject(obj);
    JSObject* newTypedArray = js_CreateTypedArrayWithArray(cx, src->type, obj);
    if (!newTypedArray) {
      return NS_ERROR_FAILURE;
    }
    return SetPropertyOnValueOrObject(cx, OBJECT_TO_JSVAL(newTypedArray), rval,
                                      robj, rid);
  }

  
  if (js_IsArrayBuffer(obj)) {
    js::ArrayBuffer* src = js::ArrayBuffer::fromJSObject(obj);
    if (!src) {
      return NS_ERROR_FAILURE;
    }

    JSObject* newBuffer = js_CreateArrayBuffer(cx, src->byteLength);
    if (!newBuffer) {
      return NS_ERROR_FAILURE;
    }
    memcpy(js::ArrayBuffer::fromJSObject(newBuffer)->data, src->data,
           src->byteLength);
    return SetPropertyOnValueOrObject(cx, OBJECT_TO_JSVAL(newBuffer), rval,
                                      robj, rid);
  }

  
  
  

  
  if (JS_ObjectIsFunction(cx, obj)) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  
  if (obj->isWrapper() && !obj->getClass()->ext.innerObject)
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;

  
  
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  nsContentUtils::XPConnect()->
    GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wrapper));
  if (wrapper) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  *wasCloned = PR_FALSE;
  return NS_OK;
}

} 


nsresult
nsContentUtils::CreateStructuredClone(JSContext* cx,
                                      jsval val,
                                      jsval* rval)
{
  JSAutoRequest ar(cx);

  nsCOMPtr<nsIXPConnect> xpconnect(sXPConnect);
  NS_ENSURE_STATE(xpconnect);

  PRBool wasCloned;
  nsresult rv = CloneSimpleValues(cx, val, rval, &wasCloned);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (wasCloned) {
    return NS_OK;
  }

  NS_ASSERTION(JSVAL_IS_OBJECT(val), "Not an object?!");
  JSObject* obj = CreateEmptyObjectOrArray(cx, JSVAL_TO_OBJECT(val));
  if (!obj) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  jsval output = OBJECT_TO_JSVAL(obj);
  js::AutoValueRooter tvr(cx, output);

  CloneStack stack(cx);
  if (!stack.Push(val, OBJECT_TO_JSVAL(obj),
                  JS_Enumerate(cx, JSVAL_TO_OBJECT(val)))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  while (!stack.IsEmpty()) {
    CloneStackFrame* frame = stack.Peek();

    NS_ASSERTION(!!frame->ids &&
                 frame->ids.length() >= frame->index &&
                 !JSVAL_IS_PRIMITIVE(frame->source) &&
                 !JSVAL_IS_PRIMITIVE(frame->clone),
                 "Bad frame state!");

    if (frame->index == frame->ids.length()) {
      
      stack.Pop();
      continue;
    }

    
    jsid id = frame->ids[frame->index++];

    if (!JS_GetPropertyById(cx, JSVAL_TO_OBJECT(frame->source), id,
                            &frame->temp)) {
      return NS_ERROR_FAILURE;
    }

    if (!JSVAL_IS_PRIMITIVE(frame->temp) &&
        stack.Search(JSVAL_TO_OBJECT(frame->temp))) {
      
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }

    JSObject* clone = JSVAL_TO_OBJECT(frame->clone);

    PRBool wasCloned;
    nsresult rv = CloneSimpleValues(cx, frame->temp, nsnull, &wasCloned, clone,
                                    id);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (!wasCloned) {
      NS_ASSERTION(JSVAL_IS_OBJECT(frame->temp), "Not an object?!");
      obj = CreateEmptyObjectOrArray(cx, JSVAL_TO_OBJECT(frame->temp));
      if (!obj ||
          !stack.Push(frame->temp, OBJECT_TO_JSVAL(obj),
                      JS_Enumerate(cx, JSVAL_TO_OBJECT(frame->temp)))) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      
      
      if (!JS_DefinePropertyById(cx, clone, id, OBJECT_TO_JSVAL(obj), nsnull,
                                 nsnull, JSPROP_ENUMERATE)) {
        return NS_ERROR_FAILURE;
      }
    }
  }

  *rval = output;
  return NS_OK;
}
struct ClassMatchingInfo {
  nsAttrValue::AtomArray mClasses;
  nsCaseTreatment mCaseTreatment;
};

static PRBool
MatchClassNames(nsIContent* aContent, PRInt32 aNamespaceID, nsIAtom* aAtom,
                void* aData)
{
  
  const nsAttrValue* classAttr = aContent->GetClasses();
  if (!classAttr) {
    return PR_FALSE;
  }
  
  
  ClassMatchingInfo* info = static_cast<ClassMatchingInfo*>(aData);
  PRUint32 length = info->mClasses.Length();
  if (!length) {
    
    return PR_FALSE;
  }
  PRUint32 i;
  for (i = 0; i < length; ++i) {
    if (!classAttr->Contains(info->mClasses[i],
                             info->mCaseTreatment)) {
      return PR_FALSE;
    }
  }
  
  return PR_TRUE;
}

static void
DestroyClassNameArray(void* aData)
{
  ClassMatchingInfo* info = static_cast<ClassMatchingInfo*>(aData);
  delete info;
}

static void*
AllocClassMatchingInfo(nsINode* aRootNode,
                       const nsString* aClasses)
{
  nsAttrValue attrValue;
  attrValue.ParseAtomArray(*aClasses);
  
  ClassMatchingInfo* info = new ClassMatchingInfo;
  NS_ENSURE_TRUE(info, nsnull);

  if (attrValue.Type() == nsAttrValue::eAtomArray) {
    info->mClasses.SwapElements(*(attrValue.GetAtomArrayValue()));
  } else if (attrValue.Type() == nsAttrValue::eAtom) {
    info->mClasses.AppendElement(attrValue.GetAtomValue());
  }

  info->mCaseTreatment =
    aRootNode->GetOwnerDoc() &&
    aRootNode->GetOwnerDoc()->GetCompatibilityMode() == eCompatibility_NavQuirks ?
    eIgnoreCase : eCaseMatters;
  return info;
}



nsresult
nsContentUtils::GetElementsByClassName(nsINode* aRootNode,
                                       const nsAString& aClasses,
                                       nsIDOMNodeList** aReturn)
{
  NS_PRECONDITION(aRootNode, "Must have root node");
  
  nsContentList* elements =
    NS_GetFuncStringContentList(aRootNode, MatchClassNames,
                                DestroyClassNameArray,
                                AllocClassMatchingInfo,
                                aClasses).get();
  NS_ENSURE_TRUE(elements, NS_ERROR_OUT_OF_MEMORY);

  
  *aReturn = elements;

  return NS_OK;
}

#ifdef DEBUG
class DebugWrapperTraversalCallback : public nsCycleCollectionTraversalCallback
{
public:
  DebugWrapperTraversalCallback(void* aWrapper) : mFound(PR_FALSE),
                                                  mWrapper(aWrapper)
  {
    mFlags = WANT_ALL_TRACES;
  }

  NS_IMETHOD_(void) DescribeNode(CCNodeType type,
                                 nsrefcnt refcount,
                                 size_t objsz,
                                 const char* objname)
  {
  }
  NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root)
  {
  }
  NS_IMETHOD_(void) NoteRoot(PRUint32 langID, void* root,
                             nsCycleCollectionParticipant* helper)
  {
  }
  NS_IMETHOD_(void) NoteScriptChild(PRUint32 langID, void* child)
  {
    if (langID == nsIProgrammingLanguage::JAVASCRIPT) {
      mFound = child == mWrapper;
    }
  }
  NS_IMETHOD_(void) NoteXPCOMChild(nsISupports *child)
  {
  }
  NS_IMETHOD_(void) NoteNativeChild(void* child,
                                    nsCycleCollectionParticipant* helper)
  {
  }

  NS_IMETHOD_(void) NoteNextEdgeName(const char* name)
  {
  }

  PRBool mFound;

private:
  void* mWrapper;
};

static void
DebugWrapperTraceCallback(PRUint32 langID, void *p, const char *name,
                          void *closure)
{
  DebugWrapperTraversalCallback* callback =
    static_cast<DebugWrapperTraversalCallback*>(closure);
  callback->NoteScriptChild(langID, p);
}


void
nsContentUtils::CheckCCWrapperTraversal(nsISupports* aScriptObjectHolder,
                                        nsWrapperCache* aCache)
{
  nsXPCOMCycleCollectionParticipant* participant;
  CallQueryInterface(aScriptObjectHolder, &participant);

  DebugWrapperTraversalCallback callback(aCache->GetWrapper());

  participant->Traverse(aScriptObjectHolder, callback);
  NS_ASSERTION(callback.mFound,
               "Cycle collection participant didn't traverse to preserved "
               "wrapper! This will probably crash.");

  callback.mFound = PR_FALSE;
  participant->Trace(aScriptObjectHolder, DebugWrapperTraceCallback, &callback);
  NS_ASSERTION(callback.mFound,
               "Cycle collection participant didn't trace preserved wrapper! "
               "This will probably crash.");
}
#endif


PRBool
nsContentUtils::IsFocusedContent(const nsIContent* aContent)
{
  nsFocusManager* fm = nsFocusManager::GetFocusManager();

  return fm && fm->GetFocusedContent() == aContent;
}

bool
nsContentUtils::IsSubDocumentTabbable(nsIContent* aContent)
{
  nsIDocument* doc = aContent->GetCurrentDoc();
  if (!doc) {
    return false;
  }

  
  
  nsIDocument* subDoc = doc->GetSubDocumentFor(aContent);
  if (!subDoc) {
    return false;
  }

  nsCOMPtr<nsISupports> container = subDoc->GetContainer();
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);
  if (!docShell) {
    return false;
  }

  nsCOMPtr<nsIContentViewer> contentViewer;
  docShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (!contentViewer) {
    return false;
  }

  nsCOMPtr<nsIContentViewer> zombieViewer;
  contentViewer->GetPreviousViewer(getter_AddRefs(zombieViewer));

  
  
  
  return !zombieViewer;
}

void
nsContentUtils::FlushLayoutForTree(nsIDOMWindow* aWindow)
{
    nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(aWindow);
    if (!piWin)
        return;

    
    
    

    nsCOMPtr<nsIDOMDocument> domDoc;
    aWindow->GetDocument(getter_AddRefs(domDoc));
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    if (doc) {
        doc->FlushPendingNotifications(Flush_Layout);
    }

    nsCOMPtr<nsIDocShellTreeNode> node =
        do_QueryInterface(piWin->GetDocShell());
    if (node) {
        PRInt32 i = 0, i_end;
        node->GetChildCount(&i_end);
        for (; i < i_end; ++i) {
            nsCOMPtr<nsIDocShellTreeItem> item;
            node->GetChildAt(i, getter_AddRefs(item));
            nsCOMPtr<nsIDOMWindow> win = do_GetInterface(item);
            if (win) {
                FlushLayoutForTree(win);
            }
        }
    }
}

void nsContentUtils::RemoveNewlines(nsString &aString)
{
  
  static const char badChars[] = {'\r', '\n', 0};
  aString.StripChars(badChars);
}

void
nsContentUtils::PlatformToDOMLineBreaks(nsString &aString)
{
  if (aString.FindChar(PRUnichar('\r')) != -1) {
    
    aString.ReplaceSubstring(NS_LITERAL_STRING("\r\n").get(),
                             NS_LITERAL_STRING("\n").get());

    
    aString.ReplaceSubstring(NS_LITERAL_STRING("\r").get(),
                             NS_LITERAL_STRING("\n").get());
  }
}

static nsIView* GetDisplayRootFor(nsIView* aView)
{
  nsIView *displayRoot = aView;
  for (;;) {
    nsIView *displayParent = displayRoot->GetParent();
    if (!displayParent)
      return displayRoot;

    if (displayRoot->GetFloating() && !displayParent->GetFloating())
      return displayRoot;
    displayRoot = displayParent;
  }
  return nsnull;
}

static already_AddRefed<LayerManager>
LayerManagerForDocumentInternal(nsIDocument *aDoc, bool aRequirePersistent,
                                bool* aAllowRetaining)
{
  nsIDocument* doc = aDoc;
  nsIDocument* displayDoc = doc->GetDisplayDocument();
  if (displayDoc) {
    doc = displayDoc;
  }

  nsIPresShell* shell = doc->GetShell();
  nsCOMPtr<nsISupports> container = doc->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem = do_QueryInterface(container);
  while (!shell && docShellTreeItem) {
    
    
    
    
    nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(docShellTreeItem);
    nsCOMPtr<nsIPresShell> presShell;
    docShell->GetPresShell(getter_AddRefs(presShell));
    if (presShell) {
      shell = presShell;
    } else {
      nsCOMPtr<nsIDocShellTreeItem> parent;
      docShellTreeItem->GetParent(getter_AddRefs(parent));
      docShellTreeItem = parent;
    }
  }

  if (shell) {
    nsIViewManager* VM = shell->GetViewManager();
    if (VM) {
      nsIView* rootView = VM->GetRootView();
      if (rootView) {
        nsIView* displayRoot = GetDisplayRootFor(rootView);
        if (displayRoot) {
          nsIWidget* widget = displayRoot->GetNearestWidget(nsnull);
          if (widget) {
            nsRefPtr<LayerManager> manager =
              widget->
                GetLayerManager(aRequirePersistent ? nsIWidget::LAYER_MANAGER_PERSISTENT : 
                                                     nsIWidget::LAYER_MANAGER_CURRENT,
                                aAllowRetaining);
            return manager.forget();
          }
        }
      }
    }
  }

  return nsnull;
}

already_AddRefed<LayerManager>
nsContentUtils::LayerManagerForDocument(nsIDocument *aDoc, bool *aAllowRetaining)
{
  return LayerManagerForDocumentInternal(aDoc, false, aAllowRetaining);
}

already_AddRefed<LayerManager>
nsContentUtils::PersistentLayerManagerForDocument(nsIDocument *aDoc, bool *aAllowRetaining)
{
  return LayerManagerForDocumentInternal(aDoc, true, aAllowRetaining);
}

bool
nsContentUtils::AllowXULXBLForPrincipal(nsIPrincipal* aPrincipal)
{
  if (IsSystemPrincipal(aPrincipal)) {
    return true;
  }
  
  nsCOMPtr<nsIURI> princURI;
  aPrincipal->GetURI(getter_AddRefs(princURI));
  
  return princURI &&
         ((sAllowXULXBL_for_file && SchemeIs(princURI, "file")) ||
          IsSitePermAllow(princURI, "allowXULXBL"));
}

already_AddRefed<nsIDocumentLoaderFactory>
nsContentUtils::FindInternalContentViewer(const char* aType,
                                          ContentViewerType* aLoaderType)
{
  if (aLoaderType) {
    *aLoaderType = TYPE_UNSUPPORTED;
  }

  
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  if (!catMan)
    return NULL;

  nsCOMPtr<nsIDocumentLoaderFactory> docFactory;

  nsXPIDLCString contractID;
  nsresult rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", aType, getter_Copies(contractID));
  if (NS_SUCCEEDED(rv)) {
    docFactory = do_GetService(contractID);
    if (docFactory && aLoaderType) {
      if (contractID.EqualsLiteral(CONTENT_DLF_CONTRACTID))
        *aLoaderType = TYPE_CONTENT;
      else if (contractID.EqualsLiteral(PLUGIN_DLF_CONTRACTID))
        *aLoaderType = TYPE_PLUGIN;
      else
      *aLoaderType = TYPE_UNKNOWN;
    }
    return docFactory.forget();
  }

#ifdef MOZ_MEDIA
#ifdef MOZ_OGG
  if (nsHTMLMediaElement::IsOggEnabled()) {
    for (unsigned int i = 0; i < NS_ARRAY_LENGTH(nsHTMLMediaElement::gOggTypes); ++i) {
      const char* type = nsHTMLMediaElement::gOggTypes[i];
      if (!strcmp(aType, type)) {
        docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
        if (docFactory && aLoaderType) {
          *aLoaderType = TYPE_CONTENT;
        }
        return docFactory.forget();
      }
    }
  }
#endif

#ifdef MOZ_WEBM
  if (nsHTMLMediaElement::IsWebMEnabled()) {
    for (unsigned int i = 0; i < NS_ARRAY_LENGTH(nsHTMLMediaElement::gWebMTypes); ++i) {
      const char* type = nsHTMLMediaElement::gWebMTypes[i];
      if (!strcmp(aType, type)) {
        docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
        if (docFactory && aLoaderType) {
          *aLoaderType = TYPE_CONTENT;
        }
        return docFactory.forget();
      }
    }
  }
#endif
#endif 

  return NULL;
}


PRBool
nsContentUtils::IsPatternMatching(nsAString& aValue, nsAString& aPattern,
                                  nsIDocument* aDocument)
{
  NS_ASSERTION(aDocument, "aDocument should be a valid pointer (not null)");
  NS_ENSURE_TRUE(aDocument->GetScriptGlobalObject(), PR_TRUE);

  JSContext* ctx = (JSContext*) aDocument->GetScriptGlobalObject()->
                                  GetContext()->GetNativeContext();
  NS_ENSURE_TRUE(ctx, PR_TRUE);

  JSAutoRequest ar(ctx);

  
  aPattern.Insert(NS_LITERAL_STRING("^(?:"), 0);
  aPattern.Append(NS_LITERAL_STRING(")$"));

  JSObject* re = JS_NewUCRegExpObjectNoStatics(ctx, reinterpret_cast<jschar*>
                                                 (aPattern.BeginWriting()),
                                                aPattern.Length(), 0);
  NS_ENSURE_TRUE(re, PR_TRUE);

  jsval rval = JSVAL_NULL;
  size_t idx = 0;
  JSBool res;

  res = JS_ExecuteRegExpNoStatics(ctx, re, reinterpret_cast<jschar*>
                                    (aValue.BeginWriting()),
                                  aValue.Length(), &idx, JS_TRUE, &rval);

  return res == JS_FALSE || rval != JSVAL_NULL;
}
