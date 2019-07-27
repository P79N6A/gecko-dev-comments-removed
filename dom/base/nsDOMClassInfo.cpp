





#include "mozilla/ArrayUtils.h"

#ifdef XP_WIN
#undef GetClassName
#endif


#include "jsapi.h"
#include "jsfriendapi.h"
#include "WrapperFactory.h"
#include "AccessCheck.h"
#include "XrayWrapper.h"

#include "xpcpublic.h"
#include "xpcprivate.h"
#include "XPCWrapper.h"

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/RegisterBindings.h"

#include "nscore.h"
#include "nsDOMClassInfo.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "nsICategoryManager.h"
#include "nsIComponentRegistrar.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIXPConnect.h"
#include "xptcall.h"
#include "nsTArray.h"


#include "nsGlobalWindow.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"
#include "nsContentUtils.h"
#include "nsIDOMGlobalPropertyInitializer.h"
#include "mozilla/Attributes.h"
#include "mozilla/Telemetry.h"


#include "nsScriptNameSpaceManager.h"


#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMConstructor.h"


#include "nsError.h"
#include "nsIDOMXULButtonElement.h"
#include "nsIDOMXULCheckboxElement.h"
#include "nsIDOMXULPopupElement.h"


#include "nsIDOMEventTarget.h"


#include "nsCSSRules.h"
#include "nsIDOMCSSRule.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"


#include "nsIDOMCSSCharsetRule.h"
#include "nsIDOMCSSImportRule.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSFontFaceRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsIDOMCSSSupportsRule.h"
#include "nsIDOMMozCSSKeyframeRule.h"
#include "nsIDOMMozCSSKeyframesRule.h"
#include "nsIDOMCSSCounterStyleRule.h"
#include "nsIDOMCSSPageRule.h"
#include "nsIDOMCSSStyleRule.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIControllers.h"
#ifdef MOZ_XUL
#include "nsITreeSelection.h"
#include "nsITreeContentView.h"
#include "nsITreeView.h"
#include "nsIXULTemplateBuilder.h"
#endif

#include "nsIEventListenerService.h"
#include "nsIMessageManager.h"

#include "mozilla/dom/TouchEvent.h"

#include "nsWrapperCacheInlines.h"
#include "mozilla/dom/HTMLCollectionBinding.h"

#include "nsIDOMMozSmsMessage.h"
#include "nsIDOMMozMmsMessage.h"
#include "nsIDOMMozMobileMessageThread.h"

#ifdef MOZ_B2G_FM
#include "FMRadio.h"
#endif

#include "nsDebug.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/Likely.h"
#include "nsIInterfaceInfoManager.h"

#ifdef MOZ_TIME_MANAGER
#include "TimeManager.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;

static NS_DEFINE_CID(kDOMSOF_CID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);




#define ARRAY_SCRIPTABLE_FLAGS                                                \
  (DOM_DEFAULT_SCRIPTABLE_FLAGS       |                                       \
   nsIXPCScriptable::WANT_GETPROPERTY |                                       \
   nsIXPCScriptable::WANT_ENUMERATE)

#define EVENTTARGET_SCRIPTABLE_FLAGS                                          \
  (DOM_DEFAULT_SCRIPTABLE_FLAGS       |                                       \
   nsIXPCScriptable::WANT_ADDPROPERTY)

#define DOMCLASSINFO_STANDARD_FLAGS                                           \
  (nsIClassInfo::MAIN_THREAD_ONLY |                                           \
   nsIClassInfo::DOM_OBJECT       |                                           \
   nsIClassInfo::SINGLETON_CLASSINFO)


#ifdef DEBUG
#define NS_DEFINE_CLASSINFO_DATA_DEBUG(_class)                                \
    eDOMClassInfo_##_class##_id,
#else
#define NS_DEFINE_CLASSINFO_DATA_DEBUG(_class)                                \
  // nothing
#endif

#define NS_DEFINE_CLASSINFO_DATA_HELPER(_class, _helper, _flags,              \
                                        _chromeOnly, _allowXBL)               \
  { #_class,                                                                  \
    nullptr,                                                                  \
    { _helper::doCreate },                                                    \
    nullptr,                                                                  \
    nullptr,                                                                  \
    nullptr,                                                                  \
    _flags,                                                                   \
    true,                                                                     \
    _chromeOnly,                                                              \
    _allowXBL,                                                                \
    false,                                                                    \
    NS_DEFINE_CLASSINFO_DATA_DEBUG(_class)                                    \
  },

#define NS_DEFINE_CLASSINFO_DATA(_class, _helper, _flags)                     \
  NS_DEFINE_CLASSINFO_DATA_HELPER(_class, _helper, _flags, false, false)

#define NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(_class, _helper, _flags)         \
  NS_DEFINE_CLASSINFO_DATA_HELPER(_class, _helper, _flags, true, false)

#define NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(_class, _helper, _flags)          \
  NS_DEFINE_CLASSINFO_DATA_HELPER(_class, _helper, _flags, true, true)












static nsDOMClassInfoData sClassInfoData[] = {
  

  NS_DEFINE_CLASSINFO_DATA(DOMPrototype, nsDOMConstructorSH,
                           DOM_BASE_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_PRECREATE |
                           nsIXPCScriptable::WANT_RESOLVE |
                           nsIXPCScriptable::WANT_HASINSTANCE |
                           nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE)
  NS_DEFINE_CLASSINFO_DATA(DOMConstructor, nsDOMConstructorSH,
                           DOM_BASE_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_PRECREATE |
                           nsIXPCScriptable::WANT_RESOLVE |
                           nsIXPCScriptable::WANT_HASINSTANCE |
                           nsIXPCScriptable::WANT_CALL |
                           nsIXPCScriptable::WANT_CONSTRUCT |
                           nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE)

  

  
  NS_DEFINE_CLASSINFO_DATA(CSSStyleRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSCharsetRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSImportRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSMediaRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSNameSpaceRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
#ifdef MOZ_XUL
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULCommandDispatcher, nsDOMGenericSH,
                                      DOM_DEFAULT_SCRIPTABLE_FLAGS)
#endif
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULControllers, nsNonDOMObjectSH,
                                      DEFAULT_SCRIPTABLE_FLAGS)
