







#ifndef nsContentUtils_h___
#define nsContentUtils_h___

#include <math.h>
#if defined(XP_WIN) || defined(XP_OS2)
#include <float.h>
#endif

#if defined(SOLARIS)
#include <ieeefp.h>
#endif


#ifdef __FreeBSD__
#include <ieeefp.h>
#ifdef __alpha__
static fp_except_t allmask = FP_X_INV|FP_X_OFL|FP_X_UFL|FP_X_DZ|FP_X_IMP;
#else
static fp_except_t allmask = FP_X_INV|FP_X_OFL|FP_X_UFL|FP_X_DZ|FP_X_IMP|FP_X_DNML;
#endif
static fp_except_t oldmask = fpsetmask(~allmask);
#endif

#include "nsAString.h"
#include "nsIStatefulFrame.h"
#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsContentList.h"
#include "nsDOMClassInfoID.h"
#include "nsIXPCScriptable.h"
#include "nsDataHashtable.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMEvent.h"
#include "nsTArray.h"
#include "nsTextFragment.h"
#include "nsReadableUtils.h"
#include "nsINode.h"
#include "nsHashtable.h"
#include "nsIDOMNode.h"
#include "nsHtml5StringParser.h"
#include "nsIParser.h"
#include "nsIDocument.h"
#include "nsIFragmentContentSink.h"
#include "nsContentSink.h"
#include "nsMathUtils.h"
#include "nsThreadUtils.h"
#include "nsIContent.h"
#include "nsCharSeparatedTokenizer.h"

#include "mozilla/AutoRestore.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/TimeStamp.h"

struct nsNativeKeyEvent; 

class nsIDOMScriptObjectFactory;
class nsIXPConnect;
class nsIContent;
class nsIDOMKeyEvent;
class nsIDocument;
class nsIDocumentObserver;
class nsIDocShell;
class nsINameSpaceManager;
class nsIScriptSecurityManager;
class nsIJSContextStack;
class nsIThreadJSContextStack;
class nsIParserService;
class nsIIOService;
class nsIURI;
class imgIContainer;
class imgIDecoderObserver;
class imgIRequest;
class imgILoader;
class imgICache;
class nsIImageLoadingContent;
class nsIDOMHTMLFormElement;
class nsIDOMDocument;
class nsIConsoleService;
class nsIStringBundleService;
class nsIStringBundle;
class nsIContentPolicy;
class nsILineBreaker;
class nsIWordBreaker;
class nsIJSRuntimeService;
class nsEventListenerManager;
class nsIScriptContext;
class nsIRunnable;
class nsIInterfaceRequestor;
template<class E> class nsCOMArray;
template<class K, class V> class nsRefPtrHashtable;
struct JSRuntime;
class nsIWidget;
class nsIDragSession;
class nsIPresShell;
class nsIXPConnectJSObjectHolder;
#ifdef MOZ_XTF
class nsIXTFService;
#endif
#ifdef IBMBIDI
class nsIBidiKeyboard;
#endif
class nsIMIMEHeaderParam;
class nsIObserver;
class nsPresContext;
class nsIChannel;
class nsAutoScriptBlockerSuppressNodeRemoved;
struct nsIntMargin;
class nsPIDOMWindow;
class nsIDocumentLoaderFactory;
class nsIDOMHTMLInputElement;

namespace mozilla {

namespace layers {
  class LayerManager;
} 

namespace dom {
class Element;
} 

} 

extern const char kLoadAsData[];

enum EventNameType {
  EventNameType_None = 0x0000,
  EventNameType_HTML = 0x0001,
  EventNameType_XUL = 0x0002,
  EventNameType_SVGGraphic = 0x0004, 
  EventNameType_SVGSVG = 0x0008, 
  EventNameType_SMIL = 0x0016, 

  EventNameType_HTMLXUL = 0x0003,
  EventNameType_All = 0xFFFF
};





struct ViewportInfo
{
    
    
    double defaultZoom;

    
    double minZoom;

    
    double maxZoom;

    
    
    PRUint32 width;

    
    
    PRUint32 height;

    
    
    
    
    bool autoSize;

    
    bool allowZoom;

    
    
    
    
    
    
    bool autoScale;
};

struct EventNameMapping
{
  nsIAtom* mAtom;
  PRUint32 mId;
  PRInt32  mType;
  PRUint32 mStructType;
};

struct nsShortcutCandidate {
  nsShortcutCandidate(PRUint32 aCharCode, bool aIgnoreShift) :
    mCharCode(aCharCode), mIgnoreShift(aIgnoreShift)
  {
  }
  PRUint32 mCharCode;
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

  



