








































#ifndef nsContentUtils_h___
#define nsContentUtils_h___

#include "jspubtd.h"
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

class nsIDOMScriptObjectFactory;
class nsIXPConnect;
class nsINode;
class nsIContent;
class nsIDOMNode;
class nsIDocument;
class nsIDocShell;
class nsINameSpaceManager;
class nsIScriptSecurityManager;
class nsIJSContextStack;
class nsIThreadJSContextStack;
class nsIParserService;
class nsIIOService;
class nsIURI;
class imgIDecoderObserver;
class imgIRequest;
class imgILoader;
class nsIPrefBranch;
class nsIImage;
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
template<class E> class nsCOMArray;
class nsIPref;
class nsVoidArray;
struct JSRuntime;
#ifdef MOZ_XTF
class nsIXTFService;
#endif
#ifdef IBMBIDI
class nsIBidiKeyboard;
#endif

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

  












  static PRBool ContentIsDescendantOf(nsINode* aPossibleDescendant,
                                      nsINode* aPossibleAncestor);

  






  static nsresult GetAncestors(nsIDOMNode* aNode,
                               nsVoidArray* aArray);

  











  static nsresult GetAncestorsAndOffsets(nsIDOMNode* aNode,
                                         PRInt32 aOffset,
                                         nsVoidArray* aAncestorNodes,
                                         nsVoidArray* aAncestorOffsets);

  





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
                               nsINode* aParent2, PRInt32 aOffset2);

  









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

  static void Shutdown();

  






  static nsresult CheckSameOrigin(nsIDOMNode* aTrustedNode,
                                  nsIDOMNode* aUnTrustedNode);

  
  static PRBool CanCallerAccess(nsIDOMNode *aNode);

  





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

  



  static PRBool HasNonEmptyAttr(nsIContent* aContent, PRInt32 aNameSpaceID,
                                nsIAtom* aName);

  














  static PRBool CanLoadImage(nsIURI* aURI,
                             nsISupports* aContext,
                             nsIDocument* aLoadingDocument,
                             PRInt16* aImageBlockingStatus = nsnull);
  








  static nsresult LoadImage(nsIURI* aURI,
                            nsIDocument* aLoadingDocument,
                            nsIURI* aReferrer,
                            imgIDecoderObserver* aObserver,
                            PRInt32 aLoadFlags,
                            imgIRequest** aRequest);

  






  static already_AddRefed<nsIImage> GetImageFromContent(nsIImageLoadingContent* aContent, imgIRequest **aRequest = nsnull);

  





  static PRBool ContentIsDraggable(nsIContent* aContent) {
    return IsDraggableImage(aContent) || IsDraggableLink(aContent);
  }

  





  static PRBool IsDraggableImage(nsIContent* aContent);

  





  static PRBool IsDraggableLink(nsIContent* aContent);

  



  static nsresult NameChanged(nsINodeInfo *aNodeInfo, nsIAtom *aName,
                              nsINodeInfo** aResult)
  {
    nsNodeInfoManager *niMgr = aNodeInfo->NodeInfoManager();

    return niMgr->GetNodeInfo(aName, aNodeInfo->GetPrefixAtom(),
                              aNodeInfo->NamespaceID(), aResult);
  }

  



  static nsresult PrefixChanged(nsINodeInfo *aNodeInfo, nsIAtom *aPrefix,
                                nsINodeInfo** aResult)
  {
    nsNodeInfoManager *niMgr = aNodeInfo->NodeInfoManager();

    return niMgr->GetNodeInfo(aNodeInfo->NameAtom(), aPrefix,
                              aNodeInfo->NamespaceID(), aResult);
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

  


  static void NotifyXPCIfExceptionPending(JSContext *aCx);

  


  static nsresult ReleasePtrOnShutdown(nsISupports** aSupportsPtr) {
    NS_ASSERTION(aSupportsPtr, "Expect to crash!");
    NS_ASSERTION(*aSupportsPtr, "Expect to crash!");
    return sPtrsToPtrsToRelease->AppendElement(aSupportsPtr) ? NS_OK :
      NS_ERROR_OUT_OF_MEMORY;
  }

  


  static nsIContentPolicy *GetContentPolicy();

  




  static nsresult AddJSGCRoot(jsval* aPtr, const char* aName) {
    return AddJSGCRoot((void*)aPtr, aName);
  }

  




  static nsresult AddJSGCRoot(JSObject** aPtr, const char* aName) {
    return AddJSGCRoot((void*)aPtr, aName);
  }

  




  static nsresult AddJSGCRoot(void* aPtr, const char* aName);  

  


  static nsresult RemoveJSGCRoot(jsval* aPtr) {
    return RemoveJSGCRoot((void*)aPtr);
  }
  static nsresult RemoveJSGCRoot(JSObject** aPtr) {
    return RemoveJSGCRoot((void*)aPtr);
  }
  static nsresult RemoveJSGCRoot(void* aPtr);

  








  static PRBool HasMutationListeners(nsINode* aNode,
                                     PRUint32 aType);

  












  static nsresult DispatchTrustedEvent(nsIDocument* aDoc,
                                       nsISupports* aTarget,
                                       const nsAString& aEventName,
                                       PRBool aCanBubble,
                                       PRBool aCancelable,
                                       PRBool *aDefaultAction = nsnull);

  







  static PRBool IsEventAttributeName(nsIAtom* aName, PRInt32 aType);

  






  static PRUint32 GetEventId(nsIAtom* aName);

  










  static void TraverseListenerManager(nsINode *aNode,
                                      nsCycleCollectionTraversalCallback &cb);

  








  static nsresult GetListenerManager(nsINode *aNode,
                                     PRBool aCreateIfNotFound,
                                     nsIEventListenerManager **aResult);

  




  static void RemoveListenerManager(nsINode *aNode);

  static PRBool IsInitialized()
  {
    return sInitialized;
  }

  








  static PRBool IsValidNodeName(nsIAtom *aLocalName, nsIAtom *aPrefix,
                                PRInt32 aNamespaceID);

  














  static nsresult SetUserData(nsINode *aNode, nsIAtom *aKey, nsIVariant *aData,
                              nsIDOMUserDataHandler *aHandler,
                              nsIVariant **aResult);

  







  static nsresult CreateContextualFragment(nsIDOMNode* aContextNode,
                                           const nsAString& aFragment,
                                           nsIDOMDocumentFragment** aReturn);

  















  static nsresult CreateDocument(const nsAString& aNamespaceURI, 
                                 const nsAString& aQualifiedName, 
                                 nsIDOMDocumentType* aDoctype,
                                 nsIURI* aDocumentURI,
                                 nsIURI* aBaseURI,
                                 nsIPrincipal* aPrincipal,
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
      nsString* matchString = NS_STATIC_CAST(nsString*, aData);
      delete matchString;
    }
  }

  


  static void DestroyAnonymousContent(nsCOMPtr<nsIContent>* aContent);

  static nsresult HoldScriptObject(PRUint32 aLangID, void *aObject);
  static nsresult DropScriptObject(PRUint32 aLangID, void *aObject);

  class ScriptObjectHolder
  {
  public:
    ScriptObjectHolder(PRUint32 aLangID) : mLangID(aLangID),
                                           mObject(nsnull)
    {
    }
    ~ScriptObjectHolder()
    {
      if (mObject)
        DropScriptObject(mLangID, mObject);
    }
    nsresult set(void *aObject)
    {
      nsresult rv = HoldScriptObject(mLangID, aObject);
      if (NS_SUCCEEDED(rv)) {
        mObject = aObject;
      }
      return rv;
    }
    PRUint32 mLangID;
    void *mObject;
  };

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

  static nsIConsoleService* sConsoleService;

  static nsDataHashtable<nsISupportsHashKey, EventNameMapping>* sEventTable;

  static nsIStringBundleService* sStringBundleService;
  static nsIStringBundle* sStringBundles[PropertiesFile_COUNT];

  static nsIContentPolicy* sContentPolicyService;
  static PRBool sTriedToGetContentPolicy;

  static nsILineBreaker* sLineBreaker;
  static nsIWordBreaker* sWordBreaker;

  
  static nsVoidArray* sPtrsToPtrsToRelease;

  
  
  static nsIJSRuntimeService* sJSRuntimeService;
  static JSRuntime* sJSScriptRuntime;
  static PRInt32 sJSScriptRootCount;

  static nsIScriptRuntime* sScriptRuntimes[NS_STID_ARRAY_UBOUND];
  static PRInt32 sScriptRootCount[NS_STID_ARRAY_UBOUND];

