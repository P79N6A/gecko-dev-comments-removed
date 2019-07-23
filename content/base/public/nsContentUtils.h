








































#ifndef nsContentUtils_h___
#define nsContentUtils_h___

#include "jsprvtd.h"
#include "jsnum.h"
#include "nsAString.h"
#include "nsIStatefulFrame.h"
#include "nsIPref.h"
#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsContentList.h"
#include "nsDOMClassInfoID.h"
#include "nsIClassInfo.h"
#include "nsIDOM3Node.h"
#include "nsDataHashtable.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMEvent.h"
#include "nsTArray.h"
#include "nsTextFragment.h"
#include "nsReadableUtils.h"

struct nsNativeKeyEvent; 

class nsIDOMScriptObjectFactory;
class nsIXPConnect;
class nsINode;
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
class nsIPrefBranch;
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
class nsIPref;
struct JSRuntime;
class nsICaseConversion;
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

extern const char kLoadAsData[];

enum EventNameType {
  EventNameType_None = 0x0000,
  EventNameType_HTML = 0x0001,
  EventNameType_XUL = 0x0002,
  EventNameType_SVGGraphic = 0x0004, 
  EventNameType_SVGSVG = 0x0008, 

  EventNameType_HTMLXUL = 0x0003,
  EventNameType_All = 0xFFFF
};

struct EventNameMapping {
  PRUint32  mId;
  PRInt32 mType;
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
public:
  static nsresult Init();

  
  
  
  
  
  
  static nsresult ReparentContentWrapper(nsIContent *aNode,
                                         nsIContent *aNewParent,
                                         nsIDocument *aNewDocument,
                                         nsIDocument *aOldDocument);

  










  static nsresult GetContextAndScopes(nsIDocument *aOldDocument,
                                      nsIDocument *aNewDocument,
                                      JSContext **aCx, JSObject **aOldScope,
                                      JSObject **aNewScope);

  



  static nsresult ReparentContentWrappersInScope(nsIScriptGlobalObject *aOldScope,
                                                 nsIScriptGlobalObject *aNewScope);

  static PRBool   IsCallerChrome();

  static PRBool   IsCallerTrustedForRead();

  static PRBool   IsCallerTrustedForWrite();

  



  static PRBool   IsCallerTrustedForCapability(const char* aCapability);

  












  static PRBool ContentIsDescendantOf(nsINode* aPossibleDescendant,
                                      nsINode* aPossibleAncestor);

  


  static PRBool ContentIsCrossDocDescendantOf(nsINode* aPossibleDescendant,
                                              nsINode* aPossibleAncestor);

  



  static nsresult GetAncestors(nsIDOMNode* aNode,
                               nsTArray<nsIDOMNode*>* aArray);

  







  static nsresult GetAncestorsAndOffsets(nsIDOMNode* aNode,
                                         PRInt32 aOffset,
                                         nsTArray<nsIContent*>* aAncestorNodes,
                                         nsTArray<PRInt32>* aAncestorOffsets);

  





  static nsresult GetCommonAncestor(nsIDOMNode *aNode,
                                    nsIDOMNode *aOther,
                                    nsIDOMNode** aCommonAncestor);

  



  static nsINode* GetCommonAncestor(nsINode* aNode1,
                                    nsINode* aNode2);

  













  static PRUint16 ComparePosition(nsINode* aNode1,
                                  nsINode* aNode2);

  



  static PRBool PositionIsBefore(nsINode* aNode1,
                                 nsINode* aNode2)
  {
    return (ComparePosition(aNode1, aNode2) &
      (nsIDOM3Node::DOCUMENT_POSITION_PRECEDING |
       nsIDOM3Node::DOCUMENT_POSITION_DISCONNECTED)) ==
      nsIDOM3Node::DOCUMENT_POSITION_PRECEDING;
  }

  








  static PRInt32 ComparePoints(nsINode* aParent1, PRInt32 aOffset1,
                               nsINode* aParent2, PRInt32 aOffset2,
                               PRBool* aDisconnected = nsnull);

  









  static nsIContent* FindFirstChildWithResolvedTag(nsIContent* aParent,
                                                   PRInt32 aNamespace,
                                                   nsIAtom* aTag);

  




  static nsIContent* MatchElementId(nsIContent *aContent,
                                    const nsAString& aId);

  


