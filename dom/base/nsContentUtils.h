







#ifndef nsContentUtils_h___
#define nsContentUtils_h___

#if defined(XP_WIN)
#include <float.h>
#endif

#if defined(SOLARIS)
#include <ieeefp.h>
#endif

#include "js/TypeDecls.h"
#include "js/Value.h"
#include "js/RootingAPI.h"
#include "mozilla/EventForwards.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/TimeStamp.h"
#include "nsContentListDeclarations.h"
#include "nsMathUtils.h"
#include "nsTArrayForwardDeclare.h"
#include "Units.h"
#include "mozilla/dom/AutocompleteInfoBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/net/ReferrerPolicy.h"
#include "nsIContentPolicy.h"

#if defined(XP_WIN)

#undef LoadImage
#endif

class imgICache;
class imgIContainer;
class imgINotificationObserver;
class imgIRequest;
class imgLoader;
class imgRequestProxy;
class nsAutoScriptBlockerSuppressNodeRemoved;
class nsHtml5StringParser;
class nsIChannel;
class nsIConsoleService;
class nsIContent;
class nsIContentPolicy;
class nsIContentSecurityPolicy;
class nsIDocShell;
class nsIDocument;
class nsIDocumentLoaderFactory;
class nsIDocumentObserver;
class nsIDOMDocument;
class nsIDOMDocumentFragment;
class nsIDOMEvent;
class nsIDOMHTMLFormElement;
class nsIDOMHTMLInputElement;
class nsIDOMKeyEvent;
class nsIDOMNode;
class nsIDOMScriptObjectFactory;
class nsIDOMWindow;
class nsIDragSession;
class nsIEditor;
class nsIFragmentContentSink;
class nsIFrame;
class nsIImageLoadingContent;
class nsIInterfaceRequestor;
class nsIIOService;
class nsIJSRuntimeService;
class nsILineBreaker;
class nsIMessageBroadcaster;
class nsNameSpaceManager;
class nsIObserver;
class nsIParser;
class nsIParserService;
class nsIPresShell;
class nsIPrincipal;
class nsIRequest;
class nsIRunnable;
class nsIScriptContext;
class nsIScriptGlobalObject;
class nsIScriptSecurityManager;
class nsIStringBundle;
class nsIStringBundleService;
class nsISupportsArray;
class nsISupportsHashKey;
class nsIURI;
class nsIWidget;
class nsIWordBreaker;
class nsIXPConnect;
class nsNodeInfoManager;
class nsPIDOMWindow;
class nsPresContext;
class nsScriptObjectTracer;
class nsStringBuffer;
class nsStringHashKey;
class nsTextFragment;
class nsViewportInfo;
class nsWrapperCache;
class nsAttrValue;
class nsITransferable;

struct JSPropertyDescriptor;
struct JSRuntime;
struct nsIntMargin;

template<class E> class nsCOMArray;
template<class K, class V> class nsDataHashtable;
template<class K, class V> class nsRefPtrHashtable;
template<class T> class nsReadingIterator;

namespace mozilla {
class ErrorResult;
class EventListenerManager;

namespace dom {
class DocumentFragment;
class Element;
class EventTarget;
class IPCDataTransfer;
class NodeInfo;
class nsIContentChild;
class nsIContentParent;
class Selection;
class TabParent;
} 

namespace gfx {
class DataSourceSurface;
} 

namespace layers {
class LayerManager;
} 

} 

class nsIBidiKeyboard;

extern const char kLoadAsData[];



const nsAFlatString& EmptyString();
const nsAFlatCString& EmptyCString();

enum EventNameType {
  EventNameType_None = 0x0000,
  EventNameType_HTML = 0x0001,
  EventNameType_XUL = 0x0002,
  EventNameType_SVGGraphic = 0x0004, 
  EventNameType_SVGSVG = 0x0008, 
  EventNameType_SMIL = 0x0010, 
  EventNameType_HTMLBodyOrFramesetOnly = 0x0020,

  EventNameType_HTMLXUL = 0x0003,
  EventNameType_All = 0xFFFF
};

struct EventNameMapping
{
  
  
  nsIAtom* MOZ_NON_OWNING_REF mAtom;
  uint32_t mId;
  int32_t  mType;
  mozilla::EventClassID mEventClassID;
};

struct nsShortcutCandidate {
  nsShortcutCandidate(uint32_t aCharCode, bool aIgnoreShift) :
    mCharCode(aCharCode), mIgnoreShift(aIgnoreShift)
  {
  }
  uint32_t mCharCode;
  bool     mIgnoreShift;
};

typedef void (*CallOnRemoteChildFunction) (mozilla::dom::TabParent* aTabParent,
                                           void* aArg);

class nsContentUtils
{
  friend class nsAutoScriptBlockerSuppressNodeRemoved;
  typedef mozilla::dom::Element Element;
  typedef mozilla::TimeDuration TimeDuration;

public:
  static nsresult Init();

  static bool     IsCallerChrome();
  static bool     ThreadsafeIsCallerChrome();
  static bool     IsCallerContentXBL();

  static bool     IsImageSrcSetDisabled();

  static bool LookupBindingMember(JSContext* aCx, nsIContent *aContent,
                                  JS::Handle<jsid> aId,
                                  JS::MutableHandle<JSPropertyDescriptor> aDesc);

  



  static nsINode* GetCrossDocParentNode(nsINode* aChild);

  












  static bool ContentIsDescendantOf(const nsINode* aPossibleDescendant,
                                      const nsINode* aPossibleAncestor);

  





  static bool ContentIsHostIncludingDescendantOf(
    const nsINode* aPossibleDescendant, const nsINode* aPossibleAncestor);

  




  static bool ContentIsCrossDocDescendantOf(nsINode* aPossibleDescendant,
                                              nsINode* aPossibleAncestor);

  



  static nsresult GetAncestors(nsINode* aNode,
                               nsTArray<nsINode*>& aArray);

  







