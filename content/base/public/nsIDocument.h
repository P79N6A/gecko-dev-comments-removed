




































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
#include "nsIVariant.h"
#include "nsIObserver.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsPIDOMWindow.h"
#ifdef MOZ_SMIL
#include "nsSMILAnimationController.h"
#endif 
#include "nsIScriptGlobalObject.h"
#include "nsIDocumentEncoder.h"
#include "nsIAnimationFrameListener.h"
#include "nsEventStates.h"
#include "nsIStructuredCloneContainer.h"
#include "nsIBFCacheEntry.h"
#include "nsDOMMemoryReporter.h"

class nsIContent;
class nsPresContext;
class nsIPresShell;
class nsIDocShell;
class nsStyleSet;
class nsIStyleSheet;
class nsIStyleRule;
class nsCSSStyleSheet;
class nsIViewManager;
class nsIDOMEvent;
class nsIDOMEventTarget;
class nsDeviceContext;
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
class imgIRequest;
class nsISHEntry;
class nsDOMNavigationTiming;

namespace mozilla {
namespace css {
class Loader;
} 

namespace dom {
class Link;
class Element;
} 
} 

#define NS_IDOCUMENT_IID \
{ 0xc3e40e8e, 0x8b91, 0x424c, \
  { 0xbe, 0x9c, 0x9c, 0xc1, 0x76, 0xa7, 0xf7, 0x24 } }


#define NS_STYLESHEET_FROM_CATALOG                (1 << 0)




#define NS_DOCUMENT_STATE_RTL_LOCALE              NS_DEFINE_EVENT_STATE_MACRO(0)

#define NS_DOCUMENT_STATE_WINDOW_INACTIVE         NS_DEFINE_EVENT_STATE_MACRO(1)





class nsIDocument : public nsINode
{
public:
  typedef mozilla::dom::Element Element;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_IID)
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
  NS_DECL_DOM_MEMORY_REPORTER_SIZEOF

#ifdef MOZILLA_INTERNAL_API
  nsIDocument()
    : nsINode(nsnull),
      mCharacterSet(NS_LITERAL_CSTRING("ISO-8859-1")),
      mNodeInfoManager(nsnull),
      mCompatMode(eCompatibility_FullStandards),
      mIsInitialDocumentInWindow(false),
      mMayStartLayout(true),
      mVisible(true),
      mRemovedFromDocShell(false),
      
      
      
      
      mAllowDNSPrefetch(true),
      mIsBeingUsedAsImage(false),
      mPartID(0)
  {
    SetInDocument();
  }
#endif
  
  



























  
  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     bool aReset,
                                     nsIContentSink* aSink = nsnull) = 0;
  virtual void StopDocumentLoad() = 0;

  





  virtual void NotifyPossibleTitleChange(bool aBoundTitleElement) = 0;

  






  nsIURI* GetDocumentURI() const
  {
    return mDocumentURI;
  }

  







  nsIURI* GetOriginalURI() const
  {
    return mOriginalURI;
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

  




  nsIURI* GetDocBaseURI() const
  {
    return mDocumentBaseURI ? mDocumentBaseURI : mDocumentURI;
  }
  virtual already_AddRefed<nsIURI> GetBaseURI() const
  {
    nsCOMPtr<nsIURI> uri = GetDocBaseURI();

    return uri.forget();
  }

  virtual nsresult SetBaseURI(nsIURI* aURI) = 0;

  


  virtual void GetBaseTarget(nsAString &aBaseTarget) = 0;
  void SetBaseTarget(const nsString& aBaseTarget) {
    mBaseTarget = aBaseTarget;
  }

  


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

  







  typedef bool (* IDTargetObserver)(Element* aOldElement,
                                      Element* aNewelement, void* aData);

  










  virtual Element* AddIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                       void* aData, bool aForImage) = 0;
  



  virtual void RemoveIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                      void* aData, bool aForImage) = 0;

  




  NS_IMETHOD GetContentType(nsAString& aContentType) = 0;

  


  virtual void SetContentType(const nsAString& aContentType) = 0;

  


  void GetContentLanguage(nsAString& aContentLanguage) const
  {
    CopyASCIItoUTF16(mContentLanguage, aContentLanguage);
  }

  
  

  



  bool GetBidiEnabled() const
  {
    return mBidiEnabled;
  }

  





  void SetBidiEnabled()
  {
    mBidiEnabled = true;
  }
  
  


  bool GetMathMLEnabled() const
  {
    return mMathMLEnabled;
  }
  
  void SetMathMLEnabled()
  {
    mMathMLEnabled = true;
  }

  


  bool IsInitialDocument() const
  {
    return mIsInitialDocumentInWindow;
  }
  
  



  void SetIsInitialDocument(bool aIsInitialDocument)
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
  virtual void DeleteShell() = 0;

  nsIPresShell* GetShell() const
  {
    return GetBFCacheEntry() ? nsnull : mPresShell;
  }

  void SetBFCacheEntry(nsIBFCacheEntry* aEntry)
  {
    mBFCacheEntry = aEntry;
  }

  nsIBFCacheEntry* GetBFCacheEntry() const
  {
    return mBFCacheEntry;
  }

  




  nsIDocument *GetParentDocument() const
  {
    return mParentDocument;
  }

  


  void SetParentDocument(nsIDocument* aParent)
  {
    mParentDocument = aParent;
  }

  


  virtual nsresult SetSubDocumentFor(Element* aContent,
                                     nsIDocument* aSubDoc) = 0;

  


  virtual nsIDocument *GetSubDocumentFor(nsIContent *aContent) const = 0;

  


  virtual Element* FindContentForSubDocument(nsIDocument* aDocument) const = 0;

  


  Element *GetRootElement() const;

