



































#ifndef nsIDocument_h___
#define nsIDocument_h___

#include "nsINode.h"
#include "nsStringGlue.h"
#include "nsIDocumentObserver.h" 
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIURI.h"
#include "nsWeakPtr.h"
#include "nsIWeakReferenceUtils.h"
#include "nsILoadGroup.h"
#include "nsCRT.h"
#include "mozFlushType.h"
#include "nsIAtom.h"
#include "nsCompatibility.h"
#include "nsTObserverArray.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsNodeInfoManager.h"
#include "nsIStreamListener.h"
#include "nsIObserver.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#ifdef MOZ_SMIL
class nsSMILAnimationController;
#endif 

class nsIContent;
class nsPresContext;
class nsIPresShell;
class nsIDocShell;
class nsStyleSet;
class nsIStyleSheet;
class nsIStyleRule;
class nsIViewManager;
class nsIScriptGlobalObject;
class nsPIDOMWindow;
class nsIDOMEvent;
class nsIDOMEventTarget;
class nsIDeviceContext;
class nsIParser;
class nsIDOMNode;
class nsIDOMElement;
class nsIDOMDocumentFragment;
class nsILineBreaker;
class nsIWordBreaker;
class nsISelection;
class nsIChannel;
class nsIPrincipal;
class nsIDOMDocument;
class nsIDOMDocumentType;
class nsScriptLoader;
class nsIContentSink;
class nsIScriptEventManager;
class nsICSSLoader;
class nsHTMLStyleSheet;
class nsHTMLCSSStyleSheet;
class nsILayoutHistoryState;
class nsIVariant;
class nsIDOMUserDataHandler;
template<class E> class nsCOMArray;
class nsIDocumentObserver;
class nsBindingManager;
class nsIDOMNodeList;
class mozAutoSubtreeModified;
struct JSObject;
class nsFrameLoader;
class nsIBoxObject;

namespace mozilla {
namespace dom {
class Link;
} 
} 


#define NS_IDOCUMENT_IID      \
  { 0xd7978655, 0x9b7d, 0x41e6, \
    { 0xad, 0x48, 0xdf, 0x32, 0x0b, 0x06, 0xb4, 0xda } }


#define NS_STYLESHEET_FROM_CATALOG                (1 << 0)





class nsIDocument : public nsINode
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_IID)
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

#ifdef MOZILLA_INTERNAL_API
  nsIDocument()
    : nsINode(nsnull),
      mCharacterSet(NS_LITERAL_CSTRING("ISO-8859-1")),
      mNodeInfoManager(nsnull),
      mCompatMode(eCompatibility_FullStandards),
      mIsInitialDocumentInWindow(PR_FALSE),
      mMayStartLayout(PR_TRUE),
      mVisible(PR_TRUE),
      mRemovedFromDocShell(PR_FALSE),
      
      
      
      
      mAllowDNSPrefetch(PR_TRUE),
      mPartID(0)
  {
    mParentPtrBits |= PARENT_BIT_INDOCUMENT;
  }
