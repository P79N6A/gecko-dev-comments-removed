





#include "mozilla/Util.h"

#include "mozilla/dom/SmsFilter.h"

#ifdef XP_WIN
#undef GetClassName
#endif


#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsprvtd.h"    
#include "jsdbgapi.h"
#include "WrapperFactory.h"
#include "AccessCheck.h"
#include "XrayWrapper.h"

#include "xpcpublic.h"
#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "XPCQuickStubs.h"
#include "nsDOMQS.h"

#include "mozilla/dom/RegisterBindings.h"

#include "nscore.h"
#include "nsDOMClassInfo.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsIComponentRegistrar.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIXPConnect.h"
#include "nsIXPCSecurityManager.h"
#include "nsIStringBundle.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "xptcall.h"
#include "prprf.h"
#include "nsTArray.h"
#include "nsCSSValue.h"
#include "nsThreadUtils.h"
#include "nsDOMEventTargetHelper.h"


#include "nsGlobalWindow.h"
#include "nsHistory.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventListener.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsDOMWindowUtils.h"
#include "nsIDOMGlobalPropertyInitializer.h"
#include "mozilla/Preferences.h"
#include "nsLocation.h"
#include "mozilla/Attributes.h"
#include "mozilla/Telemetry.h"


#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIScriptExternalNameSet.h"
#include "nsJSUtils.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsScriptNameSpaceManager.h"
#include "nsIJSNativeInitializer.h"
#include "nsJSEnvironment.h"


#include "nsIDOMLocation.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMJSWindow.h"
#include "nsIDOMWindowCollection.h"
#include "nsIDOMHistory.h"
#include "nsIDOMMediaList.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMConstructor.h"


#include "nsError.h"
#include "nsIDOMDOMException.h"
#include "nsIDOMNode.h"
#include "nsIDOMDOMStringList.h"
#include "nsIDOMUserDataHandler.h"
#include "nsIDOMGeoPositionError.h"
#include "nsIDOMLoadStatus.h"
#include "nsIDOMXPathNamespace.h"
#include "nsIDOMXULButtonElement.h"
#include "nsIDOMXULCheckboxElement.h"
#include "nsIDOMXULPopupElement.h"


#include "nsEventListenerManager.h"
#include "nsIDOMEventTarget.h"


#include "nsCSSRules.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDOMStyleSheetList.h"
#include "nsIDOMCSSRule.h"
#include "nsICSSRuleList.h"
#include "nsIDOMRect.h"
#include "nsDOMCSSAttrDeclaration.h"


#include "nsXBLService.h"
#include "nsXBLBinding.h"
#include "nsBindingManager.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIDOMElement.h"
#include "nsStyleContext.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"


#include "nsIDOMXPathEvaluator.h"
#include "nsIXSLTProcessor.h"
#include "nsIXSLTProcessorPrivate.h"

#include "nsXMLHttpRequest.h"
#include "nsIDOMContactManager.h"
#include "nsIDOMPermissionSettings.h"
#include "nsIDOMApplicationRegistry.h"


#include "nsIDOMDocumentXBL.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMLinkStyle.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMCSSCharsetRule.h"
#include "nsIDOMCSSImportRule.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSFontFaceRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsIDOMCSSSupportsRule.h"
#include "nsIDOMMozCSSKeyframeRule.h"
#include "nsIDOMMozCSSKeyframesRule.h"
#include "nsIDOMCSSPageRule.h"
#include "nsIDOMCSSStyleRule.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsIDOMDeviceMotionEvent.h" 
#include "nsIDOMXULCommandDispatcher.h"
#ifndef MOZ_DISABLE_CRYPTOLEGACY
#include "nsIDOMCRMFObject.h"
#include "nsIDOMCryptoLegacy.h"
#else
#include "nsIDOMCrypto.h"
#endif
#include "nsIControllers.h"
#include "nsISelection.h"
#include "nsIBoxObject.h"
#ifdef MOZ_XUL
#include "nsITreeSelection.h"
#include "nsITreeContentView.h"
#include "nsITreeView.h"
#include "nsIXULTemplateBuilder.h"
#include "nsTreeColumns.h"
#endif
#include "nsIDOMXPathExpression.h"
#include "nsIDOMNSXPathExpression.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIDOMXPathResult.h"
#include "nsIDOMMozBrowserFrame.h"

#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGNumber.h"


#include "DOMStorage.h"


#include "nsIDOMDataTransfer.h"


#include "nsIDOMGeoPositionCoords.h"


#include "mozilla/dom/workers/Workers.h"

#include "nsDOMFile.h"

#include "nsIEventListenerService.h"
#include "nsIMessageManager.h"
#include "mozilla/dom/Element.h"

#include "mozilla/dom/indexedDB/IDBWrapperCache.h"
#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabase.h"
#include "mozilla/dom/indexedDB/IDBObjectStore.h"
#include "mozilla/dom/indexedDB/IDBCursor.h"
#include "mozilla/dom/indexedDB/IDBKeyRange.h"
#include "mozilla/dom/indexedDB/IDBIndex.h"

using mozilla::dom::indexedDB::IDBWrapperCache;
using mozilla::dom::workers::ResolveWorkerClasses;

#include "nsIDOMMediaQueryList.h"

#include "mozilla/dom/Activity.h"

#include "nsDOMTouchEvent.h"

#include "nsWrapperCacheInlines.h"
#include "mozilla/dom/HTMLCollectionBinding.h"

#include "BatteryManager.h"
#include "nsIDOMPowerManager.h"
#include "nsIDOMWakeLock.h"
#include "nsIDOMMobileMessageManager.h"
#include "nsIDOMMozSmsMessage.h"
#include "nsIDOMMozMmsMessage.h"
#include "nsIDOMSmsFilter.h"
#include "nsIDOMSmsSegmentInfo.h"
#include "nsIDOMMozMobileMessageThread.h"
#include "nsIDOMConnection.h"

#ifdef MOZ_B2G_RIL
#include "Telephony.h"
#include "TelephonyCall.h"
#include "nsIDOMMozVoicemail.h"
#include "nsIDOMIccManager.h"
#include "nsIDOMMozCellBroadcast.h"
#include "nsIDOMMobileConnection.h"
#endif 

#ifdef MOZ_B2G_FM
#include "FMRadio.h"
#endif

#ifdef MOZ_B2G_BT
#include "BluetoothManager.h"
#include "BluetoothAdapter.h"
#include "BluetoothDevice.h"
#endif

#include "DOMCameraManager.h"
#include "DOMCameraControl.h"
#include "DOMCameraCapabilities.h"
#include "nsIOpenWindowEventDetail.h"
#include "nsIAsyncScrollEventDetail.h"
#include "nsIDOMGlobalObjectConstructor.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "LockedFile.h"
#include "nsDebug.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/Likely.h"

#ifdef MOZ_TIME_MANAGER
#include "TimeManager.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;

static NS_DEFINE_CID(kDOMSOF_CID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

static const char kDOMStringBundleURL[] =
  "chrome://global/locale/dom/dom.properties";




#define WINDOW_SCRIPTABLE_FLAGS                                               \
 (nsIXPCScriptable::WANT_PRECREATE |                                          \
  nsIXPCScriptable::WANT_FINALIZE |                                           \
  nsIXPCScriptable::WANT_ENUMERATE |                                          \
  nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE |                               \
  nsIXPCScriptable::IS_GLOBAL_OBJECT |                                        \
  nsIXPCScriptable::WANT_OUTER_OBJECT)

#define ARRAY_SCRIPTABLE_FLAGS                                                \
  (DOM_DEFAULT_SCRIPTABLE_FLAGS       |                                       \
   nsIXPCScriptable::WANT_GETPROPERTY |                                       \
   nsIXPCScriptable::WANT_ENUMERATE)

#define EVENTTARGET_SCRIPTABLE_FLAGS                                          \
  (DOM_DEFAULT_SCRIPTABLE_FLAGS       |                                       \
   nsIXPCScriptable::WANT_ADDPROPERTY)

#define IDBEVENTTARGET_SCRIPTABLE_FLAGS                                       \
  (EVENTTARGET_SCRIPTABLE_FLAGS)

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





#define DOMCI_DATA_NO_CLASS(_dom_class)                                       \
const uint32_t kDOMClassInfo_##_dom_class##_interfaces =                      \
  0;

#ifndef MOZ_DISABLE_CRYPTOLEGACY
DOMCI_DATA_NO_CLASS(CRMFObject)
#endif
DOMCI_DATA_NO_CLASS(Crypto)

DOMCI_DATA_NO_CLASS(ContentFrameMessageManager)
DOMCI_DATA_NO_CLASS(ChromeMessageBroadcaster)
DOMCI_DATA_NO_CLASS(ChromeMessageSender)

DOMCI_DATA_NO_CLASS(DOMPrototype)
DOMCI_DATA_NO_CLASS(DOMConstructor)

DOMCI_DATA_NO_CLASS(UserDataHandler)
DOMCI_DATA_NO_CLASS(LoadStatus)
DOMCI_DATA_NO_CLASS(XPathNamespace)
DOMCI_DATA_NO_CLASS(XULButtonElement)
DOMCI_DATA_NO_CLASS(XULCheckboxElement)
DOMCI_DATA_NO_CLASS(XULPopupElement)

#define NS_DEFINE_CLASSINFO_DATA_WITH_NAME(_class, _name, _helper,            \
                                           _flags)                            \
  { #_name,                                                                   \
    nullptr,                                                                   \
    { _helper::doCreate },                                                    \
    nullptr,                                                                   \
    nullptr,                                                                   \
    nullptr,                                                                   \
    _flags,                                                                   \
    true,                                                                  \
    0,                                                                        \
    false,                                                                 \
    false,                                                                 \
    NS_DEFINE_CLASSINFO_DATA_DEBUG(_class)                                    \
  },

#define NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA_WITH_NAME(_class, _name,         \
                                                       _helper, _flags)       \
  { #_name,                                                                   \
    nullptr,                                                                   \
    { _helper::doCreate },                                                    \
    nullptr,                                                                   \
    nullptr,                                                                   \
    nullptr,                                                                   \
    _flags,                                                                   \
    true,                                                                  \
    0,                                                                        \
    true,                                                                  \
    false,                                                                 \
    NS_DEFINE_CLASSINFO_DATA_DEBUG(_class)                                    \
  },

#define NS_DEFINE_CLASSINFO_DATA(_class, _helper, _flags)                     \
  NS_DEFINE_CLASSINFO_DATA_WITH_NAME(_class, _class, _helper, _flags)

#define NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(_class, _helper, _flags)          \
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA_WITH_NAME(_class, _class, _helper, \
                                                 _flags)

namespace {

class IDBEventTargetSH : public nsEventTargetSH
{
protected:
  IDBEventTargetSH(nsDOMClassInfoData* aData) : nsEventTargetSH(aData)
  { }

  virtual ~IDBEventTargetSH()
  { }

public:
  NS_IMETHOD PreCreate(nsISupports *aNativeObj, JSContext *aCx,
                       JSObject *aGlobalObj, JSObject **aParentObj);

  static nsIClassInfo *doCreate(nsDOMClassInfoData *aData)
  {
    return new IDBEventTargetSH(aData);
  }
};

} 












