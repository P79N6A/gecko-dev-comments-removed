







#include "mozilla/DebugOnly.h"
#include "mozilla/Util.h"
#include "mozilla/Likely.h"

#include "jsapi.h"
#include "jsdbgapi.h"
#include "jsfriendapi.h"

#include <math.h>

#include "mozilla/dom/TextDecoderBase.h"

#include "Layers.h"
#include "nsJSUtils.h"
#include "nsCOMPtr.h"
#include "nsAString.h"
#include "nsPrintfCString.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsDOMCID.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsINodeInfo.h"
#include "nsIIdleService.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNode.h"
#include "nsIIOService.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsIScriptSecurityManager.h"
#include "nsPIDOMWindow.h"
#include "nsIJSContextStack.h"
#include "nsIDocShell.h"
#include "nsParserCIID.h"
#include "nsIParser.h"
#include "nsIFragmentContentSink.h"
#include "nsIContentSink.h"
#include "nsContentList.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsGkAtoms.h"
#include "imgINotificationObserver.h"
#include "imgRequestProxy.h"
#include "imgIContainer.h"
#include "imgLoader.h"
#include "nsDocShellCID.h"
#include "nsIImageLoadingContent.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsILoadGroup.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsContentPolicyUtils.h"
#include "nsNodeInfoManager.h"
#include "nsCRT.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIMIMEService.h"
#include "nsLWBrkCIID.h"
#include "nsILineBreaker.h"
#include "nsIWordBreaker.h"
#include "nsUnicodeProperties.h"
#include "harfbuzz/hb.h"
#include "nsIJSRuntimeService.h"
#include "nsBindingManager.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsICharsetConverterManager.h"
#include "nsEventListenerManager.h"
#include "nsAttrName.h"
#include "nsIDOMUserDataHandler.h"
#include "nsContentCreatorFunctions.h"
#include "nsMutationEvent.h"
#include "nsIMEStateManager.h"
#include "nsError.h"
#include "nsUnicharUtilCIID.h"
#include "nsINativeKeyBindings.h"
#include "nsXULPopupManager.h"
#include "nsIPermissionManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsNullPrincipal.h"
#include "nsIRunnable.h"
#include "nsDOMJSUtils.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsReferencedElement.h"
#include "nsIDragService.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsCPrefetchService.h"
#include "nsIChromeRegistry.h"
#include "nsEventDispatcher.h"
#include "nsIDOMXULCommandEvent.h"
#include "nsDOMDataTransfer.h"
#include "nsHtml5Module.h"
#include "nsPresContext.h"
#include "nsLayoutStatics.h"
#include "nsFocusManager.h"
#include "nsTextEditorState.h"
#include "nsIPluginHost.h"
#include "nsICategoryManager.h"
#include "nsViewManager.h"
#include "nsEventStateManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsParserConstants.h"
#include "nsIWebNavigation.h"
#include "nsILoadContext.h"
#include "nsTextFragment.h"
#include "mozilla/Selection.h"
#include <algorithm>

#ifdef IBMBIDI
#include "nsIBidiKeyboard.h"
#endif
#include "nsCycleCollectionParticipant.h"


#include "nsIStringBundle.h"
#include "nsIScriptError.h"
#include "nsIConsoleService.h"

#include "mozAutoDocUpdate.h"
#include "imgICache.h"
#include "imgLoader.h"
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
#include "nsIContentViewer.h"
#include "nsIObjectLoadingContent.h"
#include "nsCCUncollectableMarker.h"
#include "mozilla/Base64.h"
#include "mozilla/Preferences.h"
#include "nsDOMMutationObserver.h"
#include "nsIDOMDocumentType.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsICharsetDetector.h"
#include "nsICharsetDetectionObserver.h"
#include "nsIPlatformCharset.h"
#include "nsIEditor.h"
#include "mozilla/Attributes.h"
#include "nsIParserService.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsSandboxFlags.h"
#include "nsSVGFeatures.h"
#include "MediaDecoder.h"
#include "DecoderTraits.h"

#include "nsWrapperCacheInlines.h"
#include "nsViewportInfo.h"

extern "C" int MOZ_XMLTranslateEntity(const char* ptr, const char* end,
                                      const char** next, PRUnichar* result);
extern "C" int MOZ_XMLCheckQName(const char* ptr, const char* end,
                                 int ns_aware, const char** colon);

class imgLoader;

using namespace mozilla::dom;
using namespace mozilla::layers;
using namespace mozilla::widget;
using namespace mozilla;

const char kLoadAsData[] = "loadAsData";

nsIDOMScriptObjectFactory *nsContentUtils::sDOMScriptObjectFactory = nullptr;
nsIXPConnect *nsContentUtils::sXPConnect;
nsIScriptSecurityManager *nsContentUtils::sSecurityManager;
nsIThreadJSContextStack *nsContentUtils::sThreadJSContextStack;
nsIParserService *nsContentUtils::sParserService = nullptr;
nsINameSpaceManager *nsContentUtils::sNameSpaceManager;
nsIIOService *nsContentUtils::sIOService;
imgLoader *nsContentUtils::sImgLoader;
imgLoader *nsContentUtils::sPrivateImgLoader;
imgICache *nsContentUtils::sImgCache;
imgICache *nsContentUtils::sPrivateImgCache;
nsIConsoleService *nsContentUtils::sConsoleService;
nsDataHashtable<nsISupportsHashKey, EventNameMapping>* nsContentUtils::sAtomEventTable = nullptr;
nsDataHashtable<nsStringHashKey, EventNameMapping>* nsContentUtils::sStringEventTable = nullptr;
nsCOMArray<nsIAtom>* nsContentUtils::sUserDefinedEvents = nullptr;
nsIStringBundleService *nsContentUtils::sStringBundleService;
nsIStringBundle *nsContentUtils::sStringBundles[PropertiesFile_COUNT];
nsIContentPolicy *nsContentUtils::sContentPolicyService;
bool nsContentUtils::sTriedToGetContentPolicy = false;
nsILineBreaker *nsContentUtils::sLineBreaker;
nsIWordBreaker *nsContentUtils::sWordBreaker;
#ifdef IBMBIDI
nsIBidiKeyboard *nsContentUtils::sBidiKeyboard = nullptr;
#endif
uint32_t nsContentUtils::sScriptBlockerCount = 0;
#ifdef DEBUG
uint32_t nsContentUtils::sDOMNodeRemovedSuppressCount = 0;
#endif
uint32_t nsContentUtils::sMicroTaskLevel = 0;
nsTArray< nsCOMPtr<nsIRunnable> >* nsContentUtils::sBlockedScriptRunners = nullptr;
uint32_t nsContentUtils::sRunnersCountAtFirstBlocker = 0;
nsIInterfaceRequestor* nsContentUtils::sSameOriginChecker = nullptr;

bool nsContentUtils::sIsHandlingKeyBoardEvent = false;
bool nsContentUtils::sAllowXULXBL_for_file = false;

nsString* nsContentUtils::sShiftText = nullptr;
nsString* nsContentUtils::sControlText = nullptr;
nsString* nsContentUtils::sMetaText = nullptr;
nsString* nsContentUtils::sOSText = nullptr;
nsString* nsContentUtils::sAltText = nullptr;
nsString* nsContentUtils::sModifierSeparator = nullptr;

bool nsContentUtils::sInitialized = false;
bool nsContentUtils::sIsFullScreenApiEnabled = false;
bool nsContentUtils::sTrustedFullScreenOnly = true;
bool nsContentUtils::sIsIdleObserverAPIEnabled = false;

uint32_t nsContentUtils::sHandlingInputTimeout = 1000;

nsHtml5StringParser* nsContentUtils::sHTMLFragmentParser = nullptr;
nsIParser* nsContentUtils::sXMLFragmentParser = nullptr;
nsIFragmentContentSink* nsContentUtils::sXMLFragmentSink = nullptr;
bool nsContentUtils::sFragmentParsingActive = false;

namespace {

static const char kJSStackContractID[] = "@mozilla.org/js/xpc/ContextStack;1";
static NS_DEFINE_CID(kParserServiceCID, NS_PARSERSERVICE_CID);
static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);

static PLDHashTable sEventListenerManagersHash;

class DOMEventListenerManagersHashReporter MOZ_FINAL : public MemoryReporterBase
{
public:
  DOMEventListenerManagersHashReporter()
    : MemoryReporterBase(
        "explicit/dom/event-listener-managers-hash",
        KIND_HEAP,
        UNITS_BYTES,
        "Memory used by the event listener manager's hash table.")
  {}

private:
  int64_t Amount()
  {
    
    
    return sEventListenerManagersHash.ops
         ? PL_DHashTableSizeOfExcludingThis(&sEventListenerManagersHash,
                                            nullptr, MallocSizeOf)
         : 0;
  }
};

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

protected:          
  const void *mKey; 

public:
  nsRefPtr<nsEventListenerManager> mListenerManager;
};

static bool
EventListenerManagerHashInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                                  const void *key)
{
  
  new (entry) EventListenerManagerMapEntry(key);
  return true;
}

static void
EventListenerManagerHashClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  EventListenerManagerMapEntry *lm =
    static_cast<EventListenerManagerMapEntry *>(entry);

  
  lm->~EventListenerManagerMapEntry();
}

class SameOriginChecker MOZ_FINAL : public nsIChannelEventSink,
                                    public nsIInterfaceRequestor
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
};

class CharsetDetectionObserver MOZ_FINAL : public nsICharsetDetectionObserver
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD Notify(const char *aCharset, nsDetectionConfident aConf)
  {
    mCharset = aCharset;
    return NS_OK;
  }

  const nsACString& GetResult() const
  {
    return mCharset;
  }

private:
  nsCString mCharset;
};

} 


TimeDuration
nsContentUtils::HandlingUserInputTimeout()
{
  return TimeDuration::FromMilliseconds(sHandlingInputTimeout);
}


nsresult
nsContentUtils::Init()
{
  if (sInitialized) {
    NS_WARNING("Init() called twice");

    return NS_OK;
  }

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
    

    sIOService = nullptr;
  }

  rv = CallGetService(NS_LBRK_CONTRACTID, &sLineBreaker);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = CallGetService(NS_WBRK_CONTRACTID, &sWordBreaker);
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
                           nullptr, sizeof(EventListenerManagerMapEntry), 16)) {
      sEventListenerManagersHash.ops = nullptr;

      return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_RegisterMemoryReporter(new DOMEventListenerManagersHashReporter);
  }

  sBlockedScriptRunners = new nsTArray< nsCOMPtr<nsIRunnable> >;

  Preferences::AddBoolVarCache(&sAllowXULXBL_for_file,
                               "dom.allow_XUL_XBL_for_file");

  Preferences::AddBoolVarCache(&sIsFullScreenApiEnabled,
                               "full-screen-api.enabled");

  Preferences::AddBoolVarCache(&sTrustedFullScreenOnly,
                               "full-screen-api.allow-trusted-requests-only");

  sIsIdleObserverAPIEnabled = Preferences::GetBool("dom.idle-observers-api.enabled", true);

  Preferences::AddUintVarCache(&sHandlingInputTimeout,
                               "dom.event.handling-user-input-time-limit",
                               1000);

  Element::InitCCCallbacks();

  sInitialized = true;

  return NS_OK;
}

void
nsContentUtils::GetShiftText(nsAString& text)
{
  if (!sShiftText)
    InitializeModifierStrings();
  text.Assign(*sShiftText);
}

void
nsContentUtils::GetControlText(nsAString& text)
{
  if (!sControlText)
    InitializeModifierStrings();
  text.Assign(*sControlText);
}

void
nsContentUtils::GetMetaText(nsAString& text)
{
  if (!sMetaText)
    InitializeModifierStrings();
  text.Assign(*sMetaText);
}

void
nsContentUtils::GetOSText(nsAString& text)
{
  if (!sOSText) {
    InitializeModifierStrings();
  }
  text.Assign(*sOSText);
}

void
nsContentUtils::GetAltText(nsAString& text)
{
  if (!sAltText)
    InitializeModifierStrings();
  text.Assign(*sAltText);
}

void
nsContentUtils::GetModifierSeparatorText(nsAString& text)
{
  if (!sModifierSeparator)
    InitializeModifierStrings();
  text.Assign(*sModifierSeparator);
}

void
nsContentUtils::InitializeModifierStrings()
{
  
  nsCOMPtr<nsIStringBundleService> bundleService =
    mozilla::services::GetStringBundleService();
  nsCOMPtr<nsIStringBundle> bundle;
  DebugOnly<nsresult> rv = NS_OK;
  if (bundleService) {
    rv = bundleService->CreateBundle( "chrome://global-platform/locale/platformKeys.properties",
                                      getter_AddRefs(bundle));
  }
  
  NS_ASSERTION(NS_SUCCEEDED(rv) && bundle, "chrome://global/locale/platformKeys.properties could not be loaded");
  nsXPIDLString shiftModifier;
  nsXPIDLString metaModifier;
  nsXPIDLString osModifier;
  nsXPIDLString altModifier;
  nsXPIDLString controlModifier;
  nsXPIDLString modifierSeparator;
  if (bundle) {
    
    bundle->GetStringFromName(NS_LITERAL_STRING("VK_SHIFT").get(), getter_Copies(shiftModifier));
    bundle->GetStringFromName(NS_LITERAL_STRING("VK_META").get(), getter_Copies(metaModifier));
    bundle->GetStringFromName(NS_LITERAL_STRING("VK_WIN").get(), getter_Copies(osModifier));
    bundle->GetStringFromName(NS_LITERAL_STRING("VK_ALT").get(), getter_Copies(altModifier));
    bundle->GetStringFromName(NS_LITERAL_STRING("VK_CONTROL").get(), getter_Copies(controlModifier));
    bundle->GetStringFromName(NS_LITERAL_STRING("MODIFIER_SEPARATOR").get(), getter_Copies(modifierSeparator));
  }
  
  sShiftText = new nsString(shiftModifier);
  sMetaText = new nsString(metaModifier);
  sOSText = new nsString(osModifier);
  sAltText = new nsString(altModifier);
  sControlText = new nsString(controlModifier);
  sModifierSeparator = new nsString(modifierSeparator);  
}

bool nsContentUtils::sImgLoaderInitialized;

void
nsContentUtils::InitImgLoader()
{
  sImgLoaderInitialized = true;

  
  sImgLoader = imgLoader::Create();
  NS_ABORT_IF_FALSE(sImgLoader, "Creation should have succeeded");

  sPrivateImgLoader = imgLoader::Create();
  NS_ABORT_IF_FALSE(sPrivateImgLoader, "Creation should have succeeded");

  NS_ADDREF(sImgCache = sImgLoader);
  NS_ADDREF(sPrivateImgCache = sPrivateImgLoader);

  sPrivateImgCache->RespectPrivacyNotifications();
}