#endif
  
  



























  
  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     PRBool aReset,
                                     nsIContentSink* aSink = nsnull) = 0;
  virtual void StopDocumentLoad() = 0;

  





  virtual void NotifyPossibleTitleChange(PRBool aBoundTitleElement) = 0;
  
  


  nsIURI* GetDocumentURI() const
  {
    return mDocumentURI;
  }

  


  virtual void SetDocumentURI(nsIURI* aURI) = 0;

  


  virtual void SetPrincipal(nsIPrincipal *aPrincipal) = 0;

  


  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup() const
  {
    nsILoadGroup *group = nsnull;
    if (mDocumentLoadGroup)
      CallQueryReferent(mDocumentLoadGroup.get(), &group);

    return group;
  }

  




  nsIURI* GetBaseURI() const
  {
    return mDocumentBaseURI ? mDocumentBaseURI : mDocumentURI;
  }
  virtual nsresult SetBaseURI(nsIURI* aURI) = 0;

  


  virtual void GetBaseTarget(nsAString &aBaseTarget) const = 0;
  virtual void SetBaseTarget(const nsAString &aBaseTarget) = 0;

  


  const nsCString& GetDocumentCharacterSet() const
  {
    return mCharacterSet;
  }

  



  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID) = 0;

  PRInt32 GetDocumentCharacterSetSource() const
  {
    return mCharacterSetSource;
  }

  
  
  void SetDocumentCharacterSetSource(PRInt32 aCharsetSource)
  {
    mCharacterSetSource = aCharsetSource;
  }

  


  virtual nsresult AddCharSetObserver(nsIObserver* aObserver) = 0;

  


  virtual void RemoveCharSetObserver(nsIObserver* aObserver) = 0;

  







  typedef PRBool (* IDTargetObserver)(nsIContent* aOldContent,
                                      nsIContent* aNewContent, void* aData);

  






  virtual nsIContent* AddIDTargetObserver(nsIAtom* aID,
                                          IDTargetObserver aObserver, void* aData) = 0;
  


  virtual void RemoveIDTargetObserver(nsIAtom* aID,
                                      IDTargetObserver aObserver, void* aData) = 0;

  




  NS_IMETHOD GetContentType(nsAString& aContentType) = 0;

  


  virtual void SetContentType(const nsAString& aContentType) = 0;

  


  void GetContentLanguage(nsAString& aContentLanguage) const
  {
    CopyASCIItoUTF16(mContentLanguage, aContentLanguage);
  }

  
  

  



  PRBool GetBidiEnabled() const
  {
    return mBidiEnabled;
  }

  





  void SetBidiEnabled()
  {
    mBidiEnabled = PR_TRUE;
  }
  
  


  PRBool GetMathMLEnabled() const
  {
    return mMathMLEnabled;
  }
  
  void SetMathMLEnabled()
  {
    mMathMLEnabled = PR_TRUE;
  }

  


  PRBool IsInitialDocument() const
  {
    return mIsInitialDocumentInWindow;
  }
  
  



  void SetIsInitialDocument(PRBool aIsInitialDocument)
  {
    mIsInitialDocumentInWindow = aIsInitialDocument;
  }
  

  



  PRUint32 GetBidiOptions() const
  {
    return mBidiOptions;
  }

  





  void SetBidiOptions(PRUint32 aBidiOptions)
  {
    mBidiOptions = aBidiOptions;
  }
  
  



  virtual void GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const = 0;
  virtual void SetHeaderData(nsIAtom* aheaderField, const nsAString& aData) = 0;

  






  virtual nsresult CreateShell(nsPresContext* aContext,
                               nsIViewManager* aViewManager,
                               nsStyleSet* aStyleSet,
                               nsIPresShell** aInstancePtrResult) = 0;
  void DeleteShell() { mPresShell = nsnull; }

  nsIPresShell* GetPrimaryShell() const
  {
    return mShellIsHidden ? nsnull : mPresShell;
  }

  void SetShellHidden(PRBool aHide) { mShellIsHidden = aHide; }
  PRBool ShellIsHidden() const { return mShellIsHidden; }

  




  nsIDocument *GetParentDocument() const
  {
    return mParentDocument;
  }

  


  void SetParentDocument(nsIDocument* aParent)
  {
    mParentDocument = aParent;
  }

  


  virtual nsresult SetSubDocumentFor(nsIContent *aContent,
                                     nsIDocument* aSubDoc) = 0;

  


  virtual nsIDocument *GetSubDocumentFor(nsIContent *aContent) const = 0;

  


  virtual nsIContent *FindContentForSubDocument(nsIDocument *aDocument) const = 0;

  


  nsIContent *GetRootContent() const
  {
    return (mCachedRootContent &&
            mCachedRootContent->GetNodeParent() == this) ?
           reinterpret_cast<nsIContent*>(mCachedRootContent.get()) :
           GetRootContentInternal();
  }
  virtual nsIContent *GetRootContentInternal() const = 0;

  
  
  nsIContent* GetHtmlContent();
  
  
  nsIContent* GetHtmlChildContent(nsIAtom* aTag);
  
  
  nsIContent* GetBodyContent() {
    return GetHtmlChildContent(nsGkAtoms::body);
  }
  
  
  nsIContent* GetHeadContent() {
    return GetHtmlChildContent(nsGkAtoms::head);
  }
  
  




  





  virtual PRInt32 GetNumberOfStyleSheets() const = 0;
  
  





  virtual nsIStyleSheet* GetStyleSheetAt(PRInt32 aIndex) const = 0;
  
  






  virtual void InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex) = 0;

  





  virtual PRInt32 GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const = 0;

  







  virtual void UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                                 nsCOMArray<nsIStyleSheet>& aNewSheets) = 0;

  


  virtual void AddStyleSheet(nsIStyleSheet* aSheet) = 0;

  


  virtual void RemoveStyleSheet(nsIStyleSheet* aSheet) = 0;

  



  virtual void SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                            PRBool aApplicable) = 0;  

  



  virtual PRInt32 GetNumberOfCatalogStyleSheets() const = 0;
  virtual nsIStyleSheet* GetCatalogStyleSheetAt(PRInt32 aIndex) const = 0;
  virtual void AddCatalogStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual void EnsureCatalogStyleSheet(const char *aStyleSheetURI) = 0;

  


  nsICSSLoader* CSSLoader() const {
    return mCSSLoader;
  }

  




  virtual nsIChannel* GetChannel() const = 0;

  



  virtual nsHTMLStyleSheet* GetAttributeStyleSheet() const = 0;

  



  virtual nsHTMLCSSStyleSheet* GetInlineStyleSheet() const = 0;

  





  virtual nsIScriptGlobalObject* GetScriptGlobalObject() const = 0;
  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject) = 0;

  








  virtual nsIScriptGlobalObject*
    GetScriptHandlingObject(PRBool& aHasHadScriptHandlingObject) const = 0;
  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject) = 0;

  






  virtual nsIScriptGlobalObject* GetScopeObject() = 0;

  


  virtual nsPIDOMWindow *GetWindow() = 0;

  




  virtual nsPIDOMWindow *GetInnerWindow() = 0;

  

 
  virtual nsScriptLoader* ScriptLoader() = 0;

  

  

  




  virtual void AddObserver(nsIDocumentObserver* aObserver) = 0;

  



  virtual PRBool RemoveObserver(nsIDocumentObserver* aObserver) = 0;

  
  
  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType) = 0;
  virtual void EndUpdate(nsUpdateType aUpdateType) = 0;
  virtual void BeginLoad() = 0;
  virtual void EndLoad() = 0;

  enum ReadyState { READYSTATE_UNINITIALIZED = 0, READYSTATE_LOADING = 1, READYSTATE_INTERACTIVE = 3, READYSTATE_COMPLETE = 4};
  virtual void SetReadyStateInternal(ReadyState rs) = 0;
  virtual ReadyState GetReadyStateEnum() = 0;

  
  
  virtual void ContentStatesChanged(nsIContent* aContent1,
                                    nsIContent* aContent2,
                                    PRInt32 aStateMask) = 0;

  
  
  virtual void StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule) = 0;
  virtual void StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) = 0;
  virtual void StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule) = 0;

  



  virtual void FlushPendingNotifications(mozFlushType aType) = 0;

  nsBindingManager* BindingManager() const
  {
    return mNodeInfoManager->GetBindingManager();
  }

  



  nsNodeInfoManager* NodeInfoManager() const
  {
    return mNodeInfoManager;
  }

  




  virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup) = 0;

  




  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup* aLoadGroup,
                          nsIPrincipal* aPrincipal) = 0;

  



  virtual void SetContainer(nsISupports *aContainer);

  


  already_AddRefed<nsISupports> GetContainer() const
  {
    nsISupports* container = nsnull;
    if (mDocumentContainer)
      CallQueryReferent(mDocumentContainer.get(), &container);

    return container;
  }

  virtual nsIScriptEventManager* GetScriptEventManager() = 0;

  





  virtual void SetXMLDeclaration(const PRUnichar *aVersion,
                                 const PRUnichar *aEncoding,
                                 const PRInt32 aStandalone) = 0;
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone) = 0;

  PRBool IsHTML() const
  {
    return mIsRegularHTML;
  }

  virtual PRBool IsScriptEnabled() = 0;

  virtual nsresult AddXMLEventsContent(nsIContent * aXMLEventsElement) = 0;

  





  virtual nsresult CreateElem(nsIAtom *aName, nsIAtom *aPrefix,
                              PRInt32 aNamespaceID,
                              PRBool aDocumentDefaultType,
                              nsIContent** aResult) = 0;

  






  nsISupports *GetSecurityInfo()
  {
    return mSecurityInfo;
  }

  



  virtual PRInt32 GetDefaultNamespaceID() const = 0;

  nsPropertyTable* PropertyTable() { return &mPropertyTable; }

  


  void SetPartID(PRUint32 aID) {
    mPartID = aID;
  }

  


  PRUint32 GetPartID() const {
    return mPartID;
  }

  



  virtual nsresult Sanitize() = 0;

  




  typedef PRBool (*nsSubDocEnumFunc)(nsIDocument *aDocument, void *aData);
  virtual void EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                     void *aData) = 0;

  
















  virtual PRBool CanSavePresentation(nsIRequest *aNewRequest) = 0;

  




  virtual void Destroy() = 0;

  





  virtual void RemovedFromDocShell() = 0;
  
  




  virtual already_AddRefed<nsILayoutHistoryState> GetLayoutHistoryState() const = 0;

  






  virtual void BlockOnload() = 0;
  





  virtual void UnblockOnload(PRBool aFireSync) = 0;

  











  virtual void OnPageShow(PRBool aPersisted,
                          nsIDOMEventTarget* aDispatchStartTarget) = 0;

  











  virtual void OnPageHide(PRBool aPersisted,
                          nsIDOMEventTarget* aDispatchStartTarget) = 0;
  
  



  


  virtual void AddStyleRelevantLink(mozilla::dom::Link* aLink) = 0;
  





  virtual void ForgetLink(mozilla::dom::Link* aLink) = 0;

  




  virtual void ClearBoxObjectFor(nsIContent *aContent) = 0;

  



  NS_IMETHOD GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult) = 0;

  


  nsCompatibility GetCompatibilityMode() const {
    return mCompatMode;
  }
  
  



  PRBool HaveFiredDOMTitleChange() const {
    return mHaveFiredTitleChange;
  }

  


  virtual nsresult GetXBLChildNodesFor(nsIContent* aContent,
                                       nsIDOMNodeList** aResult) = 0;

  


  virtual nsresult GetContentListFor(nsIContent* aContent,
                                     nsIDOMNodeList** aResult) = 0;

  





  virtual nsresult ElementFromPointHelper(PRInt32 aX, PRInt32 aY,
                                          PRBool aIgnoreRootScrollFrame,
                                          PRBool aFlushLayout,
                                          nsIDOMElement** aReturn) = 0;

  


  virtual void FlushSkinBindings() = 0;

  






  void MayDispatchMutationEvent(nsINode* aTarget)
  {
    if (mSubtreeModifiedDepth > 0) {
      mSubtreeModifiedTargets.AppendObject(aTarget);
    }
  }

  



  void MarkUncollectableForCCGeneration(PRUint32 aGeneration)
  {
    mMarkedCCGeneration = aGeneration;
  }

  


  PRUint32 GetMarkedCCGeneration()
  {
    return mMarkedCCGeneration;
  }

  PRBool IsLoadedAsData()
  {
    return mLoadedAsData;
  }

  PRBool MayStartLayout()
  {
    return mMayStartLayout;
  }

  void SetMayStartLayout(PRBool aMayStartLayout)
  {
    mMayStartLayout = aMayStartLayout;
  }

  
  
  virtual already_AddRefed<nsIParser> GetFragmentParser() {
    return nsnull;
  }

  virtual void SetFragmentParser(nsIParser* aParser) {
    
  }

  
  virtual nsresult InitializeFrameLoader(nsFrameLoader* aLoader) = 0;
  
  
  virtual nsresult FinalizeFrameLoader(nsFrameLoader* aLoader) = 0;
  
  virtual void TryCancelFrameLoaderInitialization(nsIDocShell* aShell) = 0;
  
  virtual PRBool FrameLoaderScheduledToBeFinalized(nsIDocShell* aShell) = 0;

  



  PRBool IsRootDisplayDocument() const
  {
    return !mParentDocument && !mDisplayDocument;
  }

  





  nsIDocument* GetDisplayDocument() const
  {
    return mDisplayDocument;
  }
  
  



  void SetDisplayDocument(nsIDocument* aDisplayDocument)
  {
    NS_PRECONDITION(!GetPrimaryShell() &&
                    !nsCOMPtr<nsISupports>(GetContainer()) &&
                    !GetWindow() &&
                    !GetScriptGlobalObject(),
                    "Shouldn't set mDisplayDocument on documents that already "
                    "have a presentation or a docshell or a window");
    NS_PRECONDITION(aDisplayDocument != this, "Should be different document");
    NS_PRECONDITION(!aDisplayDocument->GetDisplayDocument(),
                    "Display documents should not nest");
    mDisplayDocument = aDisplayDocument;
  }

  









  class ExternalResourceLoad : public nsISupports
  {
  public:
    virtual ~ExternalResourceLoad() {}

    void AddObserver(nsIObserver* aObserver) {
      NS_PRECONDITION(aObserver, "Must have observer");
      mObservers.AppendElement(aObserver);
    }

    const nsTArray< nsCOMPtr<nsIObserver> > & Observers() {
      return mObservers;
    }
  protected:
    nsAutoTArray< nsCOMPtr<nsIObserver>, 8 > mObservers;    
  };

  












  virtual nsIDocument*
    RequestExternalResource(nsIURI* aURI,
                            nsINode* aRequestingNode,
                            ExternalResourceLoad** aPendingLoad) = 0;

  




  virtual void EnumerateExternalResources(nsSubDocEnumFunc aCallback,
                                          void* aData) = 0;

  




  PRBool IsShowing() { return mIsShowing; }
  



  PRBool IsVisible() { return mVisible; }
  



  PRBool IsActive() { return mDocumentContainer && !mRemovedFromDocShell; }

  void RegisterFreezableElement(nsIContent* aContent);
  PRBool UnregisterFreezableElement(nsIContent* aContent);
  typedef void (* FreezableElementEnumerator)(nsIContent*, void*);
  void EnumerateFreezableElements(FreezableElementEnumerator aEnumerator,
                                  void* aData);