#ifdef MOZ_XUL
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(TreeSelection, nsDOMGenericSH,
                                      DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(TreeContentView, nsDOMGenericSH,
                                      DEFAULT_SCRIPTABLE_FLAGS)
#endif

#ifdef MOZ_XUL
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULTemplateBuilder, nsDOMGenericSH,
                                      DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULTreeBuilder, nsDOMGenericSH,
                                      DEFAULT_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(CSSMozDocumentRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSSupportsRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozSmsMessage, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozMmsMessage, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozMobileMessageThread, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSFontFaceRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(ContentFrameMessageManager,
                                       nsMessageManagerSH<nsEventTargetSH>,
                                       DOM_DEFAULT_SCRIPTABLE_FLAGS |
                                       nsIXPCScriptable::WANT_ENUMERATE |
                                       nsIXPCScriptable::IS_GLOBAL_OBJECT)
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(ContentProcessMessageManager,
                                       nsMessageManagerSH<nsDOMGenericSH>,
                                       DOM_DEFAULT_SCRIPTABLE_FLAGS |
                                       nsIXPCScriptable::WANT_ENUMERATE |
                                       nsIXPCScriptable::IS_GLOBAL_OBJECT)
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(ChromeMessageBroadcaster, nsDOMGenericSH,
                                       DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(ChromeMessageSender, nsDOMGenericSH,
                                       DOM_DEFAULT_SCRIPTABLE_FLAGS)


  NS_DEFINE_CLASSINFO_DATA(MozCSSKeyframeRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MozCSSKeyframesRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSCounterStyleRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSPageRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSFontFeatureValuesRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULControlElement, nsDOMGenericSH,
                                      DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULLabeledControlElement, nsDOMGenericSH,
                                      DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULButtonElement, nsDOMGenericSH,
                                      DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULCheckboxElement, nsDOMGenericSH,
                                      DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_XBL_CLASSINFO_DATA(XULPopupElement, nsDOMGenericSH,
                                      DOM_DEFAULT_SCRIPTABLE_FLAGS)
};

nsIXPConnect *nsDOMClassInfo::sXPConnect = nullptr;
bool nsDOMClassInfo::sIsInitialized = false;


jsid nsDOMClassInfo::sConstructor_id     = JSID_VOID;
jsid nsDOMClassInfo::sWrappedJSObject_id = JSID_VOID;

static const JSClass *sObjectClass = nullptr;




static void
FindObjectClass(JSContext* cx, JSObject* aGlobalObject)
{
  NS_ASSERTION(!sObjectClass,
               "Double set of sObjectClass");
  JS::Rooted<JSObject*> obj(cx), proto(cx, aGlobalObject);
  do {
    obj = proto;
    js::GetObjectProto(cx, obj, &proto);
  } while (proto);

  sObjectClass = js::GetObjectJSClass(obj);
}


static inline nsresult
SetParentToWindow(nsGlobalWindow *win, JSObject **parent)
{
  MOZ_ASSERT(win);
  MOZ_ASSERT(win->IsInnerWindow());
  *parent = win->FastGetGlobalJSObject();

  if (MOZ_UNLIKELY(!*parent)) {
    
    
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}



nsISupports *
nsDOMClassInfo::GetNative(nsIXPConnectWrappedNative *wrapper, JSObject *obj)
{
  return wrapper ? wrapper->Native() : static_cast<nsISupports*>(js::GetObjectPrivate(obj));
}

nsresult
nsDOMClassInfo::DefineStaticJSVals(JSContext *cx)
{
#define SET_JSID_TO_STRING(_id, _cx, _str)                                    \
  if (JSString *str = ::JS_InternString(_cx, _str))                           \
      _id = INTERNED_STRING_TO_JSID(_cx, str);                                \
  else                                                                        \
      return NS_ERROR_OUT_OF_MEMORY;

  SET_JSID_TO_STRING(sConstructor_id,     cx, "constructor");
  SET_JSID_TO_STRING(sWrappedJSObject_id, cx, "wrappedJSObject");

  return NS_OK;
}


bool
nsDOMClassInfo::ObjectIsNativeWrapper(JSContext* cx, JSObject* obj)
{
  return xpc::WrapperFactory::IsXrayWrapper(obj) &&
         xpc::AccessCheck::wrapperSubsumes(obj);
}

nsDOMClassInfo::nsDOMClassInfo(nsDOMClassInfoData* aData) : mData(aData)
{
}

nsDOMClassInfo::~nsDOMClassInfo()
{
  if (IS_EXTERNAL(mData->mCachedClassInfo)) {
    
    nsDOMClassInfoData* data = const_cast<nsDOMClassInfoData*>(mData);
    delete static_cast<nsExternalDOMClassInfoData*>(data);
  }
}

NS_IMPL_ADDREF(nsDOMClassInfo)
NS_IMPL_RELEASE(nsDOMClassInfo)

NS_INTERFACE_MAP_BEGIN(nsDOMClassInfo)
  if (aIID.Equals(NS_GET_IID(nsXPCClassInfo)))
    foundInterface = static_cast<nsIClassInfo*>(
                                    static_cast<nsXPCClassInfo*>(this));
  else
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIClassInfo)
NS_INTERFACE_MAP_END


static const JSClass sDOMConstructorProtoClass = {
  "DOM Constructor.prototype", 0
};


static const char *
CutPrefix(const char *aName) {
  static const char prefix_nsIDOM[] = "nsIDOM";
  static const char prefix_nsI[]    = "nsI";

  if (strncmp(aName, prefix_nsIDOM, sizeof(prefix_nsIDOM) - 1) == 0) {
    return aName + sizeof(prefix_nsIDOM) - 1;
  }

  if (strncmp(aName, prefix_nsI, sizeof(prefix_nsI) - 1) == 0) {
    return aName + sizeof(prefix_nsI) - 1;
  }

  return aName;
}


nsresult
nsDOMClassInfo::RegisterClassProtos(int32_t aClassInfoID)
{
  nsScriptNameSpaceManager *nameSpaceManager = GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);
  bool found_old;

  const nsIID *primary_iid = sClassInfoData[aClassInfoID].mProtoChainInterface;

  if (!primary_iid || primary_iid == &NS_GET_IID(nsISupports)) {
    return NS_OK;
  }

  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
  NS_ENSURE_TRUE(iim, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIInterfaceInfo> if_info;
  bool first = true;

  iim->GetInfoForIID(primary_iid, getter_AddRefs(if_info));

  while (if_info) {
    const nsIID *iid = nullptr;

    if_info->GetIIDShared(&iid);
    NS_ENSURE_TRUE(iid, NS_ERROR_UNEXPECTED);

    if (iid->Equals(NS_GET_IID(nsISupports))) {
      break;
    }

    const char *name = nullptr;
    if_info->GetNameShared(&name);
    NS_ENSURE_TRUE(name, NS_ERROR_UNEXPECTED);

    nameSpaceManager->RegisterClassProto(CutPrefix(name), iid, &found_old);

    if (first) {
      first = false;
    } else if (found_old) {
      break;
    }

    nsCOMPtr<nsIInterfaceInfo> tmp(if_info);
    tmp->GetParent(getter_AddRefs(if_info));
  }

  return NS_OK;
}


nsresult
nsDOMClassInfo::RegisterExternalClasses()
{
  nsScriptNameSpaceManager *nameSpaceManager = GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIComponentRegistrar> registrar;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsICategoryManager> cm =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> e;
  rv = cm->EnumerateCategory(JAVASCRIPT_DOM_CLASS, getter_AddRefs(e));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString contractId;
  nsAutoCString categoryEntry;
  nsCOMPtr<nsISupports> entry;

  while (NS_SUCCEEDED(e->GetNext(getter_AddRefs(entry)))) {
    nsCOMPtr<nsISupportsCString> category(do_QueryInterface(entry));

    if (!category) {
      NS_WARNING("Category entry not an nsISupportsCString!");
      continue;
    }

    rv = category->GetData(categoryEntry);

    cm->GetCategoryEntry(JAVASCRIPT_DOM_CLASS, categoryEntry.get(),
                         getter_Copies(contractId));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCID *cid;
    rv = registrar->ContractIDToCID(contractId, &cid);
    if (NS_FAILED(rv)) {
      NS_WARNING("Bad contract id registered with the script namespace manager");
      continue;
    }

    rv = nameSpaceManager->RegisterExternalClassName(categoryEntry.get(), *cid);
    free(cid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return nameSpaceManager->RegisterExternalInterfaces(true);
}

#define _DOM_CLASSINFO_MAP_BEGIN(_class, _ifptr, _has_class_if)               \
  {                                                                           \
    nsDOMClassInfoData &d = sClassInfoData[eDOMClassInfo_##_class##_id];      \
    d.mProtoChainInterface = _ifptr;                                          \
    d.mHasClassInterface = _has_class_if;                                     \
    static const nsIID *interface_list[] = {

#define DOM_CLASSINFO_MAP_BEGIN(_class, _interface)                           \
  _DOM_CLASSINFO_MAP_BEGIN(_class, &NS_GET_IID(_interface), true)

#define DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(_class, _interface)               \
  _DOM_CLASSINFO_MAP_BEGIN(_class, &NS_GET_IID(_interface), false)

#define DOM_CLASSINFO_MAP_ENTRY(_if)                                          \
      &NS_GET_IID(_if),

#define DOM_CLASSINFO_MAP_CONDITIONAL_ENTRY(_if, _cond)                       \
      (_cond) ? &NS_GET_IID(_if) : nullptr,

#define DOM_CLASSINFO_MAP_END                                                 \
      nullptr                                                                  \
    };                                                                        \
                                                                              \
    /* Compact the interface list */                                          \
    size_t count = ArrayLength(interface_list);                               \
    /* count is the number of array entries, which is one greater than the */ \
    /* number of interfaces due to the terminating null */                    \
    for (size_t i = 0; i < count - 1; ++i) {                                  \
      if (!interface_list[i]) {                                               \
        /* We are moving the element at index i+1 and successors, */          \
        /* so we must move only count - (i+1) elements total. */              \
        memmove(&interface_list[i], &interface_list[i+1],                     \
                sizeof(nsIID*) * (count - (i+1)));                            \
        /* Make sure to examine the new pointer we ended up with at this */   \
        /* slot, since it may be null too */                                  \
        --i;                                                                  \
        --count;                                                              \
      }                                                                       \
    }                                                                         \
                                                                              \
    d.mInterfaces = interface_list;                                           \
  }

nsresult
nsDOMClassInfo::Init()
{
  

  static_assert(sizeof(uintptr_t) == sizeof(void*),
                "BAD! You'll need to adjust the size of uintptr_t to the "
                "size of a pointer on your platform.");

  NS_ENSURE_TRUE(!sIsInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsScriptNameSpaceManager *nameSpaceManager = GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  NS_ADDREF(sXPConnect = nsContentUtils::XPConnect());

  nsCOMPtr<nsIXPCFunctionThisTranslator> elt = new nsEventListenerThisTranslator();
  sXPConnect->SetFunctionThisTranslator(NS_GET_IID(nsIDOMEventListener), elt);

  AutoSafeJSContext cx;

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(DOMPrototype, nsIDOMDOMConstructor)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMConstructor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DOMConstructor, nsIDOMDOMConstructor)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMConstructor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSStyleRule, nsIDOMCSSStyleRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSStyleRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSCharsetRule, nsIDOMCSSCharsetRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSCharsetRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSImportRule, nsIDOMCSSImportRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSImportRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSMediaRule, nsIDOMCSSMediaRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSMediaRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(CSSNameSpaceRule, nsIDOMCSSRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSRule)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(XULCommandDispatcher, nsIDOMXULCommandDispatcher)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULCommandDispatcher)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XULControllers, nsIControllers)
    DOM_CLASSINFO_MAP_ENTRY(nsIControllers)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(TreeSelection, nsITreeSelection)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeSelection)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(TreeContentView, nsITreeContentView)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeContentView)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeView)
  DOM_CLASSINFO_MAP_END
#endif

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(XULTemplateBuilder, nsIXULTemplateBuilder)
    DOM_CLASSINFO_MAP_ENTRY(nsIXULTemplateBuilder)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XULTreeBuilder, nsIXULTreeBuilder)
    DOM_CLASSINFO_MAP_ENTRY(nsIXULTreeBuilder)
    DOM_CLASSINFO_MAP_ENTRY(nsIXULTemplateBuilder)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeView)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(CSSMozDocumentRule, nsIDOMCSSMozDocumentRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSMozDocumentRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSSupportsRule, nsIDOMCSSSupportsRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSSupportsRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozSmsMessage, nsIDOMMozSmsMessage)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozSmsMessage)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozMmsMessage, nsIDOMMozMmsMessage)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozMmsMessage)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozMobileMessageThread, nsIDOMMozMobileMessageThread)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozMobileMessageThread)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSFontFaceRule, nsIDOMCSSFontFaceRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSFontFaceRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ContentFrameMessageManager, nsISupports)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageListenerManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageSender)
    DOM_CLASSINFO_MAP_ENTRY(nsISyncMessageSender)
    DOM_CLASSINFO_MAP_ENTRY(nsIContentFrameMessageManager)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ContentProcessMessageManager, nsISupports)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageListenerManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageSender)
    DOM_CLASSINFO_MAP_ENTRY(nsISyncMessageSender)
    DOM_CLASSINFO_MAP_ENTRY(nsIContentProcessMessageManager)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ChromeMessageBroadcaster, nsISupports)
    DOM_CLASSINFO_MAP_ENTRY(nsIFrameScriptLoader)
    DOM_CLASSINFO_MAP_ENTRY(nsIProcessScriptLoader)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageListenerManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageBroadcaster)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ChromeMessageSender, nsISupports)
    DOM_CLASSINFO_MAP_ENTRY(nsIProcessChecker)
    DOM_CLASSINFO_MAP_ENTRY(nsIFrameScriptLoader)
    DOM_CLASSINFO_MAP_ENTRY(nsIProcessScriptLoader)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageListenerManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageSender)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozCSSKeyframeRule, nsIDOMMozCSSKeyframeRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozCSSKeyframeRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozCSSKeyframesRule, nsIDOMMozCSSKeyframesRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozCSSKeyframesRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSCounterStyleRule, nsIDOMCSSCounterStyleRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSCounterStyleRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSPageRule, nsIDOMCSSPageRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSPageRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSFontFeatureValuesRule, nsIDOMCSSFontFeatureValuesRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSFontFeatureValuesRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XULControlElement, nsIDOMXULControlElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULControlElement)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XULLabeledControlElement, nsIDOMXULLabeledControlElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULLabeledControlElement)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XULButtonElement, nsIDOMXULButtonElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULButtonElement)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XULCheckboxElement, nsIDOMXULCheckboxElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULCheckboxElement)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XULPopupElement, nsIDOMXULPopupElement)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULPopupElement)
  DOM_CLASSINFO_MAP_END

  static_assert(MOZ_ARRAY_LENGTH(sClassInfoData) == eDOMClassInfoIDCount,
                "The number of items in sClassInfoData doesn't match the "
                "number of nsIDOMClassInfo ID's, this is bad! Fix it!");