bool
nsContentUtils::InitializeEventTable() {
  NS_ASSERTION(!sAtomEventTable, "EventTable already initialized!");
  NS_ASSERTION(!sStringEventTable, "EventTable already initialized!");

  static const EventNameMapping eventArray[] = {
#define EVENT(name_,  _id, _type, _struct)          \
    { nsGkAtoms::on##name_, _id, _type, _struct },
#define WINDOW_ONLY_EVENT EVENT
#define NON_IDL_EVENT EVENT
#include "nsEventNameList.h"
#undef WINDOW_ONLY_EVENT
#undef EVENT
    { nullptr }
  };

  sAtomEventTable = new nsDataHashtable<nsISupportsHashKey, EventNameMapping>;
  sStringEventTable = new nsDataHashtable<nsStringHashKey, EventNameMapping>;
  sUserDefinedEvents = new nsCOMArray<nsIAtom>(64);
  sAtomEventTable->Init(int(ArrayLength(eventArray) / 0.75) + 1);
  sStringEventTable->Init(int(ArrayLength(eventArray) / 0.75) + 1);

  
  for (uint32_t i = 0; i < ArrayLength(eventArray) - 1; ++i) {
    sAtomEventTable->Put(eventArray[i].mAtom, eventArray[i]);
    sStringEventTable->Put(Substring(nsDependentAtomString(eventArray[i].mAtom), 2),
                           eventArray[i]);
  }

  return true;
}

void
nsContentUtils::InitializeTouchEventTable()
{
  static bool sEventTableInitialized = false;
  if (!sEventTableInitialized && sAtomEventTable && sStringEventTable) {
    sEventTableInitialized = true;
    static const EventNameMapping touchEventArray[] = {
#define EVENT(name_,  _id, _type, _struct)
#define TOUCH_EVENT(name_,  _id, _type, _struct)      \
      { nsGkAtoms::on##name_, _id, _type, _struct },
#include "nsEventNameList.h"
#undef TOUCH_EVENT
#undef EVENT
      { nullptr }
    };
    
    for (uint32_t i = 0; i < ArrayLength(touchEventArray) - 1; ++i) {
      sAtomEventTable->Put(touchEventArray[i].mAtom, touchEventArray[i]);
      sStringEventTable->Put(Substring(nsDependentAtomString(touchEventArray[i].mAtom), 2),
                             touchEventArray[i]);
    }
  }
}

static bool
Is8bit(const nsAString& aString)
{
  static const PRUnichar EIGHT_BIT = PRUnichar(~0x00FF);

  nsAString::const_iterator done_reading;
  aString.EndReading(done_reading);

  
  uint32_t fragmentLength = 0;
  nsAString::const_iterator iter;
  for (aString.BeginReading(iter); iter != done_reading;
       iter.advance(int32_t(fragmentLength))) {
    fragmentLength = uint32_t(iter.size_forward());
    const PRUnichar* c = iter.get();
    const PRUnichar* fragmentEnd = c + fragmentLength;

    
    while (c < fragmentEnd) {
      if (*c++ & EIGHT_BIT) {
        return false;
      }
    }
  }

  return true;
}

nsresult
nsContentUtils::Btoa(const nsAString& aBinaryData,
                     nsAString& aAsciiBase64String)
{
  if (!Is8bit(aBinaryData)) {
    aAsciiBase64String.Truncate();
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  return Base64Encode(aBinaryData, aAsciiBase64String);
}

nsresult
nsContentUtils::Atob(const nsAString& aAsciiBase64String,
                     nsAString& aBinaryData)
{
  if (!Is8bit(aAsciiBase64String)) {
    aBinaryData.Truncate();
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  nsresult rv = Base64Decode(aAsciiBase64String, aBinaryData);
  if (NS_FAILED(rv) && rv == NS_ERROR_INVALID_ARG) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }
  return rv;
}

bool
nsContentUtils::IsAutocompleteEnabled(nsIDOMHTMLInputElement* aInput)
{
  NS_PRECONDITION(aInput, "aInput should not be null!");

  nsAutoString autocomplete;
  aInput->GetAutocomplete(autocomplete);

  if (autocomplete.IsEmpty()) {
    nsCOMPtr<nsIDOMHTMLFormElement> form;
    aInput->GetForm(getter_AddRefs(form));
    if (!form) {
      return true;
    }

    form->GetAutocomplete(autocomplete);
  }

  return autocomplete.EqualsLiteral("on");
}

#define SKIP_WHITESPACE(iter, end_iter, end_res)                 \
  while ((iter) != (end_iter) && nsCRT::IsAsciiSpace(*(iter))) { \
    ++(iter);                                                    \
  }                                                              \
  if ((iter) == (end_iter)) {                                    \
    return (end_res);                                            \
  }

#define SKIP_ATTR_NAME(iter, end_iter)                            \
  while ((iter) != (end_iter) && !nsCRT::IsAsciiSpace(*(iter)) && \
         *(iter) != '=') {                                        \
    ++(iter);                                                     \
  }

bool
nsContentUtils::GetPseudoAttributeValue(const nsString& aSource, nsIAtom *aName,
                                        nsAString& aValue)
{
  aValue.Truncate();

  const PRUnichar *start = aSource.get();
  const PRUnichar *end = start + aSource.Length();
  const PRUnichar *iter;

  while (start != end) {
    SKIP_WHITESPACE(start, end, false)
    iter = start;
    SKIP_ATTR_NAME(iter, end)

    if (start == iter) {
      return false;
    }

    
    const nsDependentSubstring & attrName = Substring(start, iter);

    
    start = iter;
    SKIP_WHITESPACE(start, end, false)
    if (*start != '=') {
      
      
      return false;
    }

    
    ++start;
    SKIP_WHITESPACE(start, end, false)
    PRUnichar q = *start;
    if (q != kQuote && q != kApostrophe) {
      
      return false;
    }

    ++start;  
    iter = start;

    while (iter != end && *iter != q) {
      ++iter;
    }

    if (iter == end) {
      
      return false;
    }

    
    

    if (aName->Equals(attrName)) {
      
      
      
      const PRUnichar *chunkEnd = start;
      while (chunkEnd != iter) {
        if (*chunkEnd == kLessThan) {
          aValue.Truncate();

          return false;
        }

        if (*chunkEnd == kAmpersand) {
          aValue.Append(start, chunkEnd - start);

          
          ++chunkEnd;

          const PRUnichar *afterEntity = nullptr;
          PRUnichar result[2];
          uint32_t count =
            MOZ_XMLTranslateEntity(reinterpret_cast<const char*>(chunkEnd),
                                  reinterpret_cast<const char*>(iter),
                                  reinterpret_cast<const char**>(&afterEntity),
                                  result);
          if (count == 0) {
            aValue.Truncate();

            return false;
          }

          aValue.Append(result, count);

          
          start = chunkEnd = afterEntity;
        }
        else {
          ++chunkEnd;
        }
      }

      
      aValue.Append(start, iter - start);

      return true;
    }

    
    
    start = iter + 1;
  }

  return false;
}

bool
nsContentUtils::IsJavaScriptLanguage(const nsString& aName)
{
  return aName.LowerCaseEqualsLiteral("javascript") ||
         aName.LowerCaseEqualsLiteral("livescript") ||
         aName.LowerCaseEqualsLiteral("mocha") ||
         aName.LowerCaseEqualsLiteral("javascript1.0") ||
         aName.LowerCaseEqualsLiteral("javascript1.1") ||
         aName.LowerCaseEqualsLiteral("javascript1.2") ||
         aName.LowerCaseEqualsLiteral("javascript1.3") ||
         aName.LowerCaseEqualsLiteral("javascript1.4") ||
         aName.LowerCaseEqualsLiteral("javascript1.5");
}

JSVersion
nsContentUtils::ParseJavascriptVersion(const nsAString& aVersionStr)
{
  if (aVersionStr.Length() != 3 || aVersionStr[0] != '1' ||
      aVersionStr[1] != '.') {
    return JSVERSION_UNKNOWN;
  }

  switch (aVersionStr[2]) {
  case '0': 
  case '1': 
  case '2': 
  case '3': 
  case '4': 
  case '5': return JSVERSION_DEFAULT;
  case '6': return JSVERSION_1_6;
  case '7': return JSVERSION_1_7;
  case '8': return JSVERSION_1_8;
  default:  return JSVERSION_UNKNOWN;
  }
}

void
nsContentUtils::SplitMimeType(const nsAString& aValue, nsString& aType,
                              nsString& aParams)
{
  aType.Truncate();
  aParams.Truncate();
  int32_t semiIndex = aValue.FindChar(PRUnichar(';'));
  if (-1 != semiIndex) {
    aType = Substring(aValue, 0, semiIndex);
    aParams = Substring(aValue, semiIndex + 1,
                       aValue.Length() - (semiIndex + 1));
    aParams.StripWhitespace();
  }
  else {
    aType = aValue;
  }
  aType.StripWhitespace();
}

nsresult 
nsContentUtils::IsUserIdle(uint32_t aRequestedIdleTimeInMS, bool* aUserIsIdle)
{
  nsresult rv;
  nsCOMPtr<nsIIdleService> idleService = 
    do_GetService("@mozilla.org/widget/idleservice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
    
  uint32_t idleTimeInMS;
  rv = idleService->GetIdleTime(&idleTimeInMS);
  NS_ENSURE_SUCCESS(rv, rv);

  *aUserIsIdle = idleTimeInMS >= aRequestedIdleTimeInMS;
  return NS_OK;
}






nsIParserService*
nsContentUtils::GetParserService()
{
  
  if (!sParserService) {
    
    
    nsresult rv = CallGetService(kParserServiceCID, &sParserService);
    if (NS_FAILED(rv)) {
      sParserService = nullptr;
    }
  }

  return sParserService;
}








uint32_t
nsContentUtils::ParseSandboxAttributeToFlags(const nsAString& aSandboxAttrValue)
{
  
  
  uint32_t out = SANDBOXED_NAVIGATION |
                 SANDBOXED_TOPLEVEL_NAVIGATION |
                 SANDBOXED_PLUGINS |
                 SANDBOXED_ORIGIN |
                 SANDBOXED_FORMS |
                 SANDBOXED_SCRIPTS |
                 SANDBOXED_AUTOMATIC_FEATURES |
                 SANDBOXED_POINTER_LOCK;

  if (!aSandboxAttrValue.IsEmpty()) {
    
    
    HTMLSplitOnSpacesTokenizer tokenizer(aSandboxAttrValue, ' ',
      nsCharSeparatedTokenizerTemplate<nsContentUtils::IsHTMLWhitespace>::SEPARATOR_OPTIONAL);

    while (tokenizer.hasMoreTokens()) {
      nsDependentSubstring token = tokenizer.nextToken();
      if (token.LowerCaseEqualsLiteral("allow-same-origin")) {
        out &= ~SANDBOXED_ORIGIN;
      } else if (token.LowerCaseEqualsLiteral("allow-forms")) {
        out &= ~SANDBOXED_FORMS;
      } else if (token.LowerCaseEqualsLiteral("allow-scripts")) {
        
        
        out &= ~SANDBOXED_SCRIPTS;
        out &= ~SANDBOXED_AUTOMATIC_FEATURES;
      } else if (token.LowerCaseEqualsLiteral("allow-top-navigation")) {
        out &= ~SANDBOXED_TOPLEVEL_NAVIGATION;
      } else if (token.LowerCaseEqualsLiteral("allow-pointer-lock")) {
        out &= ~SANDBOXED_POINTER_LOCK;
      }
    }
  }

  return out;
}

#ifdef IBMBIDI
nsIBidiKeyboard*
nsContentUtils::GetBidiKeyboard()
{
  if (!sBidiKeyboard) {
    nsresult rv = CallGetService("@mozilla.org/widget/bidikeyboard;1", &sBidiKeyboard);
    if (NS_FAILED(rv)) {
      sBidiKeyboard = nullptr;
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

template <>
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

template <>
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
                          bool aLastCharCR=false) :
      mLastCharCR(aLastCharCR),
      mDestination(aDestination),
      mWritten(0)
    { }

    uint32_t GetCharsWritten() {
      return mWritten;
    }

    bool IsLastCharCR() {
      return mLastCharCR;
    }

    void write(const typename OutputIterator::value_type* aSource, uint32_t aSourceLength) {

      const typename OutputIterator::value_type* done_writing = aSource + aSourceLength;

      
      if (mLastCharCR) {
        
        
        if (aSourceLength && (*aSource == value_type('\n'))) {
          ++aSource;
        }
        mLastCharCR = false;
      }

      uint32_t num_written = 0;
      while ( aSource < done_writing ) {
        if (*aSource == value_type('\r')) {
          mDestination->writechar('\n');
          ++aSource;
          
          
          if (aSource == done_writing) {
            mLastCharCR = true;
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
    bool mLastCharCR;
    OutputIterator* mDestination;
    uint32_t mWritten;
};


uint32_t
nsContentUtils::CopyNewlineNormalizedUnicodeTo(const nsAString& aSource,
                                               uint32_t aSrcOffset,
                                               PRUnichar* aDest,
                                               uint32_t aLength,
                                               bool& aLastCharCR)
{
  typedef NormalizeNewlinesCharTraits<PRUnichar*> sink_traits;

  sink_traits dest_traits(aDest);
  CopyNormalizeNewlines<sink_traits> normalizer(&dest_traits,aLastCharCR);
  nsReadingIterator<PRUnichar> fromBegin, fromEnd;
  copy_string(aSource.BeginReading(fromBegin).advance( int32_t(aSrcOffset) ),
              aSource.BeginReading(fromEnd).advance( int32_t(aSrcOffset+aLength) ),
              normalizer);
  aLastCharCR = normalizer.IsLastCharCR();
  return normalizer.GetCharsWritten();
}


uint32_t
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









bool
nsContentUtils::IsFirstLetterPunctuation(uint32_t aChar)
{
  uint8_t cat = mozilla::unicode::GetGeneralCategory(aChar);

  return (cat == HB_UNICODE_GENERAL_CATEGORY_OPEN_PUNCTUATION ||     
          cat == HB_UNICODE_GENERAL_CATEGORY_CLOSE_PUNCTUATION ||    
          cat == HB_UNICODE_GENERAL_CATEGORY_INITIAL_PUNCTUATION ||  
          cat == HB_UNICODE_GENERAL_CATEGORY_FINAL_PUNCTUATION ||    
          cat == HB_UNICODE_GENERAL_CATEGORY_OTHER_PUNCTUATION);     
}


bool
nsContentUtils::IsFirstLetterPunctuationAt(const nsTextFragment* aFrag, uint32_t aOffset)
{
  PRUnichar h = aFrag->CharAt(aOffset);
  if (!IS_SURROGATE(h)) {
    return IsFirstLetterPunctuation(h);
  }
  if (NS_IS_HIGH_SURROGATE(h) && aOffset + 1 < aFrag->GetLength()) {
    PRUnichar l = aFrag->CharAt(aOffset + 1);
    if (NS_IS_LOW_SURROGATE(l)) {
      return IsFirstLetterPunctuation(SURROGATE_TO_UCS4(h, l));
    }
  }
  return false;
}


bool nsContentUtils::IsAlphanumeric(uint32_t aChar)
{
  nsIUGenCategory::nsUGenCategory cat = mozilla::unicode::GetGenCategory(aChar);

  return (cat == nsIUGenCategory::kLetter || cat == nsIUGenCategory::kNumber);
}
 

bool nsContentUtils::IsAlphanumericAt(const nsTextFragment* aFrag, uint32_t aOffset)
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
  return false;
}


bool
nsContentUtils::IsHTMLWhitespace(PRUnichar aChar)
{
  return aChar == PRUnichar(0x0009) ||
         aChar == PRUnichar(0x000A) ||
         aChar == PRUnichar(0x000C) ||
         aChar == PRUnichar(0x000D) ||
         aChar == PRUnichar(0x0020);
}


bool
nsContentUtils::IsHTMLBlock(nsIAtom* aLocalName)
{
  return
    (aLocalName == nsGkAtoms::address) ||
    (aLocalName == nsGkAtoms::article) ||
    (aLocalName == nsGkAtoms::aside) ||
    (aLocalName == nsGkAtoms::blockquote) ||
    (aLocalName == nsGkAtoms::center) ||
    (aLocalName == nsGkAtoms::dir) ||
    (aLocalName == nsGkAtoms::div) ||
    (aLocalName == nsGkAtoms::dl) || 
    (aLocalName == nsGkAtoms::fieldset) ||
    (aLocalName == nsGkAtoms::figure) || 
    (aLocalName == nsGkAtoms::footer) ||
    (aLocalName == nsGkAtoms::form) ||
    (aLocalName == nsGkAtoms::h1) ||
    (aLocalName == nsGkAtoms::h2) ||
    (aLocalName == nsGkAtoms::h3) ||
    (aLocalName == nsGkAtoms::h4) ||
    (aLocalName == nsGkAtoms::h5) ||
    (aLocalName == nsGkAtoms::h6) ||
    (aLocalName == nsGkAtoms::header) ||
    (aLocalName == nsGkAtoms::hgroup) ||
    (aLocalName == nsGkAtoms::hr) ||
    (aLocalName == nsGkAtoms::li) ||
    (aLocalName == nsGkAtoms::listing) ||
    (aLocalName == nsGkAtoms::menu) ||
    (aLocalName == nsGkAtoms::multicol) || 
    (aLocalName == nsGkAtoms::nav) ||
    (aLocalName == nsGkAtoms::ol) ||
    (aLocalName == nsGkAtoms::p) ||
    (aLocalName == nsGkAtoms::pre) ||
    (aLocalName == nsGkAtoms::section) ||
    (aLocalName == nsGkAtoms::table) ||
    (aLocalName == nsGkAtoms::ul) ||
    (aLocalName == nsGkAtoms::xmp);
}


bool
nsContentUtils::IsHTMLVoid(nsIAtom* aLocalName)
{
  return
    (aLocalName == nsGkAtoms::area) ||
    (aLocalName == nsGkAtoms::base) ||
    (aLocalName == nsGkAtoms::basefont) ||
    (aLocalName == nsGkAtoms::bgsound) ||
    (aLocalName == nsGkAtoms::br) ||
    (aLocalName == nsGkAtoms::col) ||
    (aLocalName == nsGkAtoms::command) ||
    (aLocalName == nsGkAtoms::embed) ||
    (aLocalName == nsGkAtoms::frame) ||
    (aLocalName == nsGkAtoms::hr) ||
    (aLocalName == nsGkAtoms::img) ||
    (aLocalName == nsGkAtoms::input) ||
    (aLocalName == nsGkAtoms::keygen) ||
    (aLocalName == nsGkAtoms::link) ||
    (aLocalName == nsGkAtoms::meta) ||
    (aLocalName == nsGkAtoms::param) ||
#ifdef MOZ_MEDIA
    (aLocalName == nsGkAtoms::source) ||
#endif
    (aLocalName == nsGkAtoms::track) ||
    (aLocalName == nsGkAtoms::wbr);
}


bool
nsContentUtils::ParseIntMarginValue(const nsAString& aString, nsIntMargin& result)
{
  nsAutoString marginStr(aString);
  marginStr.CompressWhitespace(true, true);
  if (marginStr.IsEmpty()) {
    return false;
  }

  int32_t start = 0, end = 0;
  for (int count = 0; count < 4; count++) {
    if ((uint32_t)end >= marginStr.Length())
      return false;

    
    if (count < 3)
      end = Substring(marginStr, start).FindChar(',');
    else
      end = Substring(marginStr, start).Length();

    if (end <= 0)
      return false;

    nsresult ec;
    int32_t val = nsString(Substring(marginStr, start, end)).ToInteger(&ec);
    if (NS_FAILED(ec))
      return false;

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
  return true;
}


int32_t
nsContentUtils::ParseLegacyFontSize(const nsAString& aValue)
{
  nsAString::const_iterator iter, end;
  aValue.BeginReading(iter);
  aValue.EndReading(end);

  while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
    ++iter;
  }

  if (iter == end) {
    return 0;
  }

  bool relative = false;
  bool negate = false;
  if (*iter == PRUnichar('-')) {
    relative = true;
    negate = true;
    ++iter;
  } else if (*iter == PRUnichar('+')) {
    relative = true;
    ++iter;
  }

  if (*iter < PRUnichar('0') || *iter > PRUnichar('9')) {
    return 0;
  }

  
  
  int32_t value = 0;
  while (iter != end && *iter >= PRUnichar('0') && *iter <= PRUnichar('9')) {
    value = 10*value + (*iter - PRUnichar('0'));
    if (value >= 7) {
      break;
    }
    ++iter;
  }

  if (relative) {
    if (negate) {
      value = 3 - value;
    } else {
      value = 3 + value;
    }
  }

  return clamped(value, 1, 7);
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


bool
nsContentUtils::OfflineAppAllowed(nsIURI *aURI)
{
  nsCOMPtr<nsIOfflineCacheUpdateService> updateService =
    do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID);
  if (!updateService) {
    return false;
  }

  bool allowed;
  nsresult rv =
    updateService->OfflineAppAllowedForURI(aURI,
                                           Preferences::GetRootBranch(),
                                           &allowed);
  return NS_SUCCEEDED(rv) && allowed;
}


bool
nsContentUtils::OfflineAppAllowed(nsIPrincipal *aPrincipal)
{
  nsCOMPtr<nsIOfflineCacheUpdateService> updateService =
    do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID);
  if (!updateService) {
    return false;
  }

  bool allowed;
  nsresult rv = updateService->OfflineAppAllowed(aPrincipal,
                                                 Preferences::GetRootBranch(),
                                                 &allowed);
  return NS_SUCCEEDED(rv) && allowed;
}


void
nsContentUtils::Shutdown()
{
  sInitialized = false;

  NS_IF_RELEASE(sContentPolicyService);
  sTriedToGetContentPolicy = false;
  uint32_t i;
  for (i = 0; i < PropertiesFile_COUNT; ++i)
    NS_IF_RELEASE(sStringBundles[i]);

  NS_IF_RELEASE(sStringBundleService);
  NS_IF_RELEASE(sConsoleService);
  NS_IF_RELEASE(sDOMScriptObjectFactory);
  sXPConnect = nullptr;
  sThreadJSContextStack = nullptr;
  NS_IF_RELEASE(sSecurityManager);
  NS_IF_RELEASE(sNameSpaceManager);
  NS_IF_RELEASE(sParserService);
  NS_IF_RELEASE(sIOService);
  NS_IF_RELEASE(sLineBreaker);
  NS_IF_RELEASE(sWordBreaker);
  NS_IF_RELEASE(sImgLoader);
  NS_IF_RELEASE(sPrivateImgLoader);
  NS_IF_RELEASE(sImgCache);
  NS_IF_RELEASE(sPrivateImgCache);
#ifdef IBMBIDI
  NS_IF_RELEASE(sBidiKeyboard);
#endif

  delete sAtomEventTable;
  sAtomEventTable = nullptr;
  delete sStringEventTable;
  sStringEventTable = nullptr;
  delete sUserDefinedEvents;
  sUserDefinedEvents = nullptr;

  if (sEventListenerManagersHash.ops) {
    NS_ASSERTION(sEventListenerManagersHash.entryCount == 0,
                 "Event listener manager hash not empty at shutdown!");

    

    
    
    
    
    
    

    if (sEventListenerManagersHash.entryCount == 0) {
      PL_DHashTableFinish(&sEventListenerManagersHash);
      sEventListenerManagersHash.ops = nullptr;
    }
  }

  NS_ASSERTION(!sBlockedScriptRunners ||
               sBlockedScriptRunners->Length() == 0,
               "How'd this happen?");
  delete sBlockedScriptRunners;
  sBlockedScriptRunners = nullptr;

  delete sShiftText;
  sShiftText = nullptr;
  delete sControlText;  
  sControlText = nullptr;
  delete sMetaText;  
  sMetaText = nullptr;
  delete sOSText;
  sOSText = nullptr;
  delete sAltText;  
  sAltText = nullptr;
  delete sModifierSeparator;
  sModifierSeparator = nullptr;

  NS_IF_RELEASE(sSameOriginChecker);

  nsTextEditorState::ShutDown();
}









nsresult
nsContentUtils::CheckSameOrigin(const nsINode *aTrustedNode,
                                nsIDOMNode *aUnTrustedNode)
{
  MOZ_ASSERT(aTrustedNode);

  
  nsCOMPtr<nsINode> unTrustedNode = do_QueryInterface(aUnTrustedNode);
  NS_ENSURE_TRUE(unTrustedNode, NS_ERROR_UNEXPECTED);
  return CheckSameOrigin(aTrustedNode, unTrustedNode);
}

nsresult
nsContentUtils::CheckSameOrigin(const nsINode* aTrustedNode,
                                const nsINode* unTrustedNode)
{
  MOZ_ASSERT(aTrustedNode);
  MOZ_ASSERT(unTrustedNode);

  bool isSystem = false;
  nsresult rv = sSecurityManager->SubjectPrincipalIsSystem(&isSystem);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isSystem) {
    

    return NS_OK;
  }

  



  nsIPrincipal* trustedPrincipal = aTrustedNode->NodePrincipal();
  nsIPrincipal* unTrustedPrincipal = unTrustedNode->NodePrincipal();

  if (trustedPrincipal == unTrustedPrincipal) {
    return NS_OK;
  }

  bool equal;
  
  
  if (NS_FAILED(trustedPrincipal->Equals(unTrustedPrincipal, &equal)) ||
      !equal) {
    return NS_ERROR_DOM_PROP_ACCESS_DENIED;
  }

  return NS_OK;
}


bool
nsContentUtils::CanCallerAccess(nsIPrincipal* aSubjectPrincipal,
                                nsIPrincipal* aPrincipal)
{
  bool subsumes;
  nsresult rv = aSubjectPrincipal->Subsumes(aPrincipal, &subsumes);
  NS_ENSURE_SUCCESS(rv, false);

  if (subsumes) {
    return true;
  }

  
  
  return IsCallerChrome();
}


bool
nsContentUtils::CanCallerAccess(nsIDOMNode *aNode)
{
  
  
  
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsresult rv = sSecurityManager->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  NS_ENSURE_SUCCESS(rv, false);

  if (!subjectPrincipal) {
    

    return true;
  }

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, false);

  return CanCallerAccess(subjectPrincipal, node->NodePrincipal());
}


bool
nsContentUtils::CanCallerAccess(nsPIDOMWindow* aWindow)
{
  
  
  
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  nsresult rv = sSecurityManager->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
  NS_ENSURE_SUCCESS(rv, false);

  if (!subjectPrincipal) {
    

    return true;
  }

  nsCOMPtr<nsIScriptObjectPrincipal> scriptObject =
    do_QueryInterface(aWindow->IsOuterWindow() ?
                      aWindow->GetCurrentInnerWindow() : aWindow);
  NS_ENSURE_TRUE(scriptObject, false);

  return CanCallerAccess(subjectPrincipal, scriptObject->GetPrincipal());
}


bool
nsContentUtils::InProlog(nsINode *aNode)
{
  NS_PRECONDITION(aNode, "missing node to nsContentUtils::InProlog");

  nsINode* parent = aNode->GetParentNode();
  if (!parent || !parent->IsNodeOfType(nsINode::eDOCUMENT)) {
    return false;
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
    
    return nullptr;
  }

  nsIScriptContext *scx = sgo->GetContext();
  if (!scx) {
    
    return nullptr;
  }

  return scx->GetNativeContext();
}


void
nsContentUtils::TraceSafeJSContext(JSTracer* aTrc)
{
  if (!sThreadJSContextStack) {
    return;
  }
  JSContext* cx = sThreadJSContextStack->GetSafeJSContext();
  if (!cx) {
    return;
  }
  if (JSObject* global = JS_GetGlobalObject(cx)) {
    JS_CALL_OBJECT_TRACER(aTrc, global, "safe context");
  }
}

nsPIDOMWindow *
nsContentUtils::GetWindowFromCaller()
{
  JSContext *cx = nullptr;
  sThreadJSContextStack->Peek(&cx);

  if (cx) {
    nsCOMPtr<nsPIDOMWindow> win =
      do_QueryInterface(nsJSUtils::GetDynamicScriptGlobal(cx));
    return win;
  }

  return nullptr;
}

nsIDOMDocument *
nsContentUtils::GetDocumentFromCaller()
{
  JSContext *cx = nullptr;
  JSObject *obj = nullptr;
  sXPConnect->GetCaller(&cx, &obj);
  NS_ASSERTION(cx && obj, "Caller ensures something is running");

  JSAutoCompartment ac(cx, obj);

  nsCOMPtr<nsPIDOMWindow> win =
    do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(obj));
  if (!win) {
    return nullptr;
  }

  return win->GetExtantDocument();
}

nsIDOMDocument *
nsContentUtils::GetDocumentFromContext()
{
  JSContext *cx = nullptr;
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

  return nullptr;
}

bool
nsContentUtils::IsCallerChrome()
{
  bool is_caller_chrome = false;
  nsresult rv = sSecurityManager->SubjectPrincipalIsSystem(&is_caller_chrome);
  if (NS_FAILED(rv)) {
    return false;
  }
  if (is_caller_chrome) {
    return true;
  }

  
  return xpc::IsUniversalXPConnectEnabled(GetCurrentJSContext());
}

bool
nsContentUtils::IsCallerXBL()
{
    JSScript *script;
    JSContext *cx = GetCurrentJSContext();
    if (!cx)
        return false;
    
    if (xpc::IsXBLScope(js::GetContextCompartment(cx)))
        return true;
    
    if (!JS_DescribeScriptedCaller(cx, &script, nullptr) || !script)
        return false;
    return JS_GetScriptUserBit(script);
}


bool
nsContentUtils::IsImageSrcSetDisabled()
{
  return Preferences::GetBool("dom.disable_image_src_set") &&
         !IsCallerChrome();
}


bool
nsContentUtils::LookupBindingMember(JSContext* aCx, nsIContent *aContent,
                                    JS::HandleId aId, JSPropertyDescriptor* aDesc)
{
  nsXBLBinding* binding = aContent->OwnerDoc()->BindingManager()
                                  ->GetBinding(aContent);
  if (!binding)
    return true;
  return binding->LookupMember(aCx, aId, aDesc);
}


bool
nsContentUtils::IsBindingField(JSContext* aCx, nsIContent* aContent,
                               JS::HandleId aId)
{
  nsXBLBinding* binding = aContent->OwnerDoc()->BindingManager()
                                  ->GetBinding(aContent);
  if (!binding)
    return false;

  if (!JSID_IS_STRING(aId))
    return false;
  nsDependentJSString name(aId);

  return binding->HasField(name);
}


nsINode*
nsContentUtils::GetCrossDocParentNode(nsINode* aChild)
{
  NS_PRECONDITION(aChild, "The child is null!");

  nsINode* parent = aChild->GetParentNode();
  if (parent || !aChild->IsNodeOfType(nsINode::eDOCUMENT))
    return parent;

  nsIDocument* doc = static_cast<nsIDocument*>(aChild);
  nsIDocument* parentDoc = doc->GetParentDocument();
  return parentDoc ? parentDoc->FindContentForSubDocument(doc) : nullptr;
}


bool
nsContentUtils::ContentIsDescendantOf(const nsINode* aPossibleDescendant,
                                      const nsINode* aPossibleAncestor)
{
  NS_PRECONDITION(aPossibleDescendant, "The possible descendant is null!");
  NS_PRECONDITION(aPossibleAncestor, "The possible ancestor is null!");

  do {
    if (aPossibleDescendant == aPossibleAncestor)
      return true;
    aPossibleDescendant = aPossibleDescendant->GetParentNode();
  } while (aPossibleDescendant);

  return false;
}


bool
nsContentUtils::ContentIsCrossDocDescendantOf(nsINode* aPossibleDescendant,
                                              nsINode* aPossibleAncestor)
{
  NS_PRECONDITION(aPossibleDescendant, "The possible descendant is null!");
  NS_PRECONDITION(aPossibleAncestor, "The possible ancestor is null!");

  do {
    if (aPossibleDescendant == aPossibleAncestor)
      return true;
    aPossibleDescendant = GetCrossDocParentNode(aPossibleDescendant);
  } while (aPossibleDescendant);

  return false;
}



nsresult
nsContentUtils::GetAncestors(nsINode* aNode,
                             nsTArray<nsINode*>& aArray)
{
  while (aNode) {
    aArray.AppendElement(aNode);
    aNode = aNode->GetParentNode();
  }
  return NS_OK;
}


nsresult
nsContentUtils::GetAncestorsAndOffsets(nsIDOMNode* aNode,
                                       int32_t aOffset,
                                       nsTArray<nsIContent*>* aAncestorNodes,
                                       nsTArray<int32_t>* aAncestorOffsets)
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
  *aCommonAncestor = nullptr;

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

  
  nsAutoTArray<nsINode*, 30> parents1, parents2;
  do {
    parents1.AppendElement(aNode1);
    aNode1 = aNode1->GetParentNode();
  } while (aNode1);
  do {
    parents2.AppendElement(aNode2);
    aNode2 = aNode2->GetParentNode();
  } while (aNode2);

  
  uint32_t pos1 = parents1.Length();
  uint32_t pos2 = parents2.Length();
  nsINode* parent = nullptr;
  uint32_t len;
  for (len = std::min(pos1, pos2); len > 0; --len) {
    nsINode* child1 = parents1.ElementAt(--pos1);
    nsINode* child2 = parents2.ElementAt(--pos2);
    if (child1 != child2) {
      break;
    }
    parent = child1;
  }

  return parent;
}


int32_t
nsContentUtils::ComparePoints(nsINode* aParent1, int32_t aOffset1,
                              nsINode* aParent2, int32_t aOffset2,
                              bool* aDisconnected)
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
    node1 = node1->GetParentNode();
  } while (node1);
  do {
    parents2.AppendElement(node2);
    node2 = node2->GetParentNode();
  } while (node2);

  uint32_t pos1 = parents1.Length() - 1;
  uint32_t pos2 = parents2.Length() - 1;
  
  bool disconnected = parents1.ElementAt(pos1) != parents2.ElementAt(pos2);
  if (aDisconnected) {
    *aDisconnected = disconnected;
  }
  if (disconnected) {
    NS_ASSERTION(aDisconnected, "unexpected disconnected nodes");
    return 1;
  }

  
  nsINode* parent = parents1.ElementAt(pos1);
  uint32_t len;
  for (len = std::min(pos1, pos2); len > 0; --len) {
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


int32_t
nsContentUtils::ComparePoints(nsIDOMNode* aParent1, int32_t aOffset1,
                              nsIDOMNode* aParent2, int32_t aOffset2,
                              bool* aDisconnected)
{
  nsCOMPtr<nsINode> parent1 = do_QueryInterface(aParent1);
  nsCOMPtr<nsINode> parent2 = do_QueryInterface(aParent2);
  NS_ENSURE_TRUE(parent1 && parent2, -1);
  return ComparePoints(parent1, aOffset1, parent2, aOffset2);
}

inline bool
IsCharInSet(const char* aSet,
            const PRUnichar aChar)
{
  PRUnichar ch;
  while ((ch = *aSet)) {
    if (aChar == PRUnichar(ch)) {
      return true;
    }
    ++aSet;
  }
  return false;
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






template<bool IsWhitespace(PRUnichar)>
const nsDependentSubstring
nsContentUtils::TrimWhitespace(const nsAString& aStr, bool aTrimTrailing)
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
nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(const nsAString&, bool);
template
const nsDependentSubstring
nsContentUtils::TrimWhitespace<nsContentUtils::IsHTMLWhitespace>(const nsAString&, bool);

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

static inline void KeyAppendInt(int32_t aInt, nsACString& aKey)
{
  KeyAppendSep(aKey);

  aKey.Append(nsPrintfCString("%d", aInt));
}

static inline void KeyAppendAtom(nsIAtom* aAtom, nsACString& aKey)
{
  NS_PRECONDITION(aAtom, "KeyAppendAtom: aAtom can not be null!\n");

  KeyAppendString(nsAtomCString(aAtom), aKey);
}

static inline bool IsAutocompleteOff(const nsIContent* aElement)
{
  return aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::autocomplete,
                               NS_LITERAL_STRING("off"), eIgnoreCase);
}

 nsresult
nsContentUtils::GenerateStateKey(nsIContent* aContent,
                                 const nsIDocument* aDocument,
                                 nsACString& aKey)
{
  aKey.Truncate();

  uint32_t partID = aDocument ? aDocument->GetPartID() : 0;

  
  NS_ENSURE_TRUE(aContent, NS_ERROR_FAILURE);

  
  if (aContent->IsInAnonymousSubtree()) {
    return NS_OK;
  }

  if (IsAutocompleteOff(aContent)) {
    return NS_OK;
  }

  nsCOMPtr<nsIHTMLDocument> htmlDocument(do_QueryInterface(aContent->GetCurrentDoc()));

  KeyAppendInt(partID, aKey);  
  bool generatedUniqueKey = false;

  if (htmlDocument) {
    
    
    
    
    aContent->GetCurrentDoc()->FlushPendingNotifications(Flush_Content);

    nsContentList *htmlForms = htmlDocument->GetForms();
    nsContentList *htmlFormControls = htmlDocument->GetFormControls();

    NS_ENSURE_TRUE(htmlForms && htmlFormControls, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIFormControl> control(do_QueryInterface(aContent));
    if (control && htmlFormControls && htmlForms) {

      
      KeyAppendInt(control->GetType(), aKey);

      
      int32_t index = -1;
      Element *formElement = control->GetFormElement();
      if (formElement) {
        if (IsAutocompleteOff(formElement)) {
          aKey.Truncate();
          return NS_OK;
        }

        KeyAppendString(NS_LITERAL_CSTRING("f"), aKey);

        
        index = htmlForms->IndexOf(formElement, false);
        if (index <= -1) {
          
          
          
          
          
          
          
          index = htmlDocument->GetNumFormsSynchronous() - 1;
        }
        if (index > -1) {
          KeyAppendInt(index, aKey);

          
          nsCOMPtr<nsIForm> form(do_QueryInterface(formElement));
          index = form->IndexOfControl(control);

          if (index > -1) {
            KeyAppendInt(index, aKey);
            generatedUniqueKey = true;
          }
        }

        
        nsAutoString formName;
        formElement->GetAttr(kNameSpaceID_None, nsGkAtoms::name, formName);
        KeyAppendString(formName, aKey);

      } else {

        KeyAppendString(NS_LITERAL_CSTRING("d"), aKey);

        
        

        
        

        
        
        index = htmlFormControls->IndexOf(aContent, true);
        if (index > -1) {
          KeyAppendInt(index, aKey);
          generatedUniqueKey = true;
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

    
    
    
    nsINode* parent = aContent->GetParentNode();
    nsINode* content = aContent;
    while (parent) {
      KeyAppendInt(parent->IndexOf(content), aKey);
      content = parent;
      parent = content->GetParentNode();
    }
  }

  return NS_OK;
}


nsIPrincipal*
nsContentUtils::GetSubjectPrincipal()
{
  nsCOMPtr<nsIPrincipal> subject;
  sSecurityManager->GetSubjectPrincipal(getter_AddRefs(subject));

  
  if (!subject)
    sSecurityManager->GetSystemPrincipal(getter_AddRefs(subject));

  return subject;
}


nsIPrincipal*
nsContentUtils::GetObjectPrincipal(JSObject* aObj)
{
  
  
  JSCompartment *compartment = js::GetObjectCompartment(aObj);
  JSPrincipals *principals = JS_GetCompartmentPrincipals(compartment);
  return nsJSPrincipals::get(principals);
}


nsresult
nsContentUtils::NewURIWithDocumentCharset(nsIURI** aResult,
                                          const nsAString& aSpec,
                                          nsIDocument* aDocument,
                                          nsIURI* aBaseURI)
{
  return NS_NewURI(aResult, aSpec,
                   aDocument ? aDocument->GetDocumentCharacterSet().get() : nullptr,
                   aBaseURI, sIOService);
}


bool
nsContentUtils::BelongsInForm(nsIContent *aForm,
                              nsIContent *aContent)
{
  NS_PRECONDITION(aForm, "Must have a form");
  NS_PRECONDITION(aContent, "Must have a content node");

  if (aForm == aContent) {
    

    return false;
  }

  nsIContent* content = aContent->GetParent();

  while (content) {
    if (content == aForm) {
      

      return true;
    }

    if (content->Tag() == nsGkAtoms::form &&
        content->IsHTML()) {
      
      

      return false;
    }

    content = content->GetParent();
  }

  if (aForm->GetChildCount() > 0) {
    
    

    return false;
  }

  
  
  
  
  if (PositionIsBefore(aForm, aContent)) {
    
    
    
    
    return true;
  }

  return false;
}


nsresult
nsContentUtils::CheckQName(const nsAString& aQualifiedName,
                           bool aNamespaceAware,
                           const PRUnichar** aColon)
{
  const char* colon = nullptr;
  const PRUnichar* begin = aQualifiedName.BeginReading();
  const PRUnichar* end = aQualifiedName.EndReading();
  
  int result = MOZ_XMLCheckQName(reinterpret_cast<const char*>(begin),
                                 reinterpret_cast<const char*>(end),
                                 aNamespaceAware, &colon);

  if (!result) {
    if (aColon) {
      *aColon = reinterpret_cast<const PRUnichar*>(colon);
    }

    return NS_OK;
  }

  
  if (result == (1 << 0) || result == (1 << 1)) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  return NS_ERROR_DOM_NAMESPACE_ERR;
}


nsresult
nsContentUtils::SplitQName(const nsIContent* aNamespaceResolver,
                           const nsAFlatString& aQName,
                           int32_t *aNamespace, nsIAtom **aLocalName)
{
  const PRUnichar* colon;
  nsresult rv = nsContentUtils::CheckQName(aQName, true, &colon);
  NS_ENSURE_SUCCESS(rv, rv);

  if (colon) {
    const PRUnichar* end;
    aQName.EndReading(end);
    nsAutoString nameSpace;
    rv = aNamespaceResolver->LookupNamespaceURIInternal(Substring(aQName.get(),
                                                                  colon),
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
                                     uint16_t aNodeType,
                                     nsINodeInfo** aNodeInfo)
{
  const nsAFlatString& qName = PromiseFlatString(aQualifiedName);
  const PRUnichar* colon;
  nsresult rv = nsContentUtils::CheckQName(qName, true, &colon);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t nsID;
  sNameSpaceManager->RegisterNameSpace(aNamespaceURI, nsID);
  if (colon) {
    const PRUnichar* end;
    qName.EndReading(end);

    nsCOMPtr<nsIAtom> prefix = do_GetAtom(Substring(qName.get(), colon));

    rv = aNodeInfoManager->GetNodeInfo(Substring(colon + 1, end), prefix,
                                       nsID, aNodeType, aNodeInfo);
  }
  else {
    rv = aNodeInfoManager->GetNodeInfo(aQualifiedName, nullptr, nsID,
                                       aNodeType, aNodeInfo);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return nsContentUtils::IsValidNodeName((*aNodeInfo)->NameAtom(),
                                         (*aNodeInfo)->GetPrefixAtom(),
                                         (*aNodeInfo)->NamespaceID()) ?
         NS_OK : NS_ERROR_DOM_NAMESPACE_ERR;
}


void
nsContentUtils::SplitExpatName(const PRUnichar *aExpatName, nsIAtom **aPrefix,
                               nsIAtom **aLocalName, int32_t* aNameSpaceID)
{
  









  const PRUnichar *uriEnd = nullptr;
  const PRUnichar *nameEnd = nullptr;
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
      *aPrefix = nullptr;
    }
  }
  else {
    *aNameSpaceID = kNameSpaceID_None;
    nameStart = aExpatName;
    nameEnd = pos;
    *aPrefix = nullptr;
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
  return nullptr;
}


bool
nsContentUtils::CanLoadImage(nsIURI* aURI, nsISupports* aContext,
                             nsIDocument* aLoadingDocument,
                             nsIPrincipal* aLoadingPrincipal,
                             int16_t* aImageBlockingStatus)
{
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aLoadingDocument, "Must have a document");
  NS_PRECONDITION(aLoadingPrincipal, "Must have a loading principal");

  nsresult rv;

  uint32_t appType = nsIDocShell::APP_TYPE_UNKNOWN;

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
      return false;
    }
  }

  int16_t decision = nsIContentPolicy::ACCEPT;

  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_IMAGE,
                                 aURI,
                                 aLoadingPrincipal,
                                 aContext,
                                 EmptyCString(), 
                                 nullptr,         
                                 &decision,
                                 GetContentPolicy(),
                                 sSecurityManager);

  if (aImageBlockingStatus) {
    *aImageBlockingStatus =
      NS_FAILED(rv) ? nsIContentPolicy::REJECT_REQUEST : decision;
  }
  return NS_FAILED(rv) ? false : NS_CP_ACCEPTED(decision);
}

imgLoader*
nsContentUtils::GetImgLoaderForDocument(nsIDocument* aDoc)
{
  if (!sImgLoaderInitialized)
    InitImgLoader();
  if (!aDoc)
    return sImgLoader;
  bool isPrivate = false;
  nsCOMPtr<nsILoadGroup> loadGroup = aDoc->GetDocumentLoadGroup();
  nsCOMPtr<nsIInterfaceRequestor> callbacks;
  if (loadGroup) {
    loadGroup->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (callbacks) {
      nsCOMPtr<nsILoadContext> loadContext = do_GetInterface(callbacks);
      isPrivate = loadContext && loadContext->UsePrivateBrowsing();
    }
  } else {
    nsCOMPtr<nsIChannel> channel = aDoc->GetChannel();
    isPrivate = channel && NS_UsePrivateBrowsing(channel);
  }
  return isPrivate ? sPrivateImgLoader : sImgLoader;
}


imgLoader*
nsContentUtils::GetImgLoaderForChannel(nsIChannel* aChannel)
{
  if (!sImgLoaderInitialized)
    InitImgLoader();
  if (!aChannel)
    return sImgLoader;
  nsCOMPtr<nsILoadContext> context;
  NS_QueryNotificationCallbacks(aChannel, context);
  return context && context->UsePrivateBrowsing() ? sPrivateImgLoader : sImgLoader;
}


bool
nsContentUtils::IsImageInCache(nsIURI* aURI, nsIDocument* aDocument)
{
    if (!sImgLoaderInitialized)
        InitImgLoader();

    imgILoader* loader = GetImgLoaderForDocument(aDocument);
    nsCOMPtr<imgICache> cache = do_QueryInterface(loader);

    
    
    nsCOMPtr<nsIProperties> props;
    nsresult rv = cache->FindEntryProperties(aURI, getter_AddRefs(props));
    return (NS_SUCCEEDED(rv) && props);
}


nsresult
nsContentUtils::LoadImage(nsIURI* aURI, nsIDocument* aLoadingDocument,
                          nsIPrincipal* aLoadingPrincipal, nsIURI* aReferrer,
                          imgINotificationObserver* aObserver, int32_t aLoadFlags,
                          imgRequestProxy** aRequest)
{
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aLoadingDocument, "Must have a document");
  NS_PRECONDITION(aLoadingPrincipal, "Must have a principal");
  NS_PRECONDITION(aRequest, "Null out param");

  imgLoader* imgLoader = GetImgLoaderForDocument(aLoadingDocument);
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
                              aLoadingPrincipal,    
                              loadGroup,            
                              aObserver,            
                              aLoadingDocument,     
                              aLoadFlags,           
                              nullptr,               
                              channelPolicy,        
                              aRequest);
}


already_AddRefed<imgIContainer>
nsContentUtils::GetImageFromContent(nsIImageLoadingContent* aContent,
                                    imgIRequest **aRequest)
{
  if (aRequest) {
    *aRequest = nullptr;
  }

  NS_ENSURE_TRUE(aContent, nullptr);

  nsCOMPtr<imgIRequest> imgRequest;
  aContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                      getter_AddRefs(imgRequest));
  if (!imgRequest) {
    return nullptr;
  }

  nsCOMPtr<imgIContainer> imgContainer;
  imgRequest->GetImage(getter_AddRefs(imgContainer));

  if (!imgContainer) {
    return nullptr;
  }

  if (aRequest) {
    imgRequest.swap(*aRequest);
  }

  return imgContainer.forget();
}


already_AddRefed<imgRequestProxy>
nsContentUtils::GetStaticRequest(imgRequestProxy* aRequest)
{
  NS_ENSURE_TRUE(aRequest, nullptr);
  nsRefPtr<imgRequestProxy> retval;
  aRequest->GetStaticRequest(getter_AddRefs(retval));
  return retval.forget();
}


bool
nsContentUtils::ContentIsDraggable(nsIContent* aContent)
{
  nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(aContent);
  if (htmlElement) {
    bool draggable = false;
    htmlElement->GetDraggable(&draggable);
    if (draggable)
      return true;

    if (aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::draggable,
                              nsGkAtoms::_false, eIgnoreCase))
      return false;
  }

  
  return IsDraggableImage(aContent) || IsDraggableLink(aContent);
}


bool
nsContentUtils::IsDraggableImage(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "Must have content node to test");

  nsCOMPtr<nsIImageLoadingContent> imageContent(do_QueryInterface(aContent));
  if (!imageContent) {
    return false;
  }

  nsCOMPtr<imgIRequest> imgRequest;
  imageContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                           getter_AddRefs(imgRequest));

  
  
  return imgRequest != nullptr;
}


