








































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
#include "nsIDOM3Node.h"
#include "nsDataHashtable.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMEvent.h"
#include "nsTArray.h"
#include "nsTextFragment.h"
#include "nsReadableUtils.h"
#include "mozilla/AutoRestore.h"
#include "nsINode.h"
#include "nsHashtable.h"

struct nsNativeKeyEvent; 

class nsIDOMScriptObjectFactory;
class nsIXPConnect;
class nsIContent;
class nsIDOMNode;
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
class nsIEventListenerManager;
class nsIScriptContext;
class nsIRunnable;
class nsIInterfaceRequestor;
template<class E> class nsCOMArray;
template<class K, class V> class nsRefPtrHashtable;
struct JSRuntime;
class nsIUGenCategory;
class nsIWidget;
class nsIDragSession;
class nsPIDOMWindow;
class nsPIDOMEventTarget;
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

struct EventNameMapping
{
  nsIAtom* mAtom;
  PRUint32 mId;
  PRInt32  mType;
  PRUint32 mStructType;
};

struct nsShortcutCandidate {
  nsShortcutCandidate(PRUint32 aCharCode, PRBool aIgnoreShift) :
    mCharCode(aCharCode), mIgnoreShift(aIgnoreShift)
  {
  }
  PRUint32 mCharCode;
  PRBool   mIgnoreShift;
};

class nsContentUtils
{
  friend class nsAutoScriptBlockerSuppressNodeRemoved;
  typedef mozilla::dom::Element Element;

public:
  static nsresult Init();

  


  static JSContext* GetContextFromDocument(nsIDocument *aDocument);

  









  static nsresult GetContextAndScope(nsIDocument *aOldDocument,
                                     nsIDocument *aNewDocument,
                                     JSContext **aCx, JSObject **aNewScope);

  



  static nsresult ReparentContentWrappersInScope(JSContext *cx,
                                                 nsIScriptGlobalObject *aOldScope,
                                                 nsIScriptGlobalObject *aNewScope);

  static PRBool   IsCallerChrome();

  static PRBool   IsCallerTrustedForRead();

  static PRBool   IsCallerTrustedForWrite();

  



  static PRBool   IsCallerTrustedForCapability(const char* aCapability);

  


  static nsINode* GetCrossDocParentNode(nsINode* aChild);

  












  static PRBool ContentIsDescendantOf(const nsINode* aPossibleDescendant,
                                      const nsINode* aPossibleAncestor);

  


