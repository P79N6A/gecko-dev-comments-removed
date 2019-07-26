







#ifndef nsContentUtils_h___
#define nsContentUtils_h___

#include <math.h>
#if defined(XP_WIN) || defined(XP_OS2)
#include <float.h>
#endif

#if defined(SOLARIS)
#include <ieeefp.h>
#endif

#include "js/TypeDecls.h"
#include "js/RootingAPI.h"
#include "mozilla/Assertions.h"
#include "mozilla/EventForwards.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/TimeStamp.h"
#include "nsContentListDeclarations.h"
#include "nsMathUtils.h"
#include "Units.h"

class imgICache;
class imgIContainer;
class imgINotificationObserver;
class imgIRequest;
class imgLoader;
class imgRequestProxy;
class nsAutoScriptBlockerSuppressNodeRemoved;
class nsEventListenerManager;
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
class nsINameSpaceManager;
class nsINodeInfo;
class nsIObserver;
class nsIParser;
class nsIParserService;
class nsIPresShell;
class nsIPrincipal;
class nsIRunnable;
class nsIScriptContext;
class nsIScriptGlobalObject;
class nsIScriptSecurityManager;
class nsIStringBundle;
class nsIStringBundleService;
class nsISupportsHashKey;
class nsIURI;
class nsIWidget;
class nsIWordBreaker;
class nsIXPConnect;
class nsIXPConnectJSObjectHolder;
class nsNodeInfoManager;
class nsPIDOMWindow;
class nsPresContext;
class nsScriptObjectTracer;
class nsStringBuffer;
class nsStringHashKey;
class nsTextFragment;
class nsViewportInfo;
class nsWrapperCache;

struct JSPropertyDescriptor;
struct JSRuntime;
struct nsIntMargin;

template<class E> class nsCOMArray;
template<class E> class nsTArray;
template<class K, class V> class nsDataHashtable;
template<class K, class V> class nsRefPtrHashtable;
template<class T> class nsReadingIterator;

namespace mozilla {
class ErrorResult;
class Selection;

namespace dom {
class DocumentFragment;
class Element;
class EventTarget;
} 

namespace layers {
class LayerManager;
} 




typedef void* (*DeferredFinalizeAppendFunction)(void* pointers, void* thing);




typedef bool (*DeferredFinalizeFunction)(uint32_t slice, void* data);

} 

#ifdef IBMBIDI
class nsIBidiKeyboard;
#endif

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
  nsIAtom* mAtom;
  uint32_t mId;
  int32_t  mType;
  uint32_t mStructType;
};

struct nsShortcutCandidate {
  nsShortcutCandidate(uint32_t aCharCode, bool aIgnoreShift) :
    mCharCode(aCharCode), mIgnoreShift(aIgnoreShift)
  {
  }
  uint32_t mCharCode;
  bool     mIgnoreShift;
};

class nsContentUtils
{
  friend class nsAutoScriptBlockerSuppressNodeRemoved;
  typedef mozilla::dom::Element Element;
  typedef mozilla::TimeDuration TimeDuration;

public:
  static nsresult Init();

  


  static JSContext* GetContextFromDocument(nsIDocument *aDocument);

  static bool     IsCallerChrome();
  static bool     ThreadsafeIsCallerChrome();
  static bool     IsCallerXBL();

  static bool     IsImageSrcSetDisabled();

  static bool LookupBindingMember(JSContext* aCx, nsIContent *aContent,
                                  JS::HandleId aId, JS::MutableHandle<JSPropertyDescriptor> aDesc);

  


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
                                                 PRUnichar* aDest,
                                                 uint32_t aLength,
                                                 bool& aLastCharCR);

  static uint32_t CopyNewlineNormalizedUnicodeTo(nsReadingIterator<PRUnichar>& aSrcStart, const nsReadingIterator<PRUnichar>& aSrcEnd, nsAString& aDest);

  static const nsDependentSubstring TrimCharsInSet(const char* aSet,
                                                   const nsAString& aValue);

  template<bool IsWhitespace(PRUnichar)>
  static const nsDependentSubstring TrimWhitespace(const nsAString& aStr,
                                                   bool aTrimTrailing = true);

  


  static bool IsFirstLetterPunctuation(uint32_t aChar);
  static bool IsFirstLetterPunctuationAt(const nsTextFragment* aFrag, uint32_t aOffset);
 
  


  static bool IsAlphanumeric(uint32_t aChar);
  static bool IsAlphanumericAt(const nsTextFragment* aFrag, uint32_t aOffset);

  







  static bool IsHTMLWhitespace(PRUnichar aChar);

  



  static bool IsHTMLWhitespaceOrNBSP(PRUnichar aChar);

  


  static bool IsHTMLBlock(nsIAtom* aLocalName);

  


  static bool IsHTMLVoid(nsIAtom* aLocalName);

  







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

  



  static nsPIDOMWindow *GetWindowFromCaller();

  















  





  static nsIDocument* GetDocumentFromCaller();

  






  static nsIDocument* GetDocumentFromContext();

  
  
  static bool InProlog(nsINode *aNode);

  static nsIParserService* GetParserService();

  static nsINameSpaceManager* NameSpaceManager()
  {
    return sNameSpaceManager;
  }

  static nsIIOService* GetIOService()
  {
    return sIOService;
  }