#ifdef MOZ_SMIL
  
  virtual nsSMILAnimationController* GetAnimationController() = 0;
#endif 

  



  virtual void SuppressEventHandling(PRUint32 aIncrease = 1) = 0;

  




  virtual void UnsuppressEventHandlingAndFireEvents(PRBool aFireEvents) = 0;

  PRUint32 EventHandlingSuppressed() const { return mEventsSuppressed; }

  PRBool IsDNSPrefetchAllowed() const { return mAllowDNSPrefetch; }

  



  PRBool IsStaticDocument() { return mIsStaticDocument; }

  



  virtual already_AddRefed<nsIDocument>
  CreateStaticClone(nsISupports* aCloneContainer);

  



  nsIDocument* GetOriginalDocument() { return mOriginalDocument; }

  




  virtual void MaybePreLoadImage(nsIURI* uri) = 0;

  






  virtual PRBool IsDocumentRightToLeft() { return PR_FALSE; }

  enum DocumentTheme {
    Doc_Theme_Uninitialized, 
    Doc_Theme_None,
    Doc_Theme_Neutral,
    Doc_Theme_Dark,
    Doc_Theme_Bright
  };

  








  nsAString& GetPendingStateObject()
  {
    return mPendingStateObject;
  }

  


  void SetPendingStateObject(nsAString &obj)
  {
    mPendingStateObject.Assign(obj);
  }

  





  virtual int GetDocumentLWTheme() { return Doc_Theme_None; }

  




  virtual nsIContent* GetFirstBaseNodeWithHref() = 0;

  








  virtual nsresult SetFirstBaseNodeWithHref(nsIContent *node) = 0;

  virtual nsISupports* GetCurrentContentSink() = 0;