  static nsresult GetAncestorsAndOffsets(nsIDOMNode* aNode,
                                         int32_t aOffset,
                                         nsTArray<nsIContent*>* aAncestorNodes,
                                         nsTArray<int32_t>* aAncestorOffsets);

  





  static nsresult GetCommonAncestor(nsIDOMNode *aNode,
                                    nsIDOMNode *aOther,
                                    nsIDOMNode** aCommonAncestor);

  



  static nsINode* GetCommonAncestor(nsINode* aNode1,
                                    nsINode* aNode2);

  



  static bool PositionIsBefore(nsINode* aNode1, nsINode* aNode2);

  








  static int32_t ComparePoints(nsINode* aParent1, int32_t aOffset1,
                               nsINode* aParent2, int32_t aOffset2,
                               bool* aDisconnected = nullptr);
  static int32_t ComparePoints(nsIDOMNode* aParent1, int32_t aOffset1,
                               nsIDOMNode* aParent2, int32_t aOffset2,
                               bool* aDisconnected = nullptr);

  




  static Element* MatchElementId(nsIContent *aContent, const nsAString& aId);

  


  static Element* MatchElementId(nsIContent *aContent, const nsIAtom* aId);

  








  static uint16_t ReverseDocumentPosition(uint16_t aDocumentPosition);

  static uint32_t CopyNewlineNormalizedUnicodeTo(const nsAString& aSource,
                                                 uint32_t aSrcOffset,
                                                 char16_t* aDest,
                                                 uint32_t aLength,
                                                 bool& aLastCharCR);

  static uint32_t CopyNewlineNormalizedUnicodeTo(nsReadingIterator<char16_t>& aSrcStart, const nsReadingIterator<char16_t>& aSrcEnd, nsAString& aDest);

  static const nsDependentSubstring TrimCharsInSet(const char* aSet,
                                                   const nsAString& aValue);

  template<bool IsWhitespace(char16_t)>
  static const nsDependentSubstring TrimWhitespace(const nsAString& aStr,
                                                   bool aTrimTrailing = true);

  


  static bool IsFirstLetterPunctuation(uint32_t aChar);
  static bool IsFirstLetterPunctuationAt(const nsTextFragment* aFrag, uint32_t aOffset);
 
  


  static bool IsAlphanumeric(uint32_t aChar);
  static bool IsAlphanumericAt(const nsTextFragment* aFrag, uint32_t aOffset);

  







  static bool IsHTMLWhitespace(char16_t aChar);

  



  static bool IsHTMLWhitespaceOrNBSP(char16_t aChar);

  


  static bool IsHTMLBlock(nsIContent* aContent);

  enum ParseHTMLIntegerResultFlags {
    eParseHTMLInteger_NoFlags               = 0,
    eParseHTMLInteger_IsPercent             = 1 << 0,
    
    
    eParseHTMLInteger_NonStandard           = 1 << 1,
    eParseHTMLInteger_DidNotConsumeAllInput = 1 << 2,
    
    eParseHTMLInteger_Error                 = 1 << 3,
    eParseHTMLInteger_ErrorNoValue          = 1 << 4,
    eParseHTMLInteger_ErrorOverflow         = 1 << 5
  };
  static int32_t ParseHTMLInteger(const nsAString& aValue,
                                  ParseHTMLIntegerResultFlags *aResult);

  







  static bool ParseIntMarginValue(const nsAString& aString, nsIntMargin& aResult);

  






  static int32_t ParseLegacyFontSize(const nsAString& aValue);

  static void Shutdown();

  


  static nsresult CheckSameOrigin(const nsINode* aTrustedNode,
                                  nsIDOMNode* aUnTrustedNode);
  static nsresult CheckSameOrigin(const nsINode* aTrustedNode,
                                  const nsINode* unTrustedNode);

  
  static bool CanCallerAccess(nsIDOMNode *aNode);
  static bool CanCallerAccess(nsINode* aNode);

  
  
  static bool CanCallerAccess(nsPIDOMWindow* aWindow);

  









  static nsIDocument* GetDocumentFromCaller();

  
  
  static bool InProlog(nsINode *aNode);

  static nsIParserService* GetParserService();

  static nsNameSpaceManager* NameSpaceManager()
  {
    return sNameSpaceManager;
  }

  static nsIIOService* GetIOService()
  {
    return sIOService;
  }

  static nsIBidiKeyboard* GetBidiKeyboard();

  



  static nsIScriptSecurityManager* GetSecurityManager()
  {
    return sSecurityManager;
  }

  


  static bool GetContentSecurityPolicy(nsIContentSecurityPolicy** aCSP);

  
  
  static nsIPrincipal* SubjectPrincipal();

  
  
  static nsIPrincipal* ObjectPrincipal(JSObject* aObj);

  static nsresult GenerateStateKey(nsIContent* aContent,
                                   const nsIDocument* aDocument,
                                   nsACString& aKey);

  




  static nsresult NewURIWithDocumentCharset(nsIURI** aResult,
                                            const nsAString& aSpec,
                                            nsIDocument* aDocument,
                                            nsIURI* aBaseURI);

  





  static nsresult ConvertStringFromEncoding(const nsACString& aEncoding,
                                            const nsACString& aInput,
                                            nsAString& aOutput);

  








  static bool CheckForBOM(const unsigned char* aBuffer, uint32_t aLength,
                          nsACString& aCharset);

  



  static bool IsCustomElementName(nsIAtom* aName);

  static nsresult CheckQName(const nsAString& aQualifiedName,
                             bool aNamespaceAware = true,
                             const char16_t** aColon = nullptr);

  static nsresult SplitQName(const nsIContent* aNamespaceResolver,
                             const nsAFlatString& aQName,
                             int32_t *aNamespace, nsIAtom **aLocalName);

  static nsresult GetNodeInfoFromQName(const nsAString& aNamespaceURI,
                                       const nsAString& aQualifiedName,
                                       nsNodeInfoManager* aNodeInfoManager,
                                       uint16_t aNodeType,
                                       mozilla::dom::NodeInfo** aNodeInfo);