bool
nsContentUtils::IsDraggableLink(const nsIContent* aContent) {
  nsCOMPtr<nsIURI> absURI;
  return aContent->IsLink(getter_AddRefs(absURI));
}

static bool
TestSitePerm(nsIPrincipal* aPrincipal, const char* aType, uint32_t aPerm, bool aExactHostMatch)
{
  if (!aPrincipal) {
    
    
    return aPerm != nsIPermissionManager::ALLOW_ACTION;
  }

  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService("@mozilla.org/permissionmanager;1");
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t perm;
  nsresult rv;
  if (aExactHostMatch) {
    rv = permMgr->TestExactPermissionFromPrincipal(aPrincipal, aType, &perm);
  } else {
    rv = permMgr->TestPermissionFromPrincipal(aPrincipal, aType, &perm);
  }
  NS_ENSURE_SUCCESS(rv, false);

  return perm == aPerm;
}

bool
nsContentUtils::IsSitePermAllow(nsIPrincipal* aPrincipal, const char* aType)
{
  return TestSitePerm(aPrincipal, aType, nsIPermissionManager::ALLOW_ACTION, false);
}

bool
nsContentUtils::IsSitePermDeny(nsIPrincipal* aPrincipal, const char* aType)
{
  return TestSitePerm(aPrincipal, aType, nsIPermissionManager::DENY_ACTION, false);
}