protected:
  ~nsIDocument()
  {
    
    
    
    
  }

  




  virtual void WillDispatchMutationEvent(nsINode* aTarget) = 0;
  virtual void MutationEventDispatched(nsINode* aTarget) = 0;
  friend class mozAutoSubtreeModified;

  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mDocumentBaseURI;

  nsWeakPtr mDocumentLoadGroup;

  nsWeakPtr mDocumentContainer;

  nsCString mCharacterSet;
  PRInt32 mCharacterSetSource;

  
  nsIDocument* mParentDocument;

  
  
  
  nsCOMPtr<nsINode> mCachedRootContent;

  
  
  
  nsNodeInfoManager* mNodeInfoManager; 
  nsICSSLoader* mCSSLoader; 

  
  
  
  
  nsAutoPtr<nsTHashtable<nsPtrHashKey<nsIContent> > > mFreezableElements;

  
  nsPropertyTable mPropertyTable;

  
  nsCompatibility mCompatMode;

  
  PRPackedBool mBidiEnabled;
  
  PRPackedBool mMathMLEnabled;

  
  
  
  
  PRPackedBool mIsInitialDocumentInWindow;

  PRPackedBool mShellIsHidden;

  PRPackedBool mIsRegularHTML;

  
  
  PRPackedBool mLoadedAsData;

  
  
  PRPackedBool mMayStartLayout;
  
  
  PRPackedBool mHaveFiredTitleChange;

  
  PRPackedBool mIsShowing;

  
  
  PRPackedBool mVisible;

  
  
  
  PRPackedBool mRemovedFromDocShell;

  
  
  PRPackedBool mAllowDNSPrefetch;
  
  
  PRPackedBool mIsStaticDocument;

  
  PRPackedBool mCreatingStaticClone;

  
  
  nsCOMPtr<nsIDocument> mOriginalDocument;

  
  
  PRUint32 mBidiOptions;

  nsCString mContentLanguage;
  nsCString mContentType;

  
  nsCOMPtr<nsISupports> mSecurityInfo;

  
  
  PRUint32 mPartID;
  
  
  
  PRUint32 mMarkedCCGeneration;

  nsIPresShell* mPresShell;

  nsCOMArray<nsINode> mSubtreeModifiedTargets;
  PRUint32            mSubtreeModifiedDepth;

  
  
  
  nsCOMPtr<nsIDocument> mDisplayDocument;

  PRUint32 mEventsSuppressed;

  nsString mPendingStateObject;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocument, NS_IDOCUMENT_IID)