  static nsresult ReparentContentWrappersInScope(JSContext *cx,
                                                 nsIScriptGlobalObject *aOldScope,
                                                 nsIScriptGlobalObject *aNewScope);

  static bool     IsCallerChrome();

  static bool     IsCallerTrustedForRead();

  static bool     IsCallerTrustedForWrite();

  


  static bool     CallerHasUniversalXPConnect();

  static bool     IsImageSrcSetDisabled();

  


  static nsINode* GetCrossDocParentNode(nsINode* aChild);

  












  static bool ContentIsDescendantOf(const nsINode* aPossibleDescendant,
                                      const nsINode* aPossibleAncestor);

  


  static bool ContentIsCrossDocDescendantOf(nsINode* aPossibleDescendant,
                                              nsINode* aPossibleAncestor);

  



  static nsresult GetAncestors(nsINode* aNode,
                               nsTArray<nsINode*>& aArray);

  







  static nsresult GetAncestorsAndOffsets(nsIDOMNode* aNode,
                                         PRInt32 aOffset,
                                         nsTArray<nsIContent*>* aAncestorNodes,
                                         nsTArray<PRInt32>* aAncestorOffsets);

  





  static nsresult GetCommonAncestor(nsIDOMNode *aNode,
                                    nsIDOMNode *aOther,
                                    nsIDOMNode** aCommonAncestor);

  



  static nsINode* GetCommonAncestor(nsINode* aNode1,
                                    nsINode* aNode2);

  



  static bool PositionIsBefore(nsINode* aNode1,
                                 nsINode* aNode2)
  {
    return (aNode2->CompareDocPosition(aNode1) &
      (nsIDOMNode::DOCUMENT_POSITION_PRECEDING |
       nsIDOMNode::DOCUMENT_POSITION_DISCONNECTED)) ==
      nsIDOMNode::DOCUMENT_POSITION_PRECEDING;
  }

  








  static PRInt32 ComparePoints(nsINode* aParent1, PRInt32 aOffset1,
                               nsINode* aParent2, PRInt32 aOffset2,
                               bool* aDisconnected = nsnull);
  static PRInt32 ComparePoints(nsIDOMNode* aParent1, PRInt32 aOffset1,
                               nsIDOMNode* aParent2, PRInt32 aOffset2,
                               bool* aDisconnected = nsnull);

  




  static Element* MatchElementId(nsIContent *aContent, const nsAString& aId);

  


  static Element* MatchElementId(nsIContent *aContent, const nsIAtom* aId);

  








  static PRUint16 ReverseDocumentPosition(PRUint16 aDocumentPosition);

  static PRUint32 CopyNewlineNormalizedUnicodeTo(const nsAString& aSource,
                                                 PRUint32 aSrcOffset,
                                                 PRUnichar* aDest,
                                                 PRUint32 aLength,
                                                 bool& aLastCharCR);

  static PRUint32 CopyNewlineNormalizedUnicodeTo(nsReadingIterator<PRUnichar>& aSrcStart, const nsReadingIterator<PRUnichar>& aSrcEnd, nsAString& aDest);

  static const nsDependentSubstring TrimCharsInSet(const char* aSet,
                                                   const nsAString& aValue);

  template<bool IsWhitespace(PRUnichar)>
  static const nsDependentSubstring TrimWhitespace(const nsAString& aStr,
                                                   bool aTrimTrailing = true);

  


  static bool IsFirstLetterPunctuation(PRUint32 aChar);
  static bool IsFirstLetterPunctuationAt(const nsTextFragment* aFrag, PRUint32 aOffset);
 
  


  static bool IsAlphanumeric(PRUint32 aChar);
  static bool IsAlphanumericAt(const nsTextFragment* aFrag, PRUint32 aOffset);

  







  static bool IsHTMLWhitespace(PRUnichar aChar);

  


  static bool IsHTMLBlock(nsIAtom* aLocalName);

  


  static bool IsHTMLVoid(nsIAtom* aLocalName);

  







  static bool ParseIntMarginValue(const nsAString& aString, nsIntMargin& aResult);

  






  static PRInt32 ParseLegacyFontSize(const nsAString& aValue);

  static void Shutdown();

  


  static nsresult CheckSameOrigin(nsINode* aTrustedNode,
                                  nsIDOMNode* aUnTrustedNode);
  static nsresult CheckSameOrigin(nsINode* aTrustedNode,
                                  nsINode* unTrustedNode);

  
  static bool CanCallerAccess(nsIDOMNode *aNode);

  
  
  static bool CanCallerAccess(nsPIDOMWindow* aWindow);

  



  static nsPIDOMWindow *GetWindowFromCaller();

  















  





  static nsIDOMDocument *GetDocumentFromCaller();

  