#ifdef IBMBIDI
  static nsIBidiKeyboard* sBidiKeyboard;
#endif

  static PRBool sInitialized;
};


class nsCxPusher
{
public:
  nsCxPusher(nsISupports *aCurrentTarget);
  ~nsCxPusher();

  void Push(nsISupports *aCurrentTarget);
  void Pop();

private:
  nsCOMPtr<nsIJSContextStack> mStack;
  nsCOMPtr<nsIScriptContext> mScx;
  PRBool mScriptIsRunning;
};

class nsAutoGCRoot {
public:
  
  nsAutoGCRoot(jsval* aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult =
      nsContentUtils::AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  
  nsAutoGCRoot(JSObject** aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult =
      nsContentUtils::AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  
  nsAutoGCRoot(void* aPtr, nsresult* aResult) :
    mPtr(aPtr)
  {
    mResult = *aResult =
      nsContentUtils::AddJSGCRoot(aPtr, "nsAutoGCRoot");
  }

  ~nsAutoGCRoot() {
    if (NS_SUCCEEDED(mResult)) {
      nsContentUtils::RemoveJSGCRoot(mPtr);
    }
  }

private:
  void* mPtr;
  nsresult mResult;
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
    foundInterface = NS_STATIC_CAST(_interface *, _allocator);                \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else

#endif 