#ifdef IBMBIDI
  static nsIBidiKeyboard* GetBidiKeyboard();
#endif
  
  



  static nsIScriptSecurityManager* GetSecurityManager()
  {
    return sSecurityManager;
  }

  


  static bool GetContentSecurityPolicy(JSContext* aCx,
                                       nsIContentSecurityPolicy** aCSP);

  
  
  static nsIPrincipal* GetSubjectPrincipal();

  
  
  
  
  
  static nsIPrincipal* GetObjectPrincipal(JSObject* aObj);

  static nsresult GenerateStateKey(nsIContent* aContent,
                                   const nsIDocument* aDocument,
                                   nsACString& aKey);

  




  static nsresult NewURIWithDocumentCharset(nsIURI** aResult,
                                            const nsAString& aSpec,
                                            nsIDocument* aDocument,
                                            nsIURI* aBaseURI);

  




  static nsresult ConvertStringFromCharset(const nsACString& aCharset,
                                           const nsACString& aInput,
                                           nsAString& aOutput);

  








  static bool CheckForBOM(const unsigned char* aBuffer, uint32_t aLength,
                          nsACString& aCharset);

  static nsresult GuessCharset(const char *aData, uint32_t aDataLen,
                               nsACString &aCharset);

  static nsresult CheckQName(const nsAString& aQualifiedName,
                             bool aNamespaceAware = true,
                             const PRUnichar** aColon = nullptr);

  static nsresult SplitQName(const nsIContent* aNamespaceResolver,
                             const nsAFlatString& aQName,
                             int32_t *aNamespace, nsIAtom **aLocalName);

  static nsresult GetNodeInfoFromQName(const nsAString& aNamespaceURI,
                                       const nsAString& aQualifiedName,
                                       nsNodeInfoManager* aNodeInfoManager,
                                       uint16_t aNodeType,
                                       nsINodeInfo** aNodeInfo);

  static void SplitExpatName(const PRUnichar *aExpatName, nsIAtom **aPrefix,
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
                             int16_t* aImageBlockingStatus = nullptr);
  












  static nsresult LoadImage(nsIURI* aURI,
                            nsIDocument* aLoadingDocument,
                            nsIPrincipal* aLoadingPrincipal,
                            nsIURI* aReferrer,
                            imgINotificationObserver* aObserver,
                            int32_t aLoadFlags,
                            imgRequestProxy** aRequest);

  



  static imgLoader* GetImgLoaderForDocument(nsIDocument* aDoc);
  static imgLoader* GetImgLoaderForChannel(nsIChannel* aChannel);

  


  static bool IsImageInCache(nsIURI* aURI, nsIDocument* aDocument);

  






  static already_AddRefed<imgIContainer> GetImageFromContent(nsIImageLoadingContent* aContent, imgIRequest **aRequest = nullptr);

  


  static already_AddRefed<imgRequestProxy> GetStaticRequest(imgRequestProxy* aRequest);

  





  static bool ContentIsDraggable(nsIContent* aContent);

  





  static bool IsDraggableImage(nsIContent* aContent);

  





  static bool IsDraggableLink(const nsIContent* aContent);

  



  static nsresult NameChanged(nsINodeInfo* aNodeInfo, nsIAtom* aName,
                              nsINodeInfo** aResult);

  





  static void GetEventArgNames(int32_t aNameSpaceID, nsIAtom *aEventName,
                               uint32_t *aArgCount, const char*** aArgNames);

  













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
    PropertiesFile_COUNT
  };
  static nsresult ReportToConsole(uint32_t aErrorFlags,
                                  const nsACString& aCategory,
                                  nsIDocument* aDocument,
                                  PropertiesFile aFile,
                                  const char *aMessageName,
                                  const PRUnichar **aParams = nullptr,
                                  uint32_t aParamsLength = 0,
                                  nsIURI* aURI = nullptr,
                                  const nsAFlatString& aSourceLine
                                    = EmptyString(),
                                  uint32_t aLineNumber = 0,
                                  uint32_t aColumnNumber = 0);

  


  static nsresult GetLocalizedString(PropertiesFile aFile,
                                     const char* aKey,
                                     nsXPIDLString& aResult);

  






  static uint32_t ParseSandboxAttributeToFlags(const nsAString& aSandboxAttr);


  