  static nsIDOMDocument *GetDocumentFromContext();

  
  
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

  static imgILoader* GetImgLoader()
  {
    if (!sImgLoaderInitialized)
      InitImgLoader();
    return sImgLoader;
  }

#ifdef MOZ_XTF
  static nsIXTFService* GetXTFService();
#endif

#ifdef IBMBIDI
  static nsIBidiKeyboard* GetBidiKeyboard();
#endif
  
  



  static nsIScriptSecurityManager* GetSecurityManager()
  {
    return sSecurityManager;
  }

  static nsresult GenerateStateKey(nsIContent* aContent,
                                   const nsIDocument* aDocument,
                                   nsIStatefulFrame::SpecialStateID aID,
                                   nsACString& aKey);

  




  static nsresult NewURIWithDocumentCharset(nsIURI** aResult,
                                            const nsAString& aSpec,
                                            nsIDocument* aDocument,
                                            nsIURI* aBaseURI);

  




  static nsresult ConvertStringFromCharset(const nsACString& aCharset,
                                           const nsACString& aInput,
                                           nsAString& aOutput);

  








  static bool CheckForBOM(const unsigned char* aBuffer, PRUint32 aLength,
                            nsACString& aCharset, bool *bigEndian = nsnull);


  







  static bool BelongsInForm(nsIContent *aForm,
                              nsIContent *aContent);

  static nsresult CheckQName(const nsAString& aQualifiedName,
                             bool aNamespaceAware = true,
                             const PRUnichar** aColon = nsnull);

  static nsresult SplitQName(const nsIContent* aNamespaceResolver,
                             const nsAFlatString& aQName,
                             PRInt32 *aNamespace, nsIAtom **aLocalName);

  static nsresult GetNodeInfoFromQName(const nsAString& aNamespaceURI,
                                       const nsAString& aQualifiedName,
                                       nsNodeInfoManager* aNodeInfoManager,
                                       PRUint16 aNodeType,
                                       nsINodeInfo** aNodeInfo);

  static void SplitExpatName(const PRUnichar *aExpatName, nsIAtom **aPrefix,
                             nsIAtom **aTagName, PRInt32 *aNameSpaceID);

  
  
  
  
  static bool IsSitePermAllow(nsIPrincipal* aPrincipal, const char* aType);

  
  
  
  
  static bool IsSitePermDeny(nsIPrincipal* aPrincipal, const char* aType);

  
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

  



  static bool HasNonEmptyAttr(const nsIContent* aContent, PRInt32 aNameSpaceID,
                                nsIAtom* aName);

  






  static nsPresContext* GetContextForContent(const nsIContent* aContent);

  















  static bool CanLoadImage(nsIURI* aURI,
                             nsISupports* aContext,
                             nsIDocument* aLoadingDocument,
                             nsIPrincipal* aLoadingPrincipal,
                             PRInt16* aImageBlockingStatus = nsnull);
  












  static nsresult LoadImage(nsIURI* aURI,
                            nsIDocument* aLoadingDocument,
                            nsIPrincipal* aLoadingPrincipal,
                            nsIURI* aReferrer,
                            imgIDecoderObserver* aObserver,
                            PRInt32 aLoadFlags,
                            imgIRequest** aRequest);

  


  static bool IsImageInCache(nsIURI* aURI);

  






  static already_AddRefed<imgIContainer> GetImageFromContent(nsIImageLoadingContent* aContent, imgIRequest **aRequest = nsnull);

  


  static already_AddRefed<imgIRequest> GetStaticRequest(imgIRequest* aRequest);

  





  static bool ContentIsDraggable(nsIContent* aContent);

  





  static bool IsDraggableImage(nsIContent* aContent);

  





  static bool IsDraggableLink(const nsIContent* aContent);

  



  static nsresult NameChanged(nsINodeInfo *aNodeInfo, nsIAtom *aName,
                              nsINodeInfo** aResult)
  {
    nsNodeInfoManager *niMgr = aNodeInfo->NodeInfoManager();

    *aResult = niMgr->GetNodeInfo(aName, aNodeInfo->GetPrefixAtom(),
                                  aNodeInfo->NamespaceID(),
                                  aNodeInfo->NodeType(),
                                  aNodeInfo->GetExtraName()).get();
    return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  





  static void GetEventArgNames(PRInt32 aNameSpaceID, nsIAtom *aEventName,
                               PRUint32 *aArgCount, const char*** aArgNames);

  













  static bool IsInSameAnonymousTree(const nsINode* aNode, const nsIContent* aContent);

  


  static nsIXPConnect *XPConnect()
  {
    return sXPConnect;
  }

  


















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
    PropertiesFile_COUNT
  };
  static nsresult ReportToConsole(PRUint32 aErrorFlags,
                                  const char *aCategory,
                                  nsIDocument* aDocument,
                                  PropertiesFile aFile,
                                  const char *aMessageName,
                                  const PRUnichar **aParams = nsnull,
                                  PRUint32 aParamsLength = 0,
                                  nsIURI* aURI = nsnull,
                                  const nsAFlatString& aSourceLine
                                    = EmptyString(),
                                  PRUint32 aLineNumber = 0,
                                  PRUint32 aColumnNumber = 0);

  