bool
nsContentUtils::IsExactSitePermAllow(nsIPrincipal* aPrincipal, const char* aType)
{
  return TestSitePerm(aPrincipal, aType, nsIPermissionManager::ALLOW_ACTION, true);
}

bool
nsContentUtils::IsExactSitePermDeny(nsIPrincipal* aPrincipal, const char* aType)
{
  return TestSitePerm(aPrincipal, aType, nsIPermissionManager::DENY_ACTION, true);
}

static const char *gEventNames[] = {"event"};
static const char *gSVGEventNames[] = {"evt"};



static const char *gOnErrorNames[] = {"event", "source", "lineno"};


void
nsContentUtils::GetEventArgNames(int32_t aNameSpaceID,
                                 nsIAtom *aEventName,
                                 uint32_t *aArgCount,
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
    : mScriptIsRunning(false),
      mPushedSomething(false)
{
}

nsCxPusher::~nsCxPusher()
{
  Pop();
}

static bool
IsContextOnStack(nsIJSContextStack *aStack, JSContext *aContext)
{
  JSContext *ctx = nullptr;
  aStack->Peek(&ctx);
  if (!ctx)
    return false;
  if (ctx == aContext)
    return true;

  nsCOMPtr<nsIJSContextStackIterator>
    iterator(do_CreateInstance("@mozilla.org/js/xpc/ContextStackIterator;1"));
  NS_ENSURE_TRUE(iterator, false);

  nsresult rv = iterator->Reset(aStack);
  NS_ENSURE_SUCCESS(rv, false);

  bool done;
  while (NS_SUCCEEDED(iterator->Done(&done)) && !done) {
    rv = iterator->Prev(&ctx);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Broken iterator implementation");

    if (!ctx) {
      continue;
    }

    if (nsJSUtils::GetDynamicScriptContext(ctx) && ctx == aContext)
      return true;
  }

  return false;
}

bool
nsCxPusher::Push(nsIDOMEventTarget *aCurrentTarget)
{
  if (mPushedSomething) {
    NS_ERROR("Whaaa! No double pushing with nsCxPusher::Push()!");

    return false;
  }

  NS_ENSURE_TRUE(aCurrentTarget, false);
  nsresult rv;
  nsIScriptContext* scx =
    aCurrentTarget->GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, false);

  if (!scx) {
    
    JSContext* cx = aCurrentTarget->GetJSContextForEventHandlers();
    if (cx) {
      DoPush(cx);
    }

    
    
    return true;
  }

  JSContext* cx = nullptr;

  if (scx) {
    cx = scx->GetNativeContext();
    
    NS_ENSURE_TRUE(cx, false);
  }

  
  
  
  
  return Push(cx);
}

bool
nsCxPusher::RePush(nsIDOMEventTarget *aCurrentTarget)
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
      return false;
    }

    
    
    if (scx && scx == mScx &&
        scx->GetNativeContext()) {
      return true;
    }
  }

  Pop();
  return Push(aCurrentTarget);
}

bool
nsCxPusher::Push(JSContext *cx, bool aRequiresScriptContext)
{
  if (mPushedSomething) {
    NS_ERROR("Whaaa! No double pushing with nsCxPusher::Push()!");

    return false;
  }

  if (!cx) {
    return false;
  }

  
  
  
  mScx = GetScriptContextFromJSContext(cx);
  if (!mScx && aRequiresScriptContext) {
    
    return true;
  }

  return DoPush(cx);
}

bool
nsCxPusher::DoPush(JSContext* cx)
{
  nsIThreadJSContextStack* stack = nsContentUtils::ThreadJSContextStack();
  if (!stack) {
    return true;
  }

  if (cx && IsContextOnStack(stack, cx)) {
    
    
    mScriptIsRunning = true;
  }

  if (NS_FAILED(stack->Push(cx))) {
    mScriptIsRunning = false;
    mScx = nullptr;
    return false;
  }

  mPushedSomething = true;
#ifdef DEBUG
  mPushedContext = cx;
  if (cx)
    mCompartmentDepthOnEntry = js::GetEnterCompartmentDepth(cx);
#endif
  return true;
}

bool
nsCxPusher::PushNull()
{
  return DoPush(nullptr);
}

void
nsCxPusher::Pop()
{
  nsIThreadJSContextStack* stack = nsContentUtils::ThreadJSContextStack();
  if (!mPushedSomething || !stack) {
    mScx = nullptr;
    mPushedSomething = false;

    NS_ASSERTION(!mScriptIsRunning, "Huh, this can't be happening, "
                 "mScriptIsRunning can't be set here!");

    return;
  }

  
  
  
  
  
  MOZ_ASSERT_IF(mPushedContext, mCompartmentDepthOnEntry ==
                                js::GetEnterCompartmentDepth(mPushedContext));

  JSContext *unused;
  stack->Pop(&unused);

  NS_ASSERTION(unused == mPushedContext, "Unexpected context popped");

  if (!mScriptIsRunning && mScx) {
    
    

    mScx->ScriptEvaluated(true);
  }

  mScx = nullptr;
  mScriptIsRunning = false;
  mPushedSomething = false;
}

static const char gPropertiesFiles[nsContentUtils::PropertiesFile_COUNT][56] = {
  
  "chrome://global/locale/css.properties",
  "chrome://global/locale/xbl.properties",
  "chrome://global/locale/xul.properties",
  "chrome://global/locale/layout_errors.properties",
  "chrome://global/locale/layout/HtmlForm.properties",
  "chrome://global/locale/printing.properties",
  "chrome://global/locale/dom/dom.properties",
  "chrome://global/locale/layout/htmlparser.properties",
  "chrome://global/locale/svg/svg.properties",
  "chrome://branding/locale/brand.properties",
  "chrome://global/locale/commonDialogs.properties",
  "chrome://global/locale/mathml/mathml.properties"
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
                                               uint32_t aParamsLength,
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
nsContentUtils::ReportToConsole(uint32_t aErrorFlags,
                                const char *aCategory,
                                nsIDocument* aDocument,
                                PropertiesFile aFile,
                                const char *aMessageName,
                                const PRUnichar **aParams,
                                uint32_t aParamsLength,
                                nsIURI* aURI,
                                const nsAFlatString& aSourceLine,
                                uint32_t aLineNumber,
                                uint32_t aColumnNumber)
{
  NS_ASSERTION((aParams && aParamsLength) || (!aParams && !aParamsLength),
               "Supply either both parameters and their number or no"
               "parameters and 0.");

  nsresult rv;
  nsXPIDLString errorText;
  if (aParams) {
    rv = FormatLocalizedString(aFile, aMessageName, aParams, aParamsLength,
                               errorText);
  }
  else {
    rv = GetLocalizedString(aFile, aMessageName, errorText);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return ReportToConsoleNonLocalized(errorText, aErrorFlags, aCategory,
                                     aDocument, aURI, aSourceLine,
                                     aLineNumber, aColumnNumber);
}


 nsresult
nsContentUtils::ReportToConsoleNonLocalized(const nsAString& aErrorText,
                                            uint32_t aErrorFlags,
                                            const char *aCategory,
                                            nsIDocument* aDocument,
                                            nsIURI* aURI,
                                            const nsAFlatString& aSourceLine,
                                            uint32_t aLineNumber,
                                            uint32_t aColumnNumber)
{
  uint64_t innerWindowID = 0;
  if (aDocument) {
    if (!aURI) {
      aURI = aDocument->GetDocumentURI();
    }
    innerWindowID = aDocument->InnerWindowID();
  }

  nsresult rv;
  if (!sConsoleService) { 
    rv = CallGetService(NS_CONSOLESERVICE_CONTRACTID, &sConsoleService);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsAutoCString spec;
  if (!aLineNumber) {
    JSContext *cx = nullptr;
    sThreadJSContextStack->Peek(&cx);
    if (cx) {
      const char* filename;
      uint32_t lineno;
      if (nsJSUtils::GetCallingLocation(cx, &filename, &lineno)) {
        spec = filename;
        aLineNumber = lineno;
      }
    }
  }
  if (spec.IsEmpty() && aURI)
    aURI->GetSpec(spec);

  nsCOMPtr<nsIScriptError> errorObject =
      do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = errorObject->InitWithWindowID(aErrorText,
                                     NS_ConvertUTF8toUTF16(spec), 
                                     aSourceLine,
                                     aLineNumber, aColumnNumber,
                                     aErrorFlags, aCategory,
                                     innerWindowID);
  NS_ENSURE_SUCCESS(rv, rv);

  return sConsoleService->LogMessage(errorObject);
}

bool
nsContentUtils::IsChromeDoc(nsIDocument *aDocument)
{
  if (!aDocument) {
    return false;
  }
  
  nsCOMPtr<nsIPrincipal> systemPrincipal;
  sSecurityManager->GetSystemPrincipal(getter_AddRefs(systemPrincipal));

  return aDocument->NodePrincipal() == systemPrincipal;
}

bool
nsContentUtils::IsChildOfSameType(nsIDocument* aDoc)
{
  nsCOMPtr<nsISupports> container = aDoc->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(container));
  nsCOMPtr<nsIDocShellTreeItem> sameTypeParent;
  if (docShellAsItem) {
    docShellAsItem->GetSameTypeParent(getter_AddRefs(sameTypeParent));
  }
  return sameTypeParent != nullptr;
}

bool
nsContentUtils::GetWrapperSafeScriptFilename(nsIDocument *aDocument,
                                             nsIURI *aURI,
                                             nsACString& aScriptURI)
{
  bool scriptFileNameModified = false;
  aURI->GetSpec(aScriptURI);

  if (IsChromeDoc(aDocument)) {
    nsCOMPtr<nsIChromeRegistry> chromeReg =
      mozilla::services::GetChromeRegistryService();

    if (!chromeReg) {
      
      

      return scriptFileNameModified;
    }

    bool docWrappersEnabled =
      chromeReg->WrappersEnabled(aDocument->GetDocumentURI());

    bool uriWrappersEnabled = chromeReg->WrappersEnabled(aURI);

    nsIURI *docURI = aDocument->GetDocumentURI();

    if (docURI && docWrappersEnabled && !uriWrappersEnabled) {
      
      
      
      
      
      
      nsAutoCString spec;
      docURI->GetSpec(spec);
      spec.AppendASCII(" -> ");
      spec.Append(aScriptURI);

      aScriptURI = spec;

      scriptFileNameModified = true;
    }
  }

  return scriptFileNameModified;
}


bool
nsContentUtils::IsInChromeDocshell(nsIDocument *aDocument)
{
  if (!aDocument) {
    return false;
  }

  if (aDocument->GetDisplayDocument()) {
    return IsInChromeDocshell(aDocument->GetDisplayDocument());
  }

  nsCOMPtr<nsISupports> docContainer = aDocument->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryInterface(docContainer));
  int32_t itemType = nsIDocShellTreeItem::typeContent;
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
    
    sTriedToGetContentPolicy = true;
  }

  return sContentPolicyService;
}