  static PRBool ContentIsCrossDocDescendantOf(nsINode* aPossibleDescendant,
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

  



  static PRBool PositionIsBefore(nsINode* aNode1,
                                 nsINode* aNode2)
  {
    return (aNode2->CompareDocumentPosition(aNode1) &
      (nsIDOM3Node::DOCUMENT_POSITION_PRECEDING |
       nsIDOM3Node::DOCUMENT_POSITION_DISCONNECTED)) ==
      nsIDOM3Node::DOCUMENT_POSITION_PRECEDING;
  }

  








  static PRInt32 ComparePoints(nsINode* aParent1, PRInt32 aOffset1,
                               nsINode* aParent2, PRInt32 aOffset2,
                               PRBool* aDisconnected = nsnull);

  




  static Element* MatchElementId(nsIContent *aContent, const nsAString& aId);

  


  static Element* MatchElementId(nsIContent *aContent, const nsIAtom* aId);

  









  static PRUint16 ReverseDocumentPosition(PRUint16 aDocumentPosition);

  static PRUint32 CopyNewlineNormalizedUnicodeTo(const nsAString& aSource,
                                                 PRUint32 aSrcOffset,
                                                 PRUnichar* aDest,
                                                 PRUint32 aLength,
                                                 PRBool& aLastCharCR);

  static PRUint32 CopyNewlineNormalizedUnicodeTo(nsReadingIterator<PRUnichar>& aSrcStart, const nsReadingIterator<PRUnichar>& aSrcEnd, nsAString& aDest);

  static nsISupports *
  GetClassInfoInstance(nsDOMClassInfoID aID);

  static const nsDependentSubstring TrimCharsInSet(const char* aSet,
                                                   const nsAString& aValue);

  template<PRBool IsWhitespace(PRUnichar)>
  static const nsDependentSubstring TrimWhitespace(const nsAString& aStr,
                                                   PRBool aTrimTrailing = PR_TRUE);

  


  static PRBool IsPunctuationMark(PRUint32 aChar);
  static PRBool IsPunctuationMarkAt(const nsTextFragment* aFrag, PRUint32 aOffset);
 
  


  static PRBool IsAlphanumeric(PRUint32 aChar);
  static PRBool IsAlphanumericAt(const nsTextFragment* aFrag, PRUint32 aOffset);

  







  static PRBool IsHTMLWhitespace(PRUnichar aChar);

  







  static PRBool ParseIntMarginValue(const nsAString& aString, nsIntMargin& aResult);

  static void Shutdown();

  


  static nsresult CheckSameOrigin(nsINode* aTrustedNode,
                                  nsIDOMNode* aUnTrustedNode);

  
  static PRBool CanCallerAccess(nsIDOMNode *aNode);

  
  
  static PRBool CanCallerAccess(nsPIDOMWindow* aWindow);

  



  static nsPIDOMWindow *GetWindowFromCaller();

  















  





  static nsIDOMDocument *GetDocumentFromCaller();

  






  static nsIDOMDocument *GetDocumentFromContext();

  
  
  static PRBool InProlog(nsINode *aNode);

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

  








  static PRBool CheckForBOM(const unsigned char* aBuffer, PRUint32 aLength,
                            nsACString& aCharset, PRBool *bigEndian = nsnull);


  







  static PRBool BelongsInForm(nsIContent *aForm,
                              nsIContent *aContent);

  static nsresult CheckQName(const nsAString& aQualifiedName,
                             PRBool aNamespaceAware = PR_TRUE);

  static nsresult SplitQName(const nsIContent* aNamespaceResolver,
                             const nsAFlatString& aQName,
                             PRInt32 *aNamespace, nsIAtom **aLocalName);

  static nsresult GetNodeInfoFromQName(const nsAString& aNamespaceURI,
                                       const nsAString& aQualifiedName,
                                       nsNodeInfoManager* aNodeInfoManager,
                                       nsINodeInfo** aNodeInfo);

  static void SplitExpatName(const PRUnichar *aExpatName, nsIAtom **aPrefix,
                             nsIAtom **aTagName, PRInt32 *aNameSpaceID);

  
  
  
  static PRBool IsSitePermAllow(nsIURI* aURI, const char* aType);

  static nsILineBreaker* LineBreaker()
  {
    return sLineBreaker;
  }

  static nsIWordBreaker* WordBreaker()
  {
    return sWordBreaker;
  }

  static nsIUGenCategory* GetGenCat()
  {
    return sGenCat;
  }

  



  static void RegisterShutdownObserver(nsIObserver* aObserver);
  static void UnregisterShutdownObserver(nsIObserver* aObserver);

  



  static PRBool HasNonEmptyAttr(const nsIContent* aContent, PRInt32 aNameSpaceID,
                                nsIAtom* aName);

  






  static nsPresContext* GetContextForContent(const nsIContent* aContent);

  















  static PRBool CanLoadImage(nsIURI* aURI,
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

  


  static PRBool IsImageInCache(nsIURI* aURI);

  






  static already_AddRefed<imgIContainer> GetImageFromContent(nsIImageLoadingContent* aContent, imgIRequest **aRequest = nsnull);

  


  static already_AddRefed<imgIRequest> GetStaticRequest(imgIRequest* aRequest);

  





  static PRBool ContentIsDraggable(nsIContent* aContent);

  





  static PRBool IsDraggableImage(nsIContent* aContent);

  





  static PRBool IsDraggableLink(const nsIContent* aContent);

  



  static nsresult NameChanged(nsINodeInfo *aNodeInfo, nsIAtom *aName,
                              nsINodeInfo** aResult)
  {
    nsNodeInfoManager *niMgr = aNodeInfo->NodeInfoManager();

    *aResult = niMgr->GetNodeInfo(aName, aNodeInfo->GetPrefixAtom(),
                                  aNodeInfo->NamespaceID()).get();
    return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  





  static void GetEventArgNames(PRInt32 aNameSpaceID, nsIAtom *aEventName,
                               PRUint32 *aArgCount, const char*** aArgNames);

  













  static PRBool IsInSameAnonymousTree(const nsINode* aNode, const nsIContent* aContent);

  


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
    eSVG_PROPERTIES,
    eBRAND_PROPERTIES,
    eCOMMON_DIALOG_PROPERTIES,
    PropertiesFile_COUNT
  };
  static nsresult ReportToConsole(PropertiesFile aFile,
                                  const char *aMessageName,
                                  const PRUnichar **aParams,
                                  PRUint32 aParamsLength,
                                  nsIURI* aURI,
                                  const nsAFlatString& aSourceLine,
                                  PRUint32 aLineNumber,
                                  PRUint32 aColumnNumber,
                                  PRUint32 aErrorFlags,
                                  const char *aCategory,
                                  PRUint64 aWindowId = 0);

  















  static nsresult ReportToConsole(PropertiesFile aFile,
                                  const char *aMessageName,
                                  const PRUnichar **aParams,
                                  PRUint32 aParamsLength,
                                  nsIURI* aURI,
                                  const nsAFlatString& aSourceLine,
                                  PRUint32 aLineNumber,
                                  PRUint32 aColumnNumber,
                                  PRUint32 aErrorFlags,
                                  const char *aCategory,
                                  nsIDocument* aDocument);

  


  static nsresult GetLocalizedString(PropertiesFile aFile,
                                     const char* aKey,
                                     nsXPIDLString& aResult);

  



  static nsresult FormatLocalizedString(PropertiesFile aFile,
                                        const char* aKey,
                                        const PRUnichar **aParams,
                                        PRUint32 aParamsLength,
                                        nsXPIDLString& aResult);

  


  static PRBool IsChromeDoc(nsIDocument *aDocument);

  


  static PRBool IsChildOfSameType(nsIDocument* aDoc);

  








  static PRBool GetWrapperSafeScriptFilename(nsIDocument *aDocument,
                                             nsIURI *aURI,
                                             nsACString& aScriptURI);


  




  static PRBool IsInChromeDocshell(nsIDocument *aDocument);

  


  static nsIContentPolicy *GetContentPolicy();

  












  static PRBool HasMutationListeners(nsINode* aNode,
                                     PRUint32 aType,
                                     nsINode* aTargetForSubtreeModified);

  










  static PRBool HasMutationListeners(nsIDocument* aDocument,
                                     PRUint32 aType);
  











  static void MaybeFireNodeRemoved(nsINode* aChild, nsINode* aParent,
                                   nsIDocument* aOwnerDoc);

  












  static nsresult DispatchTrustedEvent(nsIDocument* aDoc,
                                       nsISupports* aTarget,
                                       const nsAString& aEventName,
                                       PRBool aCanBubble,
                                       PRBool aCancelable,
                                       PRBool *aDefaultAction = nsnull);

  














  static nsresult DispatchChromeEvent(nsIDocument* aDoc,
                                      nsISupports* aTarget,
                                      const nsAString& aEventName,
                                      PRBool aCanBubble,
                                      PRBool aCancelable,
                                      PRBool *aDefaultAction = nsnull);

  







  static PRBool IsEventAttributeName(nsIAtom* aName, PRInt32 aType);

  






  static PRUint32 GetEventId(nsIAtom* aName);

  






  static PRUint32 GetEventCategory(const nsAString& aName);

  








  static nsIAtom* GetEventIdAndAtom(const nsAString& aName,
                                    PRUint32 aEventStruct,
                                    PRUint32* aEventID);

  










  static void TraverseListenerManager(nsINode *aNode,
                                      nsCycleCollectionTraversalCallback &cb);

  







  static nsIEventListenerManager* GetListenerManager(nsINode* aNode,
                                                     PRBool aCreateIfNotFound);

  




  static void RemoveListenerManager(nsINode *aNode);

  static PRBool IsInitialized()
  {
    return sInitialized;
  }

  








  static PRBool IsValidNodeName(nsIAtom *aLocalName, nsIAtom *aPrefix,
                                PRInt32 aNamespaceID);

  














  static nsresult CreateContextualFragment(nsINode* aContextNode,
                                           const nsAString& aFragment,
                                           PRBool aWillOwnFragment,
                                           nsIDOMDocumentFragment** aReturn);

  
















  static nsresult CreateDocument(const nsAString& aNamespaceURI, 
                                 const nsAString& aQualifiedName, 
                                 nsIDOMDocumentType* aDoctype,
                                 nsIURI* aDocumentURI,
                                 nsIURI* aBaseURI,
                                 nsIPrincipal* aPrincipal,
                                 nsIScriptGlobalObject* aScriptObject,
                                 nsIDOMDocument** aResult);

  













  static nsresult SetNodeTextContent(nsIContent* aContent,
                                     const nsAString& aValue,
                                     PRBool aTryReuse);

  













  static void GetNodeTextContent(nsINode* aNode, PRBool aDeep,
                                 nsAString& aResult)
  {
    aResult.Truncate();
    AppendNodeTextContent(aNode, aDeep, aResult);
  }

  


  static void AppendNodeTextContent(nsINode* aNode, PRBool aDeep,
                                    nsAString& aResult);

  





  static PRBool HasNonEmptyTextContent(nsINode* aNode);

  


  static void DestroyMatchString(void* aData)
  {
    if (aData) {
      nsString* matchString = static_cast<nsString*>(aData);
      delete matchString;
    }
  }

  static void DropScriptObject(PRUint32 aLangID, void *aObject,
                               const char *name, void *aClosure)
  {
    DropScriptObject(aLangID, aObject, aClosure);
  }

  


  static void DestroyAnonymousContent(nsCOMPtr<nsIContent>* aContent);

  














  static nsresult HoldScriptObject(PRUint32 aLangID, void* aScriptObjectHolder,
                                   nsScriptObjectTracer* aTracer,
                                   void* aNewObject, PRBool aWasHoldingObjects)
  {
    if (aLangID == nsIProgrammingLanguage::JAVASCRIPT) {
      return aWasHoldingObjects ? NS_OK :
                                  HoldJSObjects(aScriptObjectHolder, aTracer);
    }

    return HoldScriptObject(aLangID, aNewObject);
  }

  










  static nsresult DropScriptObjects(PRUint32 aLangID, void* aScriptObjectHolder,
                                    nsScriptObjectTracer* aTracer)
  {
    if (aLangID == nsIProgrammingLanguage::JAVASCRIPT) {
      return DropJSObjects(aScriptObjectHolder);
    }

    aTracer->Trace(aScriptObjectHolder, DropScriptObject, nsnull);

    return NS_OK;
  }

  






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
      aCache->SetPreservingWrapper(PR_TRUE);
#ifdef DEBUG
      
      CheckCCWrapperTraversal(aScriptObjectHolder, aCache);
#endif
    }
  }
  static void ReleaseWrapper(nsISupports* aScriptObjectHolder,
                             nsWrapperCache* aCache)
  {
    if (aCache->PreservingWrapper()) {
      DropJSObjects(aScriptObjectHolder);
      aCache->SetPreservingWrapper(PR_FALSE);
    }
  }
  static void TraceWrapper(nsWrapperCache* aCache, TraceCallback aCallback,
                           void *aClosure)
  {
    if (aCache->PreservingWrapper()) {
      aCallback(nsIProgrammingLanguage::JAVASCRIPT,
                aCache->GetWrapperPreserveColor(),
                "Preserved wrapper", aClosure);
    }
  }

  