static nsDOMClassInfoData sClassInfoData[] = {
  

  
  
  
  
  


  NS_DEFINE_CLASSINFO_DATA(Window, nsWindowSH,
                           DEFAULT_SCRIPTABLE_FLAGS |
                           WINDOW_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(Location, nsLocationSH,
                           ((DOM_DEFAULT_SCRIPTABLE_FLAGS |
                             nsIXPCScriptable::WANT_ADDPROPERTY) &
                            ~nsIXPCScriptable::ALLOW_PROP_MODS_TO_PROTOTYPE))

  NS_DEFINE_CLASSINFO_DATA(History, nsHistorySH,
                           ARRAY_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_PRECREATE)
  NS_DEFINE_CLASSINFO_DATA(DOMPrototype, nsDOMConstructorSH,
                           DOM_BASE_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_PRECREATE |
                           nsIXPCScriptable::WANT_NEWRESOLVE |
                           nsIXPCScriptable::WANT_HASINSTANCE |
                           nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE)
  NS_DEFINE_CLASSINFO_DATA(DOMConstructor, nsDOMConstructorSH,
                           DOM_BASE_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_PRECREATE |
                           nsIXPCScriptable::WANT_NEWRESOLVE |
                           nsIXPCScriptable::WANT_HASINSTANCE |
                           nsIXPCScriptable::WANT_CALL |
                           nsIXPCScriptable::WANT_CONSTRUCT |
                           nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE)

  
  NS_DEFINE_CLASSINFO_DATA(DOMException, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  

  NS_DEFINE_CLASSINFO_DATA(DeviceAcceleration, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(DeviceRotationRate, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
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
  NS_DEFINE_CLASSINFO_DATA(CSSRuleList, nsCSSRuleListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSGroupRuleRuleList, nsCSSRuleListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MediaList, nsMediaListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(StyleSheetList, nsStyleSheetListSH,
                           ARRAY_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSStyleSheet, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(Selection, nsDOMGenericSH,
                           DEFAULT_SCRIPTABLE_FLAGS)

  
#ifdef MOZ_XUL
  NS_DEFINE_CLASSINFO_DATA(XULCommandDispatcher, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
#endif
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(XULControllers, nsNonDOMObjectSH,
                                       DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(BoxObject, nsDOMGenericSH,
                                       DEFAULT_SCRIPTABLE_FLAGS)
#ifdef MOZ_XUL
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(TreeSelection, nsDOMGenericSH,
                                       DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(TreeContentView, nsDOMGenericSH,
                                       DEFAULT_SCRIPTABLE_FLAGS)
#endif

  
#ifndef MOZ_DISABLE_CRYPTOLEGACY
  NS_DEFINE_CLASSINFO_DATA(CRMFObject, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
#endif
  NS_DEFINE_CLASSINFO_DATA(Crypto, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(ChromeWindow, nsWindowSH,
                           DEFAULT_SCRIPTABLE_FLAGS |
                           WINDOW_SCRIPTABLE_FLAGS)

#ifdef MOZ_XUL
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(XULTemplateBuilder, nsDOMGenericSH,
                                       DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(XULTreeBuilder, nsDOMGenericSH,
                                       DEFAULT_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(DOMStringList, nsStringListSH,
                           ARRAY_SCRIPTABLE_FLAGS)

#ifdef MOZ_XUL
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(TreeColumn, nsDOMGenericSH,
                                       DEFAULT_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(CSSMozDocumentRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSSupportsRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(SVGLength, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(SVGNumber, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozCanvasPrintState, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(WindowUtils, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(XSLTProcessor, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(XPathExpression, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XPathNSResolver, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XPathResult, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  

  
  
  
  
  NS_DEFINE_CLASSINFO_DATA(Storage, nsStorage2SH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS |
                           nsIXPCScriptable::WANT_NEWRESOLVE |
                           nsIXPCScriptable::WANT_GETPROPERTY |
                           nsIXPCScriptable::WANT_SETPROPERTY |
                           nsIXPCScriptable::WANT_DELPROPERTY |
                           nsIXPCScriptable::DONT_ENUM_STATIC_PROPS |
                           nsIXPCScriptable::WANT_NEWENUMERATE)

  NS_DEFINE_CLASSINFO_DATA(Blob, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(File, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(ModalContentWindow, nsWindowSH,
                           DEFAULT_SCRIPTABLE_FLAGS |
                           WINDOW_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozPowerManager, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozWakeLock, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozMobileMessageManager, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozSmsMessage, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozMmsMessage, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozSmsFilter, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozSmsSegmentInfo, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozMobileMessageThread, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozConnection, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

#ifdef MOZ_B2G_RIL
  NS_DEFINE_CLASSINFO_DATA(MozMobileConnection, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MozCellBroadcast, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(CSSFontFaceRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  
  NS_DEFINE_CLASSINFO_DATA(DataTransfer, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(EventListenerInfo, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(ContentFrameMessageManager, nsEventTargetSH,
                                       DOM_DEFAULT_SCRIPTABLE_FLAGS |
                                       nsIXPCScriptable::IS_GLOBAL_OBJECT)
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(ChromeMessageBroadcaster, nsDOMGenericSH,
                                       DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CHROME_ONLY_CLASSINFO_DATA(ChromeMessageSender, nsDOMGenericSH,
                                       DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(IDBRequest, IDBEventTargetSH,
                           IDBEVENTTARGET_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(IDBDatabase, IDBEventTargetSH,
                           IDBEVENTTARGET_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(IDBObjectStore, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(IDBCursor, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(IDBCursorWithValue, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(IDBKeyRange, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(IDBIndex, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(IDBOpenDBRequest, IDBEventTargetSH,
                           IDBEVENTTARGET_SCRIPTABLE_FLAGS)


  NS_DEFINE_CLASSINFO_DATA(MozCSSKeyframeRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MozCSSKeyframesRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(CSSPageRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(MediaQueryList, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

#ifdef MOZ_B2G_RIL
  NS_DEFINE_CLASSINFO_DATA(Telephony, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(TelephonyCall, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MozVoicemail, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(MozIccManager, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
#endif

#ifdef MOZ_B2G_FM
  NS_DEFINE_CLASSINFO_DATA(FMRadio, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)
#endif

#ifdef MOZ_B2G_BT
  NS_DEFINE_CLASSINFO_DATA(BluetoothManager, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(BluetoothAdapter, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(BluetoothDevice, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)
#endif

  NS_DEFINE_CLASSINFO_DATA(CameraControl, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CameraCapabilities, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(OpenWindowEventDetail, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(AsyncScrollEventDetail, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(LockedFile, nsEventTargetSH,
                           EVENTTARGET_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(CSSFontFeatureValuesRule, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)

  NS_DEFINE_CLASSINFO_DATA(UserDataHandler, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(GeoPositionError, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(LoadStatus, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XPathNamespace, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XULButtonElement, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XULCheckboxElement, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
  NS_DEFINE_CLASSINFO_DATA(XULPopupElement, nsDOMGenericSH,
                           DOM_DEFAULT_SCRIPTABLE_FLAGS)
};

#define NS_DEFINE_CONTRACT_CTOR(_class, _contract_id)                           \
  static nsresult                                                               \
  _class##Ctor(nsISupports** aInstancePtrResult)                                \
  {                                                                             \
    nsresult rv = NS_OK;                                                        \
    nsCOMPtr<nsISupports> native = do_CreateInstance(_contract_id, &rv);        \
    native.forget(aInstancePtrResult);                                          \
    return rv;                                                                  \
  }

NS_DEFINE_CONTRACT_CTOR(XSLTProcessor,
                        "@mozilla.org/document-transformer;1?type=xslt")

#undef NS_DEFINE_CONTRACT_CTOR

struct nsConstructorFuncMapData
{
  int32_t mDOMClassInfoID;
  nsDOMConstructorFunc mConstructorFunc;
};

#define NS_DEFINE_CONSTRUCTOR_FUNC_DATA(_class, _func)                        \
  { eDOMClassInfo_##_class##_id, _func },

static const nsConstructorFuncMapData kConstructorFuncMap[] =
{
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(Blob, nsDOMMultipartFile::NewBlob)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(File, nsDOMMultipartFile::NewFile)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(MozSmsFilter, SmsFilter::NewSmsFilter)
  NS_DEFINE_CONSTRUCTOR_FUNC_DATA(XSLTProcessor, XSLTProcessorCtor)
};
#undef NS_DEFINE_CONSTRUCTOR_FUNC_DATA

nsIXPConnect *nsDOMClassInfo::sXPConnect = nullptr;
nsIScriptSecurityManager *nsDOMClassInfo::sSecMan = nullptr;
bool nsDOMClassInfo::sIsInitialized = false;


jsid nsDOMClassInfo::sParent_id          = JSID_VOID;
jsid nsDOMClassInfo::sScrollbars_id      = JSID_VOID;
jsid nsDOMClassInfo::sLocation_id        = JSID_VOID;
jsid nsDOMClassInfo::sConstructor_id     = JSID_VOID;
jsid nsDOMClassInfo::s_content_id        = JSID_VOID;
jsid nsDOMClassInfo::sContent_id         = JSID_VOID;
jsid nsDOMClassInfo::sMenubar_id         = JSID_VOID;
jsid nsDOMClassInfo::sToolbar_id         = JSID_VOID;
jsid nsDOMClassInfo::sLocationbar_id     = JSID_VOID;
jsid nsDOMClassInfo::sPersonalbar_id     = JSID_VOID;
jsid nsDOMClassInfo::sStatusbar_id       = JSID_VOID;
jsid nsDOMClassInfo::sControllers_id     = JSID_VOID;
jsid nsDOMClassInfo::sLength_id          = JSID_VOID;
jsid nsDOMClassInfo::sScrollX_id         = JSID_VOID;
jsid nsDOMClassInfo::sScrollY_id         = JSID_VOID;
jsid nsDOMClassInfo::sScrollMaxX_id      = JSID_VOID;
jsid nsDOMClassInfo::sScrollMaxY_id      = JSID_VOID;
jsid nsDOMClassInfo::sItem_id            = JSID_VOID;
jsid nsDOMClassInfo::sNamedItem_id       = JSID_VOID;
jsid nsDOMClassInfo::sEnumerate_id       = JSID_VOID;
jsid nsDOMClassInfo::sTop_id             = JSID_VOID;
jsid nsDOMClassInfo::sDocument_id        = JSID_VOID;
jsid nsDOMClassInfo::sFrames_id          = JSID_VOID;
jsid nsDOMClassInfo::sSelf_id            = JSID_VOID;
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

static inline JSString *
IdToString(JSContext *cx, jsid id)
{
  if (JSID_IS_STRING(id))
    return JSID_TO_STRING(id);
  jsval idval;
  if (!::JS_IdToValue(cx, id, &idval))
    return nullptr;
  return JS_ValueToString(cx, idval);
}

static inline nsresult
WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
           nsWrapperCache *cache, const nsIID* aIID, jsval *vp,
           nsIXPConnectJSObjectHolder** aHolder, bool aAllowWrapping)
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

  return nsDOMClassInfo::XPConnect()->WrapNativeToJSVal(cx, scope, native,
                                                        cache, aIID,
                                                        aAllowWrapping, vp,
                                                        aHolder);
}

static inline nsresult
WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
           const nsIID* aIID, bool aAllowWrapping, jsval *vp,
           
           
           nsIXPConnectJSObjectHolder** aHolder = nullptr)
{
  return WrapNative(cx, scope, native, nullptr, aIID, vp, aHolder,
                    aAllowWrapping);
}


static inline nsresult
WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
           bool aAllowWrapping, jsval *vp,
           
           
           nsIXPConnectJSObjectHolder** aHolder = nullptr)
{
  return WrapNative(cx, scope, native, nullptr, nullptr, vp, aHolder,
                    aAllowWrapping);
}

static inline nsresult
WrapNative(JSContext *cx, JSObject *scope, nsISupports *native,
           nsWrapperCache *cache, bool aAllowWrapping, jsval *vp,
           
           
           nsIXPConnectJSObjectHolder** aHolder = nullptr)
{
  return WrapNative(cx, scope, native, cache, nullptr, vp, aHolder,
                    aAllowWrapping);
}





static inline nsresult
WrapNativeParent(JSContext *cx, JS::Handle<JSObject*> scope, nsISupports *native,
                 nsWrapperCache *nativeWrapperCache, JSObject **parentObj)
{
  
#ifdef DEBUG
  nsWrapperCache* cache = nullptr;
  CallQueryInterface(native, &cache);
  NS_PRECONDITION(nativeWrapperCache &&
                  cache == nativeWrapperCache, "What happened here?");
#endif

  JS::Rooted<JSObject*> obj(cx, nativeWrapperCache->GetWrapper());
  if (obj) {
#ifdef DEBUG
    JS::Rooted<JS::Value> debugVal(cx);
    nsresult rv = WrapNative(cx, scope, native, nativeWrapperCache, false,
                             debugVal.address());
    NS_ASSERTION(NS_SUCCEEDED(rv) && JSVAL_TO_OBJECT(debugVal) == obj,
                 "Unexpected object in nsWrapperCache");
#endif
    *parentObj = obj;
    return NS_OK;
  }

  JS::Rooted<JS::Value> v(cx);
  nsresult rv = WrapNative(cx, scope, native, nativeWrapperCache, false, v.address());
  NS_ENSURE_SUCCESS(rv, rv);
  *parentObj = v.toObjectOrNull();
  return NS_OK;
}

template<class P>
static inline nsresult
WrapNativeParent(JSContext *cx, JS::Handle<JSObject*> scope, P *parent,
                 JSObject **parentObj)
{
  return WrapNativeParent(cx, scope, ToSupports(parent), parent, parentObj);
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

  SET_JSID_TO_STRING(sParent_id,          cx, "parent");
  SET_JSID_TO_STRING(sScrollbars_id,      cx, "scrollbars");
  SET_JSID_TO_STRING(sLocation_id,        cx, "location");
  SET_JSID_TO_STRING(sConstructor_id,     cx, "constructor");
  SET_JSID_TO_STRING(s_content_id,        cx, "_content");
  SET_JSID_TO_STRING(sContent_id,         cx, "content");
  SET_JSID_TO_STRING(sMenubar_id,         cx, "menubar");
  SET_JSID_TO_STRING(sToolbar_id,         cx, "toolbar");
  SET_JSID_TO_STRING(sLocationbar_id,     cx, "locationbar");
  SET_JSID_TO_STRING(sPersonalbar_id,     cx, "personalbar");
  SET_JSID_TO_STRING(sStatusbar_id,       cx, "statusbar");
  SET_JSID_TO_STRING(sControllers_id,     cx, "controllers");
  SET_JSID_TO_STRING(sLength_id,          cx, "length");
  SET_JSID_TO_STRING(sScrollX_id,         cx, "scrollX");
  SET_JSID_TO_STRING(sScrollY_id,         cx, "scrollY");
  SET_JSID_TO_STRING(sScrollMaxX_id,      cx, "scrollMaxX");
  SET_JSID_TO_STRING(sScrollMaxY_id,      cx, "scrollMaxY");
  SET_JSID_TO_STRING(sItem_id,            cx, "item");
  SET_JSID_TO_STRING(sNamedItem_id,       cx, "namedItem");
  SET_JSID_TO_STRING(sEnumerate_id,       cx, "enumerateProperties");
  SET_JSID_TO_STRING(sTop_id,             cx, "top");
  SET_JSID_TO_STRING(sDocument_id,        cx, "document");
  SET_JSID_TO_STRING(sFrames_id,          cx, "frames");
  SET_JSID_TO_STRING(sSelf_id,            cx, "self");
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


static JSClass sDOMConstructorProtoClass = {
  "DOM Constructor.prototype", 0,
  JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nullptr
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
  nsScriptNameSpaceManager *nameSpaceManager =
    nsJSRuntime::GetNameSpaceManager();
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
  nsScriptNameSpaceManager *nameSpaceManager =
    nsJSRuntime::GetNameSpaceManager();
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
    nsMemory::Free(cid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return nameSpaceManager->RegisterExternalInterfaces(true);
}

#define _DOM_CLASSINFO_MAP_BEGIN(_class, _ifptr, _has_class_if, _disabled)    \
  {                                                                           \
    nsDOMClassInfoData &d = sClassInfoData[eDOMClassInfo_##_class##_id];      \
    d.mProtoChainInterface = _ifptr;                                          \
    d.mHasClassInterface = _has_class_if;                                     \
    d.mInterfacesBitmap = kDOMClassInfo_##_class##_interfaces;                \
    d.mDisabled = _disabled;                                                  \
    static const nsIID *interface_list[] = {

#define DOM_CLASSINFO_MAP_BEGIN(_class, _interface)                           \
  _DOM_CLASSINFO_MAP_BEGIN(_class, &NS_GET_IID(_interface), true, false)

#define DOM_CLASSINFO_MAP_BEGIN_MAYBE_DISABLE(_class, _interface, _disable)   \
  _DOM_CLASSINFO_MAP_BEGIN(_class, &NS_GET_IID(_interface), true, _disable)

#define DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(_class, _interface)               \
  _DOM_CLASSINFO_MAP_BEGIN(_class, &NS_GET_IID(_interface), false, false)

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

#ifdef MOZ_B2G
#define DOM_CLASSINFO_WINDOW_MAP_ENTRIES(_support_indexed_db)                  \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindow)                                        \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindowB2G)                                     \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMJSWindow)                                      \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)                                   \
  DOM_CLASSINFO_MAP_ENTRY(nsIInlineEventHandlers)                              \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindowPerformance)                             \
  DOM_CLASSINFO_MAP_CONDITIONAL_ENTRY(nsIDOMStorageIndexedDB,                  \
                                      _support_indexed_db)                     \
  DOM_CLASSINFO_MAP_CONDITIONAL_ENTRY(nsITouchEventReceiver,                   \
                                      nsDOMTouchEvent::PrefEnabled())
#else 
#define DOM_CLASSINFO_WINDOW_MAP_ENTRIES(_support_indexed_db)                  \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindow)                                        \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMJSWindow)                                      \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)                                   \
  DOM_CLASSINFO_MAP_ENTRY(nsIInlineEventHandlers)                              \
  DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindowPerformance)                             \
  DOM_CLASSINFO_MAP_CONDITIONAL_ENTRY(nsIDOMStorageIndexedDB,                  \
                                      _support_indexed_db)                     \
  DOM_CLASSINFO_MAP_CONDITIONAL_ENTRY(nsITouchEventReceiver,                   \
                                      nsDOMTouchEvent::PrefEnabled())
#endif 

nsresult
nsDOMClassInfo::Init()
{
  

  static_assert(sizeof(uintptr_t) == sizeof(void*),
                "BAD! You'll need to adjust the size of uintptr_t to the "
                "size of a pointer on your platform.");

  NS_ENSURE_TRUE(!sIsInitialized, NS_ERROR_ALREADY_INITIALIZED);

  nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = CallGetService(nsIXPConnect::GetCID(), &sXPConnect);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXPCFunctionThisTranslator> elt = new nsEventListenerThisTranslator();
  sXPConnect->SetFunctionThisTranslator(NS_GET_IID(nsIDOMEventListener), elt);

  nsCOMPtr<nsIScriptSecurityManager> sm =
    do_GetService("@mozilla.org/scriptsecuritymanager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sSecMan = sm;
  NS_ADDREF(sSecMan);

  AutoSafeJSContext cx;

  DOM_CLASSINFO_MAP_BEGIN(Window, nsIDOMWindow)
    DOM_CLASSINFO_WINDOW_MAP_ENTRIES(nsGlobalWindow::HasIndexedDBSupport())
#ifdef MOZ_WEBSPEECH
    DOM_CLASSINFO_MAP_ENTRY(nsISpeechSynthesisGetter)
#endif
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(WindowUtils, nsIDOMWindowUtils)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMWindowUtils)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Location, nsIDOMLocation)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLocation)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(History, nsIDOMHistory)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMHistory)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(DOMPrototype, nsIDOMDOMConstructor)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMConstructor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DOMConstructor, nsIDOMDOMConstructor)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMConstructor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DOMException, nsIDOMDOMException)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMException)
    DOM_CLASSINFO_MAP_ENTRY(nsIException)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DeviceAcceleration, nsIDOMDeviceAcceleration)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDeviceAcceleration)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DeviceRotationRate, nsIDOMDeviceRotationRate)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDeviceRotationRate)
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

  DOM_CLASSINFO_MAP_BEGIN(CSSRuleList, nsIDOMCSSRuleList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSRuleList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(CSSGroupRuleRuleList, nsIDOMCSSRuleList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSRuleList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MediaList, nsIDOMMediaList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMediaList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(StyleSheetList, nsIDOMStyleSheetList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStyleSheetList)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSStyleSheet, nsIDOMCSSStyleSheet)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSStyleSheet)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Selection, nsISelection)
    DOM_CLASSINFO_MAP_ENTRY(nsISelection)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(XULCommandDispatcher, nsIDOMXULCommandDispatcher)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXULCommandDispatcher)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XULControllers, nsIControllers)
    DOM_CLASSINFO_MAP_ENTRY(nsIControllers)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(BoxObject, nsIBoxObject)
    DOM_CLASSINFO_MAP_ENTRY(nsIBoxObject)
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

#ifndef MOZ_DISABLE_CRYPTOLEGACY
   DOM_CLASSINFO_MAP_BEGIN(CRMFObject, nsIDOMCRMFObject)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMCRMFObject)
   DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(Crypto, nsIDOMCrypto)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCrypto)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ChromeWindow, nsIDOMWindow)
    DOM_CLASSINFO_WINDOW_MAP_ENTRIES(true)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMChromeWindow)
#ifdef MOZ_WEBSPEECH
    DOM_CLASSINFO_MAP_ENTRY(nsISpeechSynthesisGetter)
#endif
  DOM_CLASSINFO_MAP_END

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

  DOM_CLASSINFO_MAP_BEGIN(DOMStringList, nsIDOMDOMStringList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDOMStringList)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_XUL
  DOM_CLASSINFO_MAP_BEGIN(TreeColumn, nsITreeColumn)
    DOM_CLASSINFO_MAP_ENTRY(nsITreeColumn)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(CSSMozDocumentRule, nsIDOMCSSMozDocumentRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSMozDocumentRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSSupportsRule, nsIDOMCSSSupportsRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSSupportsRule)
  DOM_CLASSINFO_MAP_END

  

  
  DOM_CLASSINFO_MAP_BEGIN(SVGLength, nsIDOMSVGLength)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGLength)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(SVGNumber, nsIDOMSVGNumber)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMSVGNumber)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozCanvasPrintState, nsIDOMMozCanvasPrintState)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozCanvasPrintState)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XSLTProcessor, nsIXSLTProcessor)
    DOM_CLASSINFO_MAP_ENTRY(nsIXSLTProcessor)
    DOM_CLASSINFO_MAP_ENTRY(nsIXSLTProcessorPrivate)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XPathExpression, nsIDOMXPathExpression)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathExpression)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMNSXPathExpression)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XPathNSResolver, nsIDOMXPathNSResolver)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathNSResolver)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(XPathResult, nsIDOMXPathResult)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathResult)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Storage, nsIDOMStorage)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMStorage)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(Blob, nsIDOMBlob)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMBlob)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(File, nsIDOMFile)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMBlob)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMFile)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ModalContentWindow, nsIDOMWindow)
    DOM_CLASSINFO_WINDOW_MAP_ENTRIES(nsGlobalWindow::HasIndexedDBSupport())
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMModalContentWindow)
#ifdef MOZ_WEBSPEECH
    DOM_CLASSINFO_MAP_ENTRY(nsISpeechSynthesisGetter)
#endif
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozPowerManager, nsIDOMMozPowerManager)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozPowerManager)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozWakeLock, nsIDOMMozWakeLock)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozWakeLock)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozMobileMessageManager, nsIDOMMozMobileMessageManager)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozMobileMessageManager)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozSmsMessage, nsIDOMMozSmsMessage)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozSmsMessage)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozMmsMessage, nsIDOMMozMmsMessage)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozMmsMessage)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozSmsFilter, nsIDOMMozSmsFilter)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozSmsFilter)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozSmsSegmentInfo, nsIDOMMozSmsSegmentInfo)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozSmsSegmentInfo)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozMobileMessageThread, nsIDOMMozMobileMessageThread)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozMobileMessageThread)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozConnection, nsIDOMMozConnection)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozConnection)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_B2G_RIL
  DOM_CLASSINFO_MAP_BEGIN(MozMobileConnection, nsIDOMMozMobileConnection)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozMobileConnection)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozCellBroadcast, nsIDOMMozCellBroadcast)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozCellBroadcast)
     DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END