  static nsresult GetLocalizedString(PropertiesFile aFile,
                                     const char* aKey,
                                     nsXPIDLString& aResult);

  



private:
  static nsresult FormatLocalizedString(PropertiesFile aFile,
                                        const char* aKey,
                                        const PRUnichar** aParams,
                                        PRUint32 aParamsLength,
                                        nsXPIDLString& aResult);
  
public:
  template<PRUint32 N>
  static nsresult FormatLocalizedString(PropertiesFile aFile,
                                        const char* aKey,
                                        const PRUnichar* (&aParams)[N],
                                        nsXPIDLString& aResult)
  {
    return FormatLocalizedString(aFile, aKey, aParams, N, aResult);
  }

  


  static bool IsChromeDoc(nsIDocument *aDocument);

  


  static bool IsChildOfSameType(nsIDocument* aDoc);

  








  static bool GetWrapperSafeScriptFilename(nsIDocument *aDocument,
                                             nsIURI *aURI,
                                             nsACString& aScriptURI);


  




  static bool IsInChromeDocshell(nsIDocument *aDocument);

  


  static nsIContentPolicy *GetContentPolicy();

  












  static bool HasMutationListeners(nsINode* aNode,
                                     PRUint32 aType,
                                     nsINode* aTargetForSubtreeModified);

  










  static bool HasMutationListeners(nsIDocument* aDocument,
                                     PRUint32 aType);
  











  static void MaybeFireNodeRemoved(nsINode* aChild, nsINode* aParent,
                                   nsIDocument* aOwnerDoc);

  












  static nsresult DispatchTrustedEvent(nsIDocument* aDoc,
                                       nsISupports* aTarget,
                                       const nsAString& aEventName,
                                       bool aCanBubble,
                                       bool aCancelable,
                                       bool *aDefaultAction = nsnull);
                                       
  












  static nsresult DispatchUntrustedEvent(nsIDocument* aDoc,
                                         nsISupports* aTarget,
                                         const nsAString& aEventName,
                                         bool aCanBubble,
                                         bool aCancelable,
                                         bool *aDefaultAction = nsnull);

  














  static nsresult DispatchChromeEvent(nsIDocument* aDoc,
                                      nsISupports* aTarget,
                                      const nsAString& aEventName,
                                      bool aCanBubble,
                                      bool aCancelable,
                                      bool *aDefaultAction = nsnull);

  







  static bool IsEventAttributeName(nsIAtom* aName, PRInt32 aType);

  






  static PRUint32 GetEventId(nsIAtom* aName);

  






  static PRUint32 GetEventCategory(const nsAString& aName);

  








  static nsIAtom* GetEventIdAndAtom(const nsAString& aName,
                                    PRUint32 aEventStruct,
                                    PRUint32* aEventID);

  










  static void TraverseListenerManager(nsINode *aNode,
                                      nsCycleCollectionTraversalCallback &cb);

  







  static nsEventListenerManager* GetListenerManager(nsINode* aNode,
                                                    bool aCreateIfNotFound);

  static void UnmarkGrayJSListenersInCCGenerationDocuments(PRUint32 aGeneration);

  




  static void RemoveListenerManager(nsINode *aNode);

  static bool IsInitialized()
  {
    return sInitialized;
  }

  








  static bool IsValidNodeName(nsIAtom *aLocalName, nsIAtom *aPrefix,
                                PRInt32 aNamespaceID);

  













  static nsresult CreateContextualFragment(nsINode* aContextNode,
                                           const nsAString& aFragment,
                                           bool aPreventScriptExecution,
                                           nsIDOMDocumentFragment** aReturn);

  














  static nsresult ParseFragmentHTML(const nsAString& aSourceBuffer,
                                    nsIContent* aTargetNode,
                                    nsIAtom* aContextLocalName,
                                    PRInt32 aContextNamespace,
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
                                     PRUint32 aFlags,
                                     PRUint32 aWrapCol);

  

