  static PRUint32 GetWidgetStatusFromIMEStatus(PRUint32 aState);

  




  static void NotifyInstalledMenuKeyboardListener(PRBool aInstalling);

  






















  static nsresult CheckSecurityBeforeLoad(nsIURI* aURIToLoad,
                                          nsIPrincipal* aLoadingPrincipal,
                                          PRUint32 aCheckLoadFlags,
                                          PRBool aAllowData,
                                          PRUint32 aContentPolicyType,
                                          nsISupports* aContext,
                                          const nsACString& aMimeGuess = EmptyCString(),
                                          nsISupports* aExtra = nsnull);

  


  static PRBool IsSystemPrincipal(nsIPrincipal* aPrincipal);

  














  static void TriggerLink(nsIContent *aContent, nsPresContext *aPresContext,
                          nsIURI *aLinkURI, const nsString& aTargetSpec,
                          PRBool aClick, PRBool aIsUserTriggered);

  


  static nsIWidget* GetTopLevelWidget(nsIWidget* aWidget);

  


  static const nsDependentString GetLocalizedEllipsis();

  




  static nsEvent* GetNativeEvent(nsIDOMEvent* aDOMEvent);
  static PRBool DOMEventToNativeKeyEvent(nsIDOMKeyEvent* aKeyEvent,
                                         nsNativeKeyEvent* aNativeEvent,
                                         PRBool aGetCharCode);

  