bool
nsContentUtils::IsEventAttributeName(nsIAtom* aName, int32_t aType)
{
  const PRUnichar* name = aName->GetUTF16String();
  if (name[0] != 'o' || name[1] != 'n')
    return false;

  EventNameMapping mapping;
  return (sAtomEventTable->Get(aName, &mapping) && mapping.mType & aType);
}


uint32_t
nsContentUtils::GetEventId(nsIAtom* aName)
{
  EventNameMapping mapping;
  if (sAtomEventTable->Get(aName, &mapping))
    return mapping.mId;

  return NS_USER_DEFINED_EVENT;
}


uint32_t
nsContentUtils::GetEventCategory(const nsAString& aName)
{
  EventNameMapping mapping;
  if (sStringEventTable->Get(aName, &mapping))
    return mapping.mStructType;

  return NS_EVENT;
}

nsIAtom*
nsContentUtils::GetEventIdAndAtom(const nsAString& aName,
                                  uint32_t aEventStruct,
                                  uint32_t* aEventID)
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
                           bool aCanBubble, bool aCancelable,
                           bool aTrusted, nsIDOMEvent** aEvent,
                           nsIDOMEventTarget** aTargetOut)
{
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aDoc);
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(aTarget));
  NS_ENSURE_TRUE(domDoc && target, NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv =
    domDoc->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->InitEvent(aEventName, aCanBubble, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);

  event->SetTrusted(aTrusted);

  rv = event->SetTarget(target);
  NS_ENSURE_SUCCESS(rv, rv);

  event.forget(aEvent);
  target.forget(aTargetOut);
  return NS_OK;
}


nsresult
nsContentUtils::DispatchTrustedEvent(nsIDocument* aDoc, nsISupports* aTarget,
                                     const nsAString& aEventName,
                                     bool aCanBubble, bool aCancelable,
                                     bool *aDefaultAction)
{
  return DispatchEvent(aDoc, aTarget, aEventName, aCanBubble, aCancelable,
                       true, aDefaultAction);
}


nsresult
nsContentUtils::DispatchUntrustedEvent(nsIDocument* aDoc, nsISupports* aTarget,
                                       const nsAString& aEventName,
                                       bool aCanBubble, bool aCancelable,
                                       bool *aDefaultAction)
{
  return DispatchEvent(aDoc, aTarget, aEventName, aCanBubble, aCancelable,
                       false, aDefaultAction);
}


nsresult
nsContentUtils::DispatchEvent(nsIDocument* aDoc, nsISupports* aTarget,
                              const nsAString& aEventName,
                              bool aCanBubble, bool aCancelable,
                              bool aTrusted, bool *aDefaultAction)
{
  nsCOMPtr<nsIDOMEvent> event;
  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult rv = GetEventAndTarget(aDoc, aTarget, aEventName, aCanBubble,
                                  aCancelable, aTrusted, getter_AddRefs(event),
                                  getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  return target->DispatchEvent(event, aDefaultAction ? aDefaultAction : &dummy);
}

nsresult
nsContentUtils::DispatchChromeEvent(nsIDocument *aDoc,
                                    nsISupports *aTarget,
                                    const nsAString& aEventName,
                                    bool aCanBubble, bool aCancelable,
                                    bool *aDefaultAction)
{

  nsCOMPtr<nsIDOMEvent> event;
  nsCOMPtr<nsIDOMEventTarget> target;
  nsresult rv = GetEventAndTarget(aDoc, aTarget, aEventName, aCanBubble,
                                  aCancelable, true, getter_AddRefs(event),
                                  getter_AddRefs(target));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(aDoc, "GetEventAndTarget lied?");
  if (!aDoc->GetWindow())
    return NS_ERROR_INVALID_ARG;

  nsIDOMEventTarget* piTarget = aDoc->GetWindow()->GetParentTarget();
  if (!piTarget)
    return NS_ERROR_INVALID_ARG;

  nsEventStatus status = nsEventStatus_eIgnore;
  rv = piTarget->DispatchDOMEvent(nullptr, event, nullptr, &status);
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

  return nullptr;
}


Element *
nsContentUtils::MatchElementId(nsIContent *aContent, const nsAString& aId)
{
  NS_PRECONDITION(!aId.IsEmpty(), "Will match random elements");
  
  
  nsCOMPtr<nsIAtom> id(do_GetAtom(aId));
  if (!id) {
    
    return nullptr;
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

  ErrorResult rv;
  TextDecoderBase decoder;
  decoder.Init(NS_ConvertUTF8toUTF16(aCharset), false, rv);
  if (rv.Failed()) {
    rv.ClearMessage();
    return rv.ErrorCode();
  }

  decoder.Decode(aInput.BeginReading(), aInput.Length(), false,
                 aOutput, rv);
  return rv.ErrorCode();
}


bool
nsContentUtils::CheckForBOM(const unsigned char* aBuffer, uint32_t aLength,
                            nsACString& aCharset, bool *bigEndian)
{
  bool found = true;
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
      *bigEndian = true;
  }
  else if (aLength >= 2 &&
           aBuffer[0] == 0xFF && aBuffer[1] == 0xFE) {
    aCharset = "UTF-16";
    if (bigEndian)
      *bigEndian = false;
  } else {
    found = false;
  }

  return found;
}

NS_IMPL_ISUPPORTS1(CharsetDetectionObserver,
                   nsICharsetDetectionObserver)


nsresult
nsContentUtils::GuessCharset(const char *aData, uint32_t aDataLen,
                             nsACString &aCharset)
{
  
  nsCOMPtr<nsICharsetDetector> detector =
    do_CreateInstance(NS_CHARSET_DETECTOR_CONTRACTID_BASE
                      "universal_charset_detector");
  if (!detector) {
    
    const nsAdoptingCString& detectorName =
      Preferences::GetLocalizedCString("intl.charset.detector");
    if (!detectorName.IsEmpty()) {
      nsAutoCString detectorContractID;
      detectorContractID.AssignLiteral(NS_CHARSET_DETECTOR_CONTRACTID_BASE);
      detectorContractID += detectorName;
      detector = do_CreateInstance(detectorContractID.get());
    }
  }

  nsresult rv;

  
  
  if (detector && aDataLen) {
    nsRefPtr<CharsetDetectionObserver> observer =
      new CharsetDetectionObserver();

    rv = detector->Init(observer);
    NS_ENSURE_SUCCESS(rv, rv);

    bool dummy;
    rv = detector->DoIt(aData, aDataLen, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = detector->Done();
    NS_ENSURE_SUCCESS(rv, rv);

    aCharset = observer->GetResult();
  } else {
    
    unsigned char sniffBuf[3];
    uint32_t numRead =
      (aDataLen >= sizeof(sniffBuf) ? sizeof(sniffBuf) : aDataLen);
    memcpy(sniffBuf, aData, numRead);

    bool bigEndian;
    if (CheckForBOM(sniffBuf, numRead, aCharset, &bigEndian) &&
        aCharset.EqualsLiteral("UTF-16")) {
      if (bigEndian) {
        aCharset.AppendLiteral("BE");
      }
      else {
        aCharset.AppendLiteral("LE");
      }
    }
  }

  if (aCharset.IsEmpty()) {
    
    nsCOMPtr<nsIPlatformCharset> platformCharset =
      do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = platformCharset->GetCharset(kPlatformCharsetSel_PlainTextInFile,
                                       aCharset);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to get the system charset!");
      }
    }
  }

  if (aCharset.IsEmpty()) {
    
    aCharset.AssignLiteral("UTF-8");
  }

  return NS_OK;
}


void
nsContentUtils::RegisterShutdownObserver(nsIObserver* aObserver)
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->AddObserver(aObserver, 
                                 NS_XPCOM_SHUTDOWN_OBSERVER_ID, 
                                 false);
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


bool
nsContentUtils::HasNonEmptyAttr(const nsIContent* aContent, int32_t aNameSpaceID,
                                nsIAtom* aName)
{
  static nsIContent::AttrValuesArray strings[] = {&nsGkAtoms::_empty, nullptr};
  return aContent->FindAttrValueIn(aNameSpaceID, aName, strings, eCaseMatters)
    == nsIContent::ATTR_VALUE_NO_MATCH;
}