  static nsresult CreateDocument(const nsAString& aNamespaceURI, 
                                 const nsAString& aQualifiedName, 
                                 nsIDOMDocumentType* aDoctype,
                                 nsIURI* aDocumentURI,
                                 nsIURI* aBaseURI,
                                 nsIPrincipal* aPrincipal,
                                 nsIScriptGlobalObject* aScriptObject,
                                 DocumentFlavor aFlavor,
                                 nsIDOMDocument** aResult);

  













  static nsresult SetNodeTextContent(nsIContent* aContent,
                                     const nsAString& aValue,
                                     bool aTryReuse);

  













  static void GetNodeTextContent(nsINode* aNode, bool aDeep,
                                 nsAString& aResult)
  {
    aResult.Truncate();
    AppendNodeTextContent(aNode, aDeep, aResult);
  }

  


  static void AppendNodeTextContent(nsINode* aNode, bool aDeep,
                                    nsAString& aResult);

  





  static bool HasNonEmptyTextContent(nsINode* aNode);

  


  static void DestroyMatchString(void* aData)
  {
    if (aData) {
      nsString* matchString = static_cast<nsString*>(aData);
      delete matchString;
    }
  }

  


  static void DestroyAnonymousContent(nsCOMPtr<nsIContent>* aContent);

  






  static nsresult HoldJSObjects(void* aScriptObjectHolder,
                                nsScriptObjectTracer* aTracer);

  





  static nsresult DropJSObjects(void* aScriptObjectHolder);

#ifdef DEBUG
  static void CheckCCWrapperTraversal(nsISupports* aScriptObjectHolder,
                                      nsWrapperCache* aCache);
#endif

  static void PreserveWrapper(nsISupports* aScriptObjectHolder,
                              nsWrapperCache* aCache)
  {
    if (!aCache->PreservingWrapper()) {
      nsXPCOMCycleCollectionParticipant* participant;
      CallQueryInterface(aScriptObjectHolder, &participant);
      HoldJSObjects(aScriptObjectHolder, participant);
      aCache->SetPreservingWrapper(true);
#ifdef DEBUG
      
      CheckCCWrapperTraversal(aScriptObjectHolder, aCache);
#endif
    }
  }
  static void ReleaseWrapper(nsISupports* aScriptObjectHolder,
                             nsWrapperCache* aCache);
  static void TraceWrapper(nsWrapperCache* aCache, TraceCallback aCallback,
                           void *aClosure);

  




  static void NotifyInstalledMenuKeyboardListener(bool aInstalling);

  






















  static nsresult CheckSecurityBeforeLoad(nsIURI* aURIToLoad,
                                          nsIPrincipal* aLoadingPrincipal,
                                          PRUint32 aCheckLoadFlags,
                                          bool aAllowData,
                                          PRUint32 aContentPolicyType,
                                          nsISupports* aContext,
                                          const nsACString& aMimeGuess = EmptyCString(),
                                          nsISupports* aExtra = nsnull);

  


  static bool IsSystemPrincipal(nsIPrincipal* aPrincipal);

  













  static bool CombineResourcePrincipals(nsCOMPtr<nsIPrincipal>* aResourcePrincipal,
                                        nsIPrincipal* aExtraPrincipal);

  
















  static void TriggerLink(nsIContent *aContent, nsPresContext *aPresContext,
                          nsIURI *aLinkURI, const nsString& aTargetSpec,
                          bool aClick, bool aIsUserTriggered,
                          bool aIsTrusted);

  


  static nsIWidget* GetTopLevelWidget(nsIWidget* aWidget);

  


  static const nsDependentString GetLocalizedEllipsis();

  




  static nsEvent* GetNativeEvent(nsIDOMEvent* aDOMEvent);
  static bool DOMEventToNativeKeyEvent(nsIDOMKeyEvent* aKeyEvent,
                                         nsNativeKeyEvent* aNativeEvent,
                                         bool aGetCharCode);

  






  static void GetAccelKeyCandidates(nsIDOMKeyEvent* aDOMKeyEvent,
                                    nsTArray<nsShortcutCandidate>& aCandidates);

  






  static void GetAccessKeyCandidates(nsKeyEvent* aNativeKeyEvent,
                                     nsTArray<PRUint32>& aCandidates);

  



  static void HidePopupsInDocument(nsIDocument* aDocument);

  


  static already_AddRefed<nsIDragSession> GetDragSession();

  


  static nsresult SetDataTransferInEvent(nsDragEvent* aDragEvent);

  
  
  static PRUint32 FilterDropEffect(PRUint32 aAction, PRUint32 aEffectAllowed);

  



  static bool CheckForSubFrameDrop(nsIDragSession* aDragSession,
                                   nsDragEvent* aDropEvent);

  


  static bool URIIsLocalFile(nsIURI *aURI);

  