  static void GetAccelKeyCandidates(nsIDOMKeyEvent* aDOMKeyEvent,
                                    nsTArray<nsShortcutCandidate>& aCandidates);

  






  static void GetAccessKeyCandidates(nsKeyEvent* aNativeKeyEvent,
                                     nsTArray<PRUint32>& aCandidates);

  



  static void HidePopupsInDocument(nsIDocument* aDocument);

  


  static already_AddRefed<nsIDragSession> GetDragSession();

  


  static nsresult SetDataTransferInEvent(nsDragEvent* aDragEvent);

  
  
  static PRUint32 FilterDropEffect(PRUint32 aAction, PRUint32 aEffectAllowed);

  


  static PRBool URIIsLocalFile(nsIURI *aURI);

  



  static nsresult SplitURIAtHash(nsIURI *aURI,
                                 nsACString &aBeforeHash,
                                 nsACString &aAfterHash);

  







  static void GetOfflineAppManifest(nsIDocument *aDocument, nsIURI **aURI);

  


  static PRBool OfflineAppAllowed(nsIURI *aURI);

  


  static PRBool OfflineAppAllowed(nsIPrincipal *aPrincipal);

  




  static void AddScriptBlocker();

  




  static void AddScriptBlockerAndPreventAddingRunners();

  