  static void SplitExpatName(const char16_t *aExpatName, nsIAtom **aPrefix,
                             nsIAtom **aTagName, int32_t *aNameSpaceID);

  
  
  
  
  static bool IsSitePermAllow(nsIPrincipal* aPrincipal, const char* aType);

  
  
  
  
  static bool IsSitePermDeny(nsIPrincipal* aPrincipal, const char* aType);

  
  
  
  
  
  
  static bool IsExactSitePermAllow(nsIPrincipal* aPrincipal, const char* aType);

  
  
  
  
  
  
  static bool IsExactSitePermDeny(nsIPrincipal* aPrincipal, const char* aType);

  
  static bool HaveEqualPrincipals(nsIDocument* aDoc1, nsIDocument* aDoc2);

  static nsILineBreaker* LineBreaker()
  {
    return sLineBreaker;
  }

  static nsIWordBreaker* WordBreaker()
  {
    return sWordBreaker;
  }

  



  static void RegisterShutdownObserver(nsIObserver* aObserver);
  static void UnregisterShutdownObserver(nsIObserver* aObserver);

  



  static bool HasNonEmptyAttr(const nsIContent* aContent, int32_t aNameSpaceID,
                                nsIAtom* aName);

  






  static nsPresContext* GetContextForContent(const nsIContent* aContent);

  

















  static bool CanLoadImage(nsIURI* aURI,
                           nsISupports* aContext,
                           nsIDocument* aLoadingDocument,
                           nsIPrincipal* aLoadingPrincipal,
                           int16_t* aImageBlockingStatus = nullptr,
                           uint32_t aContentPolicyType = nsIContentPolicy::TYPE_IMAGE);

  


  static bool DocumentInactiveForImageLoads(nsIDocument* aDocument);

  
















  static nsresult LoadImage(nsIURI* aURI,
                            nsIDocument* aLoadingDocument,
                            nsIPrincipal* aLoadingPrincipal,
                            nsIURI* aReferrer,
                            mozilla::net::ReferrerPolicy aReferrerPolicy,
                            imgINotificationObserver* aObserver,
                            int32_t aLoadFlags,
                            const nsAString& initiatorType,
                            imgRequestProxy** aRequest,
                            uint32_t aContentPolicyType = nsIContentPolicy::TYPE_IMAGE);

  



  static imgLoader* GetImgLoaderForDocument(nsIDocument* aDoc);
  static imgLoader* GetImgLoaderForChannel(nsIChannel* aChannel,
                                           nsIDocument* aContext);

  


  static bool IsImageInCache(nsIURI* aURI, nsIDocument* aDocument);

  






  static already_AddRefed<imgIContainer> GetImageFromContent(nsIImageLoadingContent* aContent, imgIRequest **aRequest = nullptr);

  


  static already_AddRefed<imgRequestProxy> GetStaticRequest(imgRequestProxy* aRequest);

  





  static bool ContentIsDraggable(nsIContent* aContent);

  





  static bool IsDraggableImage(nsIContent* aContent);

  





  static bool IsDraggableLink(const nsIContent* aContent);

  



  static nsresult NameChanged(mozilla::dom::NodeInfo* aNodeInfo, nsIAtom* aName,
                              mozilla::dom::NodeInfo** aResult);

  





  static void GetEventArgNames(int32_t aNameSpaceID, nsIAtom *aEventName,
                               bool aIsForWindow,
                               uint32_t *aArgCount, const char*** aArgNames);

  


  static bool IsInPrivateBrowsing(nsIDocument* aDoc);

  













  static bool IsInSameAnonymousTree(const nsINode* aNode, const nsIContent* aContent);

  


  static nsIXPConnect *XPConnect()
  {
    return sXPConnect;
  }

  




  static void LogSimpleConsoleError(const nsAString& aErrorText,
                                    const char * classification);

  














  static nsresult ReportToConsoleNonLocalized(const nsAString& aErrorText,
                                              uint32_t aErrorFlags,
                                              const nsACString& aCategory,
                                              nsIDocument* aDocument,
                                              nsIURI* aURI = nullptr,
                                              const nsAFlatString& aSourceLine
                                                = EmptyString(),
                                              uint32_t aLineNumber = 0,
                                              uint32_t aColumnNumber = 0);

  


















  enum PropertiesFile {
    eCSS_PROPERTIES,
    eXBL_PROPERTIES,
    eXUL_PROPERTIES,
    eLAYOUT_PROPERTIES,
    eFORMS_PROPERTIES,
    ePRINTING_PROPERTIES,
    eDOM_PROPERTIES,
    eHTMLPARSER_PROPERTIES,
    eSVG_PROPERTIES,
    eBRAND_PROPERTIES,
    eCOMMON_DIALOG_PROPERTIES,
    eMATHML_PROPERTIES,
    eSECURITY_PROPERTIES,
    eNECKO_PROPERTIES,
    PropertiesFile_COUNT
  };
  static nsresult ReportToConsole(uint32_t aErrorFlags,
                                  const nsACString& aCategory,
                                  nsIDocument* aDocument,
                                  PropertiesFile aFile,
                                  const char *aMessageName,
                                  const char16_t **aParams = nullptr,
                                  uint32_t aParamsLength = 0,
                                  nsIURI* aURI = nullptr,
                                  const nsAFlatString& aSourceLine
                                    = EmptyString(),
                                  uint32_t aLineNumber = 0,
                                  uint32_t aColumnNumber = 0);

  static void LogMessageToConsole(const char* aMsg, ...);
  
  


  static nsresult GetLocalizedString(PropertiesFile aFile,
                                     const char* aKey,
                                     nsXPIDLString& aResult);

  