private:
  static nsresult FormatLocalizedString(PropertiesFile aFile,
                                        const char* aKey,
                                        const PRUnichar** aParams,
                                        uint32_t aParamsLength,
                                        nsXPIDLString& aResult);
  
public:
  template<uint32_t N>
  static nsresult FormatLocalizedString(PropertiesFile aFile,
                                        const char* aKey,
                                        const PRUnichar* (&aParams)[N],
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

  







  static bool IsEventAttributeName(nsIAtom* aName, int32_t aType);

  






  static uint32_t GetEventId(nsIAtom* aName);

  






  static uint32_t GetEventCategory(const nsAString& aName);

  








  static nsIAtom* GetEventIdAndAtom(const nsAString& aName,
                                    uint32_t aEventStruct,
                                    uint32_t* aEventID);

  










  static void TraverseListenerManager(nsINode *aNode,
                                      nsCycleCollectionTraversalCallback &cb);

  







  static nsEventListenerManager* GetListenerManager(nsINode* aNode,
                                                    bool aCreateIfNotFound);

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

  













  static void GetNodeTextContent(nsINode* aNode, bool aDeep,
                                 nsAString& aResult);

  


  static void AppendNodeTextContent(nsINode* aNode, bool aDeep,
                                    nsAString& aResult);

  





  static bool HasNonEmptyTextContent(nsINode* aNode);

  


  static void DestroyMatchString(void* aData);

  


  static void DestroyAnonymousContent(nsCOMPtr<nsIContent>* aContent);

  static void DeferredFinalize(nsISupports* aSupports);
  static void DeferredFinalize(mozilla::DeferredFinalizeAppendFunction aAppendFunc,
                               mozilla::DeferredFinalizeFunction aFunc,
                               void* aThing);

  




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

  


  static nsIPrincipal* GetSystemPrincipal();

  













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

  




  static nsEvent* GetNativeEvent(nsIDOMEvent* aDOMEvent);

  






  static void GetAccelKeyCandidates(nsIDOMKeyEvent* aDOMKeyEvent,
                                    nsTArray<nsShortcutCandidate>& aCandidates);

  






  static void GetAccessKeyCandidates(
                mozilla::WidgetKeyboardEvent* aNativeKeyEvent,
                nsTArray<uint32_t>& aCandidates);

  



  static void HidePopupsInDocument(nsIDocument* aDocument);

  


  static already_AddRefed<nsIDragSession> GetDragSession();

  


  static nsresult SetDataTransferInEvent(nsDragEvent* aDragEvent);

  
  
  static uint32_t FilterDropEffect(uint32_t aAction, uint32_t aEffectAllowed);

  



  static bool CheckForSubFrameDrop(nsIDragSession* aDragSession,
                                   nsDragEvent* aDropEvent);

  


  static bool URIIsLocalFile(nsIURI *aURI);

  



  static nsresult SplitURIAtHash(nsIURI *aURI,
                                 nsACString &aBeforeHash,
                                 nsACString &aAfterHash);

  







  static void GetOfflineAppManifest(nsIDocument *aDocument, nsIURI **aURI);

  


  static bool OfflineAppAllowed(nsIURI *aURI);

  


  static bool OfflineAppAllowed(nsIPrincipal *aPrincipal);

  



  static bool MaybeAllowOfflineAppByDefault(nsIPrincipal *aPrincipal);

  




  static void AddScriptBlocker();

  






  static void RemoveScriptBlocker();

  











  static bool AddScriptRunner(nsIRunnable* aRunnable);

  





  static bool IsSafeToRunScript() {
    return sScriptBlockerCount == 0;
  }

  














  static nsViewportInfo GetViewportInfo(nsIDocument* aDocument,
                                        const mozilla::ScreenIntSize& aDisplaySize);

  
  
  static void EnterMicroTask();
  static void LeaveMicroTask();

  static bool IsInMicroTask();
  static uint32_t MicroTaskLevel();
  static void SetMicroTaskLevel(uint32_t aLevel);

  





  static nsresult ProcessViewportInfo(nsIDocument *aDocument,
                                      const nsAString &viewportInfo);

  static nsIScriptContext* GetContextForEventHandlers(nsINode* aNode,
                                                      nsresult* aRv);

  static JSContext *GetCurrentJSContext();
  static JSContext *GetSafeJSContext();
  static JSContext *GetDefaultJSContextForThread();

  



  static bool EqualsIgnoreASCIICase(const nsAString& aStr1,
                                    const nsAString& aStr2);

  




  static nsresult ASCIIToLower(nsAString& aStr);
  static nsresult ASCIIToLower(const nsAString& aSource, nsAString& aDest);

  




  static nsresult ASCIIToUpper(nsAString& aStr);
  static nsresult ASCIIToUpper(const nsAString& aSource, nsAString& aDest);

  


  static bool StringContainsASCIIUpper(const nsAString& aStr);

  
  static nsresult CheckSameOrigin(nsIChannel *aOldChannel, nsIChannel *aNewChannel);
  static nsIInterfaceRequestor* GetSameOriginChecker();

  
  static void TraceSafeJSContext(JSTracer* aTrc);


  












  static nsresult GetASCIIOrigin(nsIPrincipal* aPrincipal,
                                 nsCString& aOrigin);
  static nsresult GetASCIIOrigin(nsIURI* aURI, nsCString& aOrigin);
  static nsresult GetUTFOrigin(nsIPrincipal* aPrincipal,
                               nsString& aOrigin);
  static nsresult GetUTFOrigin(nsIURI* aURI, nsString& aOrigin);
  static void GetUTFNonNullOrigin(nsIURI* aURI, nsString& aOrigin);

  





  static nsresult DispatchXULCommand(nsIContent* aTarget,
                                     bool aTrusted,
                                     nsIDOMEvent* aSourceEvent = nullptr,
                                     nsIPresShell* aShell = nullptr,
                                     bool aCtrl = false,
                                     bool aAlt = false,
                                     bool aShift = false,
                                     bool aMeta = false);

  






  static already_AddRefed<nsIDocument>
  GetDocumentFromScriptContext(nsIScriptContext *aScriptContext);

  static bool CheckMayLoad(nsIPrincipal* aPrincipal, nsIChannel* aChannel, bool aAllowIfInheritsPrincipal);

  




  static bool CanAccessNativeAnon();

  static nsresult WrapNative(JSContext *cx, JS::Handle<JSObject*> scope,
                             nsISupports *native, const nsIID* aIID,
                             JS::Value *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nullptr,
                             bool aAllowWrapping = false)
  {
    return WrapNative(cx, scope, native, nullptr, aIID, vp, aHolder,
                      aAllowWrapping);
  }

  
  static nsresult WrapNative(JSContext *cx, JS::Handle<JSObject*> scope,
                             nsISupports *native, JS::Value *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nullptr,
                             bool aAllowWrapping = false)
  {
    return WrapNative(cx, scope, native, nullptr, nullptr, vp, aHolder,
                      aAllowWrapping);
  }
  static nsresult WrapNative(JSContext *cx, JS::Handle<JSObject*> scope,
                             nsISupports *native, nsWrapperCache *cache,
                             JS::Value *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nullptr,
                             bool aAllowWrapping = false)
  {
    return WrapNative(cx, scope, native, cache, nullptr, vp, aHolder,
                      aAllowWrapping);
  }

  


  static nsresult CreateArrayBuffer(JSContext *aCx, const nsACString& aData,
                                    JSObject** aResult);

  static nsresult CreateBlobBuffer(JSContext* aCx,
                                   const nsACString& aData,
                                   JS::MutableHandle<JS::Value> aBlob);

  static void StripNullChars(const nsAString& aInStr, nsAString& aOutStr);

  



  static void RemoveNewlines(nsString &aString);

  



  static void PlatformToDOMLineBreaks(nsString &aString);

  




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

  


  static bool IsIdleObserverAPIEnabled() { return sIsIdleObserverAPIEnabled; }

  


  static bool IsPerformanceTimingEnabled()
  {
    return sIsPerformanceTimingEnabled;
  }
  
  





  static bool HasPluginWithUncontrolledEventDispatch(nsIDocument* aDoc);

  





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
  FindInternalContentViewer(const char* aType,
                            ContentViewerType* aLoaderType = nullptr);

  















  static bool IsPatternMatching(nsAString& aValue, nsAString& aPattern,
                                  nsIDocument* aDocument);

  



  static void InitializeTouchEventTable();

  



  static nsresult URIInheritsSecurityContext(nsIURI *aURI, bool *aResult);

  









  static bool SetUpChannelOwner(nsIPrincipal* aLoadingPrincipal,
                                nsIChannel* aChannel,
                                nsIURI* aURI,
                                bool aSetUpForAboutBlank,
                                bool aForceOwner = false);

  static nsresult Btoa(const nsAString& aBinaryData,
                       nsAString& aAsciiBase64String);

  static nsresult Atob(const nsAString& aAsciiString,
                       nsAString& aBinaryData);

  








  static bool IsAutocompleteEnabled(nsIDOMHTMLInputElement* aInput);

  













  static bool GetPseudoAttributeValue(const nsString& aSource, nsIAtom *aName,
                                      nsAString& aValue);

  



  static bool IsJavaScriptLanguage(const nsString& aName);

  



  static JSVersion ParseJavascriptVersion(const nsAString& aVersionStr);

  static bool IsJavascriptMIMEType(const nsAString& aMIMEType);

  static void SplitMimeType(const nsAString& aValue, nsString& aType,
                            nsString& aParams);

  










  static nsresult IsUserIdle(uint32_t aRequestedIdleTimeInMS, bool* aUserIsIdle);

  










  static void GetSelectionInTextControl(mozilla::Selection* aSelection,
                                        Element* aRoot,
                                        int32_t& aOutStartOffset,
                                        int32_t& aOutEndOffset);

  











  static int32_t GetAdjustedOffsetInTextControl(nsIFrame* aOffsetFrame,
                                                int32_t aOffset);

  static nsIEditor* GetHTMLEditor(nsPresContext* aPresContext);

  







  static bool InternalIsSupported(nsISupports* aObject,
                                  const nsAString& aFeature,
                                  const nsAString& aVersion);

  


  static bool DOMWindowDumpEnabled();

private:
  static bool InitializeEventTable();

  static nsresult EnsureStringBundle(PropertiesFile aFile);

  static bool CanCallerAccess(nsIPrincipal* aSubjectPrincipal,
                                nsIPrincipal* aPrincipal);

  static nsresult WrapNative(JSContext *cx, JS::Handle<JSObject*> scope,
                             nsISupports *native, nsWrapperCache *cache,
                             const nsIID* aIID, JS::Value *vp,
                             nsIXPConnectJSObjectHolder** aHolder,
                             bool aAllowWrapping);

  static nsresult DispatchEvent(nsIDocument* aDoc,
                                nsISupports* aTarget,
                                const nsAString& aEventName,
                                bool aCanBubble,
                                bool aCancelable,
                                bool aTrusted,
                                bool *aDefaultAction = nullptr);

  static void InitializeModifierStrings();

  static void DropFragmentParsers();

  static bool MatchClassNames(nsIContent* aContent, int32_t aNamespaceID,
                              nsIAtom* aAtom, void* aData);
  static void DestroyClassNameArray(void* aData);
  static void* AllocClassMatchingInfo(nsINode* aRootNode,
                                      const nsString* aClasses);

  static nsIXPConnect *sXPConnect;

  static nsIScriptSecurityManager *sSecurityManager;

  static nsIParserService *sParserService;

  static nsINameSpaceManager *sNameSpaceManager;

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

#ifdef IBMBIDI
  static nsIBidiKeyboard* sBidiKeyboard;
#endif

  static bool sInitialized;
  static uint32_t sScriptBlockerCount;
#ifdef DEBUG
  static uint32_t sDOMNodeRemovedSuppressCount;
#endif
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
  static bool sIsIdleObserverAPIEnabled;
  static bool sIsPerformanceTimingEnabled;

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
  nsAutoScriptBlocker(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM) {
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
#ifdef DEBUG
    ++nsContentUtils::sDOMNodeRemovedSuppressCount;
#endif
  }
  ~nsAutoScriptBlockerSuppressNodeRemoved() {
#ifdef DEBUG
    --nsContentUtils::sDOMNodeRemovedSuppressCount;
#endif
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

#define NS_INTERFACE_MAP_ENTRY_TEAROFF(_interface, _allocator)                \
  if (aIID.Equals(NS_GET_IID(_interface))) {                                  \
    foundInterface = static_cast<_interface *>(_allocator);                   \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nullptr;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else






#define NS_ENSURE_FINITE(f, rv)                                               \
  if (!NS_finite(f)) {                                                        \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE2(f1, f2, rv)                                         \
  if (!NS_finite((f1)+(f2))) {                                                \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE4(f1, f2, f3, f4, rv)                                 \
  if (!NS_finite((f1)+(f2)+(f3)+(f4))) {                                      \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE5(f1, f2, f3, f4, f5, rv)                             \
  if (!NS_finite((f1)+(f2)+(f3)+(f4)+(f5))) {                                 \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE6(f1, f2, f3, f4, f5, f6, rv)                         \
  if (!NS_finite((f1)+(f2)+(f3)+(f4)+(f5)+(f6))) {                            \
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