#ifdef DEBUG
  for (size_t i = 0; i < eDOMClassInfoIDCount; i++) {
    if (!sClassInfoData[i].u.mConstructorFptr ||
        sClassInfoData[i].mDebugID != i) {
      MOZ_CRASH("Class info data out of sync, you forgot to update "
                "nsDOMClassInfo.h and nsDOMClassInfo.cpp! Fix this, "
                "mozilla will not work without this fixed!");
    }
  }

  for (size_t i = 0; i < eDOMClassInfoIDCount; i++) {
    if (!sClassInfoData[i].mInterfaces) {
      MOZ_CRASH("Class info data without an interface list! Fix this, "
                "mozilla will not work without this fixed!");
     }
   }
#endif

  
  DefineStaticJSVals(cx);

  int32_t i;

  for (i = 0; i < eDOMClassInfoIDCount; ++i) {
    if (i == eDOMClassInfo_DOMPrototype_id) {
      continue;
    }

    nsDOMClassInfoData& data = sClassInfoData[i];
    nameSpaceManager->RegisterClassName(data.mName, i, data.mChromeOnly,
                                        data.mAllowXBL, &data.mNameUTF16);
  }

  for (i = 0; i < eDOMClassInfoIDCount; ++i) {
    RegisterClassProtos(i);
  }

  RegisterExternalClasses();

  sIsInitialized = true;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetInterfaces(uint32_t *aCount, nsIID ***aArray)
{
  uint32_t count = 0;

  while (mData->mInterfaces[count]) {
    count++;
  }

  *aCount = count;

  if (!count) {
    *aArray = nullptr;

    return NS_OK;
  }

  *aArray = static_cast<nsIID **>(moz_xmalloc(count * sizeof(nsIID *)));
  NS_ENSURE_TRUE(*aArray, NS_ERROR_OUT_OF_MEMORY);

  uint32_t i;
  for (i = 0; i < count; i++) {
    nsIID *iid = static_cast<nsIID *>(nsMemory::Clone(mData->mInterfaces[i],
                                                         sizeof(nsIID)));

    if (!iid) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(i, *aArray);

      return NS_ERROR_OUT_OF_MEMORY;
    }

    *((*aArray) + i) = iid;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetScriptableHelper(nsIXPCScriptable **_retval)
{
  nsCOMPtr<nsIXPCScriptable> rval = this;
  rval.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetContractID(char **aContractID)
{
  *aContractID = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetClassDescription(char **aClassDescription)
{
  return GetClassName(aClassDescription);
}

NS_IMETHODIMP
nsDOMClassInfo::GetClassID(nsCID **aClassID)
{
  *aClassID = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::GetClassIDNoAlloc(nsCID *aClassID)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsDOMClassInfo::GetFlags(uint32_t *aFlags)
{
  *aFlags = DOMCLASSINFO_STANDARD_FLAGS;

  return NS_OK;
}



NS_IMETHODIMP
nsDOMClassInfo::GetClassName(char **aClassName)
{
  *aClassName = NS_strdup(mData->mName);

  return NS_OK;
}


uint32_t
nsDOMClassInfo::GetScriptableFlags()
{
  return mData->mScriptableFlags;
}

NS_IMETHODIMP
nsDOMClassInfo::PreCreate(nsISupports *nativeObj, JSContext *cx,
                          JSObject *globalObj, JSObject **parentObj)
{
  *parentObj = globalObj;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsid id, JS::Handle<JS::Value> val,
                            bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::AddProperty Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsid id, jsval *vp,
                            bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::GetProperty Don't call me!");

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsid id, jsval *vp,
                            bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::SetProperty Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, bool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, JS::AutoIdVector &properties,
                             bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::NewEnumerate Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Resolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *aObj, jsid aId, bool *resolvedp, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  JS::Rooted<jsid> id(cx, aId);

  if (id != sConstructor_id) {
    *resolvedp = false;
    return NS_OK;
  }

  JS::Rooted<JSObject*> global(cx, ::JS_GetGlobalForObject(cx, obj));

  JS::Rooted<JSPropertyDescriptor> desc(cx);
  if (!JS_GetPropertyDescriptor(cx, global, mData->mName, &desc)) {
    return NS_ERROR_UNEXPECTED;
  }

  if (desc.object() && !desc.hasGetterOrSetter() && desc.value().isObject()) {
    
    
    
    
    if (!::JS_DefinePropertyById(cx, obj, id, desc.value(),
                                 JSPROP_ENUMERATE,
                                 JS_STUBGETTER, JS_STUBSETTER)) {
      return NS_ERROR_UNEXPECTED;
    }

    *resolvedp = true;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::Finalize(nsIXPConnectWrappedNative *wrapper, JSFreeOp *fop,
                         JSObject *obj)
{
  NS_WARNING("nsDOMClassInfo::Finalize Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                     JSObject *obj, const JS::CallArgs &args, bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::Call Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, const JS::CallArgs &args,
                          bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::Construct Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, JS::Handle<JS::Value> val, bool *bp,
                            bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::HasInstance Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

static nsresult
GetExternalClassInfo(nsScriptNameSpaceManager *aNameSpaceManager,
                     const nsAString &aName,
                     const nsGlobalNameStruct *aStruct,
                     const nsGlobalNameStruct **aResult)
{
  NS_ASSERTION(aStruct->mType ==
                 nsGlobalNameStruct::eTypeExternalClassInfoCreator,
               "Wrong type!");

  nsresult rv;
  nsCOMPtr<nsIDOMCIExtension> creator(do_CreateInstance(aStruct->mCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMScriptObjectFactory> sof(do_GetService(kDOMSOF_CID));
  NS_ENSURE_TRUE(sof, NS_ERROR_FAILURE);

  rv = creator->RegisterDOMCI(NS_ConvertUTF16toUTF8(aName).get(), sof);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsGlobalNameStruct *name_struct = aNameSpaceManager->LookupName(aName);
  if (name_struct &&
      name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    *aResult = name_struct;
  }
  else {
    NS_ERROR("Couldn't get the DOM ClassInfo data.");

    *aResult = nullptr;
  }

  return NS_OK;
}


static nsresult
ResolvePrototype(nsIXPConnect *aXPConnect, nsGlobalWindow *aWin, JSContext *cx,
                 JS::Handle<JSObject*> obj, const char16_t *name,
                 const nsDOMClassInfoData *ci_data,
                 const nsGlobalNameStruct *name_struct,
                 nsScriptNameSpaceManager *nameSpaceManager,
                 JSObject *dot_prototype,
                 JS::MutableHandle<JSPropertyDescriptor> ctorDesc);

NS_IMETHODIMP
nsDOMClassInfo::PostCreatePrototype(JSContext * cx, JSObject * aProto)
{
  JS::Rooted<JSObject*> proto(cx, aProto);

  
  
  
  if (!sObjectClass) {
    FindObjectClass(cx, proto);
    NS_ASSERTION(sObjectClass && !strcmp(sObjectClass->name, "Object"),
                 "Incorrect object class!");
  }

#ifdef DEBUG
    JS::Rooted<JSObject*> proto2(cx);
    JS_GetPrototype(cx, proto, &proto2);
    NS_ASSERTION(proto2 && JS_GetClass(proto2) == sObjectClass,
                 "Hmm, somebody did something evil?");
#endif

#ifdef DEBUG
  if (mData->mHasClassInterface && mData->mProtoChainInterface &&
      mData->mProtoChainInterface != &NS_GET_IID(nsISupports)) {
    nsCOMPtr<nsIInterfaceInfoManager>
      iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));

    if (iim) {
      nsCOMPtr<nsIInterfaceInfo> if_info;
      iim->GetInfoForIID(mData->mProtoChainInterface,
                         getter_AddRefs(if_info));

      if (if_info) {
        nsXPIDLCString name;
        if_info->GetName(getter_Copies(name));
        NS_ASSERTION(nsCRT::strcmp(CutPrefix(name), mData->mName) == 0,
                     "Class name and proto chain interface name mismatch!");
      }
    }
  }
#endif

  
  
  
  
  JS::Rooted<JSObject*> global(cx, ::JS_GetGlobalForObject(cx, proto));

  
  
  nsISupports *globalNative = XPConnect()->GetNativeOfWrapper(cx, global);
  nsCOMPtr<nsPIDOMWindow> piwin = do_QueryInterface(globalNative);
  if (!piwin) {
    return NS_OK;
  }

  nsGlobalWindow *win = nsGlobalWindow::FromSupports(globalNative);
  if (win->IsClosedOrClosing()) {
    return NS_OK;
  }

  
  
  
  if (win->FastGetGlobalJSObject() &&
      js::GetObjectCompartment(global) != js::GetObjectCompartment(win->FastGetGlobalJSObject())) {
    return NS_OK;
  }

  if (win->IsOuterWindow()) {
    
    

    win = win->GetCurrentInnerWindowInternal();

    if (!win || !(global = win->GetGlobalJSObject()) ||
        win->IsClosedOrClosing()) {
      return NS_OK;
    }
  }

  
  bool contentDefinedProperty;
  if (!::JS_AlreadyHasOwnUCProperty(cx, global, reinterpret_cast<const char16_t*>(mData->mNameUTF16),
                                    NS_strlen(mData->mNameUTF16),
                                    &contentDefinedProperty)) {
    return NS_ERROR_FAILURE;
  }

  nsScriptNameSpaceManager *nameSpaceManager = GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_OK);

  JS::Rooted<JSPropertyDescriptor> desc(cx);
  nsresult rv = ResolvePrototype(sXPConnect, win, cx, global, mData->mNameUTF16,
                                 mData, nullptr, nameSpaceManager, proto,
                                 &desc);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!contentDefinedProperty && desc.object() && !desc.value().isUndefined() &&
      !JS_DefineUCProperty(cx, global, mData->mNameUTF16,
                           NS_strlen(mData->mNameUTF16), desc)) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


nsIClassInfo *
NS_GetDOMClassInfoInstance(nsDOMClassInfoID aID)
{
  if (aID >= eDOMClassInfoIDCount) {
    NS_ERROR("Bad ID!");

    return nullptr;
  }

  nsresult rv = RegisterDOMNames();
  NS_ENSURE_SUCCESS(rv, nullptr);

  if (!sClassInfoData[aID].mCachedClassInfo) {
    nsDOMClassInfoData& data = sClassInfoData[aID];

    data.mCachedClassInfo = data.u.mConstructorFptr(&data);
    NS_ENSURE_TRUE(data.mCachedClassInfo, nullptr);

    NS_ADDREF(data.mCachedClassInfo);
  }

  NS_ASSERTION(!IS_EXTERNAL(sClassInfoData[aID].mCachedClassInfo),
               "This is bad, internal class marked as external!");

  return sClassInfoData[aID].mCachedClassInfo;
}


nsIClassInfo *
nsDOMClassInfo::GetClassInfoInstance(nsDOMClassInfoData* aData)
{
  NS_ASSERTION(IS_EXTERNAL(aData->mCachedClassInfo)
               || !aData->mCachedClassInfo,
               "This is bad, external class marked as internal!");

  if (!aData->mCachedClassInfo) {
    if (aData->u.mExternalConstructorFptr) {
      aData->mCachedClassInfo =
        aData->u.mExternalConstructorFptr(aData->mName);
    } else {
      aData->mCachedClassInfo = nsDOMGenericSH::doCreate(aData);
    }
    NS_ENSURE_TRUE(aData->mCachedClassInfo, nullptr);

    NS_ADDREF(aData->mCachedClassInfo);
    aData->mCachedClassInfo = MARK_EXTERNAL(aData->mCachedClassInfo);
  }

  return GET_CLEAN_CI_PTR(aData->mCachedClassInfo);
}



void
nsDOMClassInfo::ShutDown()
{
  if (sClassInfoData[0].u.mConstructorFptr) {
    uint32_t i;

    for (i = 0; i < eDOMClassInfoIDCount; i++) {
      NS_IF_RELEASE(sClassInfoData[i].mCachedClassInfo);
    }
  }

  sConstructor_id     = JSID_VOID;
  sWrappedJSObject_id = JSID_VOID;

  NS_IF_RELEASE(sXPConnect);
  sIsInitialized = false;
}

static nsresult
BaseStubConstructor(nsIWeakReference* aWeakOwner,
                    const nsGlobalNameStruct *name_struct, JSContext *cx,
                    JS::Handle<JSObject*> obj, const JS::CallArgs &args)
{
  MOZ_ASSERT(obj);
  MOZ_ASSERT(cx == nsContentUtils::GetCurrentJSContext());

  nsresult rv;
  nsCOMPtr<nsISupports> native;
  if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    rv = NS_ERROR_NOT_AVAILABLE;
  } else if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructor) {
    native = do_CreateInstance(name_struct->mCID, &rv);
  } else if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    native = do_CreateInstance(name_struct->mAlias->mCID, &rv);
  } else {
    native = do_CreateInstance(*name_struct->mData->mConstructorCID, &rv);
  }
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to create the object");
    return rv;
  }

  js::AssertSameCompartment(cx, obj);
  return nsContentUtils::WrapNative(cx, native, args.rval(), true);
}

static nsresult
DefineInterfaceConstants(JSContext *cx, JS::Handle<JSObject*> obj, const nsIID *aIID)
{
  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
  NS_ENSURE_TRUE(iim, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIInterfaceInfo> if_info;

  nsresult rv = iim->GetInfoForIID(aIID, getter_AddRefs(if_info));
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && if_info, rv);

  uint16_t constant_count;

  if_info->GetConstantCount(&constant_count);

  if (!constant_count) {
    return NS_OK;
  }

  nsCOMPtr<nsIInterfaceInfo> parent_if_info;

  rv = if_info->GetParent(getter_AddRefs(parent_if_info));
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && parent_if_info, rv);

  uint16_t parent_constant_count, i;
  parent_if_info->GetConstantCount(&parent_constant_count);

  JS::Rooted<JS::Value> v(cx);
  for (i = parent_constant_count; i < constant_count; i++) {
    nsXPIDLCString name;
    rv = if_info->GetConstant(i, &v, getter_Copies(name));
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv), rv);

    if (!::JS_DefineProperty(cx, obj, name, v,
                             JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT,
                             JS_STUBGETTER, JS_STUBSETTER)) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
}

class nsDOMConstructor final : public nsIDOMDOMConstructor
{
protected:
  nsDOMConstructor(const char16_t* aName,
                   bool aIsConstructable,
                   nsPIDOMWindow* aOwner)
    : mClassName(aName),
      mConstructable(aIsConstructable),
      mWeakOwner(do_GetWeakReference(aOwner))
  {
  }

  ~nsDOMConstructor() {}

public:

  static nsresult Create(const char16_t* aName,
                         const nsDOMClassInfoData* aData,
                         const nsGlobalNameStruct* aNameStruct,
                         nsPIDOMWindow* aOwner,
                         nsDOMConstructor** aResult);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMCONSTRUCTOR

  nsresult PreCreate(JSContext *cx, JSObject *globalObj, JSObject **parentObj);

  nsresult Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                     JS::Handle<JSObject*> obj, const JS::CallArgs &args,
                     bool *_retval);

  nsresult HasInstance(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JS::Handle<JSObject*> obj, const jsval &val, bool *bp,
                       bool *_retval);

  nsresult ResolveInterfaceConstants(JSContext *cx, JS::Handle<JSObject*> obj);

private:
  const nsGlobalNameStruct *GetNameStruct()
  {
    if (!mClassName) {
      NS_ERROR("Can't get name");
      return nullptr;
    }

    const nsGlobalNameStruct *nameStruct;
#ifdef DEBUG
    nsresult rv =
#endif
      GetNameStruct(nsDependentString(mClassName), &nameStruct);

    NS_ASSERTION(NS_FAILED(rv) || nameStruct, "Name isn't in hash.");

    return nameStruct;
  }

  static nsresult GetNameStruct(const nsAString& aName,
                                const nsGlobalNameStruct **aNameStruct)
  {
    *aNameStruct = nullptr;

    nsScriptNameSpaceManager *nameSpaceManager = GetNameSpaceManager();
    if (!nameSpaceManager) {
      NS_ERROR("Can't get namespace manager.");
      return NS_ERROR_UNEXPECTED;
    }

    *aNameStruct = nameSpaceManager->LookupName(aName);

    
    return NS_OK;
  }

  static bool IsConstructable(const nsDOMClassInfoData *aData)
  {
    if (IS_EXTERNAL(aData->mCachedClassInfo)) {
      const nsExternalDOMClassInfoData* data =
        static_cast<const nsExternalDOMClassInfoData*>(aData);
      return data->mConstructorCID != nullptr;
    }

    return false;
  }
  static bool IsConstructable(const nsGlobalNameStruct *aNameStruct)
  {
    return
      (aNameStruct->mType == nsGlobalNameStruct::eTypeClassConstructor &&
       IsConstructable(&sClassInfoData[aNameStruct->mDOMClassInfoID])) ||
      (aNameStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo &&
       IsConstructable(aNameStruct->mData)) ||
      aNameStruct->mType == nsGlobalNameStruct::eTypeExternalConstructor ||
      aNameStruct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias;
  }

  const char16_t*   mClassName;
  const bool mConstructable;
  nsWeakPtr          mWeakOwner;
};