  static uint32_t ParseSandboxAttributeToFlags(const nsAttrValue* sandboxAttr);


  



private:
  static nsresult FormatLocalizedString(PropertiesFile aFile,
                                        const char* aKey,
                                        const char16_t** aParams,
                                        uint32_t aParamsLength,
                                        nsXPIDLString& aResult);
  
public:
  template<uint32_t N>
  static nsresult FormatLocalizedString(PropertiesFile aFile,
                                        const char* aKey,
                                        const char16_t* (&aParams)[N],
                                        nsXPIDLString& aResult)
  {
    return FormatLocalizedString(aFile, aKey, aParams, N, aResult);
  }

  


  static bool IsChromeDoc(nsIDocument *aDocument);

  


  static bool IsChildOfSameType(nsIDocument* aDoc);

  


  static bool IsPlainTextType(const nsACString& aContentType);

  








  static bool GetWrapperSafeScriptFilename(nsIDocument *aDocument,
                                             nsIURI *aURI,
                                             nsACString& aScriptURI);


  




  static bool IsInChromeDocshell(nsIDocument *aDocument);

  


  static nsIContentPolicy *GetContentPolicy();

  












  static bool HasMutationListeners(nsINode* aNode,
                                     uint32_t aType,
                                     nsINode* aTargetForSubtreeModified);

  










  static bool HasMutationListeners(nsIDocument* aDocument,
                                     uint32_t aType);
  











  static void MaybeFireNodeRemoved(nsINode* aChild, nsINode* aParent,
                                   nsIDocument* aOwnerDoc);

  












  static nsresult DispatchTrustedEvent(nsIDocument* aDoc,
                                       nsISupports* aTarget,
                                       const nsAString& aEventName,
                                       bool aCanBubble,
                                       bool aCancelable,
                                       bool *aDefaultAction = nullptr);

  












  static nsresult DispatchUntrustedEvent(nsIDocument* aDoc,
                                         nsISupports* aTarget,
                                         const nsAString& aEventName,
                                         bool aCanBubble,
                                         bool aCancelable,
                                         bool *aDefaultAction = nullptr);

  

















  static nsresult DispatchChromeEvent(nsIDocument* aDoc,
                                      nsISupports* aTarget,
                                      const nsAString& aEventName,
                                      bool aCanBubble,
                                      bool aCancelable,
                                      bool *aDefaultAction = nullptr);


  

















  static nsresult DispatchEventOnlyToChrome(nsIDocument* aDoc,
                                            nsISupports* aTarget,
                                            const nsAString& aEventName,
                                            bool aCanBubble,
                                            bool aCancelable,
                                            bool *aDefaultAction = nullptr);

  







  static bool IsEventAttributeName(nsIAtom* aName, int32_t aType);

  






  static uint32_t GetEventId(nsIAtom* aName);

  






  static mozilla::EventClassID GetEventClassID(const nsAString& aName);

  








  static nsIAtom* GetEventIdAndAtom(const nsAString& aName,
                                    mozilla::EventClassID aEventClassID,
                                    uint32_t* aEventID);

  










  static void TraverseListenerManager(nsINode *aNode,
                                      nsCycleCollectionTraversalCallback &cb);

  





  static mozilla::EventListenerManager*
    GetListenerManagerForNode(nsINode* aNode);
  





  static mozilla::EventListenerManager*
    GetExistingListenerManagerForNode(const nsINode* aNode);

  static void UnmarkGrayJSListenersInCCGenerationDocuments(uint32_t aGeneration);

  




  static void RemoveListenerManager(nsINode *aNode);

  static bool IsInitialized()
  {
    return sInitialized;
  }

  








  static bool IsValidNodeName(nsIAtom *aLocalName, nsIAtom *aPrefix,
                                int32_t aNamespaceID);

  













  static nsresult CreateContextualFragment(nsINode* aContextNode,
                                           const nsAString& aFragment,
                                           bool aPreventScriptExecution,
                                           nsIDOMDocumentFragment** aReturn);
  static already_AddRefed<mozilla::dom::DocumentFragment>
  CreateContextualFragment(nsINode* aContextNode, const nsAString& aFragment,
                           bool aPreventScriptExecution,
                           mozilla::ErrorResult& aRv);

  














  static nsresult ParseFragmentHTML(const nsAString& aSourceBuffer,
                                    nsIContent* aTargetNode,
                                    nsIAtom* aContextLocalName,
                                    int32_t aContextNamespace,
                                    bool aQuirks,
                                    bool aPreventScriptExecution);

  










  static nsresult ParseFragmentXML(const nsAString& aSourceBuffer,
                                   nsIDocument* aDocument,
                                   nsTArray<nsString>& aTagStack,
                                   bool aPreventScriptExecution,
                                   nsIDOMDocumentFragment** aReturn);

  












  static nsresult ParseDocumentHTML(const nsAString& aSourceBuffer,
                                    nsIDocument* aTargetDocument,
                                    bool aScriptingEnabledForNoscriptParsing);

  













  static nsresult ConvertToPlainText(const nsAString& aSourceBuffer,
                                     nsAString& aResultBuffer,
                                     uint32_t aFlags,
                                     uint32_t aWrapCol);

  













  static nsresult SetNodeTextContent(nsIContent* aContent,
                                     const nsAString& aValue,
                                     bool aTryReuse);

  














  static bool GetNodeTextContent(nsINode* aNode, bool aDeep,
                                 nsAString& aResult) NS_WARN_UNUSED_RESULT;

  


  static bool AppendNodeTextContent(nsINode* aNode, bool aDeep,
                                    nsAString& aResult, const mozilla::fallible_t&);

  






  enum TextContentDiscoverMode : uint8_t {
    eRecurseIntoChildren, eDontRecurseIntoChildren
  };

  static bool HasNonEmptyTextContent(
    nsINode* aNode,
    TextContentDiscoverMode aDiscoverMode = eDontRecurseIntoChildren);

  


  static void DestroyMatchString(void* aData);

  


  static void DestroyAnonymousContent(nsCOMPtr<nsIContent>* aContent);
  static void DestroyAnonymousContent(nsCOMPtr<Element>* aElement);

  