#endif 

  DOM_CLASSINFO_MAP_BEGIN(CSSFontFaceRule, nsIDOMCSSFontFaceRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSFontFaceRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(DataTransfer, nsIDOMDataTransfer)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMDataTransfer)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(EventListenerInfo, nsIEventListenerInfo)
    DOM_CLASSINFO_MAP_ENTRY(nsIEventListenerInfo)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ContentFrameMessageManager, nsISupports)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageListenerManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageSender)
    DOM_CLASSINFO_MAP_ENTRY(nsISyncMessageSender)
    DOM_CLASSINFO_MAP_ENTRY(nsIContentFrameMessageManager)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ChromeMessageBroadcaster, nsISupports)
    DOM_CLASSINFO_MAP_ENTRY(nsIFrameScriptLoader)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageListenerManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageBroadcaster)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(ChromeMessageSender, nsISupports)
    DOM_CLASSINFO_MAP_ENTRY(nsIProcessChecker)
    DOM_CLASSINFO_MAP_ENTRY(nsIFrameScriptLoader)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageListenerManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIMessageSender)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(IDBRequest, nsIIDBRequest)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBRequest)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(IDBDatabase, nsIIDBDatabase)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBDatabase)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(IDBObjectStore, nsIIDBObjectStore)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBObjectStore)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(IDBCursor, nsIIDBCursor)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBCursor)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(IDBCursorWithValue, nsIIDBCursorWithValue)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBCursor)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBCursorWithValue)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(IDBKeyRange, nsIIDBKeyRange)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBKeyRange)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(IDBIndex, nsIIDBIndex)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBIndex)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(IDBOpenDBRequest, nsIIDBOpenDBRequest)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBOpenDBRequest)
    DOM_CLASSINFO_MAP_ENTRY(nsIIDBRequest)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozCSSKeyframeRule, nsIDOMMozCSSKeyframeRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozCSSKeyframeRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozCSSKeyframesRule, nsIDOMMozCSSKeyframesRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozCSSKeyframesRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSPageRule, nsIDOMCSSPageRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSPageRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MediaQueryList, nsIDOMMediaQueryList)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMediaQueryList)
  DOM_CLASSINFO_MAP_END

#ifdef MOZ_B2G_RIL
  DOM_CLASSINFO_MAP_BEGIN(Telephony, nsIDOMTelephony)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMTelephony)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(TelephonyCall, nsIDOMTelephonyCall)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMTelephonyCall)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozVoicemail, nsIDOMMozVoicemail)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozVoicemail)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(MozIccManager, nsIDOMMozIccManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMMozIccManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END

#endif

#ifdef MOZ_B2G_FM
  DOM_CLASSINFO_MAP_BEGIN(FMRadio, nsIFMRadio)
    DOM_CLASSINFO_MAP_ENTRY(nsIFMRadio)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  DOM_CLASSINFO_MAP_END
#endif

#ifdef MOZ_B2G_BT
  DOM_CLASSINFO_MAP_BEGIN(BluetoothManager, nsIDOMBluetoothManager)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMBluetoothManager)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(BluetoothAdapter, nsIDOMBluetoothAdapter)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMBluetoothAdapter)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(BluetoothDevice, nsIDOMBluetoothDevice)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMBluetoothDevice)
  DOM_CLASSINFO_MAP_END
#endif

  DOM_CLASSINFO_MAP_BEGIN(CameraControl, nsICameraControl)
    DOM_CLASSINFO_MAP_ENTRY(nsICameraControl)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CameraCapabilities, nsICameraCapabilities)
    DOM_CLASSINFO_MAP_ENTRY(nsICameraCapabilities)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(OpenWindowEventDetail, nsIOpenWindowEventDetail)
    DOM_CLASSINFO_MAP_ENTRY(nsIOpenWindowEventDetail)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(AsyncScrollEventDetail, nsIAsyncScrollEventDetail)
    DOM_CLASSINFO_MAP_ENTRY(nsIAsyncScrollEventDetail)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(LockedFile, nsIDOMLockedFile)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLockedFile)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(CSSFontFeatureValuesRule, nsIDOMCSSFontFeatureValuesRule)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMCSSFontFeatureValuesRule)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(UserDataHandler, nsIDOMUserDataHandler)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMUserDataHandler)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN(GeoPositionError, nsIDOMGeoPositionError)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMGeoPositionError)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(LoadStatus, nsIDOMLoadStatus)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMLoadStatus)
  DOM_CLASSINFO_MAP_END

  DOM_CLASSINFO_MAP_BEGIN_NO_CLASS_IF(XPathNamespace, nsIDOMXPathNamespace)
    DOM_CLASSINFO_MAP_ENTRY(nsIDOMXPathNamespace)
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
    nsDOMClassInfoData& data = sClassInfoData[i];
    nameSpaceManager->RegisterClassName(data.mName, i, data.mChromeOnly,
                                        data.mDisabled, &data.mNameUTF16);
  }

  for (i = 0; i < eDOMClassInfoIDCount; ++i) {
    RegisterClassProtos(i);
  }

  RegisterExternalClasses();

  
  mozilla::dom::Register(nameSpaceManager);

  sIsInitialized = true;

  return NS_OK;
}