nsresult
nsDOMConstructor::Create(const char16_t* aName,
                         const nsDOMClassInfoData* aData,
                         const nsGlobalNameStruct* aNameStruct,
                         nsPIDOMWindow* aOwner,
                         nsDOMConstructor** aResult)
{
  *aResult = nullptr;
  
  
  
  
  
  nsPIDOMWindow* outerWindow = aOwner->GetOuterWindow();
  nsPIDOMWindow* currentInner =
    outerWindow ? outerWindow->GetCurrentInnerWindow() : aOwner;
  if (!currentInner ||
      (aOwner != currentInner &&
       !nsContentUtils::CanCallerAccess(currentInner) &&
       !(currentInner = aOwner)->IsInnerWindow())) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  bool constructable = aNameStruct ?
                         IsConstructable(aNameStruct) :
                         IsConstructable(aData);

  *aResult = new nsDOMConstructor(aName, constructable, currentInner);
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMPL_ADDREF(nsDOMConstructor)
NS_IMPL_RELEASE(nsDOMConstructor)
NS_INTERFACE_MAP_BEGIN(nsDOMConstructor)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMConstructor)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
#ifdef DEBUG
    {
      const nsGlobalNameStruct *name_struct = GetNameStruct();
      NS_ASSERTION(!name_struct ||
                   mConstructable == IsConstructable(name_struct),
                   "Can't change constructability dynamically!");
    }