  static nsresult SplitURIAtHash(nsIURI *aURI,
                                 nsACString &aBeforeHash,
                                 nsACString &aAfterHash);

  







  static void GetOfflineAppManifest(nsIDocument *aDocument, nsIURI **aURI);

  


  static bool OfflineAppAllowed(nsIURI *aURI);

  


  static bool OfflineAppAllowed(nsIPrincipal *aPrincipal);

  




  static void AddScriptBlocker();

  






  static void RemoveScriptBlocker();

  











  static bool AddScriptRunner(nsIRunnable* aRunnable);

  





  static bool IsSafeToRunScript() {
    return sScriptBlockerCount == 0;
  }

  









  static ViewportInfo GetViewportInfo(nsIDocument* aDocument);

  
  
  static void EnterMicroTask() { ++sMicroTaskLevel; }
  static void LeaveMicroTask();

  static bool IsInMicroTask() { return sMicroTaskLevel != 0; }
  static PRUint32 MicroTaskLevel() { return sMicroTaskLevel; }
  static void SetMicroTaskLevel(PRUint32 aLevel) { sMicroTaskLevel = aLevel; }

  





  static nsresult ProcessViewportInfo(nsIDocument *aDocument,
                                      const nsAString &viewportInfo);

  static nsIScriptContext* GetContextForEventHandlers(nsINode* aNode,
                                                      nsresult* aRv);

  static JSContext *GetCurrentJSContext();

  



  static bool EqualsIgnoreASCIICase(const nsAString& aStr1,
                                    const nsAString& aStr2);

  






  static bool EqualsLiteralIgnoreASCIICase(const nsAString& aStr1,
                                           const char* aStr2,
                                           const PRUint32 len);
#ifdef NS_DISABLE_LITERAL_TEMPLATE
  static inline bool
  EqualsLiteralIgnoreASCIICase(const nsAString& aStr1,
                               const char* aStr2)
  {
    PRUint32 len = strlen(aStr2);
    return EqualsLiteralIgnoreASCIICase(aStr1, aStr2, len);
  }
#else
  template<int N>
  static inline bool
  EqualsLiteralIgnoreASCIICase(const nsAString& aStr1,
                               const char (&aStr2)[N])
  {
    return EqualsLiteralIgnoreASCIICase(aStr1, aStr2, N-1);
  }
  template<int N>
  static inline bool
  EqualsLiteralIgnoreASCIICase(const nsAString& aStr1,
                               char (&aStr2)[N])
  {
    const char* s = aStr2;
    return EqualsLiteralIgnoreASCIICase(aStr1, s, N-1);
  }
#endif

  




  static nsresult ASCIIToLower(nsAString& aStr);
  static nsresult ASCIIToLower(const nsAString& aSource, nsAString& aDest);

  




  static nsresult ASCIIToUpper(nsAString& aStr);
  static nsresult ASCIIToUpper(const nsAString& aSource, nsAString& aDest);

  
  static nsresult CheckSameOrigin(nsIChannel *aOldChannel, nsIChannel *aNewChannel);
  static nsIInterfaceRequestor* GetSameOriginChecker();

  static nsIThreadJSContextStack* ThreadJSContextStack()
  {
    return sThreadJSContextStack;
  }

  
  static void TraceSafeJSContext(JSTracer* aTrc);


  












  static nsresult GetASCIIOrigin(nsIPrincipal* aPrincipal,
                                 nsCString& aOrigin);
  static nsresult GetASCIIOrigin(nsIURI* aURI, nsCString& aOrigin);
  static nsresult GetUTFOrigin(nsIPrincipal* aPrincipal,
                               nsString& aOrigin);
  static nsresult GetUTFOrigin(nsIURI* aURI, nsString& aOrigin);

  





  static nsresult DispatchXULCommand(nsIContent* aTarget,
                                     bool aTrusted,
                                     nsIDOMEvent* aSourceEvent = nsnull,
                                     nsIPresShell* aShell = nsnull,
                                     bool aCtrl = false,
                                     bool aAlt = false,
                                     bool aShift = false,
                                     bool aMeta = false);

  






  static already_AddRefed<nsIDocument>
  GetDocumentFromScriptContext(nsIScriptContext *aScriptContext);

  static bool CheckMayLoad(nsIPrincipal* aPrincipal, nsIChannel* aChannel);

  