int32_t
nsDOMClassInfo::GetArrayIndexFromId(JSContext *cx, JS::Handle<jsid> id, bool *aIsNumber)
{
  if (aIsNumber) {
    *aIsNumber = false;
  }

  int i;
  if (JSID_IS_INT(id)) {
      i = JSID_TO_INT(id);
  } else {
      jsval idval;
      double array_index;
      if (!::JS_IdToValue(cx, id, &idval) ||
          !::JS_ValueToNumber(cx, idval, &array_index) ||
          !::JS_DoubleIsInt32(array_index, &i)) {
        return -1;
      }
  }

  if (aIsNumber) {
    *aIsNumber = true;
  }

  return i;
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

  *aArray = static_cast<nsIID **>(nsMemory::Alloc(count * sizeof(nsIID *)));
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
nsDOMClassInfo::GetHelperForLanguage(uint32_t language, nsISupports **_retval)
{
  if (language == nsIProgrammingLanguage::JAVASCRIPT) {
    *_retval = static_cast<nsIXPCScriptable *>(this);

    NS_ADDREF(*_retval);
  } else {
    *_retval = nullptr;
  }

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
nsDOMClassInfo::GetImplementationLanguage(uint32_t *aImplLanguage)
{
  *aImplLanguage = nsIProgrammingLanguage::CPLUSPLUS;

  return NS_OK;
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
nsDOMClassInfo::Create(nsIXPConnectWrappedNative *wrapper,
                       JSContext *cx, JSObject *obj)
{
  NS_WARNING("nsDOMClassInfo::Create Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::PostCreate(nsIXPConnectWrappedNative *wrapper,
                           JSContext *cx, JSObject *obj)
{
  NS_WARNING("nsDOMClassInfo::PostCreate Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::PostTransplant(nsIXPConnectWrappedNative *wrapper,
                               JSContext *cx, JSObject *obj)
{
  MOZ_CRASH("nsDOMClassInfo::PostTransplant Don't call me!");
}

NS_IMETHODIMP
nsDOMClassInfo::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsid id, jsval *vp,
                            bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::AddProperty Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::DelProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsid id, bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::DelProperty Don't call me!");

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
#ifdef DEBUG
  if (!sSecMan) {
    NS_ERROR("No security manager!!!");
    return NS_OK;
  }

  
  nsresult rv =
    sSecMan->CheckPropertyAccess(cx, obj, mData->mName, sEnumerate_id,
                                 nsIXPCSecurityManager::ACCESS_GET_PROPERTY);

  NS_ASSERTION(NS_SUCCEEDED(rv),
               "XOWs should have stopped us from getting here!!!");
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::NewEnumerate(nsIXPConnectWrappedNative *wrapper,
                             JSContext *cx, JSObject *obj, uint32_t enum_op,
                             jsval *statep, jsid *idp, bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::NewEnumerate Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

nsresult
nsDOMClassInfo::ResolveConstructor(JSContext *cx, JSObject *aObj,
                                   JSObject **objp)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  JS::Rooted<JSObject*> global(cx, ::JS_GetGlobalForObject(cx, obj));

  JS::Rooted<JS::Value> val(cx);
  if (!::JS_LookupProperty(cx, global, mData->mName, val.address())) {
    return NS_ERROR_UNEXPECTED;
  }

  if (!JSVAL_IS_PRIMITIVE(val)) {
    
    
    
    

    if (!::JS_DefinePropertyById(cx, obj, sConstructor_id, val, JS_PropertyStub,
                                 JS_StrictPropertyStub, JSPROP_ENUMERATE)) {
      return NS_ERROR_UNEXPECTED;
    }

    *objp = obj;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, jsid id, uint32_t flags,
                           JSObject **objp, bool *_retval)
{
  if (id == sConstructor_id) {
    return ResolveConstructor(cx, obj, objp);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMClassInfo::Convert(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                        JSObject *obj, uint32_t type, jsval *vp,
                        bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::Convert Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::Finalize(nsIXPConnectWrappedNative *wrapper, JSFreeOp *fop,
                         JSObject *obj)
{
  NS_WARNING("nsDOMClassInfo::Finalize Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *obj, jsid aId, uint32_t mode,
                            jsval *vp, bool *_retval)
{
  JS::Rooted<jsid> id(cx, aId);
  uint32_t mode_type = mode & JSACC_TYPEMASK;

  if ((mode_type == JSACC_WATCH || mode_type == JSACC_PROTO) && sSecMan) {
    nsresult rv;
    JS::Rooted<JSObject*> real_obj(cx);
    if (wrapper) {
      real_obj = wrapper->GetJSObject();
      NS_ENSURE_STATE(real_obj);
    }
    else {
      real_obj = obj;
    }

    rv =
      sSecMan->CheckPropertyAccess(cx, real_obj, mData->mName, id,
                                   nsIXPCSecurityManager::ACCESS_GET_PROPERTY);

    if (NS_FAILED(rv)) {
      
      *_retval = false;
    }
  }

  return NS_OK;
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
                            JSObject *obj, const jsval &val, bool *bp,
                            bool *_retval)
{
  NS_WARNING("nsDOMClassInfo::HasInstance Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsDOMClassInfo::OuterObject(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                            JSObject * obj, JSObject * *_retval)
{
  NS_WARNING("nsDOMClassInfo::OuterObject Don't call me!");

  return NS_ERROR_UNEXPECTED;
}

static nsresult
GetExternalClassInfo(nsScriptNameSpaceManager *aNameSpaceManager,
                     const nsString &aName,
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
                 JS::Handle<JSObject*> obj, const PRUnichar *name,
                 const nsDOMClassInfoData *ci_data,
                 const nsGlobalNameStruct *name_struct,
                 nsScriptNameSpaceManager *nameSpaceManager,
                 JSObject *dot_prototype, bool install, bool *did_resolve);


NS_IMETHODIMP
nsDOMClassInfo::PostCreatePrototype(JSContext * cx, JSObject * aProto)
{
  uint32_t flags = (mData->mScriptableFlags & DONT_ENUM_STATIC_PROPS)
                   ? 0
                   : JSPROP_ENUMERATE;

  uint32_t count = 0;
  while (mData->mInterfaces[count]) {
    count++;
  }

  JS::Rooted<JSObject*> proto(cx, aProto);
  if (!xpc::DOM_DefineQuickStubs(cx, proto, flags, count, mData->mInterfaces)) {
    JS_ClearPendingException(cx);
  }

  
  
  
  if (!sObjectClass) {
    FindObjectClass(cx, proto);
    NS_ASSERTION(sObjectClass && !strcmp(sObjectClass->name, "Object"),
                 "Incorrect object class!");
  }

#ifdef DEBUG
    JS::Rooted<JSObject*> proto2(cx);
    JS_GetPrototype(cx, proto, proto2.address());
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

  
  JSBool found;
  if (!::JS_AlreadyHasOwnUCProperty(cx, global, reinterpret_cast<const jschar*>(mData->mNameUTF16),
                                    NS_strlen(mData->mNameUTF16), &found)) {
    return NS_ERROR_FAILURE;
  }

  nsScriptNameSpaceManager *nameSpaceManager =
    nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_OK);

  bool unused;
  return ResolvePrototype(sXPConnect, win, cx, global, mData->mNameUTF16,
                          mData, nullptr, nameSpaceManager, proto, !found,
                          &unused);
}


nsIClassInfo *
NS_GetDOMClassInfoInstance(nsDOMClassInfoID aID)
{
  if (aID >= eDOMClassInfoIDCount) {
    NS_ERROR("Bad ID!");

    return nullptr;
  }

  if (!nsDOMClassInfo::sIsInitialized) {
    nsresult rv = nsDOMClassInfo::Init();

    NS_ENSURE_SUCCESS(rv, nullptr);
  }

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

  sParent_id          = JSID_VOID;
  sScrollbars_id      = JSID_VOID;
  sLocation_id        = JSID_VOID;
  sConstructor_id     = JSID_VOID;
  s_content_id        = JSID_VOID;
  sContent_id         = JSID_VOID;
  sMenubar_id         = JSID_VOID;
  sToolbar_id         = JSID_VOID;
  sLocationbar_id     = JSID_VOID;
  sPersonalbar_id     = JSID_VOID;
  sStatusbar_id       = JSID_VOID;
  sControllers_id     = JSID_VOID;
  sLength_id          = JSID_VOID;
  sScrollX_id         = JSID_VOID;
  sScrollY_id         = JSID_VOID;
  sScrollMaxX_id      = JSID_VOID;
  sScrollMaxY_id      = JSID_VOID;
  sItem_id            = JSID_VOID;
  sEnumerate_id       = JSID_VOID;
  sTop_id             = JSID_VOID;
  sDocument_id        = JSID_VOID;
  sFrames_id          = JSID_VOID;
  sSelf_id            = JSID_VOID;
  sWrappedJSObject_id = JSID_VOID;

  NS_IF_RELEASE(sXPConnect);
  NS_IF_RELEASE(sSecMan);
  sIsInitialized = false;
}



NS_IMETHODIMP
nsWindowSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                      JSObject *globalObj, JSObject **parentObj)
{
  
  
  
  
  
  
  
  
  
  
  

  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(nativeObj));
  NS_ASSERTION(sgo, "nativeObj not a global object!");

  nsGlobalWindow *win = nsGlobalWindow::FromSupports(nativeObj);
  NS_ASSERTION(win->IsInnerWindow(), "Should be inner window.");

  
  if (!win->GetOuterWindowInternal())
    return NS_ERROR_FAILURE;

  
  if (win->GetOuterWindowInternal()->IsCreatingInnerWindow())
    return NS_OK;

  return SetParentToWindow(win, parentObj);
}

static JSClass sGlobalScopePolluterClass = {
  "Global Scope Polluter",
  JSCLASS_NEW_RESOLVE,
  JS_PropertyStub,
  JS_DeletePropertyStub,
  nsWindowSH::GlobalScopePolluterGetProperty,
  JS_StrictPropertyStub,
  JS_EnumerateStub,
  (JSResolveOp)nsWindowSH::GlobalScopePolluterNewResolve,
  JS_ConvertStub,
  nullptr
};



JSBool
nsWindowSH::GlobalScopePolluterGetProperty(JSContext *cx, JS::Handle<JSObject*> obj,
                                           JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp)
{
  
  

  nsresult rv =
    sSecMan->CheckPropertyAccess(cx, ::JS_GetGlobalForObject(cx, obj),
                                 "Window", id,
                                 nsIXPCSecurityManager::ACCESS_GET_PROPERTY);

  if (NS_FAILED(rv)) {
    
    

    return JS_FALSE;
  }

  return JS_TRUE;
}


static JSBool
ChildWindowGetter(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                  JS::MutableHandle<JS::Value> vp)
{
  MOZ_ASSERT(JSID_IS_STRING(id));
  
  vp.setUndefined();
  nsCOMPtr<nsISupports> winSupports =
    do_QueryInterface(nsDOMClassInfo::XPConnect()->GetNativeOfWrapper(cx, obj));
  if (!winSupports)
    return true;
  nsGlobalWindow *win = nsGlobalWindow::FromSupports(winSupports);

  
  nsDependentJSString name(id);
  nsCOMPtr<nsIDOMWindow> child = win->GetChildWindow(name);
  if (!child)
    return true;

  
  JS::Rooted<JS::Value> v(cx);
  nsresult rv = WrapNative(cx, JS::CurrentGlobalOrNull(cx), child,
                            true, v.address());
  NS_ENSURE_SUCCESS(rv, false);
  vp.set(v);
  return true;
}

static nsHTMLDocument*
GetDocument(JSObject *obj)
{
  MOZ_ASSERT(js::GetObjectJSClass(obj) == &sHTMLDocumentAllClass);
  return static_cast<nsHTMLDocument*>(
    static_cast<nsINode*>(JS_GetPrivate(obj)));
}


JSBool
nsWindowSH::GlobalScopePolluterNewResolve(JSContext *cx, JS::Handle<JSObject*> obj,
                                          JS::Handle<jsid> id, unsigned flags,
                                          JS::MutableHandle<JSObject*> objp)
{
  if (!JSID_IS_STRING(id)) {
    
    return JS_TRUE;
  }

  
  
  nsIXPConnect *xpc = XPConnect();
  NS_ENSURE_TRUE(xpc, true);

  
  JSObject *global = JS_GetGlobalForObject(cx, obj);
  nsISupports *globalNative = xpc->GetNativeOfWrapper(cx, global);
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(globalNative);
  MOZ_ASSERT(piWin);
  nsGlobalWindow* win = static_cast<nsGlobalWindow*>(piWin.get());

  if (win->GetLength() > 0) {
    nsDependentJSString name(id);
    nsCOMPtr<nsIDOMWindow> child_win = win->GetChildWindow(name);
    if (child_win) {
      
      
      
      
      if (!JS_DefinePropertyById(cx, obj, id, JS::UndefinedValue(),
                                 ChildWindowGetter, JS_StrictPropertyStub,
                                 JSPROP_SHARED | JSPROP_ENUMERATE))
      {
        return false;
      }

      objp.set(obj);
      return true;
    }
  }

  JS::Rooted<JSObject*> proto(cx);
  if (!::JS_GetPrototype(cx, obj, proto.address())) {
    return JS_FALSE;
  }
  JSBool hasProp;

  if (!proto || !::JS_HasPropertyById(cx, proto, id, &hasProp) ||
      hasProp) {
    
    

    return JS_TRUE;
  }

  
  
  
  nsCOMPtr<nsIHTMLDocument> htmlDoc =
    do_QueryInterface(win->GetExtantDoc());
  if (!htmlDoc)
    return true;
  nsHTMLDocument *document = static_cast<nsHTMLDocument*>(htmlDoc.get());

  nsDependentJSString str(id);
  nsCOMPtr<nsISupports> result;
  nsWrapperCache *cache;
  {
    Element *element = document->GetElementById(str);
    result = element;
    cache = element;
  }

  if (!result) {
    result = document->ResolveName(str, &cache);
  }

  if (result) {
    JS::Rooted<JS::Value> v(cx);
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult rv = WrapNative(cx, obj, result, cache, true, v.address(),
                             getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, JS_FALSE);

    if (!JS_WrapValue(cx, v.address()) ||
        !JS_DefinePropertyById(cx, obj, id, v, JS_PropertyStub, JS_StrictPropertyStub, 0)) {
      return JS_FALSE;
    }

    objp.set(obj);
  }

  return JS_TRUE;
}


JSBool
nsWindowSH::InvalidateGlobalScopePolluter(JSContext *cx,
                                          JS::Handle<JSObject*> aObj)
{
  JS::Rooted<JSObject*> proto(cx);
  JS::Rooted<JSObject*> obj(cx, aObj);

  for (;;) {
    if (!::JS_GetPrototype(cx, obj, proto.address())) {
      return JS_FALSE;
    }
    if (!proto) {
      break;
    }

    if (JS_GetClass(proto) == &sGlobalScopePolluterClass) {

      JS::Rooted<JSObject*> proto_proto(cx);
      if (!::JS_GetPrototype(cx, proto, proto_proto.address())) {
        return JS_FALSE;
      }

      
      
      ::JS_SplicePrototype(cx, obj, proto_proto);

      break;
    }

    obj = proto;
  }

  return JS_TRUE;
}


nsresult
nsWindowSH::InstallGlobalScopePolluter(JSContext *cx, JS::Handle<JSObject*> obj)
{
  JS::Rooted<JSObject*> gsp(cx, ::JS_NewObjectWithUniqueType(cx, &sGlobalScopePolluterClass, nullptr, obj));
  if (!gsp) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  JS::Rooted<JSObject*> o(cx, obj), proto(cx);

  
  

  for (;;) {
    if (!::JS_GetPrototype(cx, o, proto.address())) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!proto) {
      break;
    }
    if (JS_GetClass(proto) == sObjectClass) {
      
      ::JS_SplicePrototype(cx, gsp, proto);

      break;
    }

    o = proto;
  }

  
  
  ::JS_SplicePrototype(cx, o, gsp);

  return NS_OK;
}

struct ResolveGlobalNameClosure
{
  JSContext* cx;
  JS::Handle<JSObject*> obj;
  bool* retval;
};

static PLDHashOperator
ResolveGlobalName(const nsAString& aName, void* aClosure)
{
  ResolveGlobalNameClosure* closure =
    static_cast<ResolveGlobalNameClosure*>(aClosure);
  JS::Rooted<JS::Value> dummy(closure->cx);
  bool ok = JS_LookupUCProperty(closure->cx, closure->obj,
                                aName.BeginReading(), aName.Length(),
                                dummy.address());
  if (!ok) {
    *closure->retval = false;
    return PL_DHASH_STOP;
  }
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsWindowSH::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                      JSObject *aObj, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  if (!xpc::WrapperFactory::IsXrayWrapper(obj)) {
    *_retval = JS_EnumerateStandardClasses(cx, obj);
    if (!*_retval) {
      return NS_OK;
    }

    
    nsScriptNameSpaceManager *nameSpaceManager =
      nsJSRuntime::GetNameSpaceManager();
    if (!nameSpaceManager) {
      NS_ERROR("Can't get namespace manager.");
      return NS_ERROR_UNEXPECTED;
    }
    ResolveGlobalNameClosure closure = { cx, obj, _retval };
    nameSpaceManager->EnumerateGlobalNames(ResolveGlobalName, &closure);
  }

  return NS_OK;
}

static nsDOMConstructorFunc
FindConstructorFunc(const nsDOMClassInfoData *aDOMClassInfoData)
{
  for (uint32_t i = 0; i < ArrayLength(kConstructorFuncMap); ++i) {
    if (&sClassInfoData[kConstructorFuncMap[i].mDOMClassInfoID] ==
        aDOMClassInfoData) {
      return kConstructorFuncMap[i].mConstructorFunc;
    }
  }
  return nullptr;
}

static nsresult
BaseStubConstructor(nsIWeakReference* aWeakOwner,
                    const nsGlobalNameStruct *name_struct, JSContext *cx,
                    JS::Handle<JSObject*> obj, const JS::CallArgs &args)
{
  MOZ_ASSERT(obj);

  nsresult rv;
  nsCOMPtr<nsISupports> native;
  if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    const nsDOMClassInfoData* ci_data =
      &sClassInfoData[name_struct->mDOMClassInfoID];
    nsDOMConstructorFunc func = FindConstructorFunc(ci_data);
    if (func) {
      rv = func(getter_AddRefs(native));
    } else {
      rv = NS_ERROR_NOT_AVAILABLE;
    }
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

  nsCOMPtr<nsIJSNativeInitializer> initializer(do_QueryInterface(native));
  nsCOMPtr<nsIDOMGlobalObjectConstructor> constructor(do_QueryInterface(native));
  if (initializer || constructor) {
    
    
    nsCOMPtr<nsPIDOMWindow> owner = do_QueryReferent(aWeakOwner);
    nsPIDOMWindow* outerWindow = owner ? owner->GetOuterWindow() : nullptr;
    nsPIDOMWindow* currentInner =
      outerWindow ? outerWindow->GetCurrentInnerWindow() : nullptr;
    if (!currentInner ||
        (owner != currentInner &&
         !nsContentUtils::CanCallerAccess(currentInner))) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    if (initializer) {
      rv = initializer->Initialize(currentInner, cx, obj, args);
      if (NS_FAILED(rv)) {
        return rv;
      }
    } else {
      nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS = do_QueryInterface(native);

      JS::Rooted<JSObject*> thisObject(cx, wrappedJS->GetJSObject());
      if (!thisObject) {
        return NS_ERROR_UNEXPECTED;
      }

      nsCxPusher pusher;
      pusher.Push(cx);

      JSAutoCompartment ac(cx, thisObject);

      JS::Rooted<JS::Value> funval(cx);
      if (!JS_GetProperty(cx, thisObject, "constructor", &funval) ||
          !funval.isObject()) {
        return NS_ERROR_UNEXPECTED;
      }

      
      NS_ENSURE_STATE(JS_ObjectIsCallable(cx, &funval.toObject()));
      {
        
        
        unsigned argc = args.length() + 1;
        nsAutoArrayPtr<JS::Value> argv(new JS::Value[argc]);
        JS::AutoArrayRooter rooter(cx, 0, argv);

        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        nsCOMPtr<nsIDOMWindow> currentWin(do_GetInterface(currentInner));
        rv = WrapNative(cx, obj, currentWin, &NS_GET_IID(nsIDOMWindow),
                        true, &argv[0], getter_AddRefs(holder));
        if (!JS_WrapValue(cx, &argv[0]))
          return NS_ERROR_FAILURE;
        rooter.changeLength(1);

        for (size_t i = 1; i < argc; ++i) {
          argv[i] = args[i - 1];
          if (!JS_WrapValue(cx, &argv[i]))
            return NS_ERROR_FAILURE;
          rooter.changeLength(i + 1);
        }

        JS::Rooted<JS::Value> frval(cx);
        bool ret = JS_CallFunctionValue(cx, thisObject, funval, argc, argv,
                                        frval.address());

        if (!ret) {
          return NS_ERROR_FAILURE;
        }
      }
    }
  }

  return WrapNative(cx, obj, native, true, args.rval().address());
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

  for (i = parent_constant_count; i < constant_count; i++) {
    const nsXPTConstant *c = nullptr;

    rv = if_info->GetConstant(i, &c);
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && c, rv);

    uint16_t type = c->GetType().TagPart();

    jsval v;
    switch (type) {
      case nsXPTType::T_I8:
      case nsXPTType::T_U8:
      {
        v = INT_TO_JSVAL(c->GetValue()->val.u8);
        break;
      }
      case nsXPTType::T_I16:
      case nsXPTType::T_U16:
      {
        v = INT_TO_JSVAL(c->GetValue()->val.u16);
        break;
      }
      case nsXPTType::T_I32:
      {
        v = JS_NumberValue(c->GetValue()->val.i32);
        break;
      }
      case nsXPTType::T_U32:
      {
        v = JS_NumberValue(c->GetValue()->val.u32);
        break;
      }
      default:
      {
#ifdef DEBUG
        NS_ERROR("Non-numeric constant found in interface.");
#endif
        continue;
      }
    }

    if (!::JS_DefineProperty(cx, obj, c->GetName(), v,
                             JS_PropertyStub, JS_StrictPropertyStub,
                             JSPROP_ENUMERATE | JSPROP_READONLY |
                             JSPROP_PERMANENT)) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
}




struct IDBConstant
{
  const char* interface;
  const char* name;
  const char* value;

  static const char* IDBCursor;
  static const char* IDBRequest;
};

const char* IDBConstant::IDBCursor = "IDBCursor";
const char* IDBConstant::IDBRequest = "IDBRequest";

static const IDBConstant sIDBConstants[] = {
  { IDBConstant::IDBCursor,  "NEXT",              "next" },
  { IDBConstant::IDBCursor,  "NEXT_NO_DUPLICATE", "nextunique" },
  { IDBConstant::IDBCursor,  "PREV",              "prev" },
  { IDBConstant::IDBCursor,  "PREV_NO_DUPLICATE", "prevunique" },
  { IDBConstant::IDBRequest, "LOADING",           "pending" },
  { IDBConstant::IDBRequest, "DONE",              "done" },
};

static JSBool
IDBConstantGetter(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                  JS::MutableHandle<JS::Value> vp)
{
  JSString *idstr = JSID_TO_STRING(id);
  unsigned index;
  for (index = 0; index < mozilla::ArrayLength(sIDBConstants); index++) {
    JSBool match;
    if (!JS_StringEqualsAscii(cx, idstr, sIDBConstants[index].name, &match)) {
      return JS_FALSE;
    }
    if (match) {
      break;
    }
  }
  MOZ_ASSERT(index < mozilla::ArrayLength(sIDBConstants));

  const IDBConstant& c = sIDBConstants[index];

  
  nsString warnText =
    NS_LITERAL_STRING("The constant ") +
    NS_ConvertASCIItoUTF16(c.interface) +
    NS_LITERAL_STRING(".") +
    NS_ConvertASCIItoUTF16(c.name) +
    NS_LITERAL_STRING(" has been deprecated. Use the string value \"") +
    NS_ConvertASCIItoUTF16(c.value) +
    NS_LITERAL_STRING("\" instead.");

  uint64_t windowID = 0;
  nsIScriptContext* context = GetScriptContextFromJSContext(cx);
  if (context) {
    nsCOMPtr<nsPIDOMWindow> window =
      do_QueryInterface(context->GetGlobalObject());
    if (window) {
      window = window->GetCurrentInnerWindow();
    }
    NS_WARN_IF_FALSE(window, "Missing a window, got a door?");
    if (window) {
      windowID = window->WindowID();
    }
  }

  nsCOMPtr<nsIScriptError> errorObject =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
  NS_WARN_IF_FALSE(errorObject, "Failed to create error object");
  if (errorObject) {
    nsresult rv = errorObject->InitWithWindowID(warnText,
                                                EmptyString(), 
                                                EmptyString(), 
                                                0, 0, 
                                                nsIScriptError::warningFlag,
                                                "DOM Core", windowID);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to init error object");

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIConsoleService> consoleServ =
        do_GetService(NS_CONSOLESERVICE_CONTRACTID);
      if (consoleServ) {
        consoleServ->LogMessage(errorObject);
      }
    }
  }

  
  NS_ConvertASCIItoUTF16 valStr(c.value);
  JS::Rooted<JS::Value> value(cx);
  if (!xpc::StringToJsval(cx, valStr, value.address())) {
    return JS_FALSE;
  }
  if (!::JS_DefineProperty(cx, obj, c.name, value,
                           JS_PropertyStub, JS_StrictPropertyStub,
                           JSPROP_ENUMERATE)) {
    return JS_FALSE;
  }

  
  vp.set(value);
  return JS_TRUE;
}

static nsresult
DefineIDBInterfaceConstants(JSContext *cx, JS::Handle<JSObject*> obj, const nsIID *aIID)
{
  const char* interface;
  if (aIID->Equals(NS_GET_IID(nsIIDBCursor))) {
    interface = IDBConstant::IDBCursor;
  }
  else if (aIID->Equals(NS_GET_IID(nsIIDBRequest))) {
    interface = IDBConstant::IDBRequest;
  }
  else {
    MOZ_CRASH("unexpected IID");
  }

  for (int8_t i = 0; i < (int8_t)mozilla::ArrayLength(sIDBConstants); ++i) {
    const IDBConstant& c = sIDBConstants[i];
    if (c.interface != interface) {
      continue;
    }

    if (!JS_DefineProperty(cx, obj, c.name, JSVAL_VOID,
                           IDBConstantGetter, JS_StrictPropertyStub,
                           JSPROP_ENUMERATE)) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
}

class nsDOMConstructor MOZ_FINAL : public nsIDOMDOMConstructor
{
protected:
  nsDOMConstructor(const PRUnichar* aName,
                   bool aIsConstructable,
                   nsPIDOMWindow* aOwner)
    : mClassName(aName),
      mConstructable(aIsConstructable),
      mWeakOwner(do_GetWeakReference(aOwner))
  {
  }

public:

  static nsresult Create(const PRUnichar* aName,
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

  nsresult Install(JSContext *cx, JS::Handle<JSObject*> target,
                   JS::Handle<JS::Value> aThisAsVal)
  {
    JS::Rooted<JS::Value> thisAsVal(cx, aThisAsVal);
    
    JSBool ok = JS_WrapValue(cx, thisAsVal.address()) &&
      ::JS_DefineUCProperty(cx, target,
                            reinterpret_cast<const jschar *>(mClassName),
                            NS_strlen(mClassName), thisAsVal, JS_PropertyStub,
                            JS_StrictPropertyStub, 0);

    return ok ? NS_OK : NS_ERROR_UNEXPECTED;
  }

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

    nsScriptNameSpaceManager *nameSpaceManager =
      nsJSRuntime::GetNameSpaceManager();
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

    return FindConstructorFunc(aData);
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

  const PRUnichar*   mClassName;
  const bool mConstructable;
  nsWeakPtr          mWeakOwner;
};


nsresult
nsDOMConstructor::Create(const PRUnichar* aName,
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
  if (JSVAL_IS_PRIMITIVE(v)) {
    return NS_OK;
  }

  JS::Rooted<JSObject*> dom_obj(cx, v.toObjectOrNull());
  NS_ASSERTION(dom_obj, "nsDOMConstructor::HasInstance couldn't get object");

  
  JSObject *wrapped_obj = js::CheckedUnwrap(dom_obj,  false);
  if (wrapped_obj)
      dom_obj = wrapped_obj;

  JSClass *dom_class = JS_GetClass(dom_obj);
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
    
    
    JS::Rooted<JS::Value> val(cx);
    if (!JS_GetProperty(cx, obj, "prototype", &val)) {
      return NS_ERROR_UNEXPECTED;
    }

    if (JSVAL_IS_PRIMITIVE(val)) {
      return NS_OK;
    }

    JS::Rooted<JSObject*> dot_prototype(cx, val.toObjectOrNull());

    JS::Rooted<JSObject*> proto(cx, dom_obj);
    for (;;) {
      if (!JS_GetPrototype(cx, proto, proto.address())) {
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
    *bp = JS_TRUE;

    return NS_OK;
  }

  nsScriptNameSpaceManager *nameSpaceManager =
    nsJSRuntime::GetNameSpaceManager();
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
    *bp = JS_FALSE;

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
      *bp = JS_TRUE;

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

  
  if (class_iid->Equals(NS_GET_IID(nsIIDBKeyRange)) &&
      !indexedDB::IDBKeyRange::DefineConstructors(cx, obj)) {
    return NS_ERROR_FAILURE;
  }

  
  
  if (class_iid->Equals(NS_GET_IID(nsIIDBCursor)) ||
      class_iid->Equals(NS_GET_IID(nsIIDBRequest))) {
    rv = DefineIDBInterfaceConstants(cx, obj, class_iid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMConstructor::ToString(nsAString &aResult)
{
  aResult.AssignLiteral("[object ");
  aResult.Append(mClassName);
  aResult.Append(PRUnichar(']'));

  return NS_OK;
}


static nsresult
GetXPCProto(nsIXPConnect *aXPConnect, JSContext *cx, nsGlobalWindow *aWin,
            const nsGlobalNameStruct *aNameStruct,
            nsIXPConnectJSObjectHolder **aProto)
{
  NS_ASSERTION(aNameStruct->mType ==
                 nsGlobalNameStruct::eTypeClassConstructor ||
               aNameStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo,
               "Wrong type!");

  nsCOMPtr<nsIClassInfo> ci;
  if (aNameStruct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
    int32_t id = aNameStruct->mDOMClassInfoID;
    NS_ABORT_IF_FALSE(id >= 0, "Negative DOM classinfo?!?");

    nsDOMClassInfoID ci_id = (nsDOMClassInfoID)id;

    ci = NS_GetDOMClassInfoInstance(ci_id);

    
    
    
    
    
    
    
    if (ci_id == eDOMClassInfo_Window_id ||
        ci_id == eDOMClassInfo_ModalContentWindow_id ||
        ci_id == eDOMClassInfo_ChromeWindow_id) {
      nsGlobalWindow *scopeWindow = aWin->GetOuterWindowInternal();

      if (scopeWindow) {
        aWin = scopeWindow;
      }
    }
  }
  else {
    ci = nsDOMClassInfo::GetClassInfoInstance(aNameStruct->mData);
  }
  NS_ENSURE_TRUE(ci, NS_ERROR_UNEXPECTED);

  nsresult rv =
    aXPConnect->GetWrappedNativePrototype(cx, aWin->GetGlobalJSObject(), ci,
                                          aProto);
  NS_ENSURE_SUCCESS(rv, rv);

  JS::Rooted<JSObject*> proto_obj(cx, (*aProto)->GetJSObject());
  if (!JS_WrapObject(cx, proto_obj.address())) {
    return NS_ERROR_FAILURE;
  }

  NS_IF_RELEASE(*aProto);
  return aXPConnect->HoldObject(cx, proto_obj, aProto);
}



static nsresult
ResolvePrototype(nsIXPConnect *aXPConnect, nsGlobalWindow *aWin, JSContext *cx,
                 JS::Handle<JSObject*> obj, const PRUnichar *name,
                 const nsDOMClassInfoData *ci_data,
                 const nsGlobalNameStruct *name_struct,
                 nsScriptNameSpaceManager *nameSpaceManager,
                 JSObject* aDot_prototype, bool install, bool *did_resolve)
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

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  JS::Rooted<JS::Value> v(cx);

  rv = WrapNative(cx, obj, constructor, &NS_GET_IID(nsIDOMDOMConstructor),
                  false, v.address(), getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  if (install) {
    rv = constructor->Install(cx, obj, v);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  JS::Rooted<JSObject*> class_obj(cx, holder->GetJSObject());
  NS_ASSERTION(class_obj, "The return value lied");

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

    
    if (primary_iid->Equals(NS_GET_IID(nsIIDBKeyRange)) &&
        !indexedDB::IDBKeyRange::DefineConstructors(cx, class_obj)) {
      return NS_ERROR_FAILURE;
    }

    
    
    if (primary_iid->Equals(NS_GET_IID(nsIIDBCursor)) ||
        primary_iid->Equals(NS_GET_IID(nsIIDBRequest))) {
      rv = DefineIDBInterfaceConstants(cx, class_obj, primary_iid);
      NS_ENSURE_SUCCESS(rv, rv);
    }

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

      JS::Rooted<JS::Value> val(cx);
      if (!JS_LookupProperty(cx, winobj, CutPrefix(class_parent_name), val.address())) {
        return NS_ERROR_UNEXPECTED;
      }

      if (val.isObject()) {
        if (!JS_LookupProperty(cx, &val.toObject(), "prototype", val.address())) {
          return NS_ERROR_UNEXPECTED;
        }

        if (val.isObject()) {
          proto = &val.toObject();
        }
      }
    }

    if (dot_prototype) {
      JSAutoCompartment ac(cx, dot_prototype);
      JS::Rooted<JSObject*> xpc_proto_proto(cx);
      if (!::JS_GetPrototype(cx, dot_prototype, xpc_proto_proto.address())) {
        return NS_ERROR_UNEXPECTED;
      }

      if (proto &&
          (!xpc_proto_proto ||
           JS_GetClass(xpc_proto_proto) == sObjectClass)) {
        if (!JS_WrapObject(cx, proto.address()) ||
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
                                                   proto,
                                                   winobj);
      NS_ENSURE_TRUE(dot_prototype, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  v = OBJECT_TO_JSVAL(dot_prototype);

  JSAutoCompartment ac(cx, class_obj);

  
  if (!JS_WrapValue(cx, v.address()) ||
      !JS_DefineProperty(cx, class_obj, "prototype", v,
                         JS_PropertyStub, JS_StrictPropertyStub,
                         JSPROP_PERMANENT | JSPROP_READONLY)) {
    return NS_ERROR_UNEXPECTED;
  }

  *did_resolve = true;

  return NS_OK;
}

static bool
OldBindingConstructorEnabled(const nsGlobalNameStruct *aStruct,
                             nsGlobalWindow *aWin)
{
  MOZ_ASSERT(aStruct->mType == nsGlobalNameStruct::eTypeClassConstructor ||
             aStruct->mType == nsGlobalNameStruct::eTypeExternalClassInfo);

  
  if (aStruct->mChromeOnly &&
      !nsContentUtils::IsSystemPrincipal(aWin->GetPrincipal())) {
    return false;
  }

  
  if (aStruct->mDOMClassInfoID == eDOMClassInfo_CSSSupportsRule_id) {
    if (!CSSSupportsRule::PrefEnabled()) {
      return false;
    }
  }

  return true;
}


nsresult
nsWindowSH::GlobalResolve(nsGlobalWindow *aWin, JSContext *cx,
                          JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                          bool *did_resolve)
{
  *did_resolve = false;

  nsScriptNameSpaceManager *nameSpaceManager =
    nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  nsDependentJSString name(id);

  const PRUnichar *class_name = nullptr;
  const nsGlobalNameStruct *name_struct =
    nameSpaceManager->LookupName(name, &class_name);

  if (!name_struct) {
    return NS_OK;
  }

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
    
    mozilla::dom::DefineInterface define =
      name_struct->mDefineDOMInterface;
    if (define) {
      if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor &&
          !OldBindingConstructorEnabled(name_struct, aWin)) {
        return NS_OK;
      }

      Maybe<JSAutoCompartment> ac;
      JS::Rooted<JSObject*> global(cx);
      bool defineOnXray = xpc::WrapperFactory::IsXrayWrapper(obj);
      if (defineOnXray) {
        global = js::CheckedUnwrap(obj,  false);
        if (!global) {
          return NS_ERROR_DOM_SECURITY_ERR;
        }
        ac.construct(cx, global);
      } else {
        global = obj;
      }

      
      
      
      if (name_struct->mConstructorEnabled &&
          !(*name_struct->mConstructorEnabled)(cx, global)) {
        return NS_OK;
      }

      bool enabled;
      JS::Rooted<JSObject*> interfaceObject(cx, define(cx, global, id, &enabled));
      if (enabled) {
        if (!interfaceObject) {
          return NS_ERROR_FAILURE;
        }

        if (defineOnXray) {
          
          ac.destroy();
          if (!JS_WrapObject(cx, interfaceObject.address()) ||
              !JS_DefinePropertyById(cx, obj, id,
                                     JS::ObjectValue(*interfaceObject), JS_PropertyStub,
                                     JS_StrictPropertyStub, 0)) {
            return NS_ERROR_FAILURE;
          }
        }

        *did_resolve = true;

        return NS_OK;
      }
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

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    JS::Rooted<JS::Value> v(cx);
    rv = WrapNative(cx, obj, constructor, &NS_GET_IID(nsIDOMDOMConstructor),
                    false, v.address(), getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = constructor->Install(cx, obj, v);
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JSObject*> class_obj(cx, holder->GetJSObject());
    NS_ASSERTION(class_obj, "The return value lied");

    
    

    JSAutoCompartment ac(cx, class_obj);
    rv = DefineInterfaceConstants(cx, class_obj, &name_struct->mIID);
    NS_ENSURE_SUCCESS(rv, rv);

    *did_resolve = true;

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor ||
      name_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
    if (!OldBindingConstructorEnabled(name_struct, aWin)) {
      return NS_OK;
    }

    
    
    nsCOMPtr<nsIXPConnectJSObjectHolder> proto_holder;
    rv = GetXPCProto(sXPConnect, cx, aWin, name_struct,
                     getter_AddRefs(proto_holder));

    if (NS_SUCCEEDED(rv) && obj != aWin->GetGlobalJSObject()) {
      JS::Rooted<JSObject*> dot_prototype(cx, proto_holder->GetJSObject());
      NS_ENSURE_STATE(dot_prototype);

      const nsDOMClassInfoData *ci_data;
      if (name_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
        ci_data = &sClassInfoData[name_struct->mDOMClassInfoID];
      } else {
        ci_data = name_struct->mData;
      }

      return ResolvePrototype(sXPConnect, aWin, cx, obj, class_name, ci_data,
                              name_struct, nameSpaceManager, dot_prototype,
                              true, did_resolve);
    }

    *did_resolve = NS_SUCCEEDED(rv);

    return rv;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeClassProto) {
    
    
    return ResolvePrototype(sXPConnect, aWin, cx, obj, class_name, nullptr,
                            name_struct, nameSpaceManager, nullptr, true,
                            did_resolve);
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructorAlias) {
    const nsGlobalNameStruct *alias_struct =
      nameSpaceManager->GetConstructorProto(name_struct);
    NS_ENSURE_TRUE(alias_struct, NS_ERROR_UNEXPECTED);

    
    
    
    nsCOMPtr<nsIXPConnectJSObjectHolder> proto_holder;
    rv = GetXPCProto(sXPConnect, cx, aWin, alias_struct,
                     getter_AddRefs(proto_holder));
    NS_ENSURE_SUCCESS(rv, rv);

    JSObject* dot_prototype = proto_holder->GetJSObject();
    NS_ENSURE_STATE(dot_prototype);

    const nsDOMClassInfoData *ci_data;
    if (alias_struct->mType == nsGlobalNameStruct::eTypeClassConstructor) {
      ci_data = &sClassInfoData[alias_struct->mDOMClassInfoID];
    } else if (alias_struct->mType == nsGlobalNameStruct::eTypeExternalClassInfo) {
      ci_data = alias_struct->mData;
    } else {
      return NS_ERROR_UNEXPECTED;
    }

    return ResolvePrototype(sXPConnect, aWin, cx, obj, class_name, ci_data,
                            name_struct, nameSpaceManager, nullptr, true,
                            did_resolve);
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeExternalConstructor) {
    nsRefPtr<nsDOMConstructor> constructor;
    rv = nsDOMConstructor::Create(class_name, nullptr, name_struct,
                                  static_cast<nsPIDOMWindow*>(aWin),
                                  getter_AddRefs(constructor));
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JS::Value> val(cx);
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = WrapNative(cx, obj, constructor, &NS_GET_IID(nsIDOMDOMConstructor),
                    false, val.address(), getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = constructor->Install(cx, obj, val);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(holder->GetJSObject(), "Why didn't we get a JSObject?");

    *did_resolve = true;

    return NS_OK;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeProperty) {
    if (name_struct->mChromeOnly && !nsContentUtils::IsCallerChrome())
      return NS_OK;

    
    
    nsCOMPtr<nsIDOMWindow> childWin = aWin->GetChildWindow(name);
    if (childWin)
      return NS_OK;

    nsCOMPtr<nsISupports> native(do_CreateInstance(name_struct->mCID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JS::Value> prop_val(cx, JS::UndefinedValue()); 

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsCOMPtr<nsIDOMGlobalPropertyInitializer> gpi(do_QueryInterface(native));
    if (gpi) {
      rv = gpi->Init(aWin, prop_val.address());
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (JSVAL_IS_PRIMITIVE(prop_val) && !JSVAL_IS_NULL(prop_val)) {
      JSObject *scope;

      if (aWin->IsOuterWindow()) {
        nsGlobalWindow *inner = aWin->GetCurrentInnerWindowInternal();
        NS_ENSURE_TRUE(inner, NS_ERROR_UNEXPECTED);

        scope = inner->GetGlobalJSObject();
      } else {
        scope = aWin->GetGlobalJSObject();
      }

      rv = WrapNative(cx, scope, native, true, prop_val.address(),
                      getter_AddRefs(holder));
    }

    NS_ENSURE_SUCCESS(rv, rv);

    if (!JS_WrapValue(cx, prop_val.address())) {
      return NS_ERROR_UNEXPECTED;
    }

    JSBool ok = ::JS_DefinePropertyById(cx, obj, id, prop_val,
                                        JS_PropertyStub, JS_StrictPropertyStub,
                                        JSPROP_ENUMERATE);

    *did_resolve = true;

    return ok ? NS_OK : NS_ERROR_FAILURE;
  }

  if (name_struct->mType == nsGlobalNameStruct::eTypeDynamicNameSet) {
    nsCOMPtr<nsIScriptExternalNameSet> nameset =
      do_CreateInstance(name_struct->mCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsIScriptContext *context = aWin->GetContext();
    NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);

    rv = nameset->InitializeNameSet(context);

    *did_resolve = true;
  }

  return rv;
}



static JSBool
ContentWindowGetter(JSContext *cx, unsigned argc, jsval *vp)
{
  JSObject *obj = JS_THIS_OBJECT(cx, vp);
  if (!obj)
    return JS_FALSE;

  JS::Rooted<JS::Value> value(cx);
  bool result = ::JS_GetProperty(cx, obj, "content", &value);
  *vp = value;
  return result;
}

template<class Interface>
static nsresult
LocationSetterGuts(JSContext *cx, JSObject *obj, jsval *vp)
{
  
  obj = js::CheckedUnwrap(obj,  false);
  if (!IS_WN_REFLECTOR(obj))
      return NS_ERROR_XPC_BAD_CONVERT_JS;
  XPCWrappedNative *wrapper = XPCWrappedNative::Get(obj);

  
  NS_ENSURE_TRUE(!wrapper || wrapper->IsValid(), NS_ERROR_XPC_HAS_BEEN_SHUTDOWN);

  nsCOMPtr<Interface> xpcomObj = do_QueryWrappedNative(wrapper, obj);
  NS_ENSURE_TRUE(xpcomObj, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMLocation> location;
  nsresult rv = xpcomObj->GetLocation(getter_AddRefs(location));
  NS_ENSURE_SUCCESS(rv, rv);

  
  JS::Rooted<JSString*> val(cx, ::JS_ValueToString(cx, *vp));
  NS_ENSURE_TRUE(val, NS_ERROR_UNEXPECTED);

  
  JS::Anchor<JSString *> anchor(val);

  
  
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  rv = WrapNative(cx, JS::CurrentGlobalOrNull(cx), location,
                  &NS_GET_IID(nsIDOMLocation), true, vp,
                  getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!location) {
    
    return NS_OK;
  }

  nsDependentJSString depStr;
  NS_ENSURE_TRUE(depStr.init(cx, val), NS_ERROR_UNEXPECTED);

  return location->SetHref(depStr);
}

template<class Interface>
static JSBool
LocationSetter(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, JSBool strict,
               JS::MutableHandle<JS::Value> vp)
{
  nsresult rv = LocationSetterGuts<Interface>(cx, obj, vp.address());
  if (NS_FAILED(rv)) {
    xpc::Throw(cx, rv);
    return JS_FALSE;
  }

  return JS_TRUE;
}

static JSBool
LocationSetterUnwrapper(JSContext *cx, JS::Handle<JSObject*> obj_, JS::Handle<jsid> id,
                        JSBool strict, JS::MutableHandle<JS::Value> vp)
{
  JS::RootedObject obj(cx, obj_);

  JSObject *wrapped = XPCWrapper::UnsafeUnwrapSecurityWrapper(obj);
  if (wrapped) {
    obj = wrapped;
  }

  return LocationSetter<nsIDOMWindow>(cx, obj, id, strict, vp);
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
  { "nsIDOMUserDataHandler", "UserDataHandler" },
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
  { "nsIDOMGeoPositionError", "GeoPositionError" },
  { "nsIDOMHTMLMediaElement", "HTMLMediaElement" },
  { "nsIDOMMediaError", "MediaError" },
  { "nsIDOMLoadStatus", "LoadStatus" },
  { "nsIDOMOfflineResourceList", "OfflineResourceList" },
  { "nsIDOMRange", "Range" },
  { "nsIDOMSVGLength", "SVGLength" },
  { "nsIDOMNodeFilter", "NodeFilter" },
  { "nsIDOMXPathNamespace", "XPathNamespace" },
  { "nsIDOMXPathResult", "XPathResult" },
  { "nsIDOMXULButtonElement", "XULButtonElement" },
  { "nsIDOMXULCheckboxElement", "XULCheckboxElement" },
  { "nsIDOMXULPopupElement", "XULPopupElement" } };

static nsresult
DefineComponentsShim(JSContext *cx, JS::HandleObject global)
{
  
  Telemetry::Accumulate(Telemetry::COMPONENTS_SHIM_ACCESSED_BY_CONTENT, true);

  
  JS::Rooted<JSObject*> components(cx, JS_NewObject(cx, nullptr, nullptr, global));
  NS_ENSURE_TRUE(components, NS_ERROR_OUT_OF_MEMORY);
  bool ok = JS_DefineProperty(cx, global, "Components", JS::ObjectValue(*components),
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_ENUMERATE);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  
  JS::Rooted<JSObject*> interfaces(cx, JS_NewObject(cx, nullptr, nullptr, global));
  NS_ENSURE_TRUE(interfaces, NS_ERROR_OUT_OF_MEMORY);
  ok = JS_DefineProperty(cx, components, "interfaces", JS::ObjectValue(*interfaces),
                         JS_PropertyStub, JS_StrictPropertyStub,
                         JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY);
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
                           JS_PropertyStub, JS_StrictPropertyStub,
                           JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY);
    NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *obj_, jsid id_, uint32_t flags,
                       JSObject **objp, bool *_retval)
{
  JS::RootedObject obj(cx, obj_);
  JS::RootedId id(cx, id_);

  if (!JSID_IS_STRING(id)) {
    return NS_OK;
  }

  MOZ_ASSERT(*_retval == true); 
  if (id == XPCJSRuntime::Get()->GetStringID(XPCJSRuntime::IDX_COMPONENTS)) {
    *objp = obj;
    return DefineComponentsShim(cx, obj);
  }

  nsGlobalWindow *win = nsGlobalWindow::FromWrapper(wrapper);
  MOZ_ASSERT(win->IsInnerWindow());

  nsIScriptContext *my_context = win->GetContextInternal();

  
  
  if (!xpc::WrapperFactory::IsXrayWrapper(obj)) {
    JSBool did_resolve = JS_FALSE;
    JSBool ok = JS_TRUE;
    JS::Rooted<JS::Value> exn(cx, JSVAL_VOID);

    {
      
      
      
      
      
      
      AutoPushJSContext my_cx(my_context ? my_context->GetNativeContext() : cx);
      JSAutoCompartment ac(my_cx, obj);

      ok = JS_ResolveStandardClass(my_cx, obj, id, &did_resolve);

      if (!ok) {
        
        

        if (!JS_GetPendingException(my_cx, exn.address())) {
          return NS_ERROR_UNEXPECTED;
        }

        
        
        
        

        JS_ClearPendingException(my_cx);
      }
    }

    if (!ok) {
      JS_SetPendingException(cx, exn);
      *_retval = JS_FALSE;
      return NS_OK;
    }

    if (did_resolve) {
      *objp = obj;
      return NS_OK;
    }
  }

  
  
  
  if (sConstructor_id == id) {
    return ResolveConstructor(cx, obj, objp);
  }

  if (!my_context || !my_context->IsContextInitialized()) {
    
    

    return NS_OK;
  }

  if (sLocation_id == id) {
    
    
    
    

    nsCOMPtr<nsIDOMLocation> location;
    nsresult rv = win->GetLocation(getter_AddRefs(location));
    NS_ENSURE_SUCCESS(rv, rv);

    
    JS::Rooted<JSObject*> scope(cx, wrapper->GetJSObject());

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    JS::Rooted<JS::Value> v(cx);
    rv = WrapNative(cx, scope, location, &NS_GET_IID(nsIDOMLocation), true,
                    v.address(), getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    JSBool ok = JS_WrapValue(cx, v.address()) &&
                JS_DefinePropertyById(cx, obj, id, v, JS_PropertyStub,
                                      LocationSetterUnwrapper,
                                      JSPROP_PERMANENT | JSPROP_ENUMERATE);

    if (!ok) {
      return NS_ERROR_FAILURE;
    }

    *objp = obj;

    return NS_OK;
  }

  if (sTop_id == id) {
    nsCOMPtr<nsIDOMWindow> top;
    nsresult rv = win->GetScriptableTop(getter_AddRefs(top));
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JS::Value> v(cx);
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    rv = WrapNative(cx, obj, top, &NS_GET_IID(nsIDOMWindow), true,
                    v.address(), getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!JS_DefinePropertyById(cx, obj, id, v, JS_PropertyStub, JS_StrictPropertyStub,
                               JSPROP_READONLY | JSPROP_PERMANENT |
                               JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }
    *objp = obj;

    return NS_OK;
  }

  
  JS::RootedObject tmp(cx, NULL);
  if (!ResolveWorkerClasses(cx, obj, id, flags, &tmp)) {
    return NS_ERROR_FAILURE;
  }
  if (tmp) {
    *objp = tmp;
    return NS_OK;
  }

  
  
  
  
  
  bool did_resolve = false;
  nsresult rv = GlobalResolve(win, cx, obj, id, &did_resolve);
  NS_ENSURE_SUCCESS(rv, rv);

  if (did_resolve) {
    *objp = obj;
    return NS_OK;
  }

  
  
  
  if ((s_content_id == id) && !xpc::WrapperFactory::IsXrayWrapper(obj)) {
    
    
    
    JS::Rooted<JSObject*> funObj(cx);
    JSFunction *fun = ::JS_NewFunction(cx, ContentWindowGetter, 0, 0,
                                       obj, "_content");
    if (!fun) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    funObj = ::JS_GetFunctionObject(fun);

    if (!JS_WrapObject(cx, funObj.address()) ||
        !JS_DefinePropertyById(cx, obj, id, JSVAL_VOID,
                               JS_DATA_TO_FUNC_PTR(JSPropertyOp, funObj.get()),
                               JS_StrictPropertyStub,
                               JSPROP_ENUMERATE | JSPROP_GETTER |
                               JSPROP_SHARED)) {
      return NS_ERROR_FAILURE;
    }

    *objp = obj;

    return NS_OK;
  }

  if (flags & JSRESOLVE_ASSIGNING) {
    if (IsReadonlyReplaceable(id)) {
      
      
      
      

      if (!::JS_DefinePropertyById(cx, obj, id, JSVAL_VOID, JS_PropertyStub,
                                   JS_StrictPropertyStub, JSPROP_ENUMERATE)) {
        return NS_ERROR_FAILURE;
      }
      *objp = obj;

      return NS_OK;
    }
  } else {
    if (sDocument_id == id) {
      nsCOMPtr<nsIDocument> document = win->GetDoc();
      JS::Rooted<JS::Value> v(cx);
      nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
      rv = WrapNative(cx, JS::CurrentGlobalOrNull(cx), document, document,
                      &NS_GET_IID(nsIDOMDocument), v.address(), getter_AddRefs(holder),
                      false);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      *objp = obj;

      
      if (xpc::WrapperFactory::IsXrayWrapper(obj)) {
        
        

        *_retval = JS_WrapValue(cx, v.address()) &&
                   JS_DefineProperty(cx, obj, "document", v,
                                     JS_PropertyStub, JS_StrictPropertyStub,
                                     JSPROP_READONLY | JSPROP_ENUMERATE);
        if (!*_retval) {
          return NS_ERROR_UNEXPECTED;
        }
      }

      return NS_OK;
    }
  }

  rv = nsDOMGenericSH::NewResolve(wrapper, cx, obj, id, flags, objp,
                                  _retval);

  if (NS_FAILED(rv) || *objp) {
    
    return rv;
  }

  
  
  
  
  if (flags & JSRESOLVE_ASSIGNING) {
    JS::Rooted<JSObject*> realObj(cx, wrapper->GetJSObject());

    if (obj == realObj) {
      JS::Rooted<JSObject*> proto(cx);
      if (!js::GetObjectProto(cx, obj, &proto)) {
          *_retval = JS_FALSE;
          return NS_OK;
      }
      if (proto) {
        JS::Rooted<JSObject*> pobj(cx);
        JS::Rooted<JS::Value> val(cx);

        if (!::JS_LookupPropertyWithFlagsById(cx, proto, id, flags,
                                              pobj.address(), val.address())) {
          *_retval = JS_FALSE;

          return NS_OK;
        }

        if (pobj) {
          
          *objp = pobj;
          return NS_OK;
        }
      }

      
      
      
      
      
      
      
      
      
      
      JS::Rooted<JSString*> str(cx, JSID_TO_STRING(id));
      if (!js::ReportIfUndeclaredVarAssignment(cx, str) ||
          !::JS_DefinePropertyById(cx, obj, id, JSVAL_VOID, JS_PropertyStub,
                                   JS_StrictPropertyStub, JSPROP_ENUMERATE)) {
        *_retval = JS_FALSE;

        return NS_OK;
      }

      *objp = obj;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::Finalize(nsIXPConnectWrappedNative *wrapper, JSFreeOp *fop,
                     JSObject *obj)
{
  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(sgo, NS_ERROR_UNEXPECTED);

  sgo->OnFinalize(obj);

  return NS_OK;
}

NS_IMETHODIMP
nsWindowSH::OuterObject(nsIXPConnectWrappedNative *wrapper, JSContext * cx,
                        JSObject * obj, JSObject * *_retval)
{
  nsGlobalWindow *origWin = nsGlobalWindow::FromWrapper(wrapper);
  nsGlobalWindow *win = origWin->GetOuterWindowInternal();

  if (!win) {
    
    
    
    
    
    *_retval = nullptr;
    return NS_ERROR_UNEXPECTED;
  }

  JS::Rooted<JSObject*> winObj(cx, win->FastGetGlobalJSObject());
  MOZ_ASSERT(winObj);

  
  
  
  
  if (!JS_WrapObject(cx, winObj.address())) {
    *_retval = nullptr;
    return NS_ERROR_UNEXPECTED;
  }

  *_retval = winObj;
  return NS_OK;
}



NS_IMETHODIMP
nsLocationSH::CheckAccess(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsid id, uint32_t mode,
                          jsval *vp, bool *_retval)
{
  if ((mode & JSACC_TYPEMASK) == JSACC_PROTO && (mode & JSACC_WRITE)) {
    

    
    *_retval = false;

    return NS_ERROR_DOM_SECURITY_ERR;
  }

  return nsDOMGenericSH::CheckAccess(wrapper, cx, obj, id, mode, vp, _retval);
}

NS_IMETHODIMP
nsLocationSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                        JSObject *globalObj, JSObject **parentObj)
{
  
  
  
  
  *parentObj = globalObj;

  nsCOMPtr<nsIDOMLocation> safeLoc(do_QueryInterface(nativeObj));
  if (!safeLoc) {
    
    
    
    return NS_OK;
  }

  nsLocation *loc = (nsLocation *)safeLoc.get();
  nsIDocShell *ds = loc->GetDocShell();
  if (!ds) {
    NS_WARNING("Refusing to create a location in the wrong scope");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIScriptGlobalObject> sgo = do_GetInterface(ds);
  if (!sgo) {
    NS_WARNING("Refusing to create a location in the wrong scope because the "
               "docshell is being destroyed");
    return NS_ERROR_UNEXPECTED;
  }

  *parentObj = sgo->GetGlobalJSObject();
  return *parentObj ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsLocationSH::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *obj, jsid aId, jsval *vp, bool *_retval)
{
  
  
  JS::Rooted<jsid> id(cx, aId);
  if (wrapper->HasNativeMember(id)) {
    JS_ReportError(cx, "Permission denied to shadow native property");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}



NS_IMETHODIMP
nsEventTargetSH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                           JSObject *aGlobalObj, JSObject **parentObj)
{
  JS::Rooted<JSObject*> globalObj(cx, aGlobalObj);
  nsDOMEventTargetHelper *target =
    nsDOMEventTargetHelper::FromSupports(nativeObj);

  nsCOMPtr<nsIScriptGlobalObject> native_parent;
  target->GetParentObject(getter_AddRefs(native_parent));

  *parentObj = native_parent ? native_parent->GetGlobalJSObject() : globalObj;

  return *parentObj ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsEventTargetSH::AddProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsid id, jsval *vp, bool *_retval)
{
  nsEventTargetSH::PreserveWrapper(GetNative(wrapper, obj));

  return NS_OK;
}

void
nsEventTargetSH::PreserveWrapper(nsISupports *aNative)
{
  nsDOMEventTargetHelper *target =
    nsDOMEventTargetHelper::FromSupports(aNative);
  target->PreserveWrapper(aNative);
}



NS_IMETHODIMP
IDBEventTargetSH::PreCreate(nsISupports *aNativeObj, JSContext *aCx,
                            JSObject *aGlobalObj, JSObject **aParentObj)
{
  JS::Rooted<JSObject*> globalObj(aCx, aGlobalObj);
  IDBWrapperCache *target = IDBWrapperCache::FromSupports(aNativeObj);
  JSObject *parent = target->GetParentObject();
  *aParentObj = parent ? parent : globalObj;
  return NS_OK;
}



NS_IMETHODIMP
nsGenericArraySH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *aObj, jsid aId, uint32_t flags,
                             JSObject **objp, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  JS::Rooted<jsid> id(cx, aId);
  if (id == sLength_id) {
    
    return NS_OK;
  }

  bool is_number = false;
  int32_t n = GetArrayIndexFromId(cx, id, &is_number);

  if (is_number && n >= 0) {
    
    
    
    

    uint32_t length;
    nsresult rv = GetLength(wrapper, cx, obj, &length);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t index = uint32_t(n);
    if (index < length) {
      *_retval = ::JS_DefineElement(cx, obj, index, JSVAL_VOID, nullptr, nullptr,
                                    JSPROP_ENUMERATE | JSPROP_SHARED);
      *objp = obj;
    }
  }

  return NS_OK;
}

nsresult
nsGenericArraySH::GetLength(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, uint32_t *length)
{
  *length = 0;

  JS::Rooted<JS::Value> lenval(cx);
  if (!JS_GetProperty(cx, obj, "length", &lenval)) {
    return NS_ERROR_UNEXPECTED;
  }

  if (!JSVAL_IS_INT(lenval)) {
    
    
    return NS_OK;
  }

  int32_t slen = JSVAL_TO_INT(lenval);
  if (slen < 0) {
    return NS_OK;
  }

  *length = (uint32_t)slen;

  return NS_OK;
}

NS_IMETHODIMP
nsGenericArraySH::Enumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                            JSObject *aObj, bool *_retval)
{
  
  
  

  JS::Rooted<JSObject*> obj(cx, aObj);
  static bool sCurrentlyEnumerating;

  if (sCurrentlyEnumerating) {
    
    return NS_OK;
  }

  sCurrentlyEnumerating = true;

  JS::Rooted<JS::Value> len_val(cx);
  JSBool ok = ::JS_GetProperty(cx, obj, "length", &len_val);

  if (ok && JSVAL_IS_INT(len_val)) {
    int32_t length = JSVAL_TO_INT(len_val);

    for (int32_t i = 0; ok && i < length; ++i) {
      ok = ::JS_DefineElement(cx, obj, i, JSVAL_VOID, nullptr, nullptr,
                              JSPROP_ENUMERATE | JSPROP_SHARED);
    }
  }

  sCurrentlyEnumerating = false;

  return ok ? NS_OK : NS_ERROR_UNEXPECTED;
}



NS_IMETHODIMP
nsArraySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                       JSObject *aObj, jsid aId, jsval *vp, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  JS::Rooted<jsid> id(cx, aId);
  bool is_number = false;
  int32_t n = GetArrayIndexFromId(cx, id, &is_number);

  nsresult rv = NS_OK;

  if (is_number) {
    if (n < 0) {
      return NS_ERROR_DOM_INDEX_SIZE_ERR;
    }

    
    
    rv = NS_OK;
    nsWrapperCache *cache = nullptr;
    nsISupports* array_item =
      GetItemAt(GetNative(wrapper, obj), n, &cache, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (array_item) {
      rv = WrapNative(cx, JS::CurrentGlobalOrNull(cx), array_item, cache,
                      true, vp);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = NS_SUCCESS_I_DID_SOMETHING;
    }
  }

  return rv;
}




nsresult
nsStringListSH::GetStringAt(nsISupports *aNative, int32_t aIndex,
                            nsAString& aResult)
{
  nsCOMPtr<nsIDOMDOMStringList> list(do_QueryInterface(aNative));
  NS_ENSURE_TRUE(list, NS_ERROR_UNEXPECTED);

  nsresult rv = list->Item(aIndex, aResult);
#ifdef DEBUG
  if (DOMStringIsNull(aResult)) {
    uint32_t length = 0;
    list->GetLength(&length);
    NS_ASSERTION(uint32_t(aIndex) >= length, "Item should only return null for out-of-bounds access");
  }
#endif
  return rv;
}




JSClass sHTMLDocumentAllClass = {
  "HTML document.all class",
  JSCLASS_HAS_PRIVATE | JSCLASS_PRIVATE_IS_NSISUPPORTS | JSCLASS_NEW_RESOLVE |
  JSCLASS_EMULATES_UNDEFINED | JSCLASS_HAS_RESERVED_SLOTS(1),
  JS_PropertyStub,                                         
  JS_DeletePropertyStub,                                   
  nsHTMLDocumentSH::DocumentAllGetProperty,                
  JS_StrictPropertyStub,                                   
  JS_EnumerateStub,
  (JSResolveOp)nsHTMLDocumentSH::DocumentAllNewResolve,
  JS_ConvertStub,
  nsHTMLDocumentSH::ReleaseDocument,
  nullptr,                                                  
  nsHTMLDocumentSH::CallToGetPropMapper
};



JSBool
nsHTMLDocumentSH::GetDocumentAllNodeList(JSContext *cx,
                                         JS::Handle<JSObject*> obj,
                                         nsDocument *domdoc,
                                         nsContentList **nodeList)
{
  
  
  
  
  
  
  nsresult rv = NS_OK;

  JS::Rooted<JS::Value> collection(cx, JS_GetReservedSlot(obj, 0));

  if (!JSVAL_IS_PRIMITIVE(collection)) {
    
    JS::Rooted<JSObject*> obj(cx, JSVAL_TO_OBJECT(collection));
    nsIHTMLCollection* htmlCollection;
    rv = mozilla::dom::UnwrapObject<nsIHTMLCollection>(cx, obj, htmlCollection);
    if (NS_SUCCEEDED(rv)) {
      NS_ADDREF(*nodeList = static_cast<nsContentList*>(htmlCollection));
    }
    else {
      nsISupports *native = nsDOMClassInfo::XPConnect()->GetNativeOfWrapper(cx, obj);
      if (native) {
        NS_ADDREF(*nodeList = nsContentList::FromSupports(native));
        rv = NS_OK;
      }
      else {
        rv = NS_ERROR_FAILURE;
      }
    }
  } else {
    

    nsRefPtr<nsContentList> list =
      domdoc->GetElementsByTagName(NS_LITERAL_STRING("*"));
    if (!list) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    nsresult tmp = WrapNative(cx, JS::CurrentGlobalOrNull(cx),
                              static_cast<nsINodeList*>(list), list, false,
                              collection.address(), getter_AddRefs(holder));
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }

    list.forget(nodeList);

    
    JS_SetReservedSlot(obj, 0, collection);
  }

  if (NS_FAILED(rv)) {
    xpc::Throw(cx, NS_ERROR_FAILURE);

    return JS_FALSE;
  }

  return *nodeList != nullptr;
}

JSBool
nsHTMLDocumentSH::DocumentAllGetProperty(JSContext *cx, JS::Handle<JSObject*> obj_,
                                         JS::Handle<jsid> id, JS::MutableHandle<JS::Value> vp)
{
  JS::Rooted<JSObject*> obj(cx, obj_);

  
  
  
  
  if (nsDOMClassInfo::sItem_id == id || nsDOMClassInfo::sNamedItem_id == id) {
    return JS_TRUE;
  }

  JS::Rooted<JSObject*> proto(cx);
  while (js::GetObjectJSClass(obj) != &sHTMLDocumentAllClass) {
    if (!js::GetObjectProto(cx, obj, &proto)) {
      return JS_FALSE;
    }

    if (!proto) {
      NS_ERROR("The JS engine lies!");
      return JS_TRUE;
    }

    obj = proto;
  }

  nsHTMLDocument *doc = GetDocument(obj);
  nsISupports *result;
  nsWrapperCache *cache;
  nsresult rv = NS_OK;

  if (JSID_IS_STRING(id)) {
    if (nsDOMClassInfo::sLength_id == id) {
      
      
      

      nsRefPtr<nsContentList> nodeList;
      if (!GetDocumentAllNodeList(cx, obj, doc, getter_AddRefs(nodeList))) {
        return JS_FALSE;
      }

      uint32_t length;
      rv = nodeList->GetLength(&length);

      if (NS_FAILED(rv)) {
        xpc::Throw(cx, rv);

        return JS_FALSE;
      }

      vp.set(INT_TO_JSVAL(length));

      return JS_TRUE;
    }

    
    nsDependentJSString str(id);
    result = doc->GetDocumentAllResult(str, &cache, &rv);

    if (NS_FAILED(rv)) {
      xpc::Throw(cx, rv);
      return JS_FALSE;
    }
  } else if (JSID_IS_INT(id) && JSID_TO_INT(id) >= 0) {
    
    

    nsRefPtr<nsContentList> nodeList;
    if (!GetDocumentAllNodeList(cx, obj, doc, getter_AddRefs(nodeList))) {
      return JS_FALSE;
    }

    nsIContent *node = nodeList->Item(JSID_TO_INT(id));

    result = node;
    cache = node;
  } else {
    result = nullptr;
  }

  if (result) {
    rv = WrapNative(cx, JS::CurrentGlobalOrNull(cx), result, cache, true, vp.address());
    if (NS_FAILED(rv)) {
      xpc::Throw(cx, rv);

      return JS_FALSE;
    }
  } else {
    vp.setUndefined();
  }

  return JS_TRUE;
}

JSBool
nsHTMLDocumentSH::DocumentAllNewResolve(JSContext *cx, JS::Handle<JSObject*> obj,
                                        JS::Handle<jsid> id, unsigned flags,
                                        JS::MutableHandle<JSObject*> objp)
{
  JS::RootedValue v(cx);

  if (nsDOMClassInfo::sItem_id == id || nsDOMClassInfo::sNamedItem_id == id) {
    

    JSFunction *fnc = ::JS_DefineFunctionById(cx, obj, id, CallToGetPropMapper,
                                              0, JSPROP_ENUMERATE);
    objp.set(obj);

    return fnc != nullptr;
  }

  if (nsDOMClassInfo::sLength_id == id) {
    
    
    
    

    v = JSVAL_ONE;
  } else {
    if (!DocumentAllGetProperty(cx, obj, id, &v)) {
      return JS_FALSE;
    }
  }

  JSBool ok = JS_TRUE;

  if (v.get() != JSVAL_VOID) {
    ok = ::JS_DefinePropertyById(cx, obj, id, v, nullptr, nullptr, 0);
    objp.set(obj);
  }

  return ok;
}

void
nsHTMLDocumentSH::ReleaseDocument(JSFreeOp *fop, JSObject *obj)
{
  nsIHTMLDocument* doc = GetDocument(obj);
  if (doc) {
    nsContentUtils::DeferredFinalize(doc);
  }
}

JSBool
nsHTMLDocumentSH::CallToGetPropMapper(JSContext *cx, unsigned argc, jsval *vp)
{
  

  if (argc != 1) {
    
    
    
    xpc::Throw(cx, NS_ERROR_INVALID_ARG);

    return JS_FALSE;
  }

  
  JS::Rooted<JSString*> str(cx, ::JS_ValueToString(cx, JS_ARGV(cx, vp)[0]));
  if (!str) {
    return JS_FALSE;
  }

  
  
  JSObject *self;
  JS::Value callee = JS_CALLEE(cx, vp);
  if (callee.isObject() &&
      JS_GetClass(&callee.toObject()) == &sHTMLDocumentAllClass) {
    self = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
  } else {
    self = JS_THIS_OBJECT(cx, vp);
    if (!self)
      return JS_FALSE;
  }

  size_t length;
  JS::Anchor<JSString *> anchor(str);
  const jschar *chars = ::JS_GetStringCharsAndLength(cx, str, &length);
  if (!chars) {
    return JS_FALSE;
  }

  JS::Rooted<JS::Value> value(cx);
  if (!::JS_GetUCProperty(cx, self, chars, length, &value)) {
    return false;
  }

  *vp = value;
  return true;
}



NS_IMETHODIMP
nsStringArraySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *aObj, jsid aId, jsval *vp,
                             bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  JS::Rooted<jsid> id(cx, aId);
  bool is_number = false;
  int32_t n = GetArrayIndexFromId(cx, id, &is_number);

  if (!is_number) {
    return NS_OK;
  }

  nsAutoString val;

  nsresult rv = GetStringAt(GetNative(wrapper, obj), n, val);
  NS_ENSURE_SUCCESS(rv, rv);

  if (DOMStringIsNull(val)) {
    *vp = JSVAL_VOID;
    return NS_SUCCESS_I_DID_SOMETHING;
  }

  NS_ENSURE_TRUE(xpc::NonVoidStringToJsval(cx, val, vp),
                 NS_ERROR_OUT_OF_MEMORY);
  return NS_SUCCESS_I_DID_SOMETHING;
}




NS_IMETHODIMP
nsHistorySH::PreCreate(nsISupports *nativeObj, JSContext *cx,
                       JSObject *globalObj, JSObject **parentObj)
{
  nsHistory *history = (nsHistory *)((nsIDOMHistory*)nativeObj);
  nsCOMPtr<nsPIDOMWindow> innerWindow;
  history->GetWindow(getter_AddRefs(innerWindow));
  if (!innerWindow) {
    NS_WARNING("refusing to create history object in the wrong scope");
    return NS_ERROR_FAILURE;
  }
  return SetParentToWindow(static_cast<nsGlobalWindow *>(innerWindow.get()),
                           parentObj);
}

NS_IMETHODIMP
nsHistorySH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *aObj, jsid aId, jsval *vp, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  JS::Rooted<jsid> id(cx, aId);
  bool is_number = false;
  GetArrayIndexFromId(cx, id, &is_number);

  if (!is_number) {
    return NS_OK;
  }

  return nsStringArraySH::GetProperty(wrapper, cx, obj, id, vp, _retval);
}

nsresult
nsHistorySH::GetStringAt(nsISupports *aNative, int32_t aIndex,
                         nsAString& aResult)
{
  if (aIndex < 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHistory> history(do_QueryInterface(aNative));

  nsresult rv = history->Item(aIndex, aResult);
#ifdef DEBUG
  if (DOMStringIsNull(aResult)) {
    int32_t length = 0;
    history->GetLength(&length);
    NS_ASSERTION(aIndex >= length, "Item should only return null for out-of-bounds access");
  }
#endif
  return rv;
}




nsresult
nsMediaListSH::GetStringAt(nsISupports *aNative, int32_t aIndex,
                           nsAString& aResult)
{
  if (aIndex < 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMMediaList> media_list(do_QueryInterface(aNative));

  nsresult rv = media_list->Item(uint32_t(aIndex), aResult);
#ifdef DEBUG
  if (DOMStringIsNull(aResult)) {
    uint32_t length = 0;
    media_list->GetLength(&length);
    NS_ASSERTION(uint32_t(aIndex) >= length, "Item should only return null for out-of-bounds access");
  }
#endif
  return rv;
}




nsISupports*
nsStyleSheetListSH::GetItemAt(nsISupports *aNative, uint32_t aIndex,
                              nsWrapperCache **aCache, nsresult *rv)
{
  nsDOMStyleSheetList* list = nsDOMStyleSheetList::FromSupports(aNative);

  return list->GetItemAt(aIndex);
}




nsISupports*
nsCSSRuleListSH::GetItemAt(nsISupports *aNative, uint32_t aIndex,
                           nsWrapperCache **aCache, nsresult *aResult)
{
  nsICSSRuleList* list = static_cast<nsICSSRuleList*>(aNative);
#ifdef DEBUG
  {
    nsCOMPtr<nsICSSRuleList> list_qi = do_QueryInterface(aNative);

    
    
    
    NS_ABORT_IF_FALSE(list_qi == list, "Uh, fix QI!");
  }
#endif

  return list->Item(aIndex);
}









NS_IMETHODIMP
nsStorage2SH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                         JSObject *obj, jsid aId, uint32_t flags,
                         JSObject **objp, bool *_retval)
{
  JS::Rooted<jsid> id(cx, aId);
  if (ObjectIsNativeWrapper(cx, obj)) {
    return NS_OK;
  }

  JS::Rooted<JSObject*> realObj(cx, wrapper->GetJSObject());

  JSAutoCompartment ac(cx, realObj);

  
  

  JS::Rooted<JSString*> jsstr(cx, IdToString(cx, id));
  if (!jsstr) {
    return NS_OK;
  }

  JS::Rooted<JSObject*> proto(cx);
  if (!::JS_GetPrototype(cx, realObj, proto.address())) {
    return NS_ERROR_FAILURE;
  }
  JSBool hasProp;

  if (proto &&
      (::JS_HasPropertyById(cx, proto, id, &hasProp) &&
       hasProp)) {
    
    

    return NS_OK;
  }

  
  

  nsCOMPtr<nsIDOMStorage> storage(do_QueryWrappedNative(wrapper));

  nsDependentJSString depStr;
  NS_ENSURE_TRUE(depStr.init(cx, jsstr), NS_ERROR_UNEXPECTED);

  
  
  nsAutoString data;
  nsresult rv = storage->GetItem(depStr, data);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!DOMStringIsNull(data)) {
    if (!::JS_DefinePropertyById(cx, realObj, id, JSVAL_VOID, nullptr,
                                 nullptr, JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }

    *objp = realObj;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsStorage2SH::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                          JSObject *aObj, jsid aId, jsval *vp, bool *_retval)
{
  JS::Rooted<JSObject*> obj(cx, aObj);
  JS::Rooted<jsid> id(cx, aId);
  nsCOMPtr<nsIDOMStorage> storage(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(storage, NS_ERROR_UNEXPECTED);

  JSString* key = IdToString(cx, id);
  NS_ENSURE_TRUE(key, NS_ERROR_UNEXPECTED);

  nsDependentJSString keyStr;
  NS_ENSURE_TRUE(keyStr.init(cx, key), NS_ERROR_UNEXPECTED);

  
  if (ObjectIsNativeWrapper(cx, obj)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoString val;
  nsresult rv = storage->GetItem(keyStr, val);
  NS_ENSURE_SUCCESS(rv, rv);

  if (DOMStringIsNull(val)) {
    
    *vp = JSVAL_VOID;
  } else {
    JSString* str =
      JS_NewUCStringCopyN(cx, static_cast<const jschar *>(val.get()),
                          val.Length());
    NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

    *vp = STRING_TO_JSVAL(str);
  }

  return NS_SUCCESS_I_DID_SOMETHING;
}

NS_IMETHODIMP
nsStorage2SH::SetProperty(nsIXPConnectWrappedNative *wrapper,
                          JSContext *cx, JSObject *obj, jsid aId,
                          jsval *vp, bool *_retval)
{
  JS::Rooted<jsid> id(cx, aId);
  nsCOMPtr<nsIDOMStorage> storage(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(storage, NS_ERROR_UNEXPECTED);

  JSString *key = IdToString(cx, id);
  NS_ENSURE_TRUE(key, NS_ERROR_UNEXPECTED);

  nsDependentJSString keyStr;
  NS_ENSURE_TRUE(keyStr.init(cx, key), NS_ERROR_UNEXPECTED);

  JSString *value = ::JS_ValueToString(cx, *vp);
  NS_ENSURE_TRUE(value, NS_ERROR_UNEXPECTED);

  nsDependentJSString valueStr;
  NS_ENSURE_TRUE(valueStr.init(cx, value), NS_ERROR_UNEXPECTED);

  nsresult rv = storage->SetItem(keyStr, valueStr);
  if (NS_SUCCEEDED(rv)) {
    rv = NS_SUCCESS_I_DID_SOMETHING;
  }

  return rv;
}

NS_IMETHODIMP
nsStorage2SH::DelProperty(nsIXPConnectWrappedNative *wrapper,
                          JSContext *cx, JSObject *obj, jsid aId,
                          bool *_retval)
{
  JS::Rooted<jsid> id(cx, aId);
  nsCOMPtr<nsIDOMStorage> storage(do_QueryWrappedNative(wrapper));
  NS_ENSURE_TRUE(storage, NS_ERROR_UNEXPECTED);

  JSString *key = IdToString(cx, id);
  NS_ENSURE_TRUE(key, NS_ERROR_UNEXPECTED);

  nsDependentJSString keyStr;
  NS_ENSURE_TRUE(keyStr.init(cx, key), NS_ERROR_UNEXPECTED);

  nsresult rv = storage->RemoveItem(keyStr);
  if (NS_FAILED(rv)) {
    return rv;
  }

  *_retval = true;
  return NS_SUCCESS_I_DID_SOMETHING;
}


NS_IMETHODIMP
nsStorage2SH::NewEnumerate(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                           JSObject *obj, uint32_t enum_op, jsval *statep,
                           jsid *idp, bool *_retval)
{
  if (enum_op == JSENUMERATE_INIT || enum_op == JSENUMERATE_INIT_ALL) {
    nsCOMPtr<nsPIDOMStorage> storage(do_QueryWrappedNative(wrapper));

    
    nsTArray<nsString> *keys = storage->GetKeys();
    NS_ENSURE_TRUE(keys, NS_ERROR_OUT_OF_MEMORY);

    *statep = PRIVATE_TO_JSVAL(keys);

    if (idp) {
      *idp = INT_TO_JSID(keys->Length());
    }
    return NS_OK;
  }

  nsTArray<nsString> *keys =
    (nsTArray<nsString> *)JSVAL_TO_PRIVATE(*statep);

  if (enum_op == JSENUMERATE_NEXT && keys->Length() != 0) {
    nsString& key = keys->ElementAt(0);
    JSString *str =
      JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar *>
                                              (key.get()),
                          key.Length());
    NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

    JS_ValueToId(cx, STRING_TO_JSVAL(str), idp);

    keys->RemoveElementAt(0);

    return NS_OK;
  }

  
  NS_ABORT_IF_FALSE(enum_op == JSENUMERATE_DESTROY ||
                    (enum_op == JSENUMERATE_NEXT && keys->Length() == 0),
                    "Bad call from the JS engine");
  delete keys;

  *statep = JSVAL_NULL;

  return NS_OK;
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
  *_retval = target.forget().get();
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
nsDOMConstructorSH::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                               JSObject *aObj, jsid aId, uint32_t flags,
                               JSObject **objp, bool *_retval)
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

  
  
  
  
  JSBool found;
  if (!JS_HasPropertyById(cx, nativePropsObj, id, &found)) {
    *_retval = false;
    return NS_OK;
  }

  if (found) {
    *objp = obj;
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
                                JSContext *cx, JSObject *aObj, const jsval &val,
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