bool
nsContentUtils::HasMutationListeners(nsINode* aNode,
                                     uint32_t aType,
                                     nsINode* aTargetForSubtreeModified)
{
  nsIDocument* doc = aNode->OwnerDoc();

  
  nsPIDOMWindow* window = doc->GetInnerWindow();
  
  
  if (window && !window->HasMutationListeners(aType)) {
    return false;
  }

  if (aNode->IsNodeOfType(nsINode::eCONTENT) &&
      static_cast<nsIContent*>(aNode)->ChromeOnlyAccess()) {
    return false;
  }

  doc->MayDispatchMutationEvent(aTargetForSubtreeModified);

  
  if (aNode->IsInDoc()) {
    nsCOMPtr<nsIDOMEventTarget> piTarget(do_QueryInterface(window));
    if (piTarget) {
      nsEventListenerManager* manager = piTarget->GetListenerManager(false);
      if (manager && manager->HasMutationListeners()) {
        return true;
      }
    }
  }

  
  
  
  while (aNode) {
    nsEventListenerManager* manager = aNode->GetListenerManager(false);
    if (manager && manager->HasMutationListeners()) {
      return true;
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
    aNode = aNode->GetParentNode();
  }

  return false;
}


bool
nsContentUtils::HasMutationListeners(nsIDocument* aDocument,
                                     uint32_t aType)
{
  nsPIDOMWindow* window = aDocument ?
    aDocument->GetInnerWindow() : nullptr;

  
  
  return !window || window->HasMutationListeners(aType);
}

void
nsContentUtils::MaybeFireNodeRemoved(nsINode* aChild, nsINode* aParent,
                                     nsIDocument* aOwnerDoc)
{
  NS_PRECONDITION(aChild, "Missing child");
  NS_PRECONDITION(aChild->GetParentNode() == aParent, "Wrong parent");
  NS_PRECONDITION(aChild->OwnerDoc() == aOwnerDoc, "Wrong owner-doc");

  
  
  
  
  
  
  
  
  
  
  
  NS_ASSERTION((aChild->IsNodeOfType(nsINode::eCONTENT) &&
               static_cast<nsIContent*>(aChild)->
                 IsInNativeAnonymousSubtree()) ||
               IsSafeToRunScript() ||
               sDOMNodeRemovedSuppressCount,
               "Want to fire DOMNodeRemoved event, but it's not safe");

  
  
  
  
  if (!IsSafeToRunScript()) {
    return;
  }

  if (HasMutationListeners(aChild,
        NS_EVENT_BITS_MUTATION_NODEREMOVED, aParent)) {
    nsMutationEvent mutation(true, NS_MUTATION_NODEREMOVED);
    mutation.mRelatedNode = do_QueryInterface(aParent);

    mozAutoSubtreeModified subtree(aOwnerDoc, aParent);
    nsEventDispatcher::Dispatch(aChild, nullptr, &mutation);
  }
}

PLDHashOperator
ListenerEnumerator(PLDHashTable* aTable, PLDHashEntryHdr* aEntry,
                   uint32_t aNumber, void* aArg)
{
  EventListenerManagerMapEntry* entry =
    static_cast<EventListenerManagerMapEntry*>(aEntry);
  if (entry) {
    nsINode* n = static_cast<nsINode*>(entry->mListenerManager->GetTarget());
    if (n && n->IsInDoc() &&
        nsCCUncollectableMarker::InGeneration(n->OwnerDoc()->GetMarkedCCGeneration())) {
      entry->mListenerManager->MarkForCC();
    }
  }
  return PL_DHASH_NEXT;
}

void
nsContentUtils::UnmarkGrayJSListenersInCCGenerationDocuments(uint32_t aGeneration)
{
  if (sEventListenerManagersHash.ops) {
    PL_DHashTableEnumerate(&sEventListenerManagersHash, ListenerEnumerator,
                           &aGeneration);
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
    CycleCollectionNoteChild(cb, entry->mListenerManager.get(),
                             "[via hash] mListenerManager");
  }
}

nsEventListenerManager*
nsContentUtils::GetListenerManager(nsINode *aNode,
                                   bool aCreateIfNotFound)
{
  if (!aCreateIfNotFound && !aNode->HasFlag(NODE_HAS_LISTENERMANAGER)) {
    return nullptr;
  }
  
  if (!sEventListenerManagersHash.ops) {
    
    

    return nullptr;
  }

  if (!aCreateIfNotFound) {
    EventListenerManagerMapEntry *entry =
      static_cast<EventListenerManagerMapEntry *>
                 (PL_DHashTableOperate(&sEventListenerManagersHash, aNode,
                                          PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      return entry->mListenerManager;
    }
    return nullptr;
  }

  EventListenerManagerMapEntry *entry =
    static_cast<EventListenerManagerMapEntry *>
               (PL_DHashTableOperate(&sEventListenerManagersHash, aNode,
                                        PL_DHASH_ADD));

  if (!entry) {
    return nullptr;
  }

  if (!entry->mListenerManager) {
    entry->mListenerManager = new nsEventListenerManager(aNode);

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
      nsRefPtr<nsEventListenerManager> listenerManager;
      listenerManager.swap(entry->mListenerManager);
      
      
      PL_DHashTableRawRemove(&sEventListenerManagersHash, entry);
      if (listenerManager) {
        listenerManager->Disconnect();
      }
    }
  }
}


bool
nsContentUtils::IsValidNodeName(nsIAtom *aLocalName, nsIAtom *aPrefix,
                                int32_t aNamespaceID)
{
  if (aNamespaceID == kNameSpaceID_Unknown) {
    return false;
  }

  if (!aPrefix) {
    
    
    return (aLocalName == nsGkAtoms::xmlns) ==
           (aNamespaceID == kNameSpaceID_XMLNS);
  }

  
  if (aNamespaceID == kNameSpaceID_None) {
    return false;
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
                                         bool aPreventScriptExecution,
                                         nsIDOMDocumentFragment** aReturn)
{
  *aReturn = nullptr;
  NS_ENSURE_ARG(aContextNode);

  
  
  nsCOMPtr<nsIDocument> document = aContextNode->OwnerDoc();
  bool isHTML = document->IsHTML();
#ifdef DEBUG
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(document);
  NS_ASSERTION(!isHTML || htmlDoc, "Should have HTMLDocument here!");
#endif

  if (isHTML) {
    nsCOMPtr<nsIDOMDocumentFragment> frag;
    NS_NewDocumentFragment(getter_AddRefs(frag), document->NodeInfoManager());
    
    nsCOMPtr<nsIContent> contextAsContent = do_QueryInterface(aContextNode);
    if (contextAsContent && !contextAsContent->IsElement()) {
      contextAsContent = contextAsContent->GetParent();
      if (contextAsContent && !contextAsContent->IsElement()) {
        
        contextAsContent = nullptr;
      }
    }
    
    nsresult rv;
    nsCOMPtr<nsIContent> fragment = do_QueryInterface(frag);
    if (contextAsContent && !contextAsContent->IsHTML(nsGkAtoms::html)) {
      rv = ParseFragmentHTML(aFragment,
                             fragment,
                             contextAsContent->Tag(),
                             contextAsContent->GetNameSpaceID(),
                             (document->GetCompatibilityMode() ==
                               eCompatibility_NavQuirks),
                             aPreventScriptExecution);
    } else {
      rv = ParseFragmentHTML(aFragment,
                             fragment,
                             nsGkAtoms::body,
                             kNameSpaceID_XHTML,
                             (document->GetCompatibilityMode() ==
                               eCompatibility_NavQuirks),
                             aPreventScriptExecution);
    }

    frag.forget(aReturn);
    return rv;
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

    
    uint32_t count = content->GetAttrCount();
    bool setDefaultNamespace = false;
    if (count > 0) {
      uint32_t index;

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
            setDefaultNamespace = true;
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

  return ParseFragmentXML(aFragment,
                          document,
                          tagStack,
                          aPreventScriptExecution,
                          aReturn);
}


void
nsContentUtils::DropFragmentParsers()
{
  NS_IF_RELEASE(sHTMLFragmentParser);
  NS_IF_RELEASE(sXMLFragmentParser);
  NS_IF_RELEASE(sXMLFragmentSink);
}


void
nsContentUtils::XPCOMShutdown()
{
  nsContentUtils::DropFragmentParsers();
}


nsresult
nsContentUtils::ParseFragmentHTML(const nsAString& aSourceBuffer,
                                  nsIContent* aTargetNode,
                                  nsIAtom* aContextLocalName,
                                  int32_t aContextNamespace,
                                  bool aQuirks,
                                  bool aPreventScriptExecution)
{
  if (nsContentUtils::sFragmentParsingActive) {
    NS_NOTREACHED("Re-entrant fragment parsing attempted.");
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }
  mozilla::AutoRestore<bool> guard(nsContentUtils::sFragmentParsingActive);
  nsContentUtils::sFragmentParsingActive = true;
  if (!sHTMLFragmentParser) {
    NS_ADDREF(sHTMLFragmentParser = new nsHtml5StringParser());
    
  }
  nsresult rv =
    sHTMLFragmentParser->ParseFragment(aSourceBuffer,
                                       aTargetNode,
                                       aContextLocalName,
                                       aContextNamespace,
                                       aQuirks,
                                       aPreventScriptExecution);
  return rv;
}


nsresult
nsContentUtils::ParseDocumentHTML(const nsAString& aSourceBuffer,
                                  nsIDocument* aTargetDocument,
                                  bool aScriptingEnabledForNoscriptParsing)
{
  if (nsContentUtils::sFragmentParsingActive) {
    NS_NOTREACHED("Re-entrant fragment parsing attempted.");
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }
  mozilla::AutoRestore<bool> guard(nsContentUtils::sFragmentParsingActive);
  nsContentUtils::sFragmentParsingActive = true;
  if (!sHTMLFragmentParser) {
    NS_ADDREF(sHTMLFragmentParser = new nsHtml5StringParser());
    
  }
  nsresult rv =
    sHTMLFragmentParser->ParseDocument(aSourceBuffer,
                                       aTargetDocument,
                                       aScriptingEnabledForNoscriptParsing);
  return rv;
}


nsresult
nsContentUtils::ParseFragmentXML(const nsAString& aSourceBuffer,
                                 nsIDocument* aDocument,
                                 nsTArray<nsString>& aTagStack,
                                 bool aPreventScriptExecution,
                                 nsIDOMDocumentFragment** aReturn)
{
  if (nsContentUtils::sFragmentParsingActive) {
    NS_NOTREACHED("Re-entrant fragment parsing attempted.");
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }
  mozilla::AutoRestore<bool> guard(nsContentUtils::sFragmentParsingActive);
  nsContentUtils::sFragmentParsingActive = true;
  if (!sXMLFragmentParser) {
    nsCOMPtr<nsIParser> parser = do_CreateInstance(kCParserCID);
    parser.forget(&sXMLFragmentParser);
    
  }
  if (!sXMLFragmentSink) {
    NS_NewXMLFragmentContentSink(&sXMLFragmentSink);
    
  }
  nsCOMPtr<nsIContentSink> contentsink = do_QueryInterface(sXMLFragmentSink);
  NS_ABORT_IF_FALSE(contentsink, "Sink doesn't QI to nsIContentSink!");
  sXMLFragmentParser->SetContentSink(contentsink);

  sXMLFragmentSink->SetTargetDocument(aDocument);
  sXMLFragmentSink->SetPreventScriptExecution(aPreventScriptExecution);

  nsresult rv =
    sXMLFragmentParser->ParseFragment(aSourceBuffer,
                                      aTagStack);
  if (NS_FAILED(rv)) {
    
    NS_IF_RELEASE(sXMLFragmentParser);
    NS_IF_RELEASE(sXMLFragmentSink);
    return rv;
  }

  rv = sXMLFragmentSink->FinishFragmentParsing(aReturn);

  sXMLFragmentParser->Reset();

  return rv;
}


nsresult
nsContentUtils::ConvertToPlainText(const nsAString& aSourceBuffer,
                                   nsAString& aResultBuffer,
                                   uint32_t aFlags,
                                   uint32_t aWrapCol)
{
  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), "about:blank");
  nsCOMPtr<nsIPrincipal> principal =
    do_CreateInstance(NS_NULLPRINCIPAL_CONTRACTID);
  nsCOMPtr<nsIDOMDocument> domDocument;
  nsresult rv = NS_NewDOMDocument(getter_AddRefs(domDocument),
                                  EmptyString(),
                                  EmptyString(),
                                  nullptr,
                                  uri,
                                  uri,
                                  principal,
                                  true,
                                  nullptr,
                                  DocumentFlavorHTML);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);
  rv = nsContentUtils::ParseDocumentHTML(aSourceBuffer, document,
    !(aFlags & nsIDocumentEncoder::OutputNoScriptContent));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocumentEncoder> encoder = do_CreateInstance(
    "@mozilla.org/layout/documentEncoder;1?type=text/plain");

  rv = encoder->Init(domDocument, NS_LITERAL_STRING("text/plain"), aFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  encoder->SetWrapColumn(aWrapCol);

  return encoder->EncodeToString(aResultBuffer);
}


nsresult
nsContentUtils::SetNodeTextContent(nsIContent* aContent,
                                   const nsAString& aValue,
                                   bool aTryReuse)
{
  
  nsCOMPtr<nsIContent> owningContent;

  
  mozAutoSubtreeModified subtree(nullptr, nullptr);

  
  
  {
    
    
    nsIDocument* doc = aContent->OwnerDoc();

    
    if (HasMutationListeners(doc, NS_EVENT_BITS_MUTATION_NODEREMOVED)) {
      subtree.UpdateTarget(doc, nullptr);
      owningContent = aContent;
      nsCOMPtr<nsINode> child;
      bool skipFirst = aTryReuse;
      for (child = aContent->GetFirstChild();
           child && child->GetParentNode() == aContent;
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
    UPDATE_CONTENT_MODEL, true);
  nsAutoMutationBatch mb;

  uint32_t childCount = aContent->GetChildCount();

  if (aTryReuse && !aValue.IsEmpty()) {
    uint32_t removeIndex = 0;

    for (uint32_t i = 0; i < childCount; ++i) {
      nsIContent* child = aContent->GetChildAt(removeIndex);
      if (removeIndex == 0 && child && child->IsNodeOfType(nsINode::eTEXT)) {
        nsresult rv = child->SetText(aValue, true);
        NS_ENSURE_SUCCESS(rv, rv);

        removeIndex = 1;
      }
      else {
        aContent->RemoveChildAt(removeIndex, true);
      }
    }

    if (removeIndex == 1) {
      return NS_OK;
    }
  }
  else {
    mb.Init(aContent, true, false);
    for (uint32_t i = 0; i < childCount; ++i) {
      aContent->RemoveChildAt(0, true);
    }
  }
  mb.RemovalDone();

  if (aValue.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> textContent;
  nsresult rv = NS_NewTextNode(getter_AddRefs(textContent),
                               aContent->NodeInfo()->NodeInfoManager());
  NS_ENSURE_SUCCESS(rv, rv);

  textContent->SetText(aValue, true);

  rv = aContent->AppendChildTo(textContent, true);
  mb.NodesAdded();
  return rv;
}

static void AppendNodeTextContentsRecurse(nsINode* aNode, nsAString& aResult)
{
  for (nsIContent* child = aNode->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsElement()) {
      AppendNodeTextContentsRecurse(child, aResult);
    }
    else if (child->IsNodeOfType(nsINode::eTEXT)) {
      child->AppendTextTo(aResult);
    }
  }
}


void
nsContentUtils::AppendNodeTextContent(nsINode* aNode, bool aDeep,
                                      nsAString& aResult)
{
  if (aNode->IsNodeOfType(nsINode::eTEXT)) {
    static_cast<nsIContent*>(aNode)->AppendTextTo(aResult);
  }
  else if (aDeep) {
    AppendNodeTextContentsRecurse(aNode, aResult);
  }
  else {
    for (nsIContent* child = aNode->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      if (child->IsNodeOfType(nsINode::eTEXT)) {
        child->AppendTextTo(aResult);
      }
    }
  }
}

bool
nsContentUtils::HasNonEmptyTextContent(nsINode* aNode)
{
  for (nsIContent* child = aNode->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsNodeOfType(nsINode::eTEXT) &&
        child->TextLength() > 0) {
      return true;
    }
  }

  return false;
}


bool
nsContentUtils::IsInSameAnonymousTree(const nsINode* aNode,
                                      const nsIContent* aContent)
{
  NS_PRECONDITION(aNode,
                  "Must have a node to work with");
  NS_PRECONDITION(aContent,
                  "Must have a content to work with");
  
  if (!aNode->IsNodeOfType(nsINode::eCONTENT)) {
    






    return aContent->GetBindingParent() == nullptr;
  }

  return static_cast<const nsIContent*>(aNode)->GetBindingParent() ==
         aContent->GetBindingParent();
 
}

class AnonymousContentDestroyer : public nsRunnable {
public:
  AnonymousContentDestroyer(nsCOMPtr<nsIContent>* aContent) {
    mContent.swap(*aContent);
    mParent = mContent->GetParent();
    mDoc = mContent->OwnerDoc();
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


void
nsContentUtils::HoldJSObjects(void* aScriptObjectHolder,
                              nsScriptObjectTracer* aTracer)
{
  MOZ_ASSERT(sXPConnect, "Tried to HoldJSObjects when there was no XPConnect");
  if (sXPConnect) {
    sXPConnect->AddJSHolder(aScriptObjectHolder, aTracer);
  }
}


void
nsContentUtils::DropJSObjects(void* aScriptObjectHolder)
{
  if (sXPConnect) {
    sXPConnect->RemoveJSHolder(aScriptObjectHolder);
  }
}

#ifdef DEBUG

bool
nsContentUtils::AreJSObjectsHeld(void* aScriptHolder)
{
  return sXPConnect->TestJSHolder(aScriptHolder);
}
#endif


void
nsContentUtils::NotifyInstalledMenuKeyboardListener(bool aInstalling)
{
  nsIMEStateManager::OnInstalledMenuKeyboardListener(aInstalling);
}

static bool SchemeIs(nsIURI* aURI, const char* aScheme)
{
  nsCOMPtr<nsIURI> baseURI = NS_GetInnermostURI(aURI);
  NS_ENSURE_TRUE(baseURI, false);

  bool isScheme = false;
  return NS_SUCCEEDED(baseURI->SchemeIs(aScheme, &isScheme)) && isScheme;
}


nsresult
nsContentUtils::CheckSecurityBeforeLoad(nsIURI* aURIToLoad,
                                        nsIPrincipal* aLoadingPrincipal,
                                        uint32_t aCheckLoadFlags,
                                        bool aAllowData,
                                        uint32_t aContentPolicyType,
                                        nsISupports* aContext,
                                        const nsACString& aMimeGuess,
                                        nsISupports* aExtra)
{
  NS_PRECONDITION(aLoadingPrincipal, "Must have a loading principal here");

  bool isSystemPrin = false;
  if (NS_SUCCEEDED(sSecurityManager->IsSystemPrincipal(aLoadingPrincipal,
                                                       &isSystemPrin)) &&
      isSystemPrin) {
    return NS_OK;
  }
  
  
  
  nsresult rv = sSecurityManager->
    CheckLoadURIWithPrincipal(aLoadingPrincipal, aURIToLoad, aCheckLoadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
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

  return aLoadingPrincipal->CheckMayLoad(aURIToLoad, true, false);
}

bool
nsContentUtils::IsSystemPrincipal(nsIPrincipal* aPrincipal)
{
  bool isSystem;
  nsresult rv = sSecurityManager->IsSystemPrincipal(aPrincipal, &isSystem);
  return NS_SUCCEEDED(rv) && isSystem;
}

bool
nsContentUtils::CombineResourcePrincipals(nsCOMPtr<nsIPrincipal>* aResourcePrincipal,
                                          nsIPrincipal* aExtraPrincipal)
{
  if (!aExtraPrincipal) {
    return false;
  }
  if (!*aResourcePrincipal) {
    *aResourcePrincipal = aExtraPrincipal;
    return true;
  }
  if (*aResourcePrincipal == aExtraPrincipal) {
    return false;
  }
  bool subsumes;
  if (NS_SUCCEEDED((*aResourcePrincipal)->Subsumes(aExtraPrincipal, &subsumes)) &&
      subsumes) {
    return false;
  }
  sSecurityManager->GetSystemPrincipal(getter_AddRefs(*aResourcePrincipal));
  return true;
}


void
nsContentUtils::TriggerLink(nsIContent *aContent, nsPresContext *aPresContext,
                            nsIURI *aLinkURI, const nsString &aTargetSpec,
                            bool aClick, bool aIsUserTriggered,
                            bool aIsTrusted)
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
    uint32_t flag =
      aIsUserTriggered ?
      (uint32_t)nsIScriptSecurityManager::STANDARD :
      (uint32_t)nsIScriptSecurityManager::LOAD_IS_AUTOMATIC_DOCUMENT_REPLACEMENT;
    proceed =
      sSecurityManager->CheckLoadURIWithPrincipal(aContent->NodePrincipal(),
                                                  aLinkURI, flag);
  }

  
  
  if (NS_SUCCEEDED(proceed)) {

    
    
    
    
    
    nsAutoString fileName;
    if ((!aContent->IsHTML(nsGkAtoms::a) && !aContent->IsHTML(nsGkAtoms::area) &&
         !aContent->IsSVG(nsGkAtoms::a)) ||
        !aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::download, fileName) ||
        NS_FAILED(aContent->NodePrincipal()->CheckMayLoad(aLinkURI, false, true))) {
      fileName.SetIsVoid(true); 
    }

    handler->OnLinkClick(aContent, aLinkURI,
                         fileName.IsVoid() ? aTargetSpec.get() : EmptyString().get(),
                         fileName, nullptr, nullptr, aIsTrusted);
  }
}


void
nsContentUtils::GetLinkLocation(Element* aElement, nsString& aLocationString)
{
  nsCOMPtr<nsIURI> hrefURI = aElement->GetHrefURI();
  if (hrefURI) {
    nsAutoCString specUTF8;
    nsresult rv = hrefURI->GetSpec(specUTF8);
    if (NS_SUCCEEDED(rv))
      CopyUTF8toUTF16(specUTF8, aLocationString);
  }
}


nsIWidget*
nsContentUtils::GetTopLevelWidget(nsIWidget* aWidget)
{
  if (!aWidget)
    return nullptr;

  return aWidget->GetTopLevelWidget();
}


const nsDependentString
nsContentUtils::GetLocalizedEllipsis()
{
  static PRUnichar sBuf[4] = { 0, 0, 0, 0 };
  if (!sBuf[0]) {
    nsAdoptingString tmp = Preferences::GetLocalizedString("intl.ellipsis");
    uint32_t len = std::min(uint32_t(tmp.Length()),
                          uint32_t(ArrayLength(sBuf) - 1));
    CopyUnicodeTo(tmp, 0, sBuf, len);
    if (!sBuf[0])
      sBuf[0] = PRUnichar(0x2026);
  }
  return nsDependentString(sBuf);
}


nsEvent*
nsContentUtils::GetNativeEvent(nsIDOMEvent* aDOMEvent)
{
  return aDOMEvent ? aDOMEvent->GetInternalNSEvent() : nullptr;
}


bool
nsContentUtils::DOMEventToNativeKeyEvent(nsIDOMKeyEvent* aKeyEvent,
                                         nsNativeKeyEvent* aNativeEvent,
                                         bool aGetCharCode)
{
  bool defaultPrevented;
  aKeyEvent->GetPreventDefault(&defaultPrevented);
  if (defaultPrevented)
    return false;

  bool trusted = false;
  aKeyEvent->GetIsTrusted(&trusted);
  if (!trusted)
    return false;

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

  return true;
}

static bool
HasASCIIDigit(const nsTArray<nsShortcutCandidate>& aCandidates)
{
  for (uint32_t i = 0; i < aCandidates.Length(); ++i) {
    uint32_t ch = aCandidates[i].mCharCode;
    if (ch >= '0' && ch <= '9')
      return true;
  }
  return false;
}

static bool
CharsCaseInsensitiveEqual(uint32_t aChar1, uint32_t aChar2)
{
  return aChar1 == aChar2 ||
         (IS_IN_BMP(aChar1) && IS_IN_BMP(aChar2) &&
          ToLowerCase(PRUnichar(aChar1)) == ToLowerCase(PRUnichar(aChar2)));
}

static bool
IsCaseChangeableChar(uint32_t aChar)
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
      nsShortcutCandidate key(nativeKeyEvent->charCode, false);
      aCandidates.AppendElement(key);
    }

    uint32_t len = nativeKeyEvent->alternativeCharCodes.Length();
    if (!nativeKeyEvent->IsShift()) {
      for (uint32_t i = 0; i < len; ++i) {
        uint32_t ch =
          nativeKeyEvent->alternativeCharCodes[i].mUnshiftedCharCode;
        if (!ch || ch == nativeKeyEvent->charCode)
          continue;

        nsShortcutCandidate key(ch, false);
        aCandidates.AppendElement(key);
      }
      
      
      
      
      if (!HasASCIIDigit(aCandidates)) {
        for (uint32_t i = 0; i < len; ++i) {
          uint32_t ch =
            nativeKeyEvent->alternativeCharCodes[i].mShiftedCharCode;
          if (ch >= '0' && ch <= '9') {
            nsShortcutCandidate key(ch, false);
            aCandidates.AppendElement(key);
            break;
          }
        }
      }
    } else {
      for (uint32_t i = 0; i < len; ++i) {
        uint32_t ch = nativeKeyEvent->alternativeCharCodes[i].mShiftedCharCode;
        if (!ch)
          continue;

        if (ch != nativeKeyEvent->charCode) {
          nsShortcutCandidate key(ch, false);
          aCandidates.AppendElement(key);
        }

        
        

        
        
        uint32_t unshiftCh =
          nativeKeyEvent->alternativeCharCodes[i].mUnshiftedCharCode;
        if (CharsCaseInsensitiveEqual(ch, unshiftCh))
          continue;

        
        
        
        if (IsCaseChangeableChar(ch))
          continue;

        
        
        nsShortcutCandidate key(ch, true);
        aCandidates.AppendElement(key);
      }
    }
  } else {
    uint32_t charCode;
    aDOMKeyEvent->GetCharCode(&charCode);
    if (charCode) {
      nsShortcutCandidate key(charCode, false);
      aCandidates.AppendElement(key);
    }
  }
}