  static void RemoveScriptBlocker();

  











  static PRBool AddScriptRunner(nsIRunnable* aRunnable);

  





  static PRBool IsSafeToRunScript() {
    return sScriptBlockerCount == 0;
  }

  





  static nsresult ProcessViewportInfo(nsIDocument *aDocument,
                                      const nsAString &viewportInfo);

  static nsIScriptContext* GetContextForEventHandlers(nsINode* aNode,
                                                      nsresult* aRv);

  static JSContext *GetCurrentJSContext();

  



  static PRBool EqualsIgnoreASCIICase(const nsAString& aStr1,
                                      const nsAString& aStr2);

  


  static void ASCIIToLower(nsAString& aStr);
  static void ASCIIToLower(const nsAString& aSource, nsAString& aDest);

  


  static void ASCIIToUpper(nsAString& aStr);
  static void ASCIIToUpper(const nsAString& aSource, nsAString& aDest);

  
  static nsresult CheckSameOrigin(nsIChannel *aOldChannel, nsIChannel *aNewChannel);
  static nsIInterfaceRequestor* GetSameOriginChecker();

  static nsIThreadJSContextStack* ThreadJSContextStack()
  {
    return sThreadJSContextStack;
  }
  

  












  static nsresult GetASCIIOrigin(nsIPrincipal* aPrincipal,
                                 nsCString& aOrigin);
  static nsresult GetASCIIOrigin(nsIURI* aURI, nsCString& aOrigin);
  static nsresult GetUTFOrigin(nsIPrincipal* aPrincipal,
                               nsString& aOrigin);
  static nsresult GetUTFOrigin(nsIURI* aURI, nsString& aOrigin);

  





  static nsresult DispatchXULCommand(nsIContent* aTarget,
                                     PRBool aTrusted,
                                     nsIDOMEvent* aSourceEvent = nsnull,
                                     nsIPresShell* aShell = nsnull,
                                     PRBool aCtrl = PR_FALSE,
                                     PRBool aAlt = PR_FALSE,
                                     PRBool aShift = PR_FALSE,
                                     PRBool aMeta = PR_FALSE);

  






  static already_AddRefed<nsIDocument>
  GetDocumentFromScriptContext(nsIScriptContext *aScriptContext);

  static PRBool CheckMayLoad(nsIPrincipal* aPrincipal, nsIChannel* aChannel);

  