protected:
  virtual Element *GetRootElementInternal() const = 0;

public:
  
  
  Element* GetHtmlElement();
  
  
  Element* GetHtmlChildElement(nsIAtom* aTag);
  
  
  Element* GetBodyElement() {
    return GetHtmlChildElement(nsGkAtoms::body);
  }
  
  
  Element* GetHeadElement() {
    return GetHtmlChildElement(nsGkAtoms::head);
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
                                            bool aApplicable) = 0;  

  



  virtual PRInt32 GetNumberOfCatalogStyleSheets() const = 0;
  virtual nsIStyleSheet* GetCatalogStyleSheetAt(PRInt32 aIndex) const = 0;
  virtual void AddCatalogStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual void EnsureCatalogStyleSheet(const char *aStyleSheetURI) = 0;

  


  mozilla::css::Loader* CSSLoader() const {
    return mCSSLoader;
  }

  




  virtual nsIChannel* GetChannel() const = 0;

  



  virtual nsHTMLStyleSheet* GetAttributeStyleSheet() const = 0;

  



  virtual nsHTMLCSSStyleSheet* GetInlineStyleSheet() const = 0;

  





  virtual nsIScriptGlobalObject* GetScriptGlobalObject() const = 0;
  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject) = 0;

  








  nsIScriptGlobalObject*
    GetScriptHandlingObject(bool& aHasHadScriptHandlingObject) const
  {
    aHasHadScriptHandlingObject = mHasHadScriptHandlingObject;
    return mScriptGlobalObject ? mScriptGlobalObject.get() :
                                 GetScriptHandlingObjectInternal();
  }
  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject) = 0;

  






  virtual nsIScriptGlobalObject* GetScopeObject() = 0;

  


  nsPIDOMWindow *GetWindow() const
  {
    return mWindow ? mWindow->GetOuterWindow() : GetWindowInternal();
  }

  




  nsPIDOMWindow* GetInnerWindow()
  {
    return mRemovedFromDocShell ? GetInnerWindowInternal() : mWindow;
  }

  


  PRUint64 OuterWindowID() const
  {
    nsPIDOMWindow *window = GetWindow();
    return window ? window->WindowID() : 0;
  }

  


  PRUint64 InnerWindowID()
  {
    nsPIDOMWindow *window = GetInnerWindow();
    return window ? window->WindowID() : 0;
  }

  

 
  virtual nsScriptLoader* ScriptLoader() = 0;

  


  virtual void AddToIdTable(Element* aElement, nsIAtom* aId) = 0;
  virtual void RemoveFromIdTable(Element* aElement, nsIAtom* aId) = 0;
  virtual void AddToNameTable(Element* aElement, nsIAtom* aName) = 0;
  virtual void RemoveFromNameTable(Element* aElement, nsIAtom* aName) = 0;

  





  virtual Element* GetFullScreenElement() = 0;

  



  virtual void RequestFullScreen(Element* aElement) = 0;

  



  virtual void CancelFullScreen() = 0;

  


  virtual bool IsFullScreenDoc() = 0;

  

  

  





  virtual void AddObserver(nsIDocumentObserver* aObserver) = 0;

  



  virtual bool RemoveObserver(nsIDocumentObserver* aObserver) = 0;

  
  
  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType) = 0;
  virtual void EndUpdate(nsUpdateType aUpdateType) = 0;
  virtual void BeginLoad() = 0;
  virtual void EndLoad() = 0;

  enum ReadyState { READYSTATE_UNINITIALIZED = 0, READYSTATE_LOADING = 1, READYSTATE_INTERACTIVE = 3, READYSTATE_COMPLETE = 4};
  virtual void SetReadyStateInternal(ReadyState rs) = 0;
  virtual ReadyState GetReadyStateEnum() = 0;

  
  
  virtual void ContentStateChanged(nsIContent* aContent,
                                   nsEventStates aStateMask) = 0;

  
  
  
  virtual void DocumentStatesChanged(nsEventStates aStateMask) = 0;

  
  
  virtual void StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule) = 0;
  virtual void StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) = 0;
  virtual void StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule) = 0;

  



  virtual void FlushPendingNotifications(mozFlushType aType) = 0;

  





  virtual void FlushExternalResources(mozFlushType aType) = 0;

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

  





  virtual void SetXMLDeclaration(const PRUnichar *aVersion,
                                 const PRUnichar *aEncoding,
                                 const PRInt32 aStandalone) = 0;
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone) = 0;

  bool IsHTML() const
  {
    return mIsRegularHTML;
  }
  bool IsXUL() const
  {
    return mIsXUL;
  }

  virtual bool IsScriptEnabled() = 0;

  virtual void AddXMLEventsContent(nsIContent * aXMLEventsElement) = 0;

  


  virtual nsresult CreateElem(const nsAString& aName, nsIAtom *aPrefix,
                              PRInt32 aNamespaceID,
                              nsIContent** aResult) = 0;

  






  nsISupports *GetSecurityInfo()
  {
    return mSecurityInfo;
  }

  



  PRInt32 GetDefaultNamespaceID() const
  {
    return mDefaultElementType;
  }

  void DeleteAllProperties();
  void DeleteAllPropertiesFor(nsINode* aNode);

  nsPropertyTable* PropertyTable(PRUint16 aCategory) {
    if (aCategory == 0)
      return &mPropertyTable;
    return GetExtraPropertyTable(aCategory);
  }
  PRUint32 GetPropertyTableCount()
  { return mExtraPropertyTables.Length() + 1; }

  


  void SetPartID(PRUint32 aID) {
    mPartID = aID;
  }

  


  PRUint32 GetPartID() const {
    return mPartID;
  }

  



  virtual nsresult Sanitize() = 0;

  




  typedef bool (*nsSubDocEnumFunc)(nsIDocument *aDocument, void *aData);
  virtual void EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                     void *aData) = 0;

  
















  virtual bool CanSavePresentation(nsIRequest *aNewRequest) = 0;

  




  virtual void Destroy() = 0;

  





  virtual void RemovedFromDocShell() = 0;
  
  




  virtual already_AddRefed<nsILayoutHistoryState> GetLayoutHistoryState() const = 0;

  






  virtual void BlockOnload() = 0;
  





  virtual void UnblockOnload(bool aFireSync) = 0;

  











  virtual void OnPageShow(bool aPersisted,
                          nsIDOMEventTarget* aDispatchStartTarget) = 0;

  











  virtual void OnPageHide(bool aPersisted,
                          nsIDOMEventTarget* aDispatchStartTarget) = 0;
  
  



  


  virtual void AddStyleRelevantLink(mozilla::dom::Link* aLink) = 0;
  





  virtual void ForgetLink(mozilla::dom::Link* aLink) = 0;

  




  virtual void ClearBoxObjectFor(nsIContent *aContent) = 0;

  



  NS_IMETHOD GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult) = 0;

  


  nsCompatibility GetCompatibilityMode() const {
    return mCompatMode;
  }
  
  



  bool HaveFiredDOMTitleChange() const {
    return mHaveFiredTitleChange;
  }

  


  virtual nsresult GetXBLChildNodesFor(nsIContent* aContent,
                                       nsIDOMNodeList** aResult) = 0;

  


  virtual nsresult GetContentListFor(nsIContent* aContent,
                                     nsIDOMNodeList** aResult) = 0;

  





  virtual nsresult ElementFromPointHelper(float aX, float aY,
                                          bool aIgnoreRootScrollFrame,
                                          bool aFlushLayout,
                                          nsIDOMElement** aReturn) = 0;

  virtual nsresult NodesFromRectHelper(float aX, float aY,
                                       float aTopSize, float aRightSize,
                                       float aBottomSize, float aLeftSize,
                                       bool aIgnoreRootScrollFrame,
                                       bool aFlushLayout,
                                       nsIDOMNodeList** aReturn) = 0;

  


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

  bool IsLoadedAsData()
  {
    return mLoadedAsData;
  }

  bool MayStartLayout()
  {
    return mMayStartLayout;
  }

  void SetMayStartLayout(bool aMayStartLayout)
  {
    mMayStartLayout = aMayStartLayout;
  }

  already_AddRefed<nsIDocumentEncoder> GetCachedEncoder()
  {
    return mCachedEncoder.forget();
  }

  void SetCachedEncoder(already_AddRefed<nsIDocumentEncoder> aEncoder)
  {
    mCachedEncoder = aEncoder;
  }

  
  virtual nsresult InitializeFrameLoader(nsFrameLoader* aLoader) = 0;
  
  
  virtual nsresult FinalizeFrameLoader(nsFrameLoader* aLoader) = 0;
  
  virtual void TryCancelFrameLoaderInitialization(nsIDocShell* aShell) = 0;
  
  virtual bool FrameLoaderScheduledToBeFinalized(nsIDocShell* aShell) = 0;

  



  bool IsRootDisplayDocument() const
  {
    return !mParentDocument && !mDisplayDocument;
  }

  bool IsBeingUsedAsImage() const {
    return mIsBeingUsedAsImage;
  }

  void SetIsBeingUsedAsImage() {
    mIsBeingUsedAsImage = true;
  }

  bool IsResourceDoc() const {
    return IsBeingUsedAsImage() || 
      !!mDisplayDocument;          
  }

  





  nsIDocument* GetDisplayDocument() const
  {
    return mDisplayDocument;
  }
  
  



  void SetDisplayDocument(nsIDocument* aDisplayDocument)
  {
    NS_PRECONDITION(!GetShell() &&
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

  




  bool IsShowing() const { return mIsShowing; }
  



  bool IsVisible() const { return mVisible; }
  



  bool IsActive() const { return mDocumentContainer && !mRemovedFromDocShell; }

  void RegisterFreezableElement(nsIContent* aContent);
  bool UnregisterFreezableElement(nsIContent* aContent);
  typedef void (* FreezableElementEnumerator)(nsIContent*, void*);
  void EnumerateFreezableElements(FreezableElementEnumerator aEnumerator,
                                  void* aData);

#ifdef MOZ_SMIL
  
  
  
  bool HasAnimationController()  { return !!mAnimationController; }

  
  
  
  virtual nsSMILAnimationController* GetAnimationController() = 0;
#endif 

  
  
  
  virtual void SetImagesNeedAnimating(bool aAnimating) = 0;

  



  virtual void SuppressEventHandling(PRUint32 aIncrease = 1) = 0;

  




  virtual void UnsuppressEventHandlingAndFireEvents(bool aFireEvents) = 0;

  PRUint32 EventHandlingSuppressed() const { return mEventsSuppressed; }

  bool IsEventHandlingEnabled() {
    return !EventHandlingSuppressed() && mScriptGlobalObject;
  }

  


  void BeginEvaluatingExternalScript() { ++mExternalScriptsBeingEvaluated; }

  


  void EndEvaluatingExternalScript() { --mExternalScriptsBeingEvaluated; }

  bool IsDNSPrefetchAllowed() const { return mAllowDNSPrefetch; }

  



  bool AllowXULXBL() {
    return mAllowXULXBL == eTriTrue ? true :
           mAllowXULXBL == eTriFalse ? false :
           InternalAllowXULXBL();
  }

  void ForceEnableXULXBL() {
    mAllowXULXBL = eTriTrue;
  }

  



  bool IsStaticDocument() { return mIsStaticDocument; }

  



  virtual already_AddRefed<nsIDocument>
  CreateStaticClone(nsISupports* aCloneContainer);

  



  nsIDocument* GetOriginalDocument() { return mOriginalDocument; }

  




  virtual void MaybePreLoadImage(nsIURI* uri,
                                 const nsAString& aCrossOriginAttr) = 0;

  



  virtual void PreloadStyle(nsIURI* aURI, const nsAString& aCharset) = 0;

  







  virtual nsresult LoadChromeSheetSync(nsIURI* aURI, bool aIsAgentSheet,
                                       nsCSSStyleSheet** aSheet) = 0;

  






  virtual bool IsDocumentRightToLeft() { return false; }

  enum DocumentTheme {
    Doc_Theme_Uninitialized, 
    Doc_Theme_None,
    Doc_Theme_Neutral,
    Doc_Theme_Dark,
    Doc_Theme_Bright
  };

  



  void SetStateObject(nsIStructuredCloneContainer *scContainer)
  {
    mStateObjectContainer = scContainer;
    mStateObjectCached = nsnull;
  }

  





  virtual int GetDocumentLWTheme() { return Doc_Theme_None; }

  




  virtual nsEventStates GetDocumentState() = 0;

  virtual nsISupports* GetCurrentContentSink() = 0;

  





  virtual void RegisterFileDataUri(const nsACString& aUri) = 0;
  virtual void UnregisterFileDataUri(const nsACString& aUri) = 0;

  virtual void SetScrollToRef(nsIURI *aDocumentURI) = 0;
  virtual void ScrollToRef() = 0;
  virtual void ResetScrolledToRefAlready() = 0;
  virtual void SetChangeScrollPosWhenScrollingToRef(bool aValue) = 0;

  





  virtual Element* GetElementById(const nsAString& aElementId) = 0;

  




  virtual const nsSmallVoidArray* GetAllElementsForId(const nsAString& aElementId) const = 0;

  






  virtual Element* LookupImageElement(const nsAString& aElementId) = 0;

  void ScheduleBeforePaintEvent(nsIAnimationFrameListener* aListener);
  void BeforePaintEventFiring()
  {
    mHavePendingPaint = false;
  }

  typedef nsTArray< nsCOMPtr<nsIAnimationFrameListener> > AnimationListenerList;
  



  void TakeAnimationFrameListeners(AnimationListenerList& aListeners);

  
  bool InUnlinkOrDeletion() { return mInUnlinkOrDeletion; }

  












  
  virtual nsresult AddImage(imgIRequest* aImage) = 0;
  virtual nsresult RemoveImage(imgIRequest* aImage) = 0;

  
  
  virtual nsresult SetImageLockingState(bool aLocked) = 0;

  virtual nsresult GetStateObject(nsIVariant** aResult) = 0;

  virtual nsDOMNavigationTiming* GetNavigationTiming() const = 0;

  virtual nsresult SetNavigationTiming(nsDOMNavigationTiming* aTiming) = 0;

  virtual Element* FindImageMap(const nsAString& aNormalizedMapName) = 0;

#define DEPRECATED_OPERATION(_op) e##_op,
  enum DeprecatedOperations {
#include "nsDeprecatedOperationList.h"
    eDeprecatedOperationCount
  };
#undef DEPRECATED_OPERATION
  void WarnOnceAbout(DeprecatedOperations aOperation);

  virtual void PostVisibilityUpdateEvent() = 0;

private:
  PRUint64 mWarnedAbout;

protected:
  ~nsIDocument()
  {
    
    
    
    
  }

  nsPropertyTable* GetExtraPropertyTable(PRUint16 aCategory);

  
  virtual nsPIDOMWindow *GetWindowInternal() const = 0;

  
  virtual nsPIDOMWindow *GetInnerWindowInternal() = 0;

  
  virtual nsIScriptGlobalObject* GetScriptHandlingObjectInternal() const = 0;

  
  virtual bool InternalAllowXULXBL() = 0;

  




  virtual void WillDispatchMutationEvent(nsINode* aTarget) = 0;
  virtual void MutationEventDispatched(nsINode* aTarget) = 0;
  friend class mozAutoSubtreeModified;

  virtual Element* GetNameSpaceElement()
  {
    return GetRootElement();
  }

  void SetContentTypeInternal(const nsACString& aType)
  {
    mCachedEncoder = nsnull;
    mContentType = aType;
  }

  nsCString GetContentTypeInternal() const
  {
    return mContentType;
  }

  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mOriginalURI;
  nsCOMPtr<nsIURI> mDocumentBaseURI;

  nsWeakPtr mDocumentLoadGroup;

  nsWeakPtr mDocumentContainer;

  nsCString mCharacterSet;
  PRInt32 mCharacterSetSource;

  
  nsIDocument* mParentDocument;

  
  mozilla::dom::Element* mCachedRootElement;

  
  
  
  nsNodeInfoManager* mNodeInfoManager; 
  mozilla::css::Loader* mCSSLoader; 

  
  
  
  
  nsAutoPtr<nsTHashtable<nsPtrHashKey<nsIContent> > > mFreezableElements;

#ifdef MOZ_SMIL
  
  nsRefPtr<nsSMILAnimationController> mAnimationController;
#endif 

  
  nsPropertyTable mPropertyTable;
  nsTArray<nsAutoPtr<nsPropertyTable> > mExtraPropertyTables;

  
  nsCompatibility mCompatMode;

  
  bool mBidiEnabled;
  
  bool mMathMLEnabled;

  
  
  
  
  bool mIsInitialDocumentInWindow;

  bool mIsRegularHTML;
  bool mIsXUL;

  enum {
    eTriUnset = 0,
    eTriFalse,
    eTriTrue
  } mAllowXULXBL;

  
  
  bool mLoadedAsData;

  
  
  bool mMayStartLayout;
  
  
  bool mHaveFiredTitleChange;

  
  bool mIsShowing;

  
  
  bool mVisible;

  
  
  
  bool mRemovedFromDocShell;

  
  
  bool mAllowDNSPrefetch;
  
  
  bool mIsStaticDocument;

  
  bool mCreatingStaticClone;

  
  bool mInUnlinkOrDeletion;

  
  bool mHasHadScriptHandlingObject;

  
  bool mHavePendingPaint;

  
  bool mIsBeingUsedAsImage;

  
  
  bool mIsSyntheticDocument;

  
  
  
  nsCOMPtr<nsIScriptGlobalObject> mScriptGlobalObject;

  
  
  nsCOMPtr<nsIDocument> mOriginalDocument;

  
  
  PRUint32 mBidiOptions;

  nsCString mContentLanguage;
private:
  nsCString mContentType;
protected:

  
  nsCOMPtr<nsISupports> mSecurityInfo;

  
  
  PRUint32 mPartID;
  
  
  
  PRUint32 mMarkedCCGeneration;

  nsIPresShell* mPresShell;

  nsCOMArray<nsINode> mSubtreeModifiedTargets;
  PRUint32            mSubtreeModifiedDepth;

  
  
  
  nsCOMPtr<nsIDocument> mDisplayDocument;

  PRUint32 mEventsSuppressed;

  



  PRUint32 mExternalScriptsBeingEvaluated;

  
  
  nsPIDOMWindow *mWindow;

  nsCOMPtr<nsIDocumentEncoder> mCachedEncoder;

  AnimationListenerList mAnimationFrameListeners;

  
  
  nsIBFCacheEntry *mBFCacheEntry;

  
  nsString mBaseTarget;

  nsCOMPtr<nsIStructuredCloneContainer> mStateObjectContainer;
  nsCOMPtr<nsIVariant> mStateObjectCached;

  PRUint8 mDefaultElementType;
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

nsresult
NS_NewSVGDocument(nsIDocument** aInstancePtrResult);

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
                  bool aLoadedAsData,
                  nsIScriptGlobalObject* aEventObject,
                  bool aSVGDocument);
nsresult
NS_NewPluginDocument(nsIDocument** aInstancePtrResult);

inline nsIDocument*
nsINode::GetOwnerDocument() const
{
  nsIDocument* ownerDoc = OwnerDoc();

  return ownerDoc != this ? ownerDoc : nsnull;
}

#endif 
