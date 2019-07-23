











































#include "nsJSUtils.h"
#include "nsCOMPtr.h"
#include "nsAString.h"
#include "nsPrintfCString.h"
#include "nsUnicharUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefLocalizedString.h"
#include "nsServiceManagerUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsDOMCID.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsIContent.h"
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
#include "nsIImageLoadingContent.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsILoadGroup.h"
#include "nsContentPolicyUtils.h"
#include "nsNodeInfoManager.h"
#include "nsIXBLService.h"
#include "nsCRT.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDocumentEvent.h"
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
#include "nsICaseConversion.h"
#include "nsCompressedCharMap.h"
#include "nsINativeKeyBindings.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsXULPopupManager.h"
#include "nsIPermissionManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIRunnable.h"
#include "nsDOMJSUtils.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsReferencedElement.h"
#include "nsIUGenCategory.h"
#include "nsIDragService.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsCPrefetchService.h"
#include "nsIChromeRegistry.h"
#include "nsIMIMEHeaderParam.h"
#include "nsIDOMXULCommandEvent.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMDragEvent.h"
#include "nsDOMDataTransfer.h"
#include "nsHtml5Module.h"
#include "nsPresContext.h"

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
nsIPrefBranch *nsContentUtils::sPrefBranch = nsnull;
nsIPref *nsContentUtils::sPref = nsnull;
imgILoader *nsContentUtils::sImgLoader;
imgICache *nsContentUtils::sImgCache;
nsIConsoleService *nsContentUtils::sConsoleService;
nsDataHashtable<nsISupportsHashKey, EventNameMapping>* nsContentUtils::sEventTable = nsnull;
nsIStringBundleService *nsContentUtils::sStringBundleService;
nsIStringBundle *nsContentUtils::sStringBundles[PropertiesFile_COUNT];
nsIContentPolicy *nsContentUtils::sContentPolicyService;
PRBool nsContentUtils::sTriedToGetContentPolicy = PR_FALSE;
nsILineBreaker *nsContentUtils::sLineBreaker;
nsIWordBreaker *nsContentUtils::sWordBreaker;
nsICaseConversion *nsContentUtils::sCaseConv;
nsIUGenCategory *nsContentUtils::sGenCat;
nsTArray<nsISupports**> *nsContentUtils::sPtrsToPtrsToRelease;
nsIScriptRuntime *nsContentUtils::sScriptRuntimes[NS_STID_ARRAY_UBOUND];
PRInt32 nsContentUtils::sScriptRootCount[NS_STID_ARRAY_UBOUND];
PRUint32 nsContentUtils::sJSGCThingRootCount;
#ifdef IBMBIDI
nsIBidiKeyboard *nsContentUtils::sBidiKeyboard = nsnull;
#endif
PRUint32 nsContentUtils::sScriptBlockerCount = 0;
PRUint32 nsContentUtils::sRemovableScriptBlockerCount = 0;
nsCOMArray<nsIRunnable>* nsContentUtils::sBlockedScriptRunners = nsnull;
PRUint32 nsContentUtils::sRunnersCountAtFirstBlocker = 0;
nsIInterfaceRequestor* nsContentUtils::sSameOriginChecker = nsnull;

nsIJSRuntimeService *nsAutoGCRoot::sJSRuntimeService;
JSRuntime *nsAutoGCRoot::sJSScriptRuntime;

PRBool nsContentUtils::sInitialized = PR_FALSE;

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


nsresult
nsContentUtils::Init()
{
  if (sInitialized) {
    NS_WARNING("Init() called twice");

    return NS_OK;
  }

  nsresult rv = CallGetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID,
                               &sSecurityManager);
  NS_ENSURE_SUCCESS(rv, rv);

  
  CallGetService(NS_PREFSERVICE_CONTRACTID, &sPrefBranch);

  
  CallGetService(NS_PREF_CONTRACTID, &sPref);

  rv = NS_GetNameSpaceManager(&sNameSpaceManager);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallGetService(nsIXPConnect::GetCID(), &sXPConnect);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallGetService(kJSStackContractID, &sThreadJSContextStack);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallGetService(NS_IOSERVICE_CONTRACTID, &sIOService);
  if (NS_FAILED(rv)) {
    

    sIOService = nsnull;
  }

  rv = CallGetService(NS_LBRK_CONTRACTID, &sLineBreaker);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = CallGetService(NS_WBRK_CONTRACTID, &sWordBreaker);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = CallGetService(NS_UNICHARUTIL_CONTRACTID, &sCaseConv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallGetService(NS_UNICHARCATEGORY_CONTRACTID, &sGenCat);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = CallGetService("@mozilla.org/image/loader;1", &sImgLoader);
  if (NS_FAILED(rv)) {
    
    sImgLoader = nsnull;
    sImgCache = nsnull;
  } else {
    if (NS_FAILED(CallGetService("@mozilla.org/image/cache;1", &sImgCache )))
      sImgCache = nsnull;
  }

  sPtrsToPtrsToRelease = new nsTArray<nsISupports**>();
  if (!sPtrsToPtrsToRelease) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

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

  sInitialized = PR_TRUE;

  return NS_OK;
}