  static void NotifyInstalledMenuKeyboardListener(bool aInstalling);

  






















  static nsresult CheckSecurityBeforeLoad(nsIURI* aURIToLoad,
                                          nsIPrincipal* aLoadingPrincipal,
                                          uint32_t aCheckLoadFlags,
                                          bool aAllowData,
                                          uint32_t aContentPolicyType,
                                          nsISupports* aContext,
                                          const nsAFlatCString& aMimeGuess = EmptyCString(),
                                          nsISupports* aExtra = nullptr);

  


  static bool IsSystemPrincipal(nsIPrincipal* aPrincipal);

  


  static bool IsExpandedPrincipal(nsIPrincipal* aPrincipal);

  


  static bool IsSystemOrExpandedPrincipal(nsIPrincipal* aPrincipal)
  {
    return IsSystemPrincipal(aPrincipal) || IsExpandedPrincipal(aPrincipal);
  }

  


  static nsIPrincipal* GetSystemPrincipal();

  



  static nsIPrincipal* GetNullSubjectPrincipal() { return sNullSubjectPrincipal; }

  













  static bool CombineResourcePrincipals(nsCOMPtr<nsIPrincipal>* aResourcePrincipal,
                                        nsIPrincipal* aExtraPrincipal);

  
















  static void TriggerLink(nsIContent *aContent, nsPresContext *aPresContext,
                          nsIURI *aLinkURI, const nsString& aTargetSpec,
                          bool aClick, bool aIsUserTriggered,
                          bool aIsTrusted);

  


  static void GetLinkLocation(mozilla::dom::Element* aElement,
                              nsString& aLocationString);

  


  static nsIWidget* GetTopLevelWidget(nsIWidget* aWidget);

  


  static const nsDependentString GetLocalizedEllipsis();

  






  static void GetAccelKeyCandidates(nsIDOMKeyEvent* aDOMKeyEvent,
                                    nsTArray<nsShortcutCandidate>& aCandidates);

  






  static void GetAccessKeyCandidates(
                mozilla::WidgetKeyboardEvent* aNativeKeyEvent,
                nsTArray<uint32_t>& aCandidates);

  



  static void HidePopupsInDocument(nsIDocument* aDocument);

  


  static already_AddRefed<nsIDragSession> GetDragSession();

  


  static nsresult SetDataTransferInEvent(mozilla::WidgetDragEvent* aDragEvent);

  
  
  static uint32_t FilterDropEffect(uint32_t aAction, uint32_t aEffectAllowed);

  



  static bool CheckForSubFrameDrop(nsIDragSession* aDragSession,
                                   mozilla::WidgetDragEvent* aDropEvent);

  


  static bool URIIsLocalFile(nsIURI *aURI);

  



  static nsresult SplitURIAtHash(nsIURI *aURI,
                                 nsACString &aBeforeHash,
                                 nsACString &aAfterHash);

  







  static void GetOfflineAppManifest(nsIDocument *aDocument, nsIURI **aURI);

  


  static bool OfflineAppAllowed(nsIURI *aURI);

  


  static bool OfflineAppAllowed(nsIPrincipal *aPrincipal);

  



  static bool MaybeAllowOfflineAppByDefault(nsIPrincipal *aPrincipal, nsIDOMWindow *aWindow);

  




  static void AddScriptBlocker();

  






  static void RemoveScriptBlocker();

  











  static bool AddScriptRunner(nsIRunnable* aRunnable);

  





  static bool IsSafeToRunScript() {
    return sScriptBlockerCount == 0;
  }

  




  static void WarnScriptWasIgnored(nsIDocument* aDocument);

  














  static nsViewportInfo GetViewportInfo(nsIDocument* aDocument,
                                        const mozilla::ScreenIntSize& aDisplaySize);

  
  
  static void EnterMicroTask();
  static void LeaveMicroTask();

  static bool IsInMicroTask();
  static uint32_t MicroTaskLevel();
  static void SetMicroTaskLevel(uint32_t aLevel);

  static void PerformMainThreadMicroTaskCheckpoint();

  





  static nsresult ProcessViewportInfo(nsIDocument *aDocument,
                                      const nsAString &viewportInfo);

  static nsIScriptContext* GetContextForEventHandlers(nsINode* aNode,
                                                      nsresult* aRv);

  static JSContext *GetCurrentJSContext();
  static JSContext *GetSafeJSContext();
  static JSContext *GetCurrentJSContextForThread();
  static JSContext *GetDefaultJSContextForThread();
  inline static JSContext *RootingCx() { return GetSafeJSContext(); }
  inline static JSContext *RootingCxForThread() { return GetDefaultJSContextForThread(); }

  



  static bool EqualsIgnoreASCIICase(const nsAString& aStr1,
                                    const nsAString& aStr2);

  




  static void ASCIIToLower(nsAString& aStr);
  static void ASCIIToLower(const nsAString& aSource, nsAString& aDest);

  




  static void ASCIIToUpper(nsAString& aStr);
  static void ASCIIToUpper(const nsAString& aSource, nsAString& aDest);

  


  static bool StringContainsASCIIUpper(const nsAString& aStr);

  
  static nsresult CheckSameOrigin(nsIChannel *aOldChannel, nsIChannel *aNewChannel);
  static nsIInterfaceRequestor* SameOriginChecker();

  












  static nsresult GetASCIIOrigin(nsIPrincipal* aPrincipal,
                                 nsACString& aOrigin);
  static nsresult GetASCIIOrigin(nsIURI* aURI, nsACString& aOrigin);
  static nsresult GetUTFOrigin(nsIPrincipal* aPrincipal,
                               nsAString& aOrigin);
  static nsresult GetUTFOrigin(nsIURI* aURI, nsAString& aOrigin);

  