  static nsIContent* MatchElementId(nsIContent *aContent,
                                    nsIAtom* aId);

  














  static nsIContent* GetReferencedElement(nsIURI* aURI,
                                          nsIContent *aFromContent);

  









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

  static const nsDependentSubstring TrimWhitespace(const nsAString& aStr,
                                                   PRBool aTrimTrailing = PR_TRUE);

  


  static PRBool IsPunctuationMark(PRUint32 aChar);
  static PRBool IsPunctuationMarkAt(const nsTextFragment* aFrag, PRUint32 aOffset);
 
  


  static PRBool IsAlphanumeric(PRUint32 aChar);
  static PRBool IsAlphanumericAt(const nsTextFragment* aFrag, PRUint32 aOffset);

  







  static PRBool IsHTMLWhitespace(PRUnichar aChar);

  static void Shutdown();

  






  static nsresult CheckSameOrigin(nsIDOMNode* aTrustedNode,
                                  nsIDOMNode* aUnTrustedNode);

  
  static PRBool CanCallerAccess(nsIDOMNode *aNode);

  
  
  static PRBool CanCallerAccess(nsPIDOMWindow* aWindow);

  





  static nsIDocShell *GetDocShellFromCaller();

  















  





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
                                   nsIDocument* aDocument,
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


  







  static PRBool BelongsInForm(nsIDOMHTMLFormElement *aForm,
                              nsIContent *aContent);

  static nsresult CheckQName(const nsAString& aQualifiedName,
                             PRBool aNamespaceAware = PR_TRUE);

  static nsresult SplitQName(nsIContent* aNamespaceResolver,
                             const nsAFlatString& aQName,
                             PRInt32 *aNamespace, nsIAtom **aLocalName);

  static nsresult LookupNamespaceURI(nsIContent* aNamespaceResolver,
                                     const nsAString& aNamespacePrefix,
                                     nsAString& aNamespaceURI);

  static nsresult GetNodeInfoFromQName(const nsAString& aNamespaceURI,
                                       const nsAString& aQualifiedName,
                                       nsNodeInfoManager* aNodeInfoManager,
                                       nsINodeInfo** aNodeInfo);

  static void SplitExpatName(const PRUnichar *aExpatName, nsIAtom **aPrefix,
                             nsIAtom **aTagName, PRInt32 *aNameSpaceID);

  static nsAdoptingCString GetCharPref(const char *aPref);
  static PRPackedBool GetBoolPref(const char *aPref,
                                  PRBool aDefault = PR_FALSE);
  static PRInt32 GetIntPref(const char *aPref, PRInt32 aDefault = 0);
  static nsAdoptingString GetLocalizedStringPref(const char *aPref);
  static nsAdoptingString GetStringPref(const char *aPref);
  static void RegisterPrefCallback(const char *aPref,
                                   PrefChangedFunc aCallback,
                                   void * aClosure);
  static void UnregisterPrefCallback(const char *aPref,
                                     PrefChangedFunc aCallback,
                                     void * aClosure);
  static void AddBoolPrefVarCache(const char* aPref, PRBool* aVariable);
  static void AddIntPrefVarCache(const char* aPref, PRInt32* aVariable);
  static nsIPrefBranch *GetPrefBranch()
  {
    return sPrefBranch;
  }

  static nsILineBreaker* LineBreaker()
  {
    return sLineBreaker;
  }

  static nsIWordBreaker* WordBreaker()
  {
    return sWordBreaker;
  }
  
  static nsICaseConversion* GetCaseConv()
  {
    return sCaseConv;
  }

  static nsIUGenCategory* GetGenCat()
  {
    return sGenCat;
  }

  



  static PRBool HasNonEmptyAttr(nsIContent* aContent, PRInt32 aNameSpaceID,
                                nsIAtom* aName);

  






  static nsPresContext* GetContextForContent(nsIContent* aContent);

  















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

  





  static PRBool IsDraggableLink(nsIContent* aContent);

  