class NS_STACK_CLASS mozAutoSubtreeModified
{
public:
  





  mozAutoSubtreeModified(nsIDocument* aSubtreeOwner, nsINode* aTarget)
  {
    UpdateTarget(aSubtreeOwner, aTarget);
  }

  ~mozAutoSubtreeModified()
  {
    UpdateTarget(nsnull, nsnull);
  }

  void UpdateTarget(nsIDocument* aSubtreeOwner, nsINode* aTarget)
  {
    if (mSubtreeOwner) {
      mSubtreeOwner->MutationEventDispatched(mTarget);
    }

    mTarget = aTarget;
    mSubtreeOwner = aSubtreeOwner;
    if (mSubtreeOwner) {
      mSubtreeOwner->WillDispatchMutationEvent(mTarget);
    }
  }

private:
  nsCOMPtr<nsINode>     mTarget;
  nsCOMPtr<nsIDocument> mSubtreeOwner;
};


nsresult
NS_NewHTMLDocument(nsIDocument** aInstancePtrResult);

nsresult
NS_NewXMLDocument(nsIDocument** aInstancePtrResult);

#ifdef MOZ_SVG
nsresult
NS_NewSVGDocument(nsIDocument** aInstancePtrResult);
#endif

nsresult
NS_NewImageDocument(nsIDocument** aInstancePtrResult);

#ifdef MOZ_MEDIA
nsresult
NS_NewVideoDocument(nsIDocument** aInstancePtrResult);
#endif

nsresult
NS_NewDocumentFragment(nsIDOMDocumentFragment** aInstancePtrResult,
                       nsNodeInfoManager *aNodeInfoManager);




nsresult
NS_NewDOMDocument(nsIDOMDocument** aInstancePtrResult,
                  const nsAString& aNamespaceURI, 
                  const nsAString& aQualifiedName, 
                  nsIDOMDocumentType* aDoctype,
                  nsIURI* aDocumentURI,
                  nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal,
                  PRBool aLoadedAsData);
nsresult
NS_NewPluginDocument(nsIDocument** aInstancePtrResult);

inline nsIDocument*
nsINode::GetOwnerDocument() const
{
  nsIDocument* ownerDoc = GetOwnerDoc();

  return ownerDoc != this ? ownerDoc : nsnull;
}

#endif 