  static bool CanAccessNativeAnon();

  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, const nsIID* aIID, jsval *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nsnull,
                             bool aAllowWrapping = false)
  {
    return WrapNative(cx, scope, native, nsnull, aIID, vp, aHolder,
                      aAllowWrapping);
  }

  
  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, jsval *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nsnull,
                             bool aAllowWrapping = false)
  {
    return WrapNative(cx, scope, native, nsnull, nsnull, vp, aHolder,
                      aAllowWrapping);
  }
  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, nsWrapperCache *cache,
                             jsval *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nsnull,
                             bool aAllowWrapping = false)
  {
    return WrapNative(cx, scope, native, cache, nsnull, vp, aHolder,
                      aAllowWrapping);
  }

  


  static nsresult CreateArrayBuffer(JSContext *aCx, const nsACString& aData,
                                    JSObject** aResult);

  static void StripNullChars(const nsAString& aInStr, nsAString& aOutStr);

  



  static void RemoveNewlines(nsString &aString);

  



  static void PlatformToDOMLineBreaks(nsString &aString);

  static bool IsHandlingKeyBoardEvent()
  {
    return sIsHandlingKeyBoardEvent;
  }

  static void SetIsHandlingKeyBoardEvent(bool aHandling)
  {
    sIsHandlingKeyBoardEvent = aHandling;
  }

  



  static nsresult GetElementsByClassName(nsINode* aRootNode,
                                         const nsAString& aClasses,
                                         nsIDOMNodeList** aReturn);

  




  static nsIWidget *WidgetForDocument(nsIDocument *aDoc);

  









  static already_AddRefed<mozilla::layers::LayerManager>
  LayerManagerForDocument(nsIDocument *aDoc, bool *aAllowRetaining = nsnull);

  













  static already_AddRefed<mozilla::layers::LayerManager>
  PersistentLayerManagerForDocument(nsIDocument *aDoc, bool *aAllowRetaining = nsnull);

  





  static bool IsFocusedContent(const nsIContent *aContent);

  


  static bool IsFullScreenApiEnabled();

  





  static bool IsRequestFullScreenAllowed();

  





  static bool HasPluginWithUncontrolledEventDispatch(nsIDocument* aDoc);

  





  static bool HasPluginWithUncontrolledEventDispatch(nsIContent* aContent);

  



  static nsIDocument* GetRootDocument(nsIDocument* aDoc);

  




  static TimeDuration HandlingUserInputTimeout();

  static void GetShiftText(nsAString& text);
  static void GetControlText(nsAString& text);
  static void GetMetaText(nsAString& text);
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
                            ContentViewerType* aLoaderType = nsnull);

  















  static bool IsPatternMatching(nsAString& aValue, nsAString& aPattern,
                                  nsIDocument* aDocument);

  



  static void InitializeTouchEventTable();

  



  static nsresult URIInheritsSecurityContext(nsIURI *aURI, bool *aResult);

  








  static bool SetUpChannelOwner(nsIPrincipal* aLoadingPrincipal,
                                nsIChannel* aChannel,
                                nsIURI* aURI,
                                bool aSetUpForAboutBlank);

  static nsresult Btoa(const nsAString& aBinaryData,
                       nsAString& aAsciiBase64String);

  static nsresult Atob(const nsAString& aAsciiString,
                       nsAString& aBinaryData);

  


 
  static nsresult JSArrayToAtomArray(JSContext* aCx, const JS::Value& aJSArray,
                                     nsCOMArray<nsIAtom>& aRetVal);

  








  static bool IsAutocompleteEnabled(nsIDOMHTMLInputElement* aInput);

  









  static bool URIIsChromeOrInPref(nsIURI *aURI, const char *aPref);

  













  static bool GetPseudoAttributeValue(const nsString& aSource, nsIAtom *aName,
                                      nsAString& aValue);

  



  static bool IsJavaScriptLanguage(const nsString& aName, PRUint32 *aVerFlags);

  static void SplitMimeType(const nsAString& aValue, nsString& aType,
                            nsString& aParams);

private:
  static bool InitializeEventTable();

  static nsresult EnsureStringBundle(PropertiesFile aFile);

  static bool CanCallerAccess(nsIPrincipal* aSubjectPrincipal,
                                nsIPrincipal* aPrincipal);

  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, nsWrapperCache *cache,
                             const nsIID* aIID, jsval *vp,
                             nsIXPConnectJSObjectHolder** aHolder,
                             bool aAllowWrapping);
                            
  static nsresult DispatchEvent(nsIDocument* aDoc,
                                nsISupports* aTarget,
                                const nsAString& aEventName,
                                bool aCanBubble,
                                bool aCancelable,
                                bool aTrusted,
                                bool *aDefaultAction = nsnull);

  static void InitializeModifierStrings();

  static void DropFragmentParsers();

  static nsIDOMScriptObjectFactory *sDOMScriptObjectFactory;

  static nsIXPConnect *sXPConnect;

  static nsIScriptSecurityManager *sSecurityManager;

  static nsIThreadJSContextStack *sThreadJSContextStack;

  static nsIParserService *sParserService;

  static nsINameSpaceManager *sNameSpaceManager;

  static nsIIOService *sIOService;