  static nsresult NameChanged(nsINodeInfo *aNodeInfo, nsIAtom *aName,
                              nsINodeInfo** aResult)
  {
    nsNodeInfoManager *niMgr = aNodeInfo->NodeInfoManager();

    *aResult = niMgr->GetNodeInfo(aName, aNodeInfo->GetPrefixAtom(),
                                  aNodeInfo->NamespaceID()).get();
    return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  



  static nsresult PrefixChanged(nsINodeInfo *aNodeInfo, nsIAtom *aPrefix,
                                nsINodeInfo** aResult)
  {
    nsNodeInfoManager *niMgr = aNodeInfo->NodeInfoManager();

    *aResult = niMgr->GetNodeInfo(aNodeInfo->NameAtom(), aPrefix,
                                  aNodeInfo->NamespaceID()).get();
    return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  





  static void GetEventArgNames(PRInt32 aNameSpaceID, nsIAtom *aEventName,
                               PRUint32 *aArgCount, const char*** aArgNames);

  













  static PRBool IsInSameAnonymousTree(nsINode* aNode, nsIContent* aContent);

  


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
#ifdef MOZ_SVG
    eSVG_PROPERTIES,
#endif
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
                                  const char *aCategory);

  


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

  


  static nsresult ReleasePtrOnShutdown(nsISupports** aSupportsPtr) {
    NS_ASSERTION(aSupportsPtr, "Expect to crash!");
    NS_ASSERTION(*aSupportsPtr, "Expect to crash!");
    return sPtrsToPtrsToRelease->AppendElement(aSupportsPtr) != nsnull ? NS_OK :
      NS_ERROR_OUT_OF_MEMORY;
  }

  


  static nsIContentPolicy *GetContentPolicy();

  












  static PRBool HasMutationListeners(nsINode* aNode,
                                     PRUint32 aType,
                                     nsINode* aTargetForSubtreeModified);

  












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

  









  static nsresult CreateContextualFragment(nsIDOMNode* aContextNode,
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
      aCallback(nsIProgrammingLanguage::JAVASCRIPT, aCache->GetWrapper(),
                aClosure);
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

  



  static nsIAtom* IsNamedItem(nsIContent* aContent);

  







  static void GetOfflineAppManifest(nsIDocument *aDocument, nsIURI **aURI);

  


  static PRBool OfflineAppAllowed(nsIURI *aURI);

  


  static PRBool OfflineAppAllowed(nsIPrincipal *aPrincipal);

  




  static void AddScriptBlocker();

  






  static void RemoveScriptBlocker();

  











  static PRBool AddScriptRunner(nsIRunnable* aRunnable);

  





  static PRBool IsSafeToRunScript() {
    return sScriptBlockerCount == 0;
  }

  





  static void AddRemovableScriptBlocker()
  {
    AddScriptBlocker();
    ++sRemovableScriptBlockerCount;
  }
  static void RemoveRemovableScriptBlocker()
  {
    NS_ASSERTION(sRemovableScriptBlockerCount != 0,
                "Number of removable blockers should never go below zero");
    --sRemovableScriptBlockerCount;
    RemoveScriptBlocker();
  }
  static PRUint32 GetRemovableScriptBlockerLevel()
  {
    return sRemovableScriptBlockerCount;
  }

  





  static nsresult ProcessViewportInfo(nsIDocument *aDocument,
                                      const nsAString &viewportInfo);

  static nsIScriptContext* GetContextForEventHandlers(nsINode* aNode,
                                                      nsresult* aRv);

  static JSContext *GetCurrentJSContext();

                                             
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

  




  static PRBool CanAccessNativeAnon();

  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native, const nsIID* aIID, jsval *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nsnull,
                             PRBool aAllowWrapping = PR_FALSE);

  
  static nsresult WrapNative(JSContext *cx, JSObject *scope,
                             nsISupports *native,  jsval *vp,
                             
                             
                             nsIXPConnectJSObjectHolder** aHolder = nsnull,
                             PRBool aAllowWrapping = PR_FALSE)
  {
    return WrapNative(cx, scope, native, nsnull, vp, aHolder, aAllowWrapping);
  }

private:

  static PRBool InitializeEventTable();

  static nsresult doReparentContentWrapper(nsIContent *aChild,
                                           JSContext *cx,
                                           JSObject *aOldGlobal,
                                           JSObject *aNewGlobal,
                                           nsIDocument *aOldDocument,
                                           nsIDocument *aNewDocument);

  static nsresult EnsureStringBundle(PropertiesFile aFile);

  static nsIDOMScriptObjectFactory *GetDOMScriptObjectFactory();

  static nsresult HoldScriptObject(PRUint32 aLangID, void* aObject);
  static void DropScriptObject(PRUint32 aLangID, void *aObject, void *aClosure);

  static PRBool CanCallerAccess(nsIPrincipal* aSubjectPrincipal,
                                nsIPrincipal* aPrincipal);

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

  static nsIPrefBranch *sPrefBranch;

  static nsIPref *sPref;

  static imgILoader* sImgLoader;
  static imgICache* sImgCache;

  static nsIConsoleService* sConsoleService;

  static nsDataHashtable<nsISupportsHashKey, EventNameMapping>* sEventTable;

  static nsIStringBundleService* sStringBundleService;
  static nsIStringBundle* sStringBundles[PropertiesFile_COUNT];

  static nsIContentPolicy* sContentPolicyService;
  static PRBool sTriedToGetContentPolicy;

  static nsILineBreaker* sLineBreaker;
  static nsIWordBreaker* sWordBreaker;
  static nsICaseConversion* sCaseConv;
  static nsIUGenCategory* sGenCat;

  
  static nsTArray<nsISupports**>* sPtrsToPtrsToRelease;

  static nsIScriptRuntime* sScriptRuntimes[NS_STID_ARRAY_UBOUND];
  static PRInt32 sScriptRootCount[NS_STID_ARRAY_UBOUND];
  static PRUint32 sJSGCThingRootCount;

#ifdef IBMBIDI
  static nsIBidiKeyboard* sBidiKeyboard;
#endif

  static PRBool sInitialized;
  static PRUint32 sScriptBlockerCount;
  static PRUint32 sRemovableScriptBlockerCount;
  static nsCOMArray<nsIRunnable>* sBlockedScriptRunners;
  static PRUint32 sRunnersCountAtFirstBlocker;

  static nsIInterfaceRequestor* sSameOriginChecker;
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
  
  
  PRBool Push(JSContext *cx);
  
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

class nsAutoGCRoot {
public:
  
  nsAutoGCRoot(jsval* aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult = AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  
  nsAutoGCRoot(JSObject** aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult = AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  
  nsAutoGCRoot(void* aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult = AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  ~nsAutoGCRoot() {
    if (NS_SUCCEEDED(mResult)) {
      RemoveJSGCRoot(mPtr);
    }
  }

  static void Shutdown();

private:
  static nsresult AddJSGCRoot(void *aPtr, const char* aName);
  static nsresult RemoveJSGCRoot(void *aPtr);

  static nsIJSRuntimeService* sJSRuntimeService;
  static JSRuntime* sJSScriptRuntime;

  void* mPtr;
  nsresult mResult;
};

class nsAutoScriptBlocker {
public:
  nsAutoScriptBlocker() {
    nsContentUtils::AddScriptBlocker();
  }
  ~nsAutoScriptBlocker() {
    nsContentUtils::RemoveScriptBlocker();
  }
};

class mozAutoRemovableBlockerRemover
{
public:
  mozAutoRemovableBlockerRemover(nsIDocument* aDocument);
  ~mozAutoRemovableBlockerRemover();

private:
  PRUint32 mNestingLevel;
  nsCOMPtr<nsIDocument> mDocument;
  nsCOMPtr<nsIDocumentObserver> mObserver;
};

#define NS_AUTO_GCROOT_PASTE2(tok,line) tok##line
#define NS_AUTO_GCROOT_PASTE(tok,line) \
  NS_AUTO_GCROOT_PASTE2(tok,line)
#define NS_AUTO_GCROOT(ptr, result) \ \
  nsAutoGCRoot NS_AUTO_GCROOT_PASTE(_autoGCRoot_, __LINE__) \
  (ptr, result)

#define NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(_class)                      \
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(_class)

#define NS_INTERFACE_MAP_ENTRY_TEAROFF(_interface, _allocator)                \
  if (aIID.Equals(NS_GET_IID(_interface))) {                                  \
    foundInterface = static_cast<_interface *>(_allocator);                   \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else















inline NS_HIDDEN_(PRBool) NS_FloatIsFinite(jsdouble f) {
  return JSDOUBLE_IS_FINITE(f);
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