  static nsresult DispatchXULCommand(nsIContent* aTarget,
                                     bool aTrusted,
                                     nsIDOMEvent* aSourceEvent = nullptr,
                                     nsIPresShell* aShell = nullptr,
                                     bool aCtrl = false,
                                     bool aAlt = false,
                                     bool aShift = false,
                                     bool aMeta = false);

  






  static nsIDocument*
  GetDocumentFromScriptContext(nsIScriptContext* aScriptContext);

  static bool CheckMayLoad(nsIPrincipal* aPrincipal, nsIChannel* aChannel, bool aAllowIfInheritsPrincipal);

  




  static bool CanAccessNativeAnon();

  MOZ_WARN_UNUSED_RESULT
  static nsresult WrapNative(JSContext *cx, nsISupports *native,
                             const nsIID* aIID, JS::MutableHandle<JS::Value> vp,
                             bool aAllowWrapping = true)
  {
    return WrapNative(cx, native, nullptr, aIID, vp, aAllowWrapping);
  }

  
  MOZ_WARN_UNUSED_RESULT
  static nsresult WrapNative(JSContext *cx, nsISupports *native,
                             JS::MutableHandle<JS::Value> vp,
                             bool aAllowWrapping = true)
  {
    return WrapNative(cx, native, nullptr, nullptr, vp, aAllowWrapping);
  }

  MOZ_WARN_UNUSED_RESULT
  static nsresult WrapNative(JSContext *cx, nsISupports *native,
                             nsWrapperCache *cache,
                             JS::MutableHandle<JS::Value> vp,
                             bool aAllowWrapping = true)
  {
    return WrapNative(cx, native, cache, nullptr, vp, aAllowWrapping);
  }

  


  static nsresult CreateArrayBuffer(JSContext *aCx, const nsACString& aData,
                                    JSObject** aResult);

  static nsresult CreateBlobBuffer(JSContext* aCx,
                                   nsISupports* aParent,
                                   const nsACString& aData,
                                   JS::MutableHandle<JS::Value> aBlob);

  static void StripNullChars(const nsAString& aInStr, nsAString& aOutStr);

  



  static void RemoveNewlines(nsString &aString);

  



  static void PlatformToDOMLineBreaks(nsString &aString);
  static NS_WARN_UNUSED_RESULT bool PlatformToDOMLineBreaks(nsString &aString,
                                                            const mozilla::fallible_t&);

  




  static void PopulateStringFromStringBuffer(nsStringBuffer* aBuf,
                                             nsAString& aResultString);

  static bool IsHandlingKeyBoardEvent()
  {
    return sIsHandlingKeyBoardEvent;
  }

  static void SetIsHandlingKeyBoardEvent(bool aHandling)
  {
    sIsHandlingKeyBoardEvent = aHandling;
  }

  



  static already_AddRefed<nsContentList>
  GetElementsByClassName(nsINode* aRootNode, const nsAString& aClasses)
  {
    NS_PRECONDITION(aRootNode, "Must have root node");

    return NS_GetFuncStringHTMLCollection(aRootNode, MatchClassNames,
                                          DestroyClassNameArray,
                                          AllocClassMatchingInfo,
                                          aClasses);
  }

  








  static nsIPresShell* FindPresShellForDocument(const nsIDocument* aDoc);

  




  static nsIWidget* WidgetForDocument(const nsIDocument* aDoc);

  









  static already_AddRefed<mozilla::layers::LayerManager>
  LayerManagerForDocument(const nsIDocument *aDoc, bool *aAllowRetaining = nullptr);

  













  static already_AddRefed<mozilla::layers::LayerManager>
  PersistentLayerManagerForDocument(nsIDocument *aDoc, bool *aAllowRetaining = nullptr);

  





  static bool IsFocusedContent(const nsIContent *aContent);

  


  static bool IsFullScreenApiEnabled();

  





  static bool IsRequestFullScreenAllowed();

  























  static bool IsFullscreenApiContentOnly();

  


  static bool IsPerformanceTimingEnabled()
  {
    return sIsPerformanceTimingEnabled;
  }
  
  


  static bool IsUserTimingLoggingEnabled()
  {
    return sIsUserTimingLoggingEnabled;
  }

  


  static bool IsResourceTimingEnabled()
  {
    return sIsResourceTimingEnabled;
  }

  



  static bool EncodeDecodeURLHash()
  {
    return sEncodeDecodeURLHash;
  }

  





  static bool HasPluginWithUncontrolledEventDispatch(nsIDocument* aDoc);

  







  static void FireMutationEventsForDirectParsing(nsIDocument* aDoc,
                                                 nsIContent* aDest,
                                                 int32_t aOldChildCount);

  





  static bool HasPluginWithUncontrolledEventDispatch(nsIContent* aContent);

  





  static nsIDocument* GetFullscreenAncestor(nsIDocument* aDoc);

  



  static bool IsInPointerLockContext(nsIDOMWindow* aWin);

  




  static TimeDuration HandlingUserInputTimeout();

  static void GetShiftText(nsAString& text);
  static void GetControlText(nsAString& text);
  static void GetMetaText(nsAString& text);
  static void GetOSText(nsAString& text);
  static void GetAltText(nsAString& text);
  static void GetModifierSeparatorText(nsAString& text);

  







  static bool IsSubDocumentTabbable(nsIContent* aContent);

  






  static bool IsUserFocusIgnored(nsINode* aNode);

  



  static bool HasScrollgrab(nsIContent* aContent);

  





  static void FlushLayoutForTree(nsIDOMWindow* aWindow);

  



  static bool AllowXULXBLForPrincipal(nsIPrincipal* aPrincipal);

  


  static void XPCOMShutdown();

  enum ContentViewerType
  {
      TYPE_UNSUPPORTED,
      TYPE_CONTENT,
      TYPE_PLUGIN,
      TYPE_UNKNOWN
  };

  static already_AddRefed<nsIDocumentLoaderFactory>
  FindInternalContentViewer(const nsACString& aType,
                            ContentViewerType* aLoaderType = nullptr);

  