PRBool
nsContentUtils::InitializeEventTable() {
  NS_ASSERTION(!sEventTable, "EventTable already initialized!");

  struct EventItem
  {
    nsIAtom** mAtom;
    EventNameMapping mValue;
  };

  static const EventItem eventArray[] = {
    { &nsGkAtoms::onmousedown,                   { NS_MOUSE_BUTTON_DOWN, EventNameType_All }},
    { &nsGkAtoms::onmouseup,                     { NS_MOUSE_BUTTON_UP, EventNameType_All }},
    { &nsGkAtoms::onclick,                       { NS_MOUSE_CLICK, EventNameType_All }},
    { &nsGkAtoms::ondblclick,                    { NS_MOUSE_DOUBLECLICK, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onmouseover,                   { NS_MOUSE_ENTER_SYNTH, EventNameType_All }},
    { &nsGkAtoms::onmouseout,                    { NS_MOUSE_EXIT_SYNTH, EventNameType_All }},
    { &nsGkAtoms::onmousemove,                   { NS_MOUSE_MOVE, EventNameType_All }},
    { &nsGkAtoms::oncontextmenu,                 { NS_CONTEXTMENU, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onkeydown,                     { NS_KEY_DOWN, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onkeyup,                       { NS_KEY_UP, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onkeypress,                    { NS_KEY_PRESS, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onfocus,                       { NS_FOCUS_CONTENT, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onblur,                        { NS_BLUR_CONTENT, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onoffline,                     { NS_OFFLINE, EventNameType_HTMLXUL }},
    { &nsGkAtoms::ononline,                      { NS_ONLINE, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onsubmit,                      { NS_FORM_SUBMIT, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onreset,                       { NS_FORM_RESET, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onchange,                      { NS_FORM_CHANGE, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onselect,                      { NS_FORM_SELECTED, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onload,                        { NS_LOAD, EventNameType_All }},
    { &nsGkAtoms::onunload,                      { NS_PAGE_UNLOAD,
                                                 (EventNameType_HTMLXUL | EventNameType_SVGSVG) }},
    { &nsGkAtoms::onhashchange,                  { NS_HASHCHANGE, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onbeforeunload,                { NS_BEFORE_PAGE_UNLOAD, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onabort,                       { NS_IMAGE_ABORT,
                                                 (EventNameType_HTMLXUL | EventNameType_SVGSVG) }},
    { &nsGkAtoms::onerror,                       { NS_LOAD_ERROR,
                                                 (EventNameType_HTMLXUL | EventNameType_SVGSVG) }},
    { &nsGkAtoms::onDOMAttrModified,             { NS_MUTATION_ATTRMODIFIED, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMCharacterDataModified,    { NS_MUTATION_CHARACTERDATAMODIFIED, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMNodeInserted,             { NS_MUTATION_NODEINSERTED, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMNodeRemoved,              { NS_MUTATION_NODEREMOVED, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMNodeInsertedIntoDocument, { NS_MUTATION_NODEINSERTEDINTODOCUMENT, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMNodeRemovedFromDocument,  { NS_MUTATION_NODEREMOVEDFROMDOCUMENT, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMSubtreeModified,          { NS_MUTATION_SUBTREEMODIFIED, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMActivate,                 { NS_UI_ACTIVATE, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMFocusIn,                  { NS_UI_FOCUSIN, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMFocusOut,                 { NS_UI_FOCUSOUT, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onDOMMouseScroll,              { NS_MOUSE_SCROLL, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onMozMousePixelScroll,         { NS_MOUSE_PIXEL_SCROLL, EventNameType_HTMLXUL }},
    { &nsGkAtoms::oninput,                       { NS_FORM_INPUT, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onpageshow,                    { NS_PAGE_SHOW, EventNameType_HTML }},
    { &nsGkAtoms::onpagehide,                    { NS_PAGE_HIDE, EventNameType_HTML }},
    { &nsGkAtoms::onresize,                      { NS_RESIZE_EVENT,
                                                 (EventNameType_HTMLXUL | EventNameType_SVGSVG) }},
    { &nsGkAtoms::onscroll,                      { NS_SCROLL_EVENT,
                                                 (EventNameType_HTMLXUL | EventNameType_SVGSVG) }},
    { &nsGkAtoms::oncopy,                        { NS_COPY, EventNameType_HTMLXUL }},
    { &nsGkAtoms::oncut,                         { NS_CUT, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onpaste,                       { NS_PASTE, EventNameType_HTMLXUL }},
    
    { &nsGkAtoms::ontext,                        { NS_TEXT_TEXT, EventNameType_XUL }},
    { &nsGkAtoms::oncompositionstart,            { NS_COMPOSITION_START, EventNameType_XUL }},
    { &nsGkAtoms::oncompositionend,              { NS_COMPOSITION_END, EventNameType_XUL }},
    { &nsGkAtoms::onclose,                       { NS_XUL_CLOSE, EventNameType_XUL }},
    { &nsGkAtoms::onpopupshowing,                { NS_XUL_POPUP_SHOWING, EventNameType_XUL }},
    { &nsGkAtoms::onpopupshown,                  { NS_XUL_POPUP_SHOWN, EventNameType_XUL }},
    { &nsGkAtoms::onpopuphiding,                 { NS_XUL_POPUP_HIDING, EventNameType_XUL }},
    { &nsGkAtoms::onpopuphidden,                 { NS_XUL_POPUP_HIDDEN, EventNameType_XUL }},
    { &nsGkAtoms::oncommand,                     { NS_XUL_COMMAND, EventNameType_XUL }},
    { &nsGkAtoms::onbroadcast,                   { NS_XUL_BROADCAST, EventNameType_XUL }},
    { &nsGkAtoms::oncommandupdate,               { NS_XUL_COMMAND_UPDATE, EventNameType_XUL }},
    { &nsGkAtoms::ondragenter,                   { NS_DRAGDROP_ENTER, EventNameType_HTMLXUL }},
    { &nsGkAtoms::ondragover,                    { NS_DRAGDROP_OVER_SYNTH, EventNameType_HTMLXUL }},
    { &nsGkAtoms::ondragexit,                    { NS_DRAGDROP_EXIT_SYNTH, EventNameType_XUL }},
    { &nsGkAtoms::ondragdrop,                    { NS_DRAGDROP_DRAGDROP, EventNameType_XUL }},
    { &nsGkAtoms::ondraggesture,                 { NS_DRAGDROP_GESTURE, EventNameType_XUL }},
    { &nsGkAtoms::ondrag,                        { NS_DRAGDROP_DRAG, EventNameType_HTMLXUL }},
    { &nsGkAtoms::ondragend,                     { NS_DRAGDROP_END, EventNameType_HTMLXUL }},
    { &nsGkAtoms::ondragstart,                   { NS_DRAGDROP_START, EventNameType_HTMLXUL }},
    { &nsGkAtoms::ondragleave,                   { NS_DRAGDROP_LEAVE_SYNTH, EventNameType_HTMLXUL }},
    { &nsGkAtoms::ondrop,                        { NS_DRAGDROP_DROP, EventNameType_HTMLXUL }},
    { &nsGkAtoms::onoverflow,                    { NS_SCROLLPORT_OVERFLOW, EventNameType_XUL }},
    { &nsGkAtoms::onunderflow,                   { NS_SCROLLPORT_UNDERFLOW, EventNameType_XUL }},
#ifdef MOZ_SVG
    { &nsGkAtoms::onSVGLoad,                     { NS_SVG_LOAD, EventNameType_None }},
    { &nsGkAtoms::onSVGUnload,                   { NS_SVG_UNLOAD, EventNameType_None }},
    { &nsGkAtoms::onSVGAbort,                    { NS_SVG_ABORT, EventNameType_None }},
    { &nsGkAtoms::onSVGError,                    { NS_SVG_ERROR, EventNameType_None }},
    { &nsGkAtoms::onSVGResize,                   { NS_SVG_RESIZE, EventNameType_None }},
    { &nsGkAtoms::onSVGScroll,                   { NS_SVG_SCROLL, EventNameType_None }},
    { &nsGkAtoms::onSVGZoom,                     { NS_SVG_ZOOM, EventNameType_None }},
    { &nsGkAtoms::onzoom,                        { NS_SVG_ZOOM, EventNameType_SVGSVG }},
#endif 
#ifdef MOZ_MEDIA 
    { &nsGkAtoms::onloadstart,                   { NS_LOADSTART, EventNameType_HTML }},
    { &nsGkAtoms::onprogress,                    { NS_PROGRESS, EventNameType_HTML }},
    { &nsGkAtoms::onsuspend,                     { NS_SUSPEND, EventNameType_HTML }},
    { &nsGkAtoms::onemptied,                     { NS_EMPTIED, EventNameType_HTML }},
    { &nsGkAtoms::onstalled,                     { NS_STALLED, EventNameType_HTML }},
    { &nsGkAtoms::onplay,                        { NS_PLAY, EventNameType_HTML }},
    { &nsGkAtoms::onpause,                       { NS_PAUSE, EventNameType_HTML }},
    { &nsGkAtoms::onloadedmetadata,              { NS_LOADEDMETADATA, EventNameType_HTML }},
    { &nsGkAtoms::onloadeddata,                  { NS_LOADEDDATA, EventNameType_HTML }},
    { &nsGkAtoms::onwaiting,                     { NS_WAITING, EventNameType_HTML }},
    { &nsGkAtoms::onplaying,                     { NS_PLAYING, EventNameType_HTML }},
    { &nsGkAtoms::oncanplay,                     { NS_CANPLAY, EventNameType_HTML }},
    { &nsGkAtoms::oncanplaythrough,              { NS_CANPLAYTHROUGH, EventNameType_HTML }},
    { &nsGkAtoms::onseeking,                     { NS_SEEKING, EventNameType_HTML }},
    { &nsGkAtoms::onseeked,                      { NS_SEEKED, EventNameType_HTML }},
    { &nsGkAtoms::ontimeupdate,                  { NS_TIMEUPDATE, EventNameType_HTML }},
    { &nsGkAtoms::onended,                       { NS_ENDED, EventNameType_HTML }},
    { &nsGkAtoms::onratechange,                  { NS_RATECHANGE, EventNameType_HTML }},
    { &nsGkAtoms::ondurationchange,              { NS_DURATIONCHANGE, EventNameType_HTML }},
    { &nsGkAtoms::onvolumechange,                { NS_VOLUMECHANGE, EventNameType_HTML }},
#endif 
    { &nsGkAtoms::onMozAfterPaint,               { NS_AFTERPAINT, EventNameType_None }},

    
    { &nsGkAtoms::onMozSwipeGesture,             { NS_SIMPLE_GESTURE_SWIPE, EventNameType_None } },
    { &nsGkAtoms::onMozMagnifyGestureStart,      { NS_SIMPLE_GESTURE_MAGNIFY_START, EventNameType_None } },
    { &nsGkAtoms::onMozMagnifyGestureUpdate,     { NS_SIMPLE_GESTURE_MAGNIFY_UPDATE, EventNameType_None } },
    { &nsGkAtoms::onMozMagnifyGesture,           { NS_SIMPLE_GESTURE_MAGNIFY, EventNameType_None } },
    { &nsGkAtoms::onMozRotateGestureStart,       { NS_SIMPLE_GESTURE_ROTATE_START, EventNameType_None } },
    { &nsGkAtoms::onMozRotateGestureUpdate,      { NS_SIMPLE_GESTURE_ROTATE_UPDATE, EventNameType_None } },
    { &nsGkAtoms::onMozRotateGesture,            { NS_SIMPLE_GESTURE_ROTATE, EventNameType_None } },
    { &nsGkAtoms::onMozTapGesture,               { NS_SIMPLE_GESTURE_TAP, EventNameType_None } },
    { &nsGkAtoms::onMozPressTapGesture,          { NS_SIMPLE_GESTURE_PRESSTAP, EventNameType_None } },
  };

  sEventTable = new nsDataHashtable<nsISupportsHashKey, EventNameMapping>;
  if (!sEventTable ||
      !sEventTable->Init(int(NS_ARRAY_LENGTH(eventArray) / 0.75) + 1)) {
    delete sEventTable;
    sEventTable = nsnull;
    return PR_FALSE;
  }

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(eventArray); ++i) {
    if (!sEventTable->Put(*(eventArray[i].mAtom), eventArray[i].mValue)) {
      delete sEventTable;
      sEventTable = nsnull;
      return PR_FALSE;
    }
  }

  return PR_TRUE;
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



void
nsContentUtils::GetOfflineAppManifest(nsIDocument *aDocument, nsIURI **aURI)
{
  nsCOMPtr<nsIContent> docElement = aDocument->GetRootContent();
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
                                            aDocument, aDocument->GetBaseURI());
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
  NS_IF_RELEASE(sStringBundleService);
  NS_IF_RELEASE(sConsoleService);
  NS_IF_RELEASE(sDOMScriptObjectFactory);
  if (sJSGCThingRootCount == 0 && sXPConnect)
    NS_RELEASE(sXPConnect);
  NS_IF_RELEASE(sSecurityManager);
  NS_IF_RELEASE(sThreadJSContextStack);
  NS_IF_RELEASE(sNameSpaceManager);
  NS_IF_RELEASE(sParserService);
  NS_IF_RELEASE(sIOService);
  NS_IF_RELEASE(sLineBreaker);
  NS_IF_RELEASE(sWordBreaker);
  NS_IF_RELEASE(sCaseConv);
  NS_IF_RELEASE(sGenCat);
#ifdef MOZ_XTF
  NS_IF_RELEASE(sXTFService);
#endif
  NS_IF_RELEASE(sImgLoader);
  NS_IF_RELEASE(sImgCache);
  NS_IF_RELEASE(sPrefBranch);
  NS_IF_RELEASE(sPref);
#ifdef IBMBIDI
  NS_IF_RELEASE(sBidiKeyboard);
#endif

  delete sEventTable;
  sEventTable = nsnull;

  if (sPtrsToPtrsToRelease) {
    for (i = 0; i < sPtrsToPtrsToRelease->Length(); ++i) {
      nsISupports** ptrToPtr = sPtrsToPtrsToRelease->ElementAt(i);
      NS_RELEASE(*ptrToPtr);
    }
    delete sPtrsToPtrsToRelease;
    sPtrsToPtrsToRelease = nsnull;
  }

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
  
  nsAutoGCRoot::Shutdown();
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
nsContentUtils::CheckSameOrigin(nsIDOMNode *aTrustedNode,
                                nsIDOMNode *aUnTrustedNode)
{
  NS_PRECONDITION(aTrustedNode, "There must be a trusted node");

  PRBool isSystem = PR_FALSE;
  sSecurityManager->SubjectPrincipalIsSystem(&isSystem);
  if (isSystem) {
    

    return NS_OK;
  }

  


  nsCOMPtr<nsINode> trustedNode = do_QueryInterface(aTrustedNode);
  nsCOMPtr<nsINode> unTrustedNode = do_QueryInterface(aUnTrustedNode);

  
  NS_ENSURE_TRUE(trustedNode && unTrustedNode, NS_ERROR_UNEXPECTED);

  nsIPrincipal* trustedPrincipal = trustedNode->NodePrincipal();
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
  nsIContent* root = doc->GetRootContent();

  return !root || doc->IndexOf(aNode) < doc->IndexOf(root);
}


nsresult
nsContentUtils::doReparentContentWrapper(nsIContent *aNode,
                                         JSContext *cx,
                                         JSObject *aOldGlobal,
                                         JSObject *aNewGlobal,
                                         nsIDocument *aOldDocument,
                                         nsIDocument *aNewDocument)
{
  nsCOMPtr<nsIXPConnectJSObjectHolder> old_wrapper;

  nsresult rv;

  rv = sXPConnect->ReparentWrappedNativeIfFound(cx, aOldGlobal, aNewGlobal,
                                                aNode,
                                                getter_AddRefs(old_wrapper));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  

  PRUint32 i, count = aNode->GetChildCount();

  for (i = 0; i < count; i++) {
    nsIContent *child = aNode->GetChildAt(i);
    NS_ENSURE_TRUE(child, NS_ERROR_UNEXPECTED);

    rv = doReparentContentWrapper(child, cx, 
                                  aOldGlobal, aNewGlobal,
                                  aOldDocument, aNewDocument);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

static JSContext *
GetContextFromDocument(nsIDocument *aDocument, JSObject** aGlobalObject)
{
  nsIScriptGlobalObject *sgo = aDocument->GetScopeObject();
  if (!sgo) {
    

    *aGlobalObject = nsnull;

    return nsnull;
  }

  *aGlobalObject = sgo->GetGlobalJSObject();

  nsIScriptContext *scx = sgo->GetContext();
  if (!scx) {
    

    return nsnull;
  }

  return (JSContext *)scx->GetNativeContext();
}


nsresult
nsContentUtils::ReparentContentWrapper(nsIContent *aContent,
                                       nsIContent *aNewParent,
                                       nsIDocument *aNewDocument,
                                       nsIDocument *aOldDocument)
{
  
  
  if (!aOldDocument || !aNewDocument || aNewDocument == aOldDocument) {
    return NS_OK;
  }

  JSContext *cx;
  JSObject *oldScope, *newScope;
  nsresult rv = GetContextAndScopes(aOldDocument, aNewDocument, &cx, &oldScope,
                                    &newScope);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!cx) {
    return NS_OK;
  }

  return doReparentContentWrapper(aContent, cx, oldScope, newScope, 
                                  aOldDocument, aNewDocument);
}


nsresult
nsContentUtils::GetContextAndScopes(nsIDocument *aOldDocument,
                                    nsIDocument *aNewDocument, JSContext **aCx,
                                    JSObject **aOldScope, JSObject **aNewScope)
{
  *aCx = nsnull;
  *aOldScope = nsnull;
  *aNewScope = nsnull;

  JSObject *newScope = nsnull;
  nsIScriptGlobalObject *newSGO = aNewDocument->GetScopeObject();
  if (!newSGO || !(newScope = newSGO->GetGlobalJSObject())) {
    return NS_OK;
  }

  NS_ENSURE_TRUE(sXPConnect, NS_ERROR_NOT_INITIALIZED);

  
  
  
  
  
  
  
  JSObject *oldScope = nsnull;
  JSContext *cx = GetContextFromDocument(aOldDocument, &oldScope);

  if (!oldScope) {
    return NS_OK;
  }

  if (!cx) {
    JSObject *dummy;
    cx = GetContextFromDocument(aNewDocument, &dummy);

    if (!cx) {
      
      
      

      sThreadJSContextStack->Peek(&cx);

      if (!cx) {
        sThreadJSContextStack->GetSafeJSContext(&cx);

        if (!cx) {
          
          NS_WARNING("No context reachable in ReparentContentWrapper()!");

          return NS_ERROR_NOT_AVAILABLE;
        }
      }
    }
  }

  *aCx = cx;
  *aOldScope = oldScope;
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

  return sXPConnect->ReparentScopeAwareWrappers(cx, oldScopeObj, newScopeObj);
}

nsIDocShell *
nsContentUtils::GetDocShellFromCaller()
{
  JSContext *cx = nsnull;
  sThreadJSContextStack->Peek(&cx);

  if (cx) {
    nsIScriptGlobalObject *sgo = nsJSUtils::GetDynamicScriptGlobal(cx);
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(sgo));

    if (win) {
      return win->GetDocShell();
    }
  }

  return nsnull;
}

nsIDOMDocument *
nsContentUtils::GetDocumentFromCaller()
{
  JSContext *cx = nsnull;
  sThreadJSContextStack->Peek(&cx);

  nsIDOMDocument *doc = nsnull;

  if (cx) {
    JSObject *callee = nsnull;
    JSStackFrame *fp = nsnull;
    while (!callee && (fp = ::JS_FrameIterator(cx, &fp))) {
      callee = ::JS_GetFrameCalleeObject(cx, fp);
    }

    nsCOMPtr<nsPIDOMWindow> win =
      do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(cx, callee));
    if (win) {
      doc = win->GetExtantDocument();
    }
  }

  return doc;
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


PRBool
nsContentUtils::ContentIsDescendantOf(nsINode* aPossibleDescendant,
                                      nsINode* aPossibleAncestor)
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



nsresult
nsContentUtils::GetAncestors(nsIDOMNode* aNode,
                             nsTArray<nsIDOMNode*>* aArray)
{
  NS_ENSURE_ARG_POINTER(aNode);

  nsCOMPtr<nsIDOMNode> node(aNode);
  nsCOMPtr<nsIDOMNode> ancestor;

  do {
    aArray->AppendElement(node.get());
    node->GetParentNode(getter_AddRefs(ancestor));
    node.swap(ancestor);
  } while (node);

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
  for (len = PR_MIN(pos1, pos2); len > 0; --len) {
    nsINode* child1 = parents1.ElementAt(--pos1);
    nsINode* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      break;
    }
    parent = child1;
  }

  return parent;
}

PRUint16
nsContentUtils::ComparePosition(nsINode* aNode1,
                                nsINode* aNode2)
{
  NS_PRECONDITION(aNode1 && aNode2, "don't pass null");

  if (aNode1 == aNode2) {
    return 0;
  }

  nsAutoTPtrArray<nsINode, 32> parents1, parents2;

  
  nsIAttribute* attr1 = nsnull;
  if (aNode1->IsNodeOfType(nsINode::eATTRIBUTE)) {
    attr1 = static_cast<nsIAttribute*>(aNode1);
    nsIContent* elem = attr1->GetContent();
    
    
    if (elem) {
      aNode1 = elem;
      parents1.AppendElement(static_cast<nsINode*>(attr1));
    }
  }
  if (aNode2->IsNodeOfType(nsINode::eATTRIBUTE)) {
    nsIAttribute* attr2 = static_cast<nsIAttribute*>(aNode2);
    nsIContent* elem = attr2->GetContent();
    if (elem == aNode1 && attr1) {
      
      

      PRUint32 i;
      const nsAttrName* attrName;
      for (i = 0; (attrName = elem->GetAttrNameAt(i)); ++i) {
        if (attrName->Equals(attr1->NodeInfo())) {
          NS_ASSERTION(!attrName->Equals(attr2->NodeInfo()),
                       "Different attrs at same position");
          return nsIDOM3Node::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC |
            nsIDOM3Node::DOCUMENT_POSITION_PRECEDING;
        }
        if (attrName->Equals(attr2->NodeInfo())) {
          return nsIDOM3Node::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC |
            nsIDOM3Node::DOCUMENT_POSITION_FOLLOWING;
        }
      }
      NS_NOTREACHED("neither attribute in the element");
      return nsIDOM3Node::DOCUMENT_POSITION_DISCONNECTED;
    }

    if (elem) {
      aNode2 = elem;
      parents2.AppendElement(static_cast<nsINode*>(attr2));
    }
  }

  
  
  
  

  
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
  nsINode* top1 = parents1.ElementAt(--pos1);
  nsINode* top2 = parents2.ElementAt(--pos2);
  if (top1 != top2) {
    return top1 < top2 ?
      (nsIDOM3Node::DOCUMENT_POSITION_PRECEDING |
       nsIDOM3Node::DOCUMENT_POSITION_DISCONNECTED |
       nsIDOM3Node::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC) :
      (nsIDOM3Node::DOCUMENT_POSITION_FOLLOWING |
       nsIDOM3Node::DOCUMENT_POSITION_DISCONNECTED |
       nsIDOM3Node::DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC);
  }

  
  nsINode* parent = top1;
  PRUint32 len;
  for (len = PR_MIN(pos1, pos2); len > 0; --len) {
    nsINode* child1 = parents1.ElementAt(--pos1);
    nsINode* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      
      
      
      return parent->IndexOf(child1) < parent->IndexOf(child2) ?
        static_cast<PRUint16>(nsIDOM3Node::DOCUMENT_POSITION_PRECEDING) :
        static_cast<PRUint16>(nsIDOM3Node::DOCUMENT_POSITION_FOLLOWING);
    }
    parent = child1;
  }

  
  
  
  return pos1 < pos2 ?
    (nsIDOM3Node::DOCUMENT_POSITION_PRECEDING |
     nsIDOM3Node::DOCUMENT_POSITION_CONTAINS) :
    (nsIDOM3Node::DOCUMENT_POSITION_FOLLOWING |
     nsIDOM3Node::DOCUMENT_POSITION_CONTAINED_BY);    
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
  for (len = PR_MIN(pos1, pos2); len > 0; --len) {
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

nsIContent*
nsContentUtils::FindFirstChildWithResolvedTag(nsIContent* aParent,
                                              PRInt32 aNamespace,
                                              nsIAtom* aTag)
{
  nsIDocument* doc;
  if (!aParent || !(doc = aParent->GetOwnerDoc())) {
    return nsnull;
  }
  
  nsBindingManager* bindingManager = doc->BindingManager();

  PRInt32 namespaceID;
  PRUint32 count = aParent->GetChildCount();

  PRUint32 i;

  for (i = 0; i < count; i++) {
    nsIContent *child = aParent->GetChildAt(i);
    nsIAtom* tag =  bindingManager->ResolveTag(child, &namespaceID);
    if (tag == aTag && namespaceID == aNamespace) {
      return child;
    }
  }

  
  nsCOMPtr<nsIDOMNodeList> children;
  bindingManager->GetXBLChildNodesFor(aParent, getter_AddRefs(children));
  if (!children) {
    return nsnull;
  }

  PRUint32 length;
  children->GetLength(&length);
  for (i = 0; i < length; i++) {
    nsCOMPtr<nsIDOMNode> childNode;
    children->Item(i, getter_AddRefs(childNode));
    nsCOMPtr<nsIContent> childContent = do_QueryInterface(childNode);
    nsIAtom* tag = bindingManager->ResolveTag(childContent, &namespaceID);
    if (tag == aTag && namespaceID == aNamespace) {
      return childContent;
    }
  }

  return nsnull;
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






const nsDependentSubstring
nsContentUtils::TrimWhitespace(const nsAString& aStr, PRBool aTrimTrailing)
{
  nsAString::const_iterator start, end;

  aStr.BeginReading(start);
  aStr.EndReading(end);

  
  while (start != end && nsCRT::IsAsciiSpace(*start)) {
    ++start;
  }

  if (aTrimTrailing) {
    
    while (end != start) {
      --end;

      if (!nsCRT::IsAsciiSpace(*end)) {
        
        ++end;

        break;
      }
    }
  }

  
  

  return Substring(start, end);
}

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

  const char* atomString = nsnull;
  aAtom->GetUTF8String(&atomString);

  KeyAppendString(nsDependentCString(atomString), aKey);
}

static inline PRBool IsAutocompleteOff(nsIDOMElement* aElement)
{
  nsAutoString autocomplete;
  aElement->GetAttribute(NS_LITERAL_STRING("autocomplete"), autocomplete);
  return autocomplete.LowerCaseEqualsLiteral("off");
}

 nsresult
nsContentUtils::GenerateStateKey(nsIContent* aContent,
                                 nsIDocument* aDocument,
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

  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(aContent));
  if (element && IsAutocompleteOff(element)) {
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
      nsCOMPtr<nsIDOMHTMLFormElement> formElement;
      control->GetForm(getter_AddRefs(formElement));
      if (formElement) {

        if (IsAutocompleteOff(formElement)) {
          aKey.Truncate();
          return NS_OK;
        }

        KeyAppendString(NS_LITERAL_CSTRING("f"), aKey);

        
        nsCOMPtr<nsIContent> formContent(do_QueryInterface(formElement));
        index = htmlForms->IndexOf(formContent, PR_FALSE);
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
        formElement->GetName(formName);
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
    
    
    
    
    KeyAppendString(NS_LITERAL_CSTRING("o"), aKey);
    
    
    
    
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
nsContentUtils::BelongsInForm(nsIDOMHTMLFormElement *aForm,
                              nsIContent *aContent)
{
  NS_PRECONDITION(aForm, "Must have a form");
  NS_PRECONDITION(aContent, "Must have a content node");

  nsCOMPtr<nsIContent> form(do_QueryInterface(aForm));

  if (!form) {
    NS_ERROR("This should not happen, form is not an nsIContent!");

    return PR_TRUE;
  }

  if (form == aContent) {
    

    return PR_FALSE;
  }

  nsIContent* content = aContent->GetParent();

  while (content) {
    if (content == form) {
      

      return PR_TRUE;
    }

    if (content->Tag() == nsGkAtoms::form &&
        content->IsNodeOfType(nsINode::eHTML)) {
      
      

      return PR_FALSE;
    }

    content = content->GetParent();
  }

  if (form->GetChildCount() > 0) {
    
    

    return PR_FALSE;
  }

  
  
  
  
  if (PositionIsBefore(form, aContent)) {
    
    
    
    
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
nsContentUtils::SplitQName(nsIContent* aNamespaceResolver,
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
    rv = LookupNamespaceURI(aNamespaceResolver, Substring(aQName.get(), colon),
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
nsContentUtils::LookupNamespaceURI(nsIContent* aNamespaceResolver,
                                   const nsAString& aNamespacePrefix,
                                   nsAString& aNamespaceURI)
{
  if (aNamespacePrefix.EqualsLiteral("xml")) {
    
    aNamespaceURI.AssignLiteral("http://www.w3.org/XML/1998/namespace");
    return NS_OK;
  }

  if (aNamespacePrefix.EqualsLiteral("xmlns")) {
    
    aNamespaceURI.AssignLiteral("http://www.w3.org/2000/xmlns/");
    return NS_OK;
  }

  nsCOMPtr<nsIAtom> name;
  if (!aNamespacePrefix.IsEmpty()) {
    name = do_GetAtom(aNamespacePrefix);
    NS_ENSURE_TRUE(name, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    name = nsGkAtoms::xmlns;
  }
  
  
  for (nsIContent* content = aNamespaceResolver; content;
       content = content->GetParent()) {
    if (content->GetAttr(kNameSpaceID_XMLNS, name, aNamespaceURI))
      return NS_OK;
  }
  return NS_ERROR_FAILURE;
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
      *aPrefix = NS_NewAtom(NS_ConvertUTF16toUTF8(prefixStart,
                                                  pos - prefixStart));
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
  *aLocalName = NS_NewAtom(NS_ConvertUTF16toUTF8(nameStart,
                                                 nameEnd - nameStart));
}


nsPresContext*
nsContentUtils::GetContextForContent(nsIContent* aContent)
{
  nsIDocument* doc = aContent->GetCurrentDoc();
  if (doc) {
    nsIPresShell *presShell = doc->GetPrimaryShell();
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

  if (!sImgLoader) {
    
    return NS_OK;
  }

  nsCOMPtr<nsILoadGroup> loadGroup = aLoadingDocument->GetDocumentLoadGroup();
  NS_ASSERTION(loadGroup, "Could not get loadgroup; onload may fire too early");

  nsIURI *documentURI = aLoadingDocument->GetDocumentURI();

  
  NS_TryToSetImmutable(aURI);

  
  

  
  
  return sImgLoader->LoadImage(aURI,                 
                               documentURI,          
                               aReferrer,            
                               loadGroup,            
                               aObserver,            
                               aLoadingDocument,     
                               aLoadFlags,           
                               nsnull,               
                               nsnull,               
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
nsContentUtils::IsDraggableLink(nsIContent* aContent) {
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


PRPackedBool
nsContentUtils::GetBoolPref(const char *aPref, PRBool aDefault)
{
  PRBool result;

  if (!sPrefBranch ||
      NS_FAILED(sPrefBranch->GetBoolPref(aPref, &result))) {
    result = aDefault;
  }

  return (PRPackedBool)result;
}


PRInt32
nsContentUtils::GetIntPref(const char *aPref, PRInt32 aDefault)
{
  PRInt32 result;

  if (!sPrefBranch ||
      NS_FAILED(sPrefBranch->GetIntPref(aPref, &result))) {
    result = aDefault;
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
  if (sPref)
    sPref->RegisterCallback(aPref, aCallback, aClosure);
}


void
nsContentUtils::UnregisterPrefCallback(const char *aPref,
                                       PrefChangedFunc aCallback,
                                       void * aClosure)
{
  if (sPref)
    sPref->UnregisterCallback(aPref, aCallback, aClosure);
}

static int
BoolVarChanged(const char *aPref, void *aClosure)
{
  PRBool* cache = static_cast<PRBool*>(aClosure);
  *cache = nsContentUtils::GetBoolPref(aPref, PR_FALSE);
  
  return 0;
}

void
nsContentUtils::AddBoolPrefVarCache(const char *aPref,
                                    PRBool* aCache)
{
  *aCache = GetBoolPref(aPref, PR_FALSE);
  RegisterPrefCallback(aPref, BoolVarChanged, aCache);
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
nsCxPusher::Push(JSContext *cx)
{
  if (mPushedSomething) {
    NS_ERROR("Whaaa! No double pushing with nsCxPusher::Push()!");

    return PR_FALSE;
  }

  if (!cx) {
    return PR_FALSE;
  }

  
  
  
  mScx = GetScriptContextFromJSContext(cx);
  if (!mScx) {
    
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
                                const char *aCategory)
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

  nsCOMPtr<nsIScriptError> errorObject =
      do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = errorObject->Init(errorText.get(),
                         NS_ConvertUTF8toUTF16(spec).get(), 
                         aSourceLine.get(),
                         aLineNumber, aColumnNumber,
                         aErrorFlags, aCategory);
  NS_ENSURE_SUCCESS(rv, rv);

  return sConsoleService->LogMessage(errorObject);
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
nsContentUtils::GetWrapperSafeScriptFilename(nsIDocument *aDocument,
                                             nsIURI *aURI,
                                             nsACString& aScriptURI)
{
  PRBool scriptFileNameModified = PR_FALSE;
  aURI->GetSpec(aScriptURI);

  if (IsChromeDoc(aDocument)) {
    nsCOMPtr<nsIChromeRegistry> chromeReg =
      do_GetService(NS_CHROMEREGISTRY_CONTRACTID);

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


nsresult
nsAutoGCRoot::AddJSGCRoot(void* aPtr, const char* aName)
{
  if (!sJSScriptRuntime) {
    nsresult rv = CallGetService("@mozilla.org/js/xpc/RuntimeService;1",
                                 &sJSRuntimeService);
    NS_ENSURE_TRUE(sJSRuntimeService, rv);

    sJSRuntimeService->GetRuntime(&sJSScriptRuntime);
    if (!sJSScriptRuntime) {
      NS_RELEASE(sJSRuntimeService);
      NS_WARNING("Unable to get JS runtime from JS runtime service");
      return NS_ERROR_FAILURE;
    }
  }

  PRBool ok;
  ok = ::JS_AddNamedRootRT(sJSScriptRuntime, aPtr, aName);
  if (!ok) {
    NS_WARNING("JS_AddNamedRootRT failed");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}


nsresult
nsAutoGCRoot::RemoveJSGCRoot(void* aPtr)
{
  if (!sJSScriptRuntime) {
    NS_NOTREACHED("Trying to remove a JS GC root when none were added");
    return NS_ERROR_UNEXPECTED;
  }

  ::JS_RemoveRootRT(sJSScriptRuntime, aPtr);

  return NS_OK;
}


PRBool
nsContentUtils::IsEventAttributeName(nsIAtom* aName, PRInt32 aType)
{
  const char* name;
  aName->GetUTF8String(&name);
  if (name[0] != 'o' || name[1] != 'n')
    return PR_FALSE;

  EventNameMapping mapping;
  return (sEventTable->Get(aName, &mapping) && mapping.mType & aType);
}


PRUint32
nsContentUtils::GetEventId(nsIAtom* aName)
{
  EventNameMapping mapping;
  if (sEventTable->Get(aName, &mapping))
    return mapping.mId;

  return NS_USER_DEFINED_EVENT;
}

static
nsresult GetEventAndTarget(nsIDocument* aDoc, nsISupports* aTarget,
                           const nsAString& aEventName,
                           PRBool aCanBubble, PRBool aCancelable,
                           nsIDOMEvent** aEvent,
                           nsIDOMEventTarget** aTargetOut)
{
  nsCOMPtr<nsIDOMDocumentEvent> docEvent(do_QueryInterface(aDoc));
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(aTarget));
  NS_ENSURE_TRUE(docEvent && target, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv =
    docEvent->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
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
  if (!aDoc->GetWindow()->GetChromeEventHandler())
    return NS_ERROR_INVALID_ARG;

  nsEventStatus status = nsEventStatus_eIgnore;
  rv = aDoc->GetWindow()->GetChromeEventHandler()->DispatchDOMEvent(nsnull,
                                                                    event,
                                                                    nsnull,
                                                                    &status);
  if (aDefaultAction) {
    *aDefaultAction = (status != nsEventStatus_eConsumeNoDefault);
  }
  return rv;
}


nsIContent*
nsContentUtils::MatchElementId(nsIContent *aContent, nsIAtom* aId)
{
  if (aId == aContent->GetID()) {
    return aContent;
  }
  
  nsIContent *result = nsnull;
  PRUint32 i, count = aContent->GetChildCount();

  for (i = 0; i < count && result == nsnull; i++) {
    result = MatchElementId(aContent->GetChildAt(i), aId);
  }

  return result;
}




nsIContent *
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
  else if (aLength >= 4 &&
           aBuffer[0] == 0x00 &&
           aBuffer[1] == 0x00 &&
           aBuffer[2] == 0xFE &&
           aBuffer[3] == 0xFF) {
    aCharset = "UTF-32";
    if (bigEndian)
      *bigEndian = PR_TRUE;
  }
  else if (aLength >= 4 &&
           aBuffer[0] == 0xFF &&
           aBuffer[1] == 0xFE &&
           aBuffer[2] == 0x00 &&
           aBuffer[3] == 0x00) {
    aCharset = "UTF-32";
    if (bigEndian)
      *bigEndian = PR_FALSE;
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


nsIContent*
nsContentUtils::GetReferencedElement(nsIURI* aURI, nsIContent *aFromContent)
{
  nsReferencedElement ref;
  ref.Reset(aFromContent, aURI);
  return ref.get();
}


PRBool
nsContentUtils::HasNonEmptyAttr(nsIContent* aContent, PRInt32 aNameSpaceID,
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
nsContentUtils::CreateContextualFragment(nsIDOMNode* aContextNode,
                                         const nsAString& aFragment,
                                         PRBool aWillOwnFragment,
                                         nsIDOMDocumentFragment** aReturn)
{
  NS_ENSURE_ARG(aContextNode);
  *aReturn = nsnull;

  nsresult rv;
  nsCOMPtr<nsINode> node = do_QueryInterface(aContextNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NOT_AVAILABLE);

  
  
  nsCOMPtr<nsIDocument> document = node->GetOwnerDoc();
  NS_ENSURE_TRUE(document, NS_ERROR_NOT_AVAILABLE);
  
  PRBool bCaseSensitive = !document->IsHTML();

  nsCOMPtr<nsIHTMLDocument> htmlDoc(do_QueryInterface(document));
  PRBool isHTML = htmlDoc && !bCaseSensitive;

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
      document->SetFragmentParser(parser);
    }
    nsCOMPtr<nsIDOMDocumentFragment> frag;
    rv = NS_NewDocumentFragment(getter_AddRefs(frag), document->NodeInfoManager());
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIContent> contextAsContent = do_QueryInterface(aContextNode);
    if (contextAsContent && !contextAsContent->IsNodeOfType(nsINode::eELEMENT)) {
      contextAsContent = contextAsContent->GetParent();
      if (contextAsContent && !contextAsContent->IsNodeOfType(nsINode::eELEMENT)) {
        
        contextAsContent = nsnull;
      }
    }
    
    if (contextAsContent) {
      parser->ParseFragment(aFragment, 
                            frag, 
                            contextAsContent->Tag(), 
                            contextAsContent->GetNameSpaceID(), 
                            (document->GetCompatibilityMode() == eCompatibility_NavQuirks));    
    } else {
      parser->ParseFragment(aFragment, 
                            frag, 
                            nsGkAtoms::body, 
                            kNameSpaceID_XHTML, 
                            (document->GetCompatibilityMode() == eCompatibility_NavQuirks));
    }
  
    NS_ADDREF(*aReturn = frag);
    return NS_OK;
  }

  nsAutoTArray<nsString, 32> tagStack;
  nsAutoString uriStr, nameStr;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aContextNode);
  
  if (content && !content->IsNodeOfType(nsINode::eELEMENT))
    content = content->GetParent();

  while (content && content->IsNodeOfType(nsINode::eELEMENT)) {
    nsString& tagName = *tagStack.AppendElement();
    NS_ENSURE_TRUE(&tagName, NS_ERROR_OUT_OF_MEMORY);

    content->NodeInfo()->GetQualifiedName(tagName);

    
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
    if (child->IsNodeOfType(nsINode::eELEMENT)) {
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
nsContentUtils::IsInSameAnonymousTree(nsINode* aNode,
                                      nsIContent* aContent)
{
  NS_PRECONDITION(aNode,
                  "Must have a node to work with");
  NS_PRECONDITION(aContent,
                  "Must have a content to work with");
  
  if (!aNode->IsNodeOfType(nsINode::eCONTENT)) {
    






    return aContent->GetBindingParent() == nsnull;
  }

  return static_cast<nsIContent*>(aNode)->GetBindingParent() ==
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
  nsresult rv = sXPConnect->AddJSHolder(aScriptObjectHolder, aTracer);
  NS_ENSURE_SUCCESS(rv, rv);

  ++sJSGCThingRootCount;
  NS_LOG_ADDREF(sXPConnect, sJSGCThingRootCount, "HoldJSObjects",
                sizeof(void*));

  return NS_OK;
}


nsresult
nsContentUtils::DropJSObjects(void* aScriptObjectHolder)
{
  NS_LOG_RELEASE(sXPConnect, sJSGCThingRootCount - 1, "HoldJSObjects");
  nsresult rv = sXPConnect->RemoveJSHolder(aScriptObjectHolder);
  if (--sJSGCThingRootCount == 0 && !sInitialized) {
    NS_RELEASE(sXPConnect);
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
    PRUint32 len = PR_MIN(tmp.Length(), NS_ARRAY_LENGTH(sBuf) - 1);
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
nsContentUtils::RemoveScriptBlocker()
{
  NS_ASSERTION(sScriptBlockerCount != 0, "Negative script blockers");
  --sScriptBlockerCount;
  if (sScriptBlockerCount) {
    return;
  }

  PRUint32 firstBlocker = sRunnersCountAtFirstBlocker;
  PRUint32 lastBlocker = sBlockedScriptRunners->Count();
  sRunnersCountAtFirstBlocker = 0;
  NS_ASSERTION(firstBlocker <= lastBlocker,
               "bad sRunnersCountAtFirstBlocker");

  while (firstBlocker < lastBlocker) {
    nsCOMPtr<nsIRunnable> runnable = (*sBlockedScriptRunners)[firstBlocker];
    sBlockedScriptRunners->RemoveObjectAt(firstBlocker);
    --lastBlocker;

    runnable->Run();
    NS_ASSERTION(lastBlocker == sBlockedScriptRunners->Count() &&
                 sRunnersCountAtFirstBlocker == 0,
                 "Bad count");
    NS_ASSERTION(!sScriptBlockerCount, "This is really bad");
  }
}



PRBool
nsContentUtils::AddScriptRunner(nsIRunnable* aRunnable)
{
  if (!aRunnable) {
    return PR_FALSE;
  }

  if (sScriptBlockerCount) {
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

  
  const nsAString &key = Substring(tail, tip);
  const nsAString &value = Substring(++tip, end);

  

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

#define IS_SEPARATOR(c) ((c == ' ') || (c == ',') || (c == ';'))

nsresult
nsContentUtils::ProcessViewportInfo(nsIDocument *aDocument,
                                    const nsAString &viewportInfo) {

  
  nsresult rv = NS_OK;

  
  nsAString::const_iterator tip, tail, end;
  viewportInfo.BeginReading(tip);
  tail = tip;
  viewportInfo.EndReading(end);

  
  while ((tip != end) && IS_SEPARATOR(*tip))
    ++tip;

  
  while (tip != end) {
    
    
    tail = tip;

    
    while ((tip != end) && !IS_SEPARATOR(*tip))
      ++tip;

    
    ProcessViewportToken(aDocument, Substring(tail, tip));

    
    while ((tip != end) && IS_SEPARATOR(*tip))
      ++tip;
  }

  return rv;

}

#undef IS_SEPARATOR


void
nsContentUtils::HidePopupsInDocument(nsIDocument* aDocument)
{
  NS_PRECONDITION(aDocument, "Null document");

#ifdef MOZ_XUL
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
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
    
    
    
    
    
    PRUint32 action = 0;
    dragSession->GetDragAction(&action);
    initialDataTransfer =
      new nsDOMDataTransfer(aDragEvent->message, action);
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
nsAutoGCRoot::Shutdown()
{
  NS_IF_RELEASE(sJSRuntimeService);
}

nsIAtom*
nsContentUtils::IsNamedItem(nsIContent* aContent)
{
  
  
  
  
  nsGenericHTMLElement* elm = nsGenericHTMLElement::FromContent(aContent);
  if (!elm) {
    return nsnull;
  }

  nsIAtom* tag = elm->Tag();
  if (tag != nsGkAtoms::img    &&
      tag != nsGkAtoms::form   &&
      tag != nsGkAtoms::applet &&
      tag != nsGkAtoms::embed  &&
      tag != nsGkAtoms::object) {
    return nsnull;
  }

  const nsAttrValue* val = elm->GetParsedAttr(nsGkAtoms::name);
  if (val && val->Type() == nsAttrValue::eAtom) {
    return val->GetAtomValue();
  }

  return nsnull;
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


NS_IMPL_ISUPPORTS2(nsSameOriginChecker,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor)

NS_IMETHODIMP
nsSameOriginChecker::OnChannelRedirect(nsIChannel *aOldChannel,
                                       nsIChannel *aNewChannel,
                                       PRUint32    aFlags)
{
  NS_PRECONDITION(aNewChannel, "Redirecting to null channel?");
  if (!nsContentUtils::GetSecurityManager()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

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
  } else if (!fp->script) {
    fp = nsnull;
  }

  void *annotation = fp ? JS_GetFrameAnnotation(cx, fp) : nsnull;
  PRBool privileged;
  if (NS_SUCCEEDED(principal->IsCapabilityEnabled("UniversalXPConnect",
                                                  annotation,
                                                  &privileged)) &&
      privileged) {
    
    return PR_TRUE;
  }

  
  
  static const char prefix[] = "chrome://global/";
  const char *filename;
  if (fp && fp->script &&
      (filename = fp->script->filename) &&
      !strncmp(filename, prefix, NS_ARRAY_LENGTH(prefix) - 1)) {
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
  nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(doc);
  NS_ENSURE_STATE(docEvent);
  nsCOMPtr<nsIDOMEvent> event;
  docEvent->CreateEvent(NS_LITERAL_STRING("xulcommandevent"),
                        getter_AddRefs(event));
  nsCOMPtr<nsIDOMXULCommandEvent> xulCommand = do_QueryInterface(event);
  nsCOMPtr<nsIPrivateDOMEvent> pEvent = do_QueryInterface(xulCommand);
  NS_ENSURE_STATE(pEvent);
  nsCOMPtr<nsIDOMAbstractView> view = do_QueryInterface(doc->GetWindow());
  nsresult rv = xulCommand->InitCommandEvent(NS_LITERAL_STRING("command"),
                                             PR_TRUE, PR_TRUE, view,
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