#ifdef MOZ_XTF
  static nsIXTFService *sXTFService;
#endif

  static bool sImgLoaderInitialized;
  static void InitImgLoader();

  
  static imgILoader* sImgLoader;
  static imgICache* sImgCache;

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

  static PRUint32 sJSGCThingRootCount;

#ifdef IBMBIDI
  static nsIBidiKeyboard* sBidiKeyboard;
#endif

  static bool sInitialized;
  static PRUint32 sScriptBlockerCount;
#ifdef DEBUG
  static PRUint32 sDOMNodeRemovedSuppressCount;
#endif
  static PRUint32 sMicroTaskLevel;
  
  static nsTArray< nsCOMPtr<nsIRunnable> >* sBlockedScriptRunners;
  static PRUint32 sRunnersCountAtFirstBlocker;
  static PRUint32 sScriptBlockerCountWhereRunnersPrevented;

  static nsIInterfaceRequestor* sSameOriginChecker;

  static bool sIsHandlingKeyBoardEvent;
  static bool sAllowXULXBL_for_file;
  static bool sIsFullScreenApiEnabled;
  static bool sTrustedFullScreenOnly;
  static PRUint32 sHandlingInputTimeout;

  static nsHtml5StringParser* sHTMLFragmentParser;
  static nsIParser* sXMLFragmentParser;
  static nsIFragmentContentSink* sXMLFragmentSink;

  


  static bool sFragmentParsingActive;

  static nsString* sShiftText;
  static nsString* sControlText;
  static nsString* sMetaText;
  static nsString* sAltText;
  static nsString* sModifierSeparator;
};

typedef nsCharSeparatedTokenizerTemplate<nsContentUtils::IsHTMLWhitespace>
                                                    HTMLSplitOnSpacesTokenizer;

#define NS_HOLD_JS_OBJECTS(obj, clazz)                                         \
  nsContentUtils::HoldJSObjects(NS_CYCLE_COLLECTION_UPCAST(obj, clazz),        \
                                &NS_CYCLE_COLLECTION_NAME(clazz))

#define NS_DROP_JS_OBJECTS(obj, clazz)                                         \
  nsContentUtils::DropJSObjects(NS_CYCLE_COLLECTION_UPCAST(obj, clazz))


class NS_STACK_CLASS nsCxPusher
{
public:
  nsCxPusher();
  ~nsCxPusher(); 

  
  bool Push(nsIDOMEventTarget *aCurrentTarget);
  
  
  bool RePush(nsIDOMEventTarget *aCurrentTarget);
  
  
  bool Push(JSContext *cx, bool aRequiresScriptContext = true);
  
  bool PushNull();

  
  void Pop();

  nsIScriptContext* GetCurrentScriptContext() { return mScx; }
private:
  
  bool DoPush(JSContext* cx);

  nsCOMPtr<nsIScriptContext> mScx;
  bool mScriptIsRunning;
  bool mPushedSomething;
#ifdef DEBUG
  JSContext* mPushedContext;
#endif
};

class NS_STACK_CLASS nsAutoScriptBlocker {
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

class NS_STACK_CLASS nsAutoScriptBlockerSuppressNodeRemoved :
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

class NS_STACK_CLASS nsAutoMicroTask
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
      *aInstancePtr = nsnull;                                                 \
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

#define NS_ENSURE_FINITE3(f1, f2, f3, rv)                                     \
  if (!NS_finite((f1)+(f2)+(f3))) {                                           \
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
    (ptr_)->member_ = nsnull;                                                 \
    while (cur) {                                                             \
      type_ *next = cur->member_;                                             \
      cur->member_ = nsnull;                                                  \
      delete cur;                                                             \
      cur = next;                                                             \
    }                                                                         \
  }

class nsContentTypeParser {
public:
  nsContentTypeParser(const nsAString& aString);
  ~nsContentTypeParser();

  nsresult GetParameter(const char* aParameterName, nsAString& aResult);
  nsresult GetType(nsAString& aResult)
  {
    return GetParameter(nsnull, aResult);
  }

private:
  NS_ConvertUTF16toUTF8 mString;
  nsIMIMEHeaderParam*   mService;
};

class nsDocElementCreatedNotificationRunner : public nsRunnable
{
public:
    nsDocElementCreatedNotificationRunner(nsIDocument* aDoc)
        : mDoc(aDoc)
    {
    }

    NS_IMETHOD Run()
    {
        nsContentSink::NotifyDocElementCreated(mDoc);
        return NS_OK;
    }

    nsCOMPtr<nsIDocument> mDoc;
};

#endif