  static bool IsPatternMatching(nsAString& aValue, nsAString& aPattern,
                                  nsIDocument* aDocument);

  



  static void InitializeTouchEventTable();

  



  static nsresult URIInheritsSecurityContext(nsIURI *aURI, bool *aResult);

  












  static bool ChannelShouldInheritPrincipal(nsIPrincipal* aLoadingPrincipal,
                                            nsIURI* aURI,
                                            bool aInheritForAboutBlank,
                                            bool aForceInherit);

  static nsresult Btoa(const nsAString& aBinaryData,
                       nsAString& aAsciiBase64String);

  static nsresult Atob(const nsAString& aAsciiString,
                       nsAString& aBinaryData);

  








  static bool IsAutocompleteEnabled(nsIDOMHTMLInputElement* aInput);

  enum AutocompleteAttrState : uint8_t
  {
    eAutocompleteAttrState_Unknown = 1,
    eAutocompleteAttrState_Invalid,
    eAutocompleteAttrState_Valid,
  };
  






  static AutocompleteAttrState
  SerializeAutocompleteAttribute(const nsAttrValue* aAttr,
                                 nsAString& aResult,
                                 AutocompleteAttrState aCachedState =
                                   eAutocompleteAttrState_Unknown);

  




  static AutocompleteAttrState
  SerializeAutocompleteAttribute(const nsAttrValue* aAttr,
                                 mozilla::dom::AutocompleteInfo& aInfo,
                                 AutocompleteAttrState aCachedState =
                                   eAutocompleteAttrState_Unknown);

  













  static bool GetPseudoAttributeValue(const nsString& aSource, nsIAtom *aName,
                                      nsAString& aValue);

  



  static bool IsJavaScriptLanguage(const nsString& aName);

  



  static JSVersion ParseJavascriptVersion(const nsAString& aVersionStr);

  static bool IsJavascriptMIMEType(const nsAString& aMIMEType);

  static void SplitMimeType(const nsAString& aValue, nsString& aType,
                            nsString& aParams);

  










  static nsresult IsUserIdle(uint32_t aRequestedIdleTimeInMS, bool* aUserIsIdle);

  










  static void GetSelectionInTextControl(mozilla::dom::Selection* aSelection,
                                        Element* aRoot,
                                        int32_t& aOutStartOffset,
                                        int32_t& aOutEndOffset);

  





  static nsRect GetSelectionBoundingRect(mozilla::dom::Selection* aSel);

  











  static int32_t GetAdjustedOffsetInTextControl(nsIFrame* aOffsetFrame,
                                                int32_t aOffset);

  static nsIEditor* GetHTMLEditor(nsPresContext* aPresContext);

  







  static bool InternalIsSupported(nsISupports* aObject,
                                  const nsAString& aFeature,
                                  const nsAString& aVersion);

  


  static bool DOMWindowDumpEnabled();

  








  static bool IsContentInsertionPoint(const nsIContent* aContent);


  



  static bool HasDistributedChildren(nsIContent* aContent);

  



  static bool IsForbiddenRequestHeader(const nsACString& aHeader);

  



  static bool IsForbiddenSystemRequestHeader(const nsACString& aHeader);

  



  static bool IsAllowedNonCorsContentType(const nsACString& aHeaderValue);

  



  static bool IsForbiddenResponseHeader(const nsACString& aHeader);

  


  static uint64_t GetInnerWindowID(nsIRequest* aRequest);

  



  static void GetHostOrIPv6WithBrackets(nsIURI* aURI, nsAString& aHost);

  



  static void CallOnAllRemoteChildren(nsIDOMWindow* aWindow,
                                      CallOnRemoteChildFunction aCallback,
                                      void* aArg);

  static void TransferablesToIPCTransferables(nsISupportsArray* aTransferables,
                                              nsTArray<mozilla::dom::IPCDataTransfer>& aIPC,
                                              mozilla::dom::nsIContentChild* aChild,
                                              mozilla::dom::nsIContentParent* aParent);

  static void TransferableToIPCTransferable(nsITransferable* aTransferable,
                                            mozilla::dom::IPCDataTransfer* aIPCDataTransfer,
                                            mozilla::dom::nsIContentChild* aChild,
                                            mozilla::dom::nsIContentParent* aParent);

  





  static const uint8_t* GetSurfaceData(mozilla::gfx::DataSourceSurface* aSurface,
                                       size_t* aLength, int32_t* aStride);

private:
  static bool InitializeEventTable();

  static nsresult EnsureStringBundle(PropertiesFile aFile);

  static bool CanCallerAccess(nsIPrincipal* aSubjectPrincipal,
                                nsIPrincipal* aPrincipal);

  static nsresult WrapNative(JSContext *cx, nsISupports *native,
                             nsWrapperCache *cache, const nsIID* aIID,
                             JS::MutableHandle<JS::Value> vp,
                             bool aAllowWrapping);

  static nsresult DispatchEvent(nsIDocument* aDoc,
                                nsISupports* aTarget,
                                const nsAString& aEventName,
                                bool aCanBubble,
                                bool aCancelable,
                                bool aTrusted,
                                bool *aDefaultAction = nullptr,
                                bool aOnlyChromeDispatch = false);

  static void InitializeModifierStrings();

  static void DropFragmentParsers();

  static bool MatchClassNames(nsIContent* aContent, int32_t aNamespaceID,
                              nsIAtom* aAtom, void* aData);
  static void DestroyClassNameArray(void* aData);
  static void* AllocClassMatchingInfo(nsINode* aRootNode,
                                      const nsString* aClasses);

  
  static AutocompleteAttrState InternalSerializeAutocompleteAttribute(const nsAttrValue* aAttrVal,
                                                                      mozilla::dom::AutocompleteInfo& aInfo);

  static void CallOnAllRemoteChildren(nsIMessageBroadcaster* aManager,
                                      CallOnRemoteChildFunction aCallback,
                                      void* aArg);