void
nsContentUtils::GetAccessKeyCandidates(nsKeyEvent* aNativeKeyEvent,
                                       nsTArray<uint32_t>& aCandidates)
{
  NS_PRECONDITION(aCandidates.IsEmpty(), "aCandidates must be empty");

  
  
  
  
  if (aNativeKeyEvent->charCode) {
    uint32_t ch = aNativeKeyEvent->charCode;
    if (IS_IN_BMP(ch))
      ch = ToLowerCase(PRUnichar(ch));
    aCandidates.AppendElement(ch);
  }
  for (uint32_t i = 0;
       i < aNativeKeyEvent->alternativeCharCodes.Length(); ++i) {
    uint32_t ch[2] =
      { aNativeKeyEvent->alternativeCharCodes[i].mUnshiftedCharCode,
        aNativeKeyEvent->alternativeCharCodes[i].mShiftedCharCode };
    for (uint32_t j = 0; j < 2; ++j) {
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
    sRunnersCountAtFirstBlocker = sBlockedScriptRunners->Length();
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

  uint32_t firstBlocker = sRunnersCountAtFirstBlocker;
  uint32_t lastBlocker = sBlockedScriptRunners->Length();
  uint32_t originalFirstBlocker = firstBlocker;
  uint32_t blockersCount = lastBlocker - firstBlocker;
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
  sBlockedScriptRunners->RemoveElementsAt(originalFirstBlocker, blockersCount);
}


bool
nsContentUtils::AddScriptRunner(nsIRunnable* aRunnable)
{
  if (!aRunnable) {
    return false;
  }

  if (sScriptBlockerCount) {
    return sBlockedScriptRunners->AppendElement(aRunnable) != nullptr;
  }
  
  nsCOMPtr<nsIRunnable> run = aRunnable;
  run->Run();

  return true;
}

void
nsContentUtils::LeaveMicroTask()
{
  if (--sMicroTaskLevel == 0) {
    nsDOMMutationObserver::HandleMutations();
  }
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
                                                        true);
  const nsAString &value =
    nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(Substring(++tip, end),
                                                        true);

  

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


nsViewportInfo
nsContentUtils::GetViewportInfo(nsIDocument *aDocument,
                                uint32_t aDisplayWidth,
                                uint32_t aDisplayHeight)
{
  return aDocument->GetViewportInfo(aDisplayWidth, aDisplayHeight);
}


double
nsContentUtils::GetDevicePixelsPerMetaViewportPixel(nsIWidget* aWidget)
{
  int32_t prefValue = Preferences::GetInt("browser.viewport.scaleRatio", 0);
  if (prefValue > 0) {
    return double(prefValue) / 100.0;
  }

  float dpi = aWidget->GetDPI();
  if (dpi < 200.0) {
    
    return 1.0;
  }
  if (dpi < 300.0) {
    
    return 1.5;
  }
  
  return floor(dpi / 150.0);
}


nsresult
nsContentUtils::ProcessViewportInfo(nsIDocument *aDocument,
                                    const nsAString &viewportInfo) {

  
  nsresult rv = NS_OK;

  aDocument->SetHeaderData(nsGkAtoms::viewport, viewportInfo);

  
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
  nsIDragSession* dragSession = nullptr;
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  if (dragService)
    dragService->GetCurrentSession(&dragSession);
  return dragSession;
}


nsresult
nsContentUtils::SetDataTransferInEvent(nsDragEvent* aDragEvent)
{
  if (aDragEvent->dataTransfer || !aDragEvent->mFlags.mIsTrusted)
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

  bool isCrossDomainSubFrameDrop = false;
  if (aDragEvent->message == NS_DRAGDROP_DROP ||
      aDragEvent->message == NS_DRAGDROP_DRAGDROP) {
    isCrossDomainSubFrameDrop = CheckForSubFrameDrop(dragSession, aDragEvent);
  }

  
  initialDataTransfer->Clone(aDragEvent->message, aDragEvent->userCancelled,
                             isCrossDomainSubFrameDrop,
                             getter_AddRefs(aDragEvent->dataTransfer));
  NS_ENSURE_TRUE(aDragEvent->dataTransfer, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  if (aDragEvent->message == NS_DRAGDROP_ENTER ||
      aDragEvent->message == NS_DRAGDROP_OVER) {
    uint32_t action, effectAllowed;
    dragSession->GetDragAction(&action);
    aDragEvent->dataTransfer->GetEffectAllowedInt(&effectAllowed);
    aDragEvent->dataTransfer->SetDropEffectInt(FilterDropEffect(action, effectAllowed));
  }
  else if (aDragEvent->message == NS_DRAGDROP_DROP ||
           aDragEvent->message == NS_DRAGDROP_DRAGDROP ||
           aDragEvent->message == NS_DRAGDROP_END) {
    
    
    
    
    uint32_t dropEffect;
    initialDataTransfer->GetDropEffectInt(&dropEffect);
    aDragEvent->dataTransfer->SetDropEffectInt(dropEffect);
  }

  return NS_OK;
}


uint32_t
nsContentUtils::FilterDropEffect(uint32_t aAction, uint32_t aEffectAllowed)
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


bool
nsContentUtils::CheckForSubFrameDrop(nsIDragSession* aDragSession, nsDragEvent* aDropEvent)
{
  nsCOMPtr<nsIContent> target = do_QueryInterface(aDropEvent->originalTarget);
  if (!target && !target->OwnerDoc()) {
    return true;
  }
  
  nsIDocument* targetDoc = target->OwnerDoc();
  nsCOMPtr<nsIWebNavigation> twebnav = do_GetInterface(targetDoc->GetWindow());
  nsCOMPtr<nsIDocShellTreeItem> tdsti = do_QueryInterface(twebnav);
  if (!tdsti) {
    return true;
  }

  int32_t type = -1;
  if (NS_FAILED(tdsti->GetItemType(&type))) {
    return true;
  }

  
  if (type == nsIDocShellTreeItem::typeChrome) {
    return false;
  }

  
  
  nsCOMPtr<nsIDOMDocument> sourceDocument;
  aDragSession->GetSourceDocument(getter_AddRefs(sourceDocument));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(sourceDocument);
  if (doc) {
    
    
    do {
      doc = doc->GetParentDocument();
      if (doc == targetDoc) {
        
        return true;
      }
    } while (doc);
  }

  return false;
}


bool
nsContentUtils::URIIsLocalFile(nsIURI *aURI)
{
  bool isFile;
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

  nsAutoCString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t index = spec.FindChar('#');
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
  bool hasHadScriptObject = true;
  nsIScriptGlobalObject* sgo =
    aNode->OwnerDoc()->GetScriptHandlingObject(hasHadScriptObject);
  
  
  if (!sgo && hasHadScriptObject) {
    *aRv = NS_ERROR_UNEXPECTED;
    return nullptr;
  }

  if (sgo) {
    nsIScriptContext* scx = sgo->GetContext();
    
    if (!scx) {
      *aRv = NS_ERROR_UNEXPECTED;
      return nullptr;
    }
    return scx;
  }

  return nullptr;
}


JSContext *
nsContentUtils::GetCurrentJSContext()
{
  JSContext *cx = nullptr;

  sThreadJSContextStack->Peek(&cx);

  return cx;
}


JSContext *
nsContentUtils::GetSafeJSContext()
{
  return sThreadJSContextStack->GetSafeJSContext();
}


nsresult
nsContentUtils::ASCIIToLower(nsAString& aStr)
{
  PRUnichar* iter = aStr.BeginWriting();
  PRUnichar* end = aStr.EndWriting();
  if (MOZ_UNLIKELY(!iter || !end)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  while (iter != end) {
    PRUnichar c = *iter;
    if (c >= 'A' && c <= 'Z') {
      *iter = c + ('a' - 'A');
    }
    ++iter;
  }
  return NS_OK;
}


nsresult
nsContentUtils::ASCIIToLower(const nsAString& aSource, nsAString& aDest)
{
  uint32_t len = aSource.Length();
  aDest.SetLength(len);
  if (aDest.Length() == len) {
    PRUnichar* dest = aDest.BeginWriting();
    if (MOZ_UNLIKELY(!dest)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    const PRUnichar* iter = aSource.BeginReading();
    const PRUnichar* end = aSource.EndReading();
    while (iter != end) {
      PRUnichar c = *iter;
      *dest = (c >= 'A' && c <= 'Z') ?
         c + ('a' - 'A') : c;
      ++iter;
      ++dest;
    }
    return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}


nsresult
nsContentUtils::ASCIIToUpper(nsAString& aStr)
{
  PRUnichar* iter = aStr.BeginWriting();
  PRUnichar* end = aStr.EndWriting();
  if (MOZ_UNLIKELY(!iter || !end)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  while (iter != end) {
    PRUnichar c = *iter;
    if (c >= 'a' && c <= 'z') {
      *iter = c + ('A' - 'a');
    }
    ++iter;
  }
  return NS_OK;
}


nsresult
nsContentUtils::ASCIIToUpper(const nsAString& aSource, nsAString& aDest)
{
  uint32_t len = aSource.Length();
  aDest.SetLength(len);
  if (aDest.Length() == len) {
    PRUnichar* dest = aDest.BeginWriting();
    if (MOZ_UNLIKELY(!dest)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    const PRUnichar* iter = aSource.BeginReading();
    const PRUnichar* end = aSource.EndReading();
    while (iter != end) {
      PRUnichar c = *iter;
      *dest = (c >= 'a' && c <= 'z') ?
         c + ('A' - 'a') : c;
      ++iter;
      ++dest;
    }
    return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}


bool
nsContentUtils::EqualsIgnoreASCIICase(const nsAString& aStr1,
                                      const nsAString& aStr2)
{
  uint32_t len = aStr1.Length();
  if (len != aStr2.Length()) {
    return false;
  }

  const PRUnichar* str1 = aStr1.BeginReading();
  const PRUnichar* str2 = aStr2.BeginReading();
  const PRUnichar* end = str1 + len;

  while (str1 < end) {
    PRUnichar c1 = *str1++;
    PRUnichar c2 = *str2++;

    
    if ((c1 ^ c2) & 0xffdf) {
      return false;
    }

    
    
    if (c1 != c2) {
      
      
      PRUnichar c1Upper = c1 & 0xffdf;
      if (!('A' <= c1Upper && c1Upper <= 'Z')) {
        return false;
      }
    }
  }

  return true;
}


bool
nsContentUtils::StringContainsASCIIUpper(const nsAString& aStr)
{
  const PRUnichar* iter = aStr.BeginReading();
  const PRUnichar* end = aStr.EndReading();
  while (iter != end) {
    PRUnichar c = *iter;
    if (c >= 'A' && c <= 'Z') {
      return true;
    }
    ++iter;
  }

  return false;
}


nsIInterfaceRequestor*
nsContentUtils::GetSameOriginChecker()
{
  if (!sSameOriginChecker) {
    sSameOriginChecker = new SameOriginChecker();
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

  nsresult rv = oldPrincipal->CheckMayLoad(newURI, false, false);
  if (NS_SUCCEEDED(rv) && newOriginalURI != newURI) {
    rv = oldPrincipal->CheckMayLoad(newOriginalURI, false, false);
  }

  return rv;
}

NS_IMPL_ISUPPORTS2(SameOriginChecker,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor)

NS_IMETHODIMP
SameOriginChecker::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                          nsIChannel *aNewChannel,
                                          uint32_t aFlags,
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
SameOriginChecker::GetInterface(const nsIID & aIID, void **aResult)
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

    int32_t port = -1;
    uri->GetPort(&port);
    if (port != -1 && port == NS_GetDefaultPort(scheme.get()))
      port = -1;

    nsCString hostPort;
    rv = NS_GenerateHostPort(host, port, hostPort);
    NS_ENSURE_SUCCESS(rv, rv);

    aOrigin = scheme + NS_LITERAL_CSTRING("://") + hostPort;
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

    int32_t port = -1;
    uri->GetPort(&port);
    if (port != -1 && port == NS_GetDefaultPort(scheme.get()))
      port = -1;

    nsCString hostPort;
    rv = NS_GenerateHostPort(host, port, hostPort);
    NS_ENSURE_SUCCESS(rv, rv);

    aOrigin = NS_ConvertUTF8toUTF16(
      scheme + NS_LITERAL_CSTRING("://") + hostPort);
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
    return nullptr;

  nsCOMPtr<nsIDOMWindow> window =
    do_QueryInterface(aScriptContext->GetGlobalObject());
  nsIDocument *doc = nullptr;
  if (window) {
    nsCOMPtr<nsIDOMDocument> domdoc;
    window->GetDocument(getter_AddRefs(domdoc));
    if (domdoc) {
      CallQueryInterface(domdoc, &doc);
    }
  }
  return doc;
}


bool
nsContentUtils::CheckMayLoad(nsIPrincipal* aPrincipal, nsIChannel* aChannel, bool aAllowIfInheritsPrincipal)
{
  nsCOMPtr<nsIURI> channelURI;
  nsresult rv = NS_GetFinalChannelURI(aChannel, getter_AddRefs(channelURI));
  NS_ENSURE_SUCCESS(rv, false);

  return NS_SUCCEEDED(aPrincipal->CheckMayLoad(channelURI, false, aAllowIfInheritsPrincipal));
}

nsContentTypeParser::nsContentTypeParser(const nsAString& aString)
  : mString(aString), mService(nullptr)
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
                                EmptyCString(), false, nullptr,
                                aResult);
}



bool
nsContentUtils::CanAccessNativeAnon()
{
  return IsCallerChrome() || IsCallerXBL();
}

 nsresult
nsContentUtils::DispatchXULCommand(nsIContent* aTarget,
                                   bool aTrusted,
                                   nsIDOMEvent* aSourceEvent,
                                   nsIPresShell* aShell,
                                   bool aCtrl,
                                   bool aAlt,
                                   bool aShift,
                                   bool aMeta)
{
  NS_ENSURE_STATE(aTarget);
  nsIDocument* doc = aTarget->OwnerDoc();
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
  NS_ENSURE_STATE(domDoc);
  nsCOMPtr<nsIDOMEvent> event;
  domDoc->CreateEvent(NS_LITERAL_STRING("xulcommandevent"),
                      getter_AddRefs(event));
  nsCOMPtr<nsIDOMXULCommandEvent> xulCommand = do_QueryInterface(event);
  nsresult rv = xulCommand->InitCommandEvent(NS_LITERAL_STRING("command"),
                                             true, true, doc->GetWindow(),
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
  bool dummy;
  return target->DispatchEvent(event, &dummy);
}


nsresult
nsContentUtils::WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
                           nsWrapperCache *cache, const nsIID* aIID, jsval *vp,
                           nsIXPConnectJSObjectHolder **aHolder,
                           bool aAllowWrapping)
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

  
  
  
  
  
  
  bool isMainThread = NS_IsMainThread();

  if (isMainThread) {
    nsLayoutStatics::AddRef();
  }
  else {
    sXPConnect->AddRef();
  }

  JSContext *topJSContext;
  nsresult rv = sThreadJSContextStack->Peek(&topJSContext);
  if (NS_SUCCEEDED(rv)) {
    bool push = topJSContext != cx;
    if (push) {
      rv = sThreadJSContextStack->Push(cx);
    }
    if (NS_SUCCEEDED(rv)) {
      rv = sXPConnect->WrapNativeToJSVal(cx, scope, native, cache, aIID,
                                         aAllowWrapping, vp, aHolder);
      if (push) {
        sThreadJSContextStack->Pop(nullptr);
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

nsresult
nsContentUtils::CreateArrayBuffer(JSContext *aCx, const nsACString& aData,
                                  JSObject** aResult)
{
  if (!aCx) {
    return NS_ERROR_FAILURE;
  }

  int32_t dataLen = aData.Length();
  *aResult = JS_NewArrayBuffer(aCx, dataLen);
  if (!*aResult) {
    return NS_ERROR_FAILURE;
  }

  if (dataLen > 0) {
    NS_ASSERTION(JS_IsArrayBufferObject(*aResult), "What happened?");
    memcpy(JS_GetArrayBufferData(*aResult), aData.BeginReading(), dataLen);
  }

  return NS_OK;
}



nsresult
nsContentUtils::CreateBlobBuffer(JSContext* aCx,
                                 const nsACString& aData,
                                 jsval& aBlob)
{
  uint32_t blobLen = aData.Length();
  void* blobData = moz_malloc(blobLen);
  nsCOMPtr<nsIDOMBlob> blob;
  if (blobData) {
    memcpy(blobData, aData.BeginReading(), blobLen);
    blob = new nsDOMMemoryFile(blobData, blobLen, EmptyString());
  } else {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  JSObject* scope = JS_GetGlobalForScopeChain(aCx);
  return nsContentUtils::WrapNative(aCx, scope, blob, &aBlob, nullptr, true);
}

void
nsContentUtils::StripNullChars(const nsAString& aInStr, nsAString& aOutStr)
{
  
  
  int32_t firstNullPos = aInStr.FindChar('\0');
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

struct ClassMatchingInfo {
  nsAttrValue::AtomArray mClasses;
  nsCaseTreatment mCaseTreatment;
};


bool
nsContentUtils::MatchClassNames(nsIContent* aContent, int32_t aNamespaceID,
                                nsIAtom* aAtom, void* aData)
{
  
  const nsAttrValue* classAttr = aContent->GetClasses();
  if (!classAttr) {
    return false;
  }
  
  
  ClassMatchingInfo* info = static_cast<ClassMatchingInfo*>(aData);
  uint32_t length = info->mClasses.Length();
  if (!length) {
    
    return false;
  }
  uint32_t i;
  for (i = 0; i < length; ++i) {
    if (!classAttr->Contains(info->mClasses[i],
                             info->mCaseTreatment)) {
      return false;
    }
  }
  
  return true;
}


void
nsContentUtils::DestroyClassNameArray(void* aData)
{
  ClassMatchingInfo* info = static_cast<ClassMatchingInfo*>(aData);
  delete info;
}


void*
nsContentUtils::AllocClassMatchingInfo(nsINode* aRootNode,
                                       const nsString* aClasses)
{
  nsAttrValue attrValue;
  attrValue.ParseAtomArray(*aClasses);
  
  ClassMatchingInfo* info = new ClassMatchingInfo;
  if (attrValue.Type() == nsAttrValue::eAtomArray) {
    info->mClasses.SwapElements(*(attrValue.GetAtomArrayValue()));
  } else if (attrValue.Type() == nsAttrValue::eAtom) {
    info->mClasses.AppendElement(attrValue.GetAtomValue());
  }

  info->mCaseTreatment =
    aRootNode->OwnerDoc()->GetCompatibilityMode() == eCompatibility_NavQuirks ?
    eIgnoreCase : eCaseMatters;
  return info;
}

#ifdef DEBUG
class DebugWrapperTraversalCallback : public nsCycleCollectionTraversalCallback
{
public:
  DebugWrapperTraversalCallback(void* aWrapper) : mFound(false),
                                                  mWrapper(aWrapper)
  {
    mFlags = WANT_ALL_TRACES;
  }

  NS_IMETHOD_(void) DescribeRefCountedNode(nsrefcnt refCount,
                                           const char *objName)
  {
  }
  NS_IMETHOD_(void) DescribeGCedNode(bool isMarked,
                                     const char *objName)
  {
  }

  NS_IMETHOD_(void) NoteXPCOMRoot(nsISupports *root)
  {
  }
  NS_IMETHOD_(void) NoteJSRoot(void* root)
  {
  }
  NS_IMETHOD_(void) NoteNativeRoot(void* root,
                                   nsCycleCollectionParticipant* helper)
  {
  }

  NS_IMETHOD_(void) NoteJSChild(void* child)
  {
    if (child == mWrapper) {
      mFound = true;
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

  NS_IMETHOD_(void) NoteWeakMapping(void* map, void* key, void* kdelegate, void* val)
  {
  }

  bool mFound;

private:
  void* mWrapper;
};

static void
DebugWrapperTraceCallback(void *p, const char *name, void *closure)
{
  DebugWrapperTraversalCallback* callback =
    static_cast<DebugWrapperTraversalCallback*>(closure);
  callback->NoteJSChild(p);
}


void
nsContentUtils::CheckCCWrapperTraversal(void* aScriptObjectHolder,
                                        nsWrapperCache* aCache,
                                        nsScriptObjectTracer* aTracer)
{
  JSObject* wrapper = aCache->GetWrapper();
  if (!wrapper) {
    return;
  }

  DebugWrapperTraversalCallback callback(wrapper);

  aTracer->Traverse(aScriptObjectHolder, callback);
  MOZ_ASSERT(callback.mFound,
             "Cycle collection participant didn't traverse to preserved "
             "wrapper! This will probably crash.");

  callback.mFound = false;
  aTracer->Trace(aScriptObjectHolder, DebugWrapperTraceCallback, &callback);
  MOZ_ASSERT(callback.mFound,
             "Cycle collection participant didn't trace preserved wrapper! "
             "This will probably crash.");
}
#endif


bool
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

  
  
  if (nsEventStateManager::IsRemoteTarget(aContent)) {
    return true;
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
        int32_t i = 0, i_end;
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

nsIPresShell*
nsContentUtils::FindPresShellForDocument(nsIDocument* aDoc)
{
  nsIDocument* doc = aDoc;
  nsIDocument* displayDoc = doc->GetDisplayDocument();
  if (displayDoc) {
    doc = displayDoc;
  }

  nsIPresShell* shell = doc->GetShell();
  if (shell) {
    return shell;
  }

  nsCOMPtr<nsISupports> container = doc->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem = do_QueryInterface(container);
  while (docShellTreeItem) {
    
    
    
    
    nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(docShellTreeItem);
    nsIPresShell* presShell = docShell->GetPresShell();
    if (presShell) {
      return presShell;
    }
    nsCOMPtr<nsIDocShellTreeItem> parent;
    docShellTreeItem->GetParent(getter_AddRefs(parent));
    docShellTreeItem = parent;
  }

  return nullptr;
}

nsIWidget*
nsContentUtils::WidgetForDocument(nsIDocument* aDoc)
{
  nsIPresShell* shell = FindPresShellForDocument(aDoc);
  if (shell) {
    nsViewManager* VM = shell->GetViewManager();
    if (VM) {
      nsView* rootView = VM->GetRootView();
      if (rootView) {
        nsView* displayRoot = nsViewManager::GetDisplayRootFor(rootView);
        if (displayRoot) {
          return displayRoot->GetNearestWidget(nullptr);
        }
      }
    }
  }

  return nullptr;
}

static already_AddRefed<LayerManager>
LayerManagerForDocumentInternal(nsIDocument *aDoc, bool aRequirePersistent,
                                bool* aAllowRetaining)
{
  nsIWidget *widget = nsContentUtils::WidgetForDocument(aDoc);
  if (widget) {
    nsRefPtr<LayerManager> manager =
      widget->GetLayerManager(aRequirePersistent ? nsIWidget::LAYER_MANAGER_PERSISTENT : 
                              nsIWidget::LAYER_MANAGER_CURRENT,
                              aAllowRetaining);
    return manager.forget();
  }

  return nullptr;
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
          IsSitePermAllow(aPrincipal, "allowXULXBL"));
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
  if (DecoderTraits::IsOggType(nsDependentCString(aType))) {
    docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
    if (docFactory && aLoaderType) {
      *aLoaderType = TYPE_CONTENT;
    }
    return docFactory.forget();
  }
#endif

#ifdef MOZ_WIDGET_GONK
  if (DecoderTraits::IsOmxSupportedType(nsDependentCString(aType))) {
    docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
    if (docFactory && aLoaderType) {
      *aLoaderType = TYPE_CONTENT;
    }
    return docFactory.forget();
  }
#endif

#ifdef MOZ_WEBM
  if (DecoderTraits::IsWebMType(nsDependentCString(aType))) {
    docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
    if (docFactory && aLoaderType) {
      *aLoaderType = TYPE_CONTENT;
    }
    return docFactory.forget();
  }
#endif

#ifdef MOZ_DASH
  if (DecoderTraits::IsDASHMPDType(nsDependentCString(aType))) {
    docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
    if (docFactory && aLoaderType) {
      *aLoaderType = TYPE_CONTENT;
    }
    return docFactory.forget();
  }
#endif

#ifdef MOZ_GSTREAMER
  if (DecoderTraits::IsGStreamerSupportedType(nsDependentCString(aType))) {
    docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
    if (docFactory && aLoaderType) {
      *aLoaderType = TYPE_CONTENT;
    }
    return docFactory.forget();
  }
#endif

#ifdef MOZ_MEDIA_PLUGINS
  if (mozilla::MediaDecoder::IsMediaPluginsEnabled() &&
      DecoderTraits::IsMediaPluginsType(nsDependentCString(aType))) {
    docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
    if (docFactory && aLoaderType) {
      *aLoaderType = TYPE_CONTENT;
    }
    return docFactory.forget();
  }
#endif 

#ifdef MOZ_WMF
  if (DecoderTraits::IsWMFSupportedType(nsDependentCString(aType))) {
    docFactory = do_GetService("@mozilla.org/content/document-loader-factory;1");
    if (docFactory && aLoaderType) {
      *aLoaderType = TYPE_CONTENT;
    }
    return docFactory.forget();
  }
#endif

#endif 

  return NULL;
}


bool
nsContentUtils::IsPatternMatching(nsAString& aValue, nsAString& aPattern,
                                  nsIDocument* aDocument)
{
  NS_ASSERTION(aDocument, "aDocument should be a valid pointer (not null)");
  NS_ENSURE_TRUE(aDocument->GetScriptGlobalObject(), true);

  JSContext* cx = aDocument->GetScriptGlobalObject()->
                                  GetContext()->GetNativeContext();
  NS_ENSURE_TRUE(cx, true);

  JSAutoRequest ar(cx);

  
  aPattern.Insert(NS_LITERAL_STRING("^(?:"), 0);
  aPattern.Append(NS_LITERAL_STRING(")$"));

  JSObject* re = JS_NewUCRegExpObjectNoStatics(cx, static_cast<jschar*>
                                                 (aPattern.BeginWriting()),
                                               aPattern.Length(), 0);
  if (!re) {
    JS_ClearPendingException(cx);
    return true;
  }

  JS::Value rval = JS::NullValue();
  size_t idx = 0;
  if (!JS_ExecuteRegExpNoStatics(cx, re,
                                 static_cast<jschar*>(aValue.BeginWriting()),
                                 aValue.Length(), &idx, true, &rval)) {
    JS_ClearPendingException(cx);
    return true;
  }

  return !rval.isNull();
}


nsresult
nsContentUtils::URIInheritsSecurityContext(nsIURI *aURI, bool *aResult)
{
  
  
  return NS_URIChainHasFlags(aURI,
                             nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                             aResult);
}


bool
nsContentUtils::SetUpChannelOwner(nsIPrincipal* aLoadingPrincipal,
                                  nsIChannel* aChannel,
                                  nsIURI* aURI,
                                  bool aSetUpForAboutBlank,
                                  bool aForceOwner)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool inherit;
  
  
  
  
  if (aForceOwner || ((NS_SUCCEEDED(URIInheritsSecurityContext(aURI, &inherit)) &&
      (inherit || (aSetUpForAboutBlank && NS_IsAboutBlank(aURI)))))) {
#ifdef DEBUG
    
    if (aForceOwner) {
      nsCOMPtr<nsIURI> ownerURI;
      nsresult rv = aLoadingPrincipal->GetURI(getter_AddRefs(ownerURI));
      MOZ_ASSERT(NS_SUCCEEDED(rv) && SchemeIs(ownerURI, NS_NULLPRINCIPAL_SCHEME));
    }
#endif
    aChannel->SetOwner(aLoadingPrincipal);
    return true;
  }

  
  
  
  
  
  
  
  
  if (URIIsLocalFile(aURI) && aLoadingPrincipal &&
      NS_SUCCEEDED(aLoadingPrincipal->CheckMayLoad(aURI, false, false)) &&
      
      
      !IsSystemPrincipal(aLoadingPrincipal)) {
    aChannel->SetOwner(aLoadingPrincipal);
    return true;
  }

  return false;
}

bool
nsContentUtils::IsFullScreenApiEnabled()
{
  return sIsFullScreenApiEnabled;
}

bool
nsContentUtils::IsRequestFullScreenAllowed()
{
  return !sTrustedFullScreenOnly ||
         nsEventStateManager::IsHandlingUserInput() ||
         IsCallerChrome();
}


bool
nsContentUtils::HaveEqualPrincipals(nsIDocument* aDoc1, nsIDocument* aDoc2)
{
  if (!aDoc1 || !aDoc2) {
    return false;
  }
  bool principalsEqual = false;
  aDoc1->NodePrincipal()->Equals(aDoc2->NodePrincipal(), &principalsEqual);
  return principalsEqual;
}

static void
CheckForWindowedPlugins(nsIContent* aContent, void* aResult)
{
  if (!aContent->IsInDoc()) {
    return;
  }
  nsCOMPtr<nsIObjectLoadingContent> olc(do_QueryInterface(aContent));
  if (!olc) {
    return;
  }
  nsRefPtr<nsNPAPIPluginInstance> plugin;
  olc->GetPluginInstance(getter_AddRefs(plugin));
  if (!plugin) {
    return;
  }
  bool isWindowless = false;
  nsresult res = plugin->IsWindowless(&isWindowless);
  if (NS_SUCCEEDED(res) && !isWindowless) {
    *static_cast<bool*>(aResult) = true;
  }
}

static bool
DocTreeContainsWindowedPlugins(nsIDocument* aDoc, void* aResult)
{
  if (!nsContentUtils::IsChromeDoc(aDoc)) {
    aDoc->EnumerateFreezableElements(CheckForWindowedPlugins, aResult);
  }
  if (*static_cast<bool*>(aResult)) {
    
    return false;
  }
  aDoc->EnumerateSubDocuments(DocTreeContainsWindowedPlugins, aResult);
  
  
  return !*static_cast<bool*>(aResult);
}


bool
nsContentUtils::HasPluginWithUncontrolledEventDispatch(nsIDocument* aDoc)
{
#ifdef XP_MACOSX
  
  return false;
#endif
  bool result = false;
  
  
  nsIDocument* doc = aDoc;
  nsIDocument* parent = nullptr;
  while (doc && (parent = doc->GetParentDocument()) && !IsChromeDoc(parent)) {
    doc = parent;
  }

  DocTreeContainsWindowedPlugins(doc, &result);
  return result;
}


bool
nsContentUtils::HasPluginWithUncontrolledEventDispatch(nsIContent* aContent)
{
#ifdef XP_MACOSX
  
  return false;
#endif
  bool result = false;
  CheckForWindowedPlugins(aContent, &result);
  return result;
}


nsIDocument*
nsContentUtils::GetRootDocument(nsIDocument* aDoc)
{
  if (!aDoc) {
    return nullptr;
  }
  nsIDocument* doc = aDoc;
  while (doc->GetParentDocument()) {
    doc = doc->GetParentDocument();
  }
  return doc;
}


void
nsContentUtils::ReleaseWrapper(void* aScriptObjectHolder,
                               nsWrapperCache* aCache)
{
  if (aCache->PreservingWrapper()) {
    
    
    
    JSObject* obj = aCache->GetWrapperPreserveColor();
    if (aCache->IsDOMBinding() && obj) {
      xpc::GetObjectScope(obj)->RemoveDOMExpandoObject(obj);
    }
    aCache->SetPreservingWrapper(false);
    DropJSObjects(aScriptObjectHolder);
  }
}


void
nsContentUtils::TraceWrapper(nsWrapperCache* aCache, TraceCallback aCallback,
                             void *aClosure)
{
  if (aCache->PreservingWrapper()) {
    JSObject *wrapper = aCache->GetWrapperPreserveColor();
    if (wrapper) {
      aCallback(wrapper, "Preserved wrapper", aClosure);
    }
  }
}

nsresult
nsContentUtils::JSArrayToAtomArray(JSContext* aCx, const JS::Value& aJSArray,
                                   nsCOMArray<nsIAtom>& aRetVal)
{
  JSAutoRequest ar(aCx);
  if (!aJSArray.isObject()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  
  JSObject* obj = &aJSArray.toObject();
  JSAutoCompartment ac(aCx, obj);
  
  uint32_t length;
  if (!JS_IsArrayObject(aCx, obj) || !JS_GetArrayLength(aCx, obj, &length)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  JSString* str = nullptr;
  JS::Anchor<JSString *> deleteProtector(str);
  for (uint32_t i = 0; i < length; ++i) {
    jsval v;
    if (!JS_GetElement(aCx, obj, i, &v) ||
        !(str = JS_ValueToString(aCx, v))) {
      return NS_ERROR_ILLEGAL_VALUE;
    }

    nsDependentJSString depStr;
    if (!depStr.init(aCx, str)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIAtom> a = do_GetAtom(depStr);
    if (!a) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aRetVal.AppendObject(a);
  }
  return NS_OK;
}


void
nsContentUtils::GetSelectionInTextControl(Selection* aSelection,
                                          Element* aRoot,
                                          int32_t& aOutStartOffset,
                                          int32_t& aOutEndOffset)
{
  MOZ_ASSERT(aSelection && aRoot);

  if (!aSelection->GetRangeCount()) {
    
    aOutStartOffset = aOutEndOffset = 0;
    return;
  }

  nsCOMPtr<nsINode> anchorNode = aSelection->GetAnchorNode();
  int32_t anchorOffset = aSelection->GetAnchorOffset();
  nsCOMPtr<nsINode> focusNode = aSelection->GetFocusNode();
  int32_t focusOffset = aSelection->GetFocusOffset();

  
  
  NS_ASSERTION(aRoot->GetChildCount() <= 2, "Unexpected children");
  nsCOMPtr<nsIContent> firstChild = aRoot->GetFirstChild();
#ifdef DEBUG
  nsCOMPtr<nsIContent> lastChild = aRoot->GetLastChild();
  NS_ASSERTION(anchorNode == aRoot || anchorNode == firstChild ||
               anchorNode == lastChild, "Unexpected anchorNode");
  NS_ASSERTION(focusNode == aRoot || focusNode == firstChild ||
               focusNode == lastChild, "Unexpected focusNode");
#endif
  if (!firstChild || !firstChild->IsNodeOfType(nsINode::eTEXT)) {
    
    anchorOffset = focusOffset = 0;
  } else {
    
    
    
    
    if ((anchorNode == aRoot && anchorOffset != 0) ||
        (anchorNode != aRoot && anchorNode != firstChild)) {
      anchorOffset = firstChild->Length();
    }
    if ((focusNode == aRoot && focusOffset != 0) ||
        (focusNode != aRoot && focusNode != firstChild)) {
      focusOffset = firstChild->Length();
    }
  }

  
  aOutStartOffset = std::min(anchorOffset, focusOffset);
  aOutEndOffset = std::max(anchorOffset, focusOffset);
}

nsIEditor*
nsContentUtils::GetHTMLEditor(nsPresContext* aPresContext)
{
  nsCOMPtr<nsISupports> container = aPresContext->GetContainer();
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
  bool isEditable;
  if (!docShell ||
      NS_FAILED(docShell->GetEditable(&isEditable)) || !isEditable)
    return nullptr;

  nsCOMPtr<nsIEditor> editor;
  docShell->GetEditor(getter_AddRefs(editor));
  return editor;
}

bool
nsContentUtils::InternalIsSupported(nsISupports* aObject,
                                    const nsAString& aFeature,
                                    const nsAString& aVersion)
{
  
  if (StringBeginsWith(aFeature,
                       NS_LITERAL_STRING("http://www.w3.org/TR/SVG"),
                       nsASCIICaseInsensitiveStringComparator()) ||
      StringBeginsWith(aFeature, NS_LITERAL_STRING("org.w3c.dom.svg"),
                       nsASCIICaseInsensitiveStringComparator()) ||
      StringBeginsWith(aFeature, NS_LITERAL_STRING("org.w3c.svg"),
                       nsASCIICaseInsensitiveStringComparator())) {
    return (aVersion.IsEmpty() || aVersion.EqualsLiteral("1.0") ||
            aVersion.EqualsLiteral("1.1")) &&
           nsSVGFeatures::HasFeature(aObject, aFeature);
  }

  
  return true;
}

AutoJSContext::AutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
  : mCx(nullptr)
{
  Init(false MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT);
}

AutoJSContext::AutoJSContext(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mCx(nullptr)
{
  Init(aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT);
}

void
AutoJSContext::Init(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
  MOZ_ASSERT(!mCx, "mCx should not be initialized!");

  MOZ_GUARD_OBJECT_NOTIFIER_INIT;

  if (!aSafe) {
    mCx = nsContentUtils::GetCurrentJSContext();
  }

  if (!mCx) {
    mCx = nsContentUtils::GetSafeJSContext();
    bool result = mPusher.Push(mCx);
    if (!result || !mCx) {
      MOZ_CRASH();
    }
  }
}

AutoJSContext::operator JSContext*()
{
  return mCx;
}

SafeAutoJSContext::SafeAutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
  : AutoJSContext(true MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
{
}