  static PRBool CanAccessNativeAnon();

  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, const nsIID* aIID, jsval *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nsnull,
                             PRBool aAllowWrapping = PR_FALSE)
  {
    return WrapNative(cx, scope, native, nsnull, aIID, vp, aHolder,
                      aAllowWrapping);
  }

  
  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, jsval *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nsnull,
                             PRBool aAllowWrapping = PR_FALSE)
  {
    return WrapNative(cx, scope, native, nsnull, nsnull, vp, aHolder,
                      aAllowWrapping);
  }
  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, nsWrapperCache *cache,
                             jsval *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nsnull,
                             PRBool aAllowWrapping = PR_FALSE)
  {
    return WrapNative(cx, scope, native, cache, nsnull, vp, aHolder,
                      aAllowWrapping);
  }

  static void StripNullChars(const nsAString& aInStr, nsAString& aOutStr);

  








  static nsresult CreateStructuredClone(JSContext* cx, jsval val, jsval* rval);

  



  static void RemoveNewlines(nsString &aString);

  



  static void PlatformToDOMLineBreaks(nsString &aString);

  static PRBool IsHandlingKeyBoardEvent()
  {
    return sIsHandlingKeyBoardEvent;
  }

  static void SetIsHandlingKeyBoardEvent(PRBool aHandling)
  {
    sIsHandlingKeyBoardEvent = aHandling;
  }

  



  static nsresult GetElementsByClassName(nsINode* aRootNode,
                                         const nsAString& aClasses,
                                         nsIDOMNodeList** aReturn);

  









  static already_AddRefed<mozilla::layers::LayerManager>
  LayerManagerForDocument(nsIDocument *aDoc, bool *aAllowRetaining = nsnull);

  













  static already_AddRefed<mozilla::layers::LayerManager>
  PersistentLayerManagerForDocument(nsIDocument *aDoc, bool *aAllowRetaining = nsnull);

  





  static PRBool IsFocusedContent(const nsIContent *aContent);

  







  static bool IsSubDocumentTabbable(nsIContent* aContent);

  





  static void FlushLayoutForTree(nsIDOMWindow* aWindow);

  



  static bool AllowXULXBLForPrincipal(nsIPrincipal* aPrincipal);

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

  















  static PRBool IsPatternMatching(nsAString& aValue, nsAString& aPattern,
                                  nsIDocument* aDocument);

  



  static void InitializeTouchEventTable();
private:
  static PRBool InitializeEventTable();

  static nsresult EnsureStringBundle(PropertiesFile aFile);

  static nsIDOMScriptObjectFactory *GetDOMScriptObjectFactory();

  static nsresult HoldScriptObject(PRUint32 aLangID, void* aObject);
  static void DropScriptObject(PRUint32 aLangID, void *aObject, void *aClosure);

  static PRBool CanCallerAccess(nsIPrincipal* aSubjectPrincipal,
                                nsIPrincipal* aPrincipal);

  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, nsWrapperCache *cache,
                             const nsIID* aIID, jsval *vp,
                             nsIXPConnectJSObjectHolder** aHolder,
                             PRBool aAllowWrapping);

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
  static PRBool sTriedToGetContentPolicy;

  static nsILineBreaker* sLineBreaker;
  static nsIWordBreaker* sWordBreaker;
  static nsIUGenCategory* sGenCat;

  static nsIScriptRuntime* sScriptRuntimes[NS_STID_ARRAY_UBOUND];
  static PRInt32 sScriptRootCount[NS_STID_ARRAY_UBOUND];
  static PRUint32 sJSGCThingRootCount;

#ifdef IBMBIDI
  static nsIBidiKeyboard* sBidiKeyboard;
#endif

  static PRBool sInitialized;
  static PRUint32 sScriptBlockerCount;
#ifdef DEBUG
  static PRUint32 sDOMNodeRemovedSuppressCount;
#endif
  static nsCOMArray<nsIRunnable>* sBlockedScriptRunners;
  static PRUint32 sRunnersCountAtFirstBlocker;
  static PRUint32 sScriptBlockerCountWhereRunnersPrevented;

  static nsIInterfaceRequestor* sSameOriginChecker;

  static PRBool sIsHandlingKeyBoardEvent;
  static PRBool sAllowXULXBL_for_file;
};

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

  
  PRBool Push(nsPIDOMEventTarget *aCurrentTarget);
  
  
  PRBool RePush(nsPIDOMEventTarget *aCurrentTarget);
  
  
  PRBool Push(JSContext *cx, PRBool aRequiresScriptContext = PR_TRUE);
  
  PRBool PushNull();

  
  void Pop();

  nsIScriptContext* GetCurrentScriptContext() { return mScx; }
private:
  
  PRBool DoPush(JSContext* cx);

  nsCOMPtr<nsIScriptContext> mScx;
  PRBool mScriptIsRunning;
  PRBool mPushedSomething;
#ifdef DEBUG
  JSContext* mPushedContext;
#endif
};