  static nsIXPConnect *sXPConnect;

  static nsIScriptSecurityManager *sSecurityManager;
  static nsIPrincipal *sSystemPrincipal;
  static nsIPrincipal *sNullSubjectPrincipal;

  static nsIParserService *sParserService;

  static nsNameSpaceManager *sNameSpaceManager;

  static nsIIOService *sIOService;

  static bool sImgLoaderInitialized;
  static void InitImgLoader();

  
  static imgLoader* sImgLoader;
  static imgLoader* sPrivateImgLoader;
  static imgICache* sImgCache;
  static imgICache* sPrivateImgCache;

  static nsIConsoleService* sConsoleService;

  static nsDataHashtable<nsISupportsHashKey, EventNameMapping>* sAtomEventTable;
  static nsDataHashtable<nsStringHashKey, EventNameMapping>* sStringEventTable;
  static nsCOMArray<nsIAtom>* sUserDefinedEvents;

  static nsIStringBundleService* sStringBundleService;
  static nsIStringBundle* sStringBundles[PropertiesFile_COUNT];

  static nsIContentPolicy* sContentPolicyService;
  static bool sTriedToGetContentPolicy;

  static nsILineBreaker* sLineBreaker;
  static nsIWordBreaker* sWordBreaker;

  static nsIBidiKeyboard* sBidiKeyboard;

  static bool sInitialized;
  static uint32_t sScriptBlockerCount;
  static uint32_t sDOMNodeRemovedSuppressCount;
  static uint32_t sMicroTaskLevel;
  
  static nsTArray< nsCOMPtr<nsIRunnable> >* sBlockedScriptRunners;
  static uint32_t sRunnersCountAtFirstBlocker;
  static uint32_t sScriptBlockerCountWhereRunnersPrevented;

  static nsIInterfaceRequestor* sSameOriginChecker;

  static bool sIsHandlingKeyBoardEvent;
  static bool sAllowXULXBL_for_file;
  static bool sIsFullScreenApiEnabled;
  static bool sTrustedFullScreenOnly;
  static bool sFullscreenApiIsContentOnly;
  static uint32_t sHandlingInputTimeout;
  static bool sIsPerformanceTimingEnabled;
  static bool sIsResourceTimingEnabled;
  static bool sIsUserTimingLoggingEnabled;
  static bool sIsExperimentalAutocompleteEnabled;
  static bool sEncodeDecodeURLHash;

  static nsHtml5StringParser* sHTMLFragmentParser;
  static nsIParser* sXMLFragmentParser;
  static nsIFragmentContentSink* sXMLFragmentSink;

  


  static bool sFragmentParsingActive;

  static nsString* sShiftText;
  static nsString* sControlText;
  static nsString* sMetaText;
  static nsString* sOSText;
  static nsString* sAltText;
  static nsString* sModifierSeparator;

#if !(defined(DEBUG) || defined(MOZ_ENABLE_JS_DUMP))
  static bool sDOMWindowDumpEnabled;
#endif
};

class MOZ_STACK_CLASS nsAutoScriptBlocker {
public:
  explicit nsAutoScriptBlocker(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM) {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    nsContentUtils::AddScriptBlocker();
  }
  ~nsAutoScriptBlocker() {
    nsContentUtils::RemoveScriptBlocker();
  }
private:
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class MOZ_STACK_CLASS nsAutoScriptBlockerSuppressNodeRemoved :
                          public nsAutoScriptBlocker {
public:
  nsAutoScriptBlockerSuppressNodeRemoved() {
    ++nsContentUtils::sDOMNodeRemovedSuppressCount;
  }
  ~nsAutoScriptBlockerSuppressNodeRemoved() {
    --nsContentUtils::sDOMNodeRemovedSuppressCount;
  }
};

class MOZ_STACK_CLASS nsAutoMicroTask
{
public:
  nsAutoMicroTask()
  {
    nsContentUtils::EnterMicroTask();
  }
  ~nsAutoMicroTask()
  {
    nsContentUtils::LeaveMicroTask();
  }
};

namespace mozilla {
namespace dom {

class TreeOrderComparator {
public:
  bool Equals(nsINode* aElem1, nsINode* aElem2) const {
    return aElem1 == aElem2;
  }
  bool LessThan(nsINode* aElem1, nsINode* aElem2) const {
    return nsContentUtils::PositionIsBefore(aElem1, aElem2);
  }
};

} 
} 

#define NS_INTERFACE_MAP_ENTRY_TEAROFF(_interface, _allocator)                \
  if (aIID.Equals(NS_GET_IID(_interface))) {                                  \
    foundInterface = static_cast<_interface *>(_allocator);                   \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nullptr;                                                \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else






#define NS_ENSURE_FINITE(f, rv)                                               \
  if (!mozilla::IsFinite(f)) {                                                \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE2(f1, f2, rv)                                         \
  if (!mozilla::IsFinite((f1)+(f2))) {                                        \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE4(f1, f2, f3, f4, rv)                                 \
  if (!mozilla::IsFinite((f1)+(f2)+(f3)+(f4))) {                              \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE5(f1, f2, f3, f4, f5, rv)                             \
  if (!mozilla::IsFinite((f1)+(f2)+(f3)+(f4)+(f5))) {                         \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE6(f1, f2, f3, f4, f5, f6, rv)                         \
  if (!mozilla::IsFinite((f1)+(f2)+(f3)+(f4)+(f5)+(f6))) {                    \
    return (rv);                                                              \
  }


#define NS_CONTENT_DELETE_LIST_MEMBER(type_, ptr_, member_)                   \
  {                                                                           \
    type_ *cur = (ptr_)->member_;                                             \
    (ptr_)->member_ = nullptr;                                                 \
    while (cur) {                                                             \
      type_ *next = cur->member_;                                             \
      cur->member_ = nullptr;                                                  \
      delete cur;                                                             \
      cur = next;                                                             \
    }                                                                         \
  }

#endif