#endif
    foundInterface =
      NS_GetDOMClassInfoInstance(mConstructable ?
                                 eDOMClassInfo_DOMConstructor_id :
                                 eDOMClassInfo_DOMPrototype_id);
    if (!foundInterface) {
      *aInstancePtr = nullptr;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  } else
NS_INTERFACE_MAP_END

nsresult
nsDOMConstructor::PreCreate(JSContext *cx, JSObject *globalObj, JSObject **parentObj)
{
  nsCOMPtr<nsPIDOMWindow> owner(do_QueryReferent(mWeakOwner));
  if (!owner) {
    
    return NS_OK;
  }

  nsGlobalWindow *win = static_cast<nsGlobalWindow *>(owner.get());
  return SetParentToWindow(win, parentObj);
}

nsresult
nsDOMConstructor::Construct(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                            JS::Handle<JSObject*> obj, const JS::CallArgs &args,
                            bool *_retval)
{
  MOZ_ASSERT(obj);

  const nsGlobalNameStruct *name_struct = GetNameStruct();
  NS_ENSURE_TRUE(name_struct, NS_ERROR_FAILURE);

  if (!IsConstructable(name_struct)) {
    
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  return BaseStubConstructor(mWeakOwner, name_struct, cx, obj, args);
}

nsresult
nsDOMConstructor::HasInstance(nsIXPConnectWrappedNative *wrapper,
                              JSContext * cx, JS::Handle<JSObject*> obj,
                              const jsval &v, bool *bp, bool *_retval)

{
  
  *bp = false;
  if (v.isPrimitive()) {
    return NS_OK;
  }

  JS::Rooted<JSObject*> dom_obj(cx, v.toObjectOrNull());
  NS_ASSERTION(dom_obj, "nsDOMConstructor::HasInstance couldn't get object");

  
  JSObject *wrapped_obj = js::CheckedUnwrap(dom_obj,  false);
  if (wrapped_obj)
      dom_obj = wrapped_obj;

  const JSClass *dom_class = JS_GetClass(dom_obj);
  if (!dom_class) {
    NS_ERROR("nsDOMConstructor::HasInstance can't get class.");
    return NS_ERROR_UNEXPECTED;
  }

  const nsGlobalNameStruct *name_struct;
  nsresult rv = GetNameStruct(NS_ConvertASCIItoUTF16(dom_class->name), &name_struct);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!name_struct) {
    
    
    JS::Rooted<JSPropertyDescriptor> desc(cx);
    if (!JS_GetPropertyDescriptor(cx, obj, "prototype", &desc)) {
      return NS_ERROR_UNEXPECTED;
    }

    if (!desc.object() || desc.hasGetterOrSetter() || !desc.value().isObject()) {
      return NS_OK;
    }

    JS::Rooted<JSObject*> dot_prototype(cx, &desc.value().toObject());

    JS::Rooted<JSObject*> proto(cx, dom_obj);
    for (;;) {
      if (!JS_GetPrototype(cx, proto, &proto)) {
        return NS_ERROR_UNEXPECTED;
      }
      if (!proto) {
        break;
      }
      if (proto == dot_prototype) {
        *bp = true;
        break;
      }
    }

    return NS_OK;
  }

  if (name_struct->mType != nsGlobalNameStruct::eTypeClassConstructor &&
      name_struct->mType != nsGlobalNameStruct::eTypeExternalClassInfo &&
      name_struct->mType != nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    
    return NS_OK;
  }

  const nsGlobalNameStruct *class_name_struct = GetNameStruct();
  NS_ENSURE_TRUE(class_name_struct, NS_ERROR_FAILURE);

  if (name_struct == class_name_struct) {
    *bp = true;

    return NS_OK;
  }

  nsScriptNameSpaceManager *nameSpaceManager = GetNameSpaceManager();
  NS_ASSERTION(nameSpaceManager, "Can't get namespace manager?");

  const nsIID *class_iid;
  if (class_name_struct->mType == nsGlobalNameStruct::eTypeInterface ||
      class_name_struct->mType == nsGlobalNameStruct::eTypeClassProto) {
    class_iid = &class_name_struct->mIID;
  } else if (class_name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    class_iid =
      sClassInfoData[class_name_struct->mDOMClassInfoID].mProtoChainInterface;
  } else if (class_name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    class_iid = class_name_struct->mData->mProtoChainInterface;
  } else if (class_name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    const nsGlobalNameStruct* alias_struct =
      nameSpaceManager->GetConstructorProto(class_name_struct);
    if (!alias_struct) {
      NS_ERROR("Couldn't get constructor prototype.");
      return NS_ERROR_UNEXPECTED;
    }

    if (alias_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
      class_iid =
        sClassInfoData[alias_struct->mDOMClassInfoID].mProtoChainInterface;
    } else if (alias_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
      class_iid = alias_struct->mData->mProtoChainInterface;
    } else {
      NS_ERROR("Expected eTypeClassConstructor or eTypeExternalClassInfo.");
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    *bp = false;

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    name_struct = nameSpaceManager->GetConstructorProto(name_struct);
    if (!name_struct) {
      NS_ERROR("Couldn't get constructor prototype.");
      return NS_ERROR_UNEXPECTED;
    }
  }

  NS_ASSERTION(name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor ||
               name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo,
               "The constructor was set up with a struct of the wrong type.");

  const nsDOMClassInfoData *ci_data = nullptr;
  if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor &&
      name_struct->mDOMClassInfoID >= 0) {
    ci_data = &sClassInfoData[name_struct->mDOMClassInfoID];
  } else if (name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    ci_data = name_struct->mData;
  }

  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
  if (!iim) {
    NS_ERROR("nsDOMConstructor::HasInstance can't get interface info mgr.");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIInterfaceInfo> if_info;
  uint32_t count = 0;
  const nsIID* class_interface;
  while ((class_interface = ci_data->mInterfaces[count++])) {
    if (class_iid->Equals(*class_interface)) {
      *bp = true;

      return NS_OK;
    }

    iim->GetInfoForIID(class_interface, getter_AddRefs(if_info));
    if (!if_info) {
      NS_ERROR("nsDOMConstructor::HasInstance can't get interface info.");
      return NS_ERROR_UNEXPECTED;
    }

    if_info->HasAncestor(class_iid, bp);

    if (*bp) {
      return NS_OK;
    }
  }

  return NS_OK;
}

nsresult
nsDOMConstructor::ResolveInterfaceConstants(JSContext *cx, JS::Handle<JSObject*> obj)
{
  const nsGlobalNameStruct *class_name_struct = GetNameStruct();
  if (!class_name_struct)
    return NS_ERROR_UNEXPECTED;

  const nsIID *class_iid;
  if (class_name_struct->mType == nsGlobalNameStruct::eTypeInterface ||
      class_name_struct->mType == nsGlobalNameStruct::eTypeClassProto) {
    class_iid = &class_name_struct->mIID;
  } else if (class_name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    class_iid =
      sClassInfoData[class_name_struct->mDOMClassInfoID].mProtoChainInterface;
  } else if (class_name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    class_iid = class_name_struct->mData->mProtoChainInterface;
  } else {
    return NS_OK;
  }

  nsresult rv = DefineInterfaceConstants(cx, obj, class_iid);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMConstructor::ToString(nsAString &aResult)
{
  aResult.AssignLiteral("[object ");
  aResult.Append(mClassName);
  aResult.Append(char16_t(']'));

  return NS_OK;
}


static nsresult
GetXPCProto(nsIXPConnect *aXPConnect, JSContext *cx, nsGlobalWindow *aWin,
            const nsGlobalNameStruct *aNameStruct,
            JS::MutableHandle<JSObject*> aProto)
{
  NS_ASSERTION(aNameStruct->mType ==
                 nsGlobalNameStruct::eTypeClassConstructor ||
               aNameStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo,
               "Wrong type!");

  nsCOMPtr<nsIClassInfo> ci;
  if (aNameStruct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    int32_t id = aNameStruct->mDOMClassInfoID;
    MOZ_ASSERT(id >= 0, "Negative DOM classinfo?!?");

    nsDOMClassInfoID ci_id = (nsDOMClassInfoID)id;

    ci = NS_GetDOMClassInfoInstance(ci_id);
  }
  else {
    ci = nsDOMClassInfo::GetClassInfoInstance(aNameStruct->mData);
  }
  NS_ENSURE_TRUE(ci, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIXPConnectJSObjectHolder> proto_holder;
  nsresult rv =
    aXPConnect->GetWrappedNativePrototype(cx, aWin->GetGlobalJSObject(), ci,
                                          getter_AddRefs(proto_holder));
  NS_ENSURE_SUCCESS(rv, rv);

  aProto.set(proto_holder->GetJSObject());
  return JS_WrapObject(cx, aProto) ? NS_OK : NS_ERROR_FAILURE;
}



static nsresult
ResolvePrototype(nsIXPConnect *aXPConnect, nsGlobalWindow *aWin, JSContext *cx,
                 JS::Handle<JSObject*> obj, const char16_t *name,
                 const nsDOMClassInfoData *ci_data,
                 const nsGlobalNameStruct *name_struct,
                 nsScriptNameSpaceManager *nameSpaceManager,
                 JSObject* aDot_prototype,
                 JS::MutableHandle<JSPropertyDescriptor> ctorDesc)
{
  JS::Rooted<JSObject*> dot_prototype(cx, aDot_prototype);
  NS_ASSERTION(ci_data ||
               (name_struct &&
                name_struct->mType == nsGlobalNameStruct::eTypeClassProto),
               "Wrong type or missing ci_data!");

  nsRefPtr<nsDOMConstructor> constructor;
  nsresult rv = nsDOMConstructor::Create(name, ci_data, name_struct, aWin,
                                         getter_AddRefs(constructor));
  NS_ENSURE_SUCCESS(rv, rv);

  JS::Rooted<JS::Value> v(cx);

  js::AssertSameCompartment(cx, obj);
  rv = nsContentUtils::WrapNative(cx, constructor,
                                  &NS_GET_IID(nsIDOMDOMConstructor), &v,
                                  false);
  NS_ENSURE_SUCCESS(rv, rv);

  FillPropertyDescriptor(ctorDesc, obj, 0, v);
  
  
  
  if (!JS_WrapValue(cx, ctorDesc.value())) {
    return NS_ERROR_UNEXPECTED;
  }

  JS::Rooted<JSObject*> class_obj(cx, &v.toObject());

  const nsIID *primary_iid = &NS_GET_IID(nsISupports);

  if (!ci_data) {
    primary_iid = &name_struct->mIID;
  }
  else if (ci_data->mProtoChainInterface) {
    primary_iid = ci_data->mProtoChainInterface;
  }

  nsCOMPtr<nsIInterfaceInfo> if_info;
  nsCOMPtr<nsIInterfaceInfo> parent;
  const char *class_parent_name = nullptr;

  if (!primary_iid->Equals(NS_GET_IID(nsISupports))) {
    JSAutoCompartment ac(cx, class_obj);

    rv = DefineInterfaceConstants(cx, class_obj, primary_iid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIInterfaceInfoManager>
      iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
    NS_ENSURE_TRUE(iim, NS_ERROR_NOT_AVAILABLE);

    iim->GetInfoForIID(primary_iid, getter_AddRefs(if_info));
    NS_ENSURE_TRUE(if_info, NS_ERROR_UNEXPECTED);

    const nsIID *iid = nullptr;

    if (ci_data && !ci_data->mHasClassInterface) {
      if_info->GetIIDShared(&iid);
    } else {
      if_info->GetParent(getter_AddRefs(parent));
      NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);

      parent->GetIIDShared(&iid);
    }

    if (iid) {
      if (!iid->Equals(NS_GET_IID(nsISupports))) {
        if (ci_data && !ci_data->mHasClassInterface) {
          
          
          

          if_info->GetNameShared(&class_parent_name);
        } else {
          
          
          
          

          NS_ASSERTION(parent, "Whoa, this is bad, null parent here!");

          parent->GetNameShared(&class_parent_name);
        }
      }
    }
  }

  {
    JS::Rooted<JSObject*> winobj(cx, aWin->FastGetGlobalJSObject());

    JS::Rooted<JSObject*> proto(cx);

    if (class_parent_name) {
      JSAutoCompartment ac(cx, winobj);

      JS::Rooted<JSPropertyDescriptor> desc(cx);
      if (!JS_GetPropertyDescriptor(cx, winobj, CutPrefix(class_parent_name), &desc)) {
        return NS_ERROR_UNEXPECTED;
      }

      if (desc.object() && !desc.hasGetterOrSetter() && desc.value().isObject()) {
        JS::Rooted<JSObject*> obj(cx, &desc.value().toObject());
        if (!JS_GetPropertyDescriptor(cx, obj, "prototype", &desc)) {
          return NS_ERROR_UNEXPECTED;
        }

        if (desc.object() && !desc.hasGetterOrSetter() && desc.value().isObject()) {
          proto = &desc.value().toObject();
        }
      }
    }

    if (dot_prototype) {
      JSAutoCompartment ac(cx, dot_prototype);
      JS::Rooted<JSObject*> xpc_proto_proto(cx);
      if (!::JS_GetPrototype(cx, dot_prototype, &xpc_proto_proto)) {
        return NS_ERROR_UNEXPECTED;
      }

      if (proto &&
          (!xpc_proto_proto ||
           JS_GetClass(xpc_proto_proto) == sObjectClass)) {
        if (!JS_WrapObject(cx, &proto) ||
            !JS_SetPrototype(cx, dot_prototype, proto)) {
          return NS_ERROR_UNEXPECTED;
        }
      }
    } else {
      JSAutoCompartment ac(cx, winobj);
      if (!proto) {
        proto = JS_GetObjectPrototype(cx, winobj);
      }
      dot_prototype = ::JS_NewObjectWithUniqueType(cx,
                                                   &sDOMConstructorProtoClass,
                                                   proto);
      NS_ENSURE_TRUE(dot_prototype, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  v = OBJECT_TO_JSVAL(dot_prototype);

  JSAutoCompartment ac(cx, class_obj);

  
  if (!JS_WrapValue(cx, &v) ||
      !JS_DefineProperty(cx, class_obj, "prototype", v,
                         JSPROP_PERMANENT | JSPROP_READONLY,
                         JS_STUBGETTER, JS_STUBSETTER)) {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

static bool
OldBindingConstructorEnabled(const nsGlobalNameStruct *aStruct,
                             nsGlobalWindow *aWin, JSContext *cx)
{
  MOZ_ASSERT(aStruct->mType == nsGlobalNameStruct::eTypeProperty ||
             aStruct->mType == nsGlobalNameStruct::eTypeClassConstructor ||
             aStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo);

  
  if (aStruct->mChromeOnly) {
    bool expose;
    if (aStruct->mAllowXBL) {
      expose = IsChromeOrXBL(cx, nullptr);
    } else {
      expose = nsContentUtils::IsSystemPrincipal(aWin->GetPrincipal());
    }

    if (!expose) {
      return false;
    }
  }

  return true;
}

static nsresult
LookupComponentsShim(JSContext *cx, JS::Handle<JSObject*> global,
                     nsPIDOMWindow *win,
                     JS::MutableHandle<JSPropertyDescriptor> desc);


bool
nsWindowSH::NameStructEnabled(JSContext* aCx, nsGlobalWindow *aWin,
                              const nsAString& aName,
                              const nsGlobalNameStruct& aNameStruct)
{
  const nsGlobalNameStruct* nameStruct = &aNameStruct;
  if (nameStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfoCreator) {
    nsresult rv = GetExternalClassInfo(GetNameSpaceManager(), aName, nameStruct,
                                       &nameStruct);
    if (NS_FAILED(rv) || !nameStruct) {
      return false;
    }
  }

  return (nameStruct->mType != nsGlobalNameStruct::eTypeProperty &&
          nameStruct->mType != nsGlobalNameStruct::eTypeClassConstructor &&
          nameStruct->mType != nsGlobalNameStruct::eTypeExternalClassInfo) ||
         OldBindingConstructorEnabled(nameStruct, aWin, aCx);
}

#ifdef RELEASE_BUILD
#define USE_CONTROLLERS_SHIM
#endif

#ifdef USE_CONTROLLERS_SHIM
static const JSClass ControllersShimClass = {
    "XULControllers", 0
};
#endif


nsresult
nsWindowSH::GlobalResolve(nsGlobalWindow *aWin, JSContext *cx,
                          JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                          JS::MutableHandle<JSPropertyDescriptor> desc)
{
  if (id == XPCJSRuntime::Get()->GetStringID(XPCJSRuntime::IDX_COMPONENTS)) {
    return LookupComponentsShim(cx, obj, aWin, desc);
  }

#ifdef USE_CONTROLLERS_SHIM
  
  
  
  if (id == XPCJSRuntime::Get()->GetStringID(XPCJSRuntime::IDX_CONTROLLERS) &&
      !xpc::IsXrayWrapper(obj) &&
      !nsContentUtils::IsSystemPrincipal(nsContentUtils::ObjectPrincipal(obj)))
  {
    if (aWin->GetDoc()) {
      aWin->GetDoc()->WarnOnceAbout(nsIDocument::eWindow_Controllers);
    }
    MOZ_ASSERT(JS_IsGlobalObject(obj));
    JS::Rooted<JSObject*> shim(cx, JS_NewObject(cx, &ControllersShimClass));
    if (NS_WARN_IF(!shim)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    FillPropertyDescriptor(desc, obj, JS::ObjectValue(*shim),  false);
    return NS_OK;
  }
#endif

  nsScriptNameSpaceManager *nameSpaceManager = GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  
  
  nsAutoJSString name;
  if (!name.init(cx, JSID_TO_STRING(id))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  const char16_t *class_name = nullptr;
  const nsGlobalNameStruct *name_struct =
    nameSpaceManager->LookupName(name, &class_name);

  if (!name_struct) {
    return NS_OK;
  }

  
  MOZ_ASSERT(name.Equals(class_name));

  NS_ENSURE_TRUE(class_name, NS_ERROR_UNEXPECTED);

  nsresult rv = NS_OK;

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfoCreator) {
    rv = GetExternalClassInfo(nameSpaceManager, name, name_struct,
                              &name_struct);
    if (NS_FAILED(rv) || !name_struct) {
      return rv;
    }
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeNewDOMBinding ||
      name_struct->mType == nsGlobalNameStruct::eTypeInterface ||
      name_struct->mType == nsGlobalNameStruct::eTypeClassProto ||
      name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    
    DefineInterface getOrCreateInterfaceObject =
      name_struct->mDefineDOMInterface;
    if (getOrCreateInterfaceObject) {
      if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor &&
          !OldBindingConstructorEnabled(name_struct, aWin, cx)) {
        return NS_OK;
      }

      ConstructorEnabled* checkEnabledForScope = name_struct->mConstructorEnabled;
      
      
      
      
      JS::Rooted<JSObject*> global(cx,
        js::CheckedUnwrap(obj,  false));
      if (!global) {
        return NS_ERROR_DOM_SECURITY_ERR;
      }
      if (checkEnabledForScope && !checkEnabledForScope(cx, global)) {
        return NS_OK;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (xpc::WrapperFactory::IsXrayWrapper(obj)) {
        JS::Rooted<JSObject*> interfaceObject(cx);
        {
          JSAutoCompartment ac(cx, global);
          interfaceObject = getOrCreateInterfaceObject(cx, global, id, false);
        }
        if (NS_WARN_IF(!interfaceObject)) {
          return NS_ERROR_FAILURE;
        }
        if (!JS_WrapObject(cx, &interfaceObject)) {
          return NS_ERROR_FAILURE;
        }

        FillPropertyDescriptor(desc, obj, 0, JS::ObjectValue(*interfaceObject));
      } else {
        JS::Rooted<JSObject*> interfaceObject(cx,
          getOrCreateInterfaceObject(cx, obj, id, true));
        if (NS_WARN_IF(!interfaceObject)) {
          return NS_ERROR_FAILURE;
        }
        
        
        
        
        
        FillPropertyDescriptor(desc, obj, JS::UndefinedValue(), false);
      }

      return NS_OK;
    }
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeInterface) {
    
    
    nsRefPtr<nsDOMConstructor> constructor;
    rv = nsDOMConstructor::Create(class_name,
                                  nullptr,
                                  name_struct,
                                  static_cast<nsPIDOMWindow*>(aWin),
                                  getter_AddRefs(constructor));
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JS::Value> v(cx);
    js::AssertSameCompartment(cx, obj);
    rv = nsContentUtils::WrapNative(cx, constructor,
                                    &NS_GET_IID(nsIDOMDOMConstructor), &v,
                                    false);
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JSObject*> class_obj(cx, &v.toObject());

    
    

    {
      JSAutoCompartment ac(cx, class_obj);
      rv = DefineInterfaceConstants(cx, class_obj, &name_struct->mIID);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!JS_WrapValue(cx, &v)) {
      return NS_ERROR_UNEXPECTED;
    }

    FillPropertyDescriptor(desc, obj, 0, v);
    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor ||
      name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    if (!OldBindingConstructorEnabled(name_struct, aWin, cx)) {
      return NS_OK;
    }

    
    
    
    JS::Rooted<JSObject*> dot_prototype(cx);
    rv = GetXPCProto(nsDOMClassInfo::sXPConnect, cx, aWin, name_struct,
                     &dot_prototype);
    NS_ENSURE_SUCCESS(rv, rv);
    MOZ_ASSERT(dot_prototype);

    bool isXray = xpc::WrapperFactory::IsXrayWrapper(obj);
    MOZ_ASSERT_IF(obj != aWin->GetGlobalJSObject(), isXray);
    if (!isXray) {
      
      FillPropertyDescriptor(desc, obj, JS::UndefinedValue(), false);
      return NS_OK;
    }

    
    
    const nsDOMClassInfoData *ci_data;
    if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
      ci_data = &sClassInfoData[name_struct->mDOMClassInfoID];
    } else {
      ci_data = name_struct->mData;
    }

    return ResolvePrototype(nsDOMClassInfo::sXPConnect, aWin, cx, obj,
                            class_name, ci_data,
                            name_struct, nameSpaceManager, dot_prototype,
                            desc);
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeClassProto) {
    
    
    return ResolvePrototype(nsDOMClassInfo::sXPConnect, aWin, cx, obj,
                            class_name, nullptr,
                            name_struct, nameSpaceManager, nullptr, desc);
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    const nsGlobalNameStruct *alias_struct =
      nameSpaceManager->GetConstructorProto(name_struct);
    NS_ENSURE_TRUE(alias_struct, NS_ERROR_UNEXPECTED);

    
    
    
    JS::Rooted<JSObject*> dot_prototype(cx);
    rv = GetXPCProto(nsDOMClassInfo::sXPConnect, cx, aWin, alias_struct,
                     &dot_prototype);
    NS_ENSURE_SUCCESS(rv, rv);
    MOZ_ASSERT(dot_prototype);

    const nsDOMClassInfoData *ci_data;
    if (alias_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
      ci_data = &sClassInfoData[alias_struct->mDOMClassInfoID];
    } else if (alias_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
      ci_data = alias_struct->mData;
    } else {
      return NS_ERROR_UNEXPECTED;
    }

    return ResolvePrototype(nsDOMClassInfo::sXPConnect, aWin, cx, obj,
                            class_name, ci_data,
                            name_struct, nameSpaceManager, nullptr, desc);
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructor) {
    nsRefPtr<nsDOMConstructor> constructor;
    rv = nsDOMConstructor::Create(class_name, nullptr, name_struct,
                                  static_cast<nsPIDOMWindow*>(aWin),
                                  getter_AddRefs(constructor));
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JS::Value> val(cx);
    js::AssertSameCompartment(cx, obj);
    rv = nsContentUtils::WrapNative(cx, constructor,
                                    &NS_GET_IID(nsIDOMDOMConstructor), &val,
                                    true);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(val.isObject(), "Why didn't we get a JSObject?");

    FillPropertyDescriptor(desc, obj, 0, val);

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeProperty) {
    if (!OldBindingConstructorEnabled(name_struct, aWin, cx))
      return NS_OK;

    
    
    nsCOMPtr<nsIDOMWindow> childWin = aWin->GetChildWindow(name);
    if (childWin)
      return NS_OK;

    nsCOMPtr<nsISupports> native(do_CreateInstance(name_struct->mCID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JS::Value> prop_val(cx, JS::UndefinedValue()); 

    nsCOMPtr<nsIDOMGlobalPropertyInitializer> gpi(do_QueryInterface(native));
    if (gpi) {
      rv = gpi->Init(aWin, &prop_val);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (prop_val.isPrimitive() && !prop_val.isNull()) {
      if (aWin->IsOuterWindow()) {
        nsGlobalWindow *inner = aWin->GetCurrentInnerWindowInternal();
        NS_ENSURE_TRUE(inner, NS_ERROR_UNEXPECTED);
      }

      rv = nsContentUtils::WrapNative(cx, native, &prop_val, true);
    }

    NS_ENSURE_SUCCESS(rv, rv);

    if (!JS_WrapValue(cx, &prop_val)) {
      return NS_ERROR_UNEXPECTED;
    }

    FillPropertyDescriptor(desc, obj, prop_val, false);

    return NS_OK;
  }

  return rv;
}

struct InterfaceShimEntry {
  const char *geckoName;
  const char *domName;
};




const InterfaceShimEntry kInterfaceShimMap[] =
{ { "nsIDOMFileReader", "FileReader" },
  { "nsIXMLHttpRequest", "XMLHttpRequest" },
  { "nsIDOMDOMException", "DOMException" },
  { "nsIDOMNode", "Node" },
  { "nsIDOMCSSPrimitiveValue", "CSSPrimitiveValue" },
  { "nsIDOMCSSRule", "CSSRule" },
  { "nsIDOMCSSValue", "CSSValue" },
  { "nsIDOMEvent", "Event" },
  { "nsIDOMNSEvent", "Event" },
  { "nsIDOMKeyEvent", "KeyEvent" },
  { "nsIDOMMouseEvent", "MouseEvent" },
  { "nsIDOMMouseScrollEvent", "MouseScrollEvent" },
  { "nsIDOMMutationEvent", "MutationEvent" },
  { "nsIDOMSimpleGestureEvent", "SimpleGestureEvent" },
  { "nsIDOMUIEvent", "UIEvent" },
  { "nsIDOMHTMLMediaElement", "HTMLMediaElement" },
  { "nsIDOMMediaError", "MediaError" },
  { "nsIDOMOfflineResourceList", "OfflineResourceList" },
  { "nsIDOMRange", "Range" },
  { "nsIDOMSVGLength", "SVGLength" },
  { "nsIDOMNodeFilter", "NodeFilter" },
  { "nsIDOMXPathResult", "XPathResult" } };

static nsresult
LookupComponentsShim(JSContext *cx, JS::Handle<JSObject*> global,
                     nsPIDOMWindow *win,
                     JS::MutableHandle<JSPropertyDescriptor> desc)
{
  
  Telemetry::Accumulate(Telemetry::COMPONENTS_SHIM_ACCESSED_BY_CONTENT, true);

  
  nsCOMPtr<nsIDocument> doc = win->GetExtantDoc();
  if (doc) {
    doc->WarnOnceAbout(nsIDocument::eComponents,  true);
  }

  
  AssertSameCompartment(cx, global);
  JS::Rooted<JSObject*> components(cx, JS_NewPlainObject(cx));
  NS_ENSURE_TRUE(components, NS_ERROR_OUT_OF_MEMORY);

  
  JS::Rooted<JSObject*> interfaces(cx, JS_NewPlainObject(cx));
  NS_ENSURE_TRUE(interfaces, NS_ERROR_OUT_OF_MEMORY);
  bool ok =
    JS_DefineProperty(cx, components, "interfaces", interfaces,
                      JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY,
                      JS_STUBGETTER, JS_STUBSETTER);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  
  
  for (uint32_t i = 0; i < ArrayLength(kInterfaceShimMap); ++i) {

    
    const char *geckoName = kInterfaceShimMap[i].geckoName;
    const char *domName = kInterfaceShimMap[i].domName;

    
    JS::Rooted<JS::Value> v(cx, JS::UndefinedValue());
    ok = JS_GetProperty(cx, global, domName, &v);
    NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
    if (!v.isObject()) {
      NS_WARNING("Unable to find interface object on global");
      continue;
    }

    
    ok = JS_DefineProperty(cx, interfaces, geckoName, v,
                           JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY,
                           JS_STUBGETTER, JS_STUBSETTER);
    NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  }

  FillPropertyDescriptor(desc, global, JS::ObjectValue(*components), false);

  return NS_OK;
}



NS_IMETHODIMP
nsEventTargetSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                           JSObject *aGlobalObj, JSObject **parentObj)
{
  JS::Rooted<JSObject*> globalObj(cx, aGlobalObj);
  DOMEventTargetHelper* target = DOMEventTargetHelper::FromSupports(nativeObj);

  nsCOMPtr<nsIScriptGlobalObject> native_parent;
  target->GetParentObject(getter_AddRefs(native_parent));

  *parentObj = native_parent ? native_parent->GetGlobalJSObject() : globalObj;

  return *parentObj ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsEventTargetSH::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsid id, JS::Handle<JS::Value> val,
                             bool *_retval)
{
  nsEventTargetSH::PreserveWrapper(GetNative(wrapper, obj));

  return NS_OK;
}

void
nsEventTargetSH::PreserveWrapper(nsISupports *aNative)
{
  DOMEventTargetHelper* target = DOMEventTargetHelper::FromSupports(aNative);
  target->PreserveWrapper(aNative);
}



NS_INTERFACE_MAP_BEGIN(nsEventListenerThisTranslator)
  NS_INTERFACE_MAP_ENTRY(nsIXPCFunctionThisTranslator)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsEventListenerThisTranslator)
NS_IMPL_RELEASE(nsEventListenerThisTranslator)


NS_IMETHODIMP
nsEventListenerThisTranslator::TranslateThis(nsISupports *aInitialThis,
                                             nsISupports **_retval)
{
  nsCOMPtr<nsIDOMEvent> event(do_QueryInterface(aInitialThis));
  NS_ENSURE_TRUE(event, NS_ERROR_UNEXPECTED);

  nsCOMPtr<EventTarget> target = event->InternalDOMEvent()->GetCurrentTarget();
  target.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMConstructorSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                              JSObject *aGlobalObj, JSObject **parentObj)
{
  JS::Rooted<JSObject*> globalObj(cx, aGlobalObj);
  nsDOMConstructor *wrapped = static_cast<nsDOMConstructor *>(nativeObj);

#ifdef DEBUG
  {
    nsCOMPtr<nsIDOMDOMConstructor> is_constructor =
      do_QueryInterface(nativeObj);
    NS_ASSERTION(is_constructor, "How did we not get a constructor?");
  }
#endif

  return wrapped->PreCreate(cx, globalObj, parentObj);
}

NS_IMETHODIMP
nsDOMConstructorSH::Resolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *aObj, jsid aId, bool *resolvedp,
                            bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  JS::Rooted<jsid> id(cx, aId);
  
  
  
  
  if (!ObjectIsNativeWrapper(cx, obj)) {
    return NS_OK;
  }

  JS::Rooted<JSObject*> nativePropsObj(cx, xpc::XrayUtils::GetNativePropertiesObject(cx, obj));
  nsDOMConstructor *wrapped =
    static_cast<nsDOMConstructor *>(wrapper->Native());
  nsresult rv = wrapped->ResolveInterfaceConstants(cx, nativePropsObj);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  bool found;
  if (!JS_HasPropertyById(cx, nativePropsObj, id, &found)) {
    *_retval = false;
    return NS_OK;
  }

  if (found) {
    *resolvedp = true;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMConstructorSH::Call(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *aObj, const JS::CallArgs &args, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  MOZ_ASSERT(obj);

  nsDOMConstructor *wrapped =
    static_cast<nsDOMConstructor *>(wrapper->Native());

#ifdef DEBUG
  {
    nsCOMPtr<nsIDOMDOMConstructor> is_constructor =
      do_QueryWrappedNative(wrapper);
    NS_ASSERTION(is_constructor, "How did we not get a constructor?");
  }
#endif

  return wrapped->Construct(wrapper, cx, obj, args, _retval);
}

NS_IMETHODIMP
nsDOMConstructorSH::Construct(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                              JSObject *aObj, const JS::CallArgs &args, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  MOZ_ASSERT(obj);

  nsDOMConstructor *wrapped =
    static_cast<nsDOMConstructor *>(wrapper->Native());

#ifdef DEBUG
  {
    nsCOMPtr<nsIDOMDOMConstructor> is_constructor =
      do_QueryWrappedNative(wrapper);
    NS_ASSERTION(is_constructor, "How did we not get a constructor?");
  }
#endif

  return wrapped->Construct(wrapper, cx, obj, args, _retval);
}

NS_IMETHODIMP
nsDOMConstructorSH::HasInstance(nsIXPConnectWrappedNative *wrapper,
                                JSContext *cx, JSObject *aObj, JS::Handle<JS::Value> val,
                                bool *bp, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  nsDOMConstructor *wrapped =
    static_cast<nsDOMConstructor *>(wrapper->Native());

#ifdef DEBUG
  {
    nsCOMPtr<nsIDOMDOMConstructor> is_constructor =
      do_QueryWrappedNative(wrapper);
    NS_ASSERTION(is_constructor, "How did we not get a constructor?");
  }
#endif

  return wrapped->HasInstance(wrapper, cx, obj, val, bp, _retval);
}

NS_IMETHODIMP
nsNonDOMObjectSH::GetFlags(uint32_t *aFlags)
{
  
  
  
  *aFlags = nsIClassInfo::MAIN_THREAD_ONLY | nsIClassInfo::SINGLETON_CLASSINFO;
  return NS_OK;
}



template<typename Super>
NS_IMETHODIMP
nsMessageManagerSH<Super>::Resolve(nsIXPConnectWrappedNative* wrapper,
                                   JSContext* cx, JSObject* obj_,
                                   jsid id_, bool* resolvedp,
                                   bool* _retval)
{
  JS::Rooted<JSObject*> obj(cx, obj_);
  JS::Rooted<jsid> id(cx, id_);

  *_retval = SystemGlobalResolve(cx, obj, id, resolvedp);
  NS_ENSURE_TRUE(*_retval, NS_ERROR_FAILURE);

  if (*resolvedp) {
    return NS_OK;
  }

  return Super::Resolve(wrapper, cx, obj, id, resolvedp, _retval);
}

template<typename Super>
NS_IMETHODIMP
nsMessageManagerSH<Super>::Enumerate(nsIXPConnectWrappedNative* wrapper,
                                     JSContext* cx, JSObject* obj_,
                                     bool* _retval)
{
  JS::Rooted<JSObject*> obj(cx, obj_);

  *_retval = SystemGlobalEnumerate(cx, obj);
  NS_ENSURE_TRUE(*_retval, NS_ERROR_FAILURE);

  
  
  return NS_OK;
}