class NS_STACK_CLASS nsAutoScriptBlocker {
public:
  nsAutoScriptBlocker(MOZILLA_GUARD_OBJECT_NOTIFIER_ONLY_PARAM) {
    MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
    nsContentUtils::AddScriptBlocker();
  }
  ~nsAutoScriptBlocker() {
    nsContentUtils::RemoveScriptBlocker();
  }
private:
  MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
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

#define NS_INTERFACE_MAP_ENTRY_TEAROFF(_interface, _allocator)                \
  if (aIID.Equals(NS_GET_IID(_interface))) {                                  \
    foundInterface = static_cast<_interface *>(_allocator);                   \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else












#if defined(__arm) || defined(__arm32__) || defined(__arm26__) || defined(__arm__)
#if !defined(__VFP_FP__)
#define FPU_IS_ARM_FPA
#endif
#endif

typedef union dpun {
    struct {
#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
        PRUint32 lo, hi;
#else
        PRUint32 hi, lo;
#endif
    } s;
    PRFloat64 d;
public:
    operator double() const {
        return d;
    }
} dpun;




#if (__GNUC__ == 2 && __GNUC_MINOR__ > 95) || __GNUC__ > 2




#define DOUBLE_HI32(x) (__extension__ ({ dpun u; u.d = (x); u.s.hi; }))
#define DOUBLE_LO32(x) (__extension__ ({ dpun u; u.d = (x); u.s.lo; }))

#else 





#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
#define DOUBLE_HI32(x)        (((PRUint32 *)&(x))[1])
#define DOUBLE_LO32(x)        (((PRUint32 *)&(x))[0])
#else
#define DOUBLE_HI32(x)        (((PRUint32 *)&(x))[0])
#define DOUBLE_LO32(x)        (((PRUint32 *)&(x))[1])
#endif

#endif 

#define DOUBLE_HI32_SIGNBIT   0x80000000
#define DOUBLE_HI32_EXPMASK   0x7ff00000
#define DOUBLE_HI32_MANTMASK  0x000fffff

#define DOUBLE_IS_NaN(x)                                                \
((DOUBLE_HI32(x) & DOUBLE_HI32_EXPMASK) == DOUBLE_HI32_EXPMASK && \
 (DOUBLE_LO32(x) || (DOUBLE_HI32(x) & DOUBLE_HI32_MANTMASK)))

#ifdef IS_BIG_ENDIAN
#define DOUBLE_NaN {{DOUBLE_HI32_EXPMASK | DOUBLE_HI32_MANTMASK,   \
                        0xffffffff}}
#else
#define DOUBLE_NaN {{0xffffffff,                                         \
                        DOUBLE_HI32_EXPMASK | DOUBLE_HI32_MANTMASK}}
#endif

#if defined(XP_WIN)
#define DOUBLE_COMPARE(LVAL, OP, RVAL)                                  \
    (!DOUBLE_IS_NaN(LVAL) && !DOUBLE_IS_NaN(RVAL) && (LVAL) OP (RVAL))
#else
#define DOUBLE_COMPARE(LVAL, OP, RVAL) ((LVAL) OP (RVAL))
#endif





inline NS_HIDDEN_(PRBool) NS_FloatIsFinite(jsdouble f) {
#ifdef WIN32
  return _finite(f);
#else
  return finite(f);
#endif
}






#define NS_ENSURE_FINITE(f, rv)                                               \
  if (!NS_FloatIsFinite(f)) {                                                 \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE2(f1, f2, rv)                                         \
  if (!NS_FloatIsFinite((f1)+(f2))) {                                         \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE3(f1, f2, f3, rv)                                     \
  if (!NS_FloatIsFinite((f1)+(f2)+(f3))) {                                    \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE4(f1, f2, f3, f4, rv)                                 \
  if (!NS_FloatIsFinite((f1)+(f2)+(f3)+(f4))) {                               \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE5(f1, f2, f3, f4, f5, rv)                             \
  if (!NS_FloatIsFinite((f1)+(f2)+(f3)+(f4)+(f5))) {                          \
    return (rv);                                                              \
  }

#define NS_ENSURE_FINITE6(f1, f2, f3, f4, f5, f6, rv)                         \
  if (!NS_FloatIsFinite((f1)+(f2)+(f3)+(f4)+(f5)+(f6))) {                     \
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

#endif
