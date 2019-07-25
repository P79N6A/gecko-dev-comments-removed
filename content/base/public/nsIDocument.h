



#ifndef nsIDocument_h___
#define nsIDocument_h___

#include "mozFlushType.h"                
#include "nsAutoPtr.h"                   
#include "nsCOMArray.h"                  
#include "nsCRT.h"                       
#include "nsCompatibility.h"             
#include "nsCOMPtr.h"                    
#include "nsGkAtoms.h"                   
#include "nsIDocumentEncoder.h"          
#include "nsIDocumentObserver.h"         
#include "nsIFrameRequestCallback.h"     
#include "nsILoadContext.h"              
#include "nsILoadGroup.h"                
#include "nsINode.h"                     
#include "nsIScriptGlobalObject.h"       
#include "nsIStructuredCloneContainer.h" 
#include "nsPIDOMWindow.h"               
#include "nsPropertyTable.h"             
#include "nsTHashtable.h"                
#include "mozilla/dom/DirectionalityUtils.h"

class imgIRequest;
class nsAString;
class nsBindingManager;
class nsCSSStyleSheet;
class nsDOMNavigationTiming;
class nsEventStates;
class nsFrameLoader;
class nsHTMLCSSStyleSheet;
class nsHTMLStyleSheet;
class nsIAtom;
class nsIBFCacheEntry;
class nsIBoxObject;
class nsIChannel;
class nsIContent;
class nsIContentSink;
class nsIDocShell;
class nsIDocumentObserver;
class nsIDOMDocument;
class nsIDOMDocumentFragment;
class nsIDOMDocumentType;
class nsIDOMElement;
class nsIDOMEventTarget;
class nsIDOMNodeList;
class nsILayoutHistoryState;
class nsIObjectLoadingContent;
class nsIObserver;
class nsIPresShell;
class nsIPrincipal;
class nsIRequest;
class nsIStreamListener;
class nsIStyleRule;
class nsIStyleSheet;
class nsIURI;
class nsIVariant;
class nsIViewManager;
class nsPresContext;
class nsScriptLoader;
class nsSMILAnimationController;
class nsStyleSet;
class nsWindowSizes;

namespace mozilla {
namespace css {
class Loader;
class ImageLoader;
} 

namespace dom {
class Link;
class Element;
} 
} 

#define NS_IDOCUMENT_IID \
{ 0x57fe44ae, 0x6656, 0x44b8, \
  { 0x8d, 0xc0, 0xfc, 0xa7, 0x43, 0x28, 0xbe, 0x86 } }


#define NS_STYLESHEET_FROM_CATALOG                (1 << 0)


enum DocumentFlavor {
  DocumentFlavorLegacyGuess, 
  DocumentFlavorHTML, 
  DocumentFlavorSVG 
};




#define NS_DOCUMENT_STATE_RTL_LOCALE              NS_DEFINE_EVENT_STATE_MACRO(0)

#define NS_DOCUMENT_STATE_WINDOW_INACTIVE         NS_DEFINE_EVENT_STATE_MACRO(1)





class nsIDocument : public nsINode
{
public:
  typedef mozilla::dom::Element Element;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_IID)
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

#ifdef MOZILLA_INTERNAL_API
  nsIDocument();
#endif

  































  
  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     bool aReset,
                                     nsIContentSink* aSink = nullptr) = 0;
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
    nsILoadGroup *group = nullptr;
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

  int32_t GetDocumentCharacterSetSource() const
  {
    return mCharacterSetSource;
  }

  
  
  void SetDocumentCharacterSetSource(int32_t aCharsetSource)
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
  

  



  uint32_t GetBidiOptions() const
  {
    return mBidiOptions;
  }

  





  void SetBidiOptions(uint32_t aBidiOptions)
  {
    mBidiOptions = aBidiOptions;
  }


  



  uint32_t GetSandboxFlags() const
  {
    return mSandboxFlags;
  }

  



  void SetSandboxFlags(uint32_t sandboxFlags)
  {
    mSandboxFlags = sandboxFlags;
  }

  inline mozilla::directionality::Directionality GetDocumentDirectionality() {
    return mDirectionality;
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
    return GetBFCacheEntry() ? nullptr : mPresShell;
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
  
  


  virtual nsresult GetAllowPlugins (bool* aAllowPlugins) = 0;

  


  virtual nsresult SetSubDocumentFor(Element* aContent,
                                     nsIDocument* aSubDoc) = 0;

  


  virtual nsIDocument *GetSubDocumentFor(nsIContent *aContent) const = 0;

  


  virtual Element* FindContentForSubDocument(nsIDocument* aDocument) const = 0;

  


  nsIContent* GetDocumentType() const;

  


  Element* GetRootElement() const;

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
  
  




  





  virtual int32_t GetNumberOfStyleSheets() const = 0;
  
  





  virtual nsIStyleSheet* GetStyleSheetAt(int32_t aIndex) const = 0;
  
  






  virtual void InsertStyleSheetAt(nsIStyleSheet* aSheet, int32_t aIndex) = 0;

  





  virtual int32_t GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const = 0;

  







  virtual void UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                                 nsCOMArray<nsIStyleSheet>& aNewSheets) = 0;

  


  virtual void AddStyleSheet(nsIStyleSheet* aSheet) = 0;

  


  virtual void RemoveStyleSheet(nsIStyleSheet* aSheet) = 0;

  



  virtual void SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                            bool aApplicable) = 0;  

  



  virtual int32_t GetNumberOfCatalogStyleSheets() const = 0;
  virtual nsIStyleSheet* GetCatalogStyleSheetAt(int32_t aIndex) const = 0;
  virtual void AddCatalogStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual void EnsureCatalogStyleSheet(const char *aStyleSheetURI) = 0;

  


  mozilla::css::Loader* CSSLoader() const {
    return mCSSLoader;
  }

  


  mozilla::css::ImageLoader* StyleImageLoader() const {
    return mStyleImageLoader;
  }

  




  virtual nsIChannel* GetChannel() const = 0;

  



  nsHTMLStyleSheet* GetAttributeStyleSheet() const {
    return mAttrStyleSheet;
  }

  



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

  bool IsInBackgroundWindow() const
  {
    nsPIDOMWindow* outer = mWindow ? mWindow->GetOuterWindow() : nullptr;
    return outer && outer->IsBackground();
  }
  
  




  nsPIDOMWindow* GetInnerWindow()
  {
    return mRemovedFromDocShell ? GetInnerWindowInternal() : mWindow;
  }

  


  uint64_t OuterWindowID() const
  {
    nsPIDOMWindow *window = GetWindow();
    return window ? window->WindowID() : 0;
  }

  


  uint64_t InnerWindowID()
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

  





  virtual void AsyncRequestFullScreen(Element* aElement) = 0;

  






  virtual nsresult RemoteFrameFullscreenChanged(nsIDOMElement* aFrameElement,
                                                const nsAString& aNewOrigin) = 0;

  








   virtual nsresult RemoteFrameFullscreenReverted() = 0;

  




  virtual void RestorePreviousFullScreenState() = 0;

  


  virtual bool IsFullScreenDoc() = 0;

  





  virtual void SetApprovedForFullscreen(bool aIsApproved) = 0;

  




  static void ExitFullScreen(bool aRunAsync);


  virtual void RequestPointerLock(Element* aElement) = 0;

  static void UnlockPointer();


  

  

  





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
    nsISupports* container = nullptr;
    if (mDocumentContainer)
      CallQueryReferent(mDocumentContainer.get(), &container);

    return container;
  }

  


  nsILoadContext* GetLoadContext() const
  {
    nsCOMPtr<nsISupports> container = GetContainer();
    if (container) {
      nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(container);
      return loadContext;
    }
    return nullptr;
  }

  





  virtual void SetXMLDeclaration(const PRUnichar *aVersion,
                                 const PRUnichar *aEncoding,
                                 const int32_t aStandalone) = 0;
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
                              int32_t aNamespaceID,
                              nsIContent** aResult) = 0;

  






  nsISupports *GetSecurityInfo()
  {
    return mSecurityInfo;
  }

  



  int32_t GetDefaultNamespaceID() const
  {
    return mDefaultElementType;
  }

  void DeleteAllProperties();
  void DeleteAllPropertiesFor(nsINode* aNode);

  nsPropertyTable* PropertyTable(uint16_t aCategory) {
    if (aCategory == 0)
      return &mPropertyTable;
    return GetExtraPropertyTable(aCategory);
  }
  uint32_t GetPropertyTableCount()
  { return mExtraPropertyTables.Length() + 1; }

  


  void SetPartID(uint32_t aID) {
    mPartID = aID;
  }

  


  uint32_t GetPartID() const {
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

  


  virtual nsIContent*
    GetAnonymousElementByAttribute(nsIContent* aElement,
                                   nsIAtom* aAttrName,
                                   const nsAString& aAttrValue) const = 0;

  





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

  



  void MarkUncollectableForCCGeneration(uint32_t aGeneration)
  {
    mMarkedCCGeneration = aGeneration;
  }

  


  uint32_t GetMarkedCCGeneration()
  {
    return mMarkedCCGeneration;
  }

  bool IsLoadedAsData()
  {
    return mLoadedAsData;
  }

  bool IsLoadedAsInteractiveData()
  {
    return mLoadedAsInteractiveData;
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

  
  
  
  bool HasAnimationController()  { return !!mAnimationController; }

  
  
  
  virtual nsSMILAnimationController* GetAnimationController() = 0;

  
  
  
  virtual void SetImagesNeedAnimating(bool aAnimating) = 0;

  



  virtual void SuppressEventHandling(uint32_t aIncrease = 1) = 0;

  




  virtual void UnsuppressEventHandlingAndFireEvents(bool aFireEvents) = 0;

  uint32_t EventHandlingSuppressed() const { return mEventsSuppressed; }

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

  



  nsIDocument* GetOriginalDocument()
  {
    MOZ_ASSERT(!mOriginalDocument || !mOriginalDocument->GetOriginalDocument());
    return mOriginalDocument;
  }

  





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
    mStateObjectCached = nullptr;
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

  nsresult ScheduleFrameRequestCallback(nsIFrameRequestCallback* aCallback,
                                        int32_t *aHandle);
  void CancelFrameRequestCallback(int32_t aHandle);

  typedef nsTArray< nsCOMPtr<nsIFrameRequestCallback> > FrameRequestCallbackList;
  



  void TakeFrameRequestCallbacks(FrameRequestCallbackList& aCallbacks);

  
  bool InUnlinkOrDeletion() { return mInUnlinkOrDeletion; }

  












  
  virtual nsresult AddImage(imgIRequest* aImage) = 0;
  virtual nsresult RemoveImage(imgIRequest* aImage) = 0;

  
  
  virtual nsresult SetImageLockingState(bool aLocked) = 0;

  virtual nsresult AddPlugin(nsIObjectLoadingContent* aPlugin) = 0;
  virtual void RemovePlugin(nsIObjectLoadingContent* aPlugin) = 0;
  virtual void GetPlugins(nsTArray<nsIObjectLoadingContent*>& aPlugins) = 0;

  virtual nsresult GetStateObject(nsIVariant** aResult) = 0;

  virtual nsDOMNavigationTiming* GetNavigationTiming() const = 0;

  virtual nsresult SetNavigationTiming(nsDOMNavigationTiming* aTiming) = 0;

  virtual Element* FindImageMap(const nsAString& aNormalizedMapName) = 0;

  
  
  
  virtual void NotifyAudioAvailableListener() = 0;

  
  virtual bool HasAudioAvailableListeners() = 0;

  
  void RegisterPendingLinkUpdate(mozilla::dom::Link* aLink);
  
  
  
  void UnregisterPendingLinkUpdate(mozilla::dom::Link* aElement);

  
  
  void FlushPendingLinkUpdates();

#define DEPRECATED_OPERATION(_op) e##_op,
  enum DeprecatedOperations {
#include "nsDeprecatedOperationList.h"
    eDeprecatedOperationCount
  };
#undef DEPRECATED_OPERATION
  void WarnOnceAbout(DeprecatedOperations aOperation, bool asError = false);

  virtual void PostVisibilityUpdateEvent() = 0;
  
  bool IsSyntheticDocument() { return mIsSyntheticDocument; }

  void SetNeedLayoutFlush() {
    mNeedLayoutFlush = true;
    if (mDisplayDocument) {
      mDisplayDocument->SetNeedLayoutFlush();
    }
  }

  void SetNeedStyleFlush() {
    mNeedStyleFlush = true;
    if (mDisplayDocument) {
      mDisplayDocument->SetNeedStyleFlush();
    }
  }

  
  
  
  
  virtual void DocSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const;
  
  
  
  virtual void DocSizeOfIncludingThis(nsWindowSizes* aWindowSizes) const;

  bool MayHaveDOMMutationObservers()
  {
    return mMayHaveDOMMutationObservers;
  }

  void SetMayHaveDOMMutationObservers()
  {
    mMayHaveDOMMutationObservers = true;
  }

  bool IsInSyncOperation()
  {
    return mInSyncOperationCount != 0;
  }

  void SetIsInSyncOperation(bool aSync)
  {
    if (aSync) {
      ++mInSyncOperationCount;
    } else {
      --mInSyncOperationCount;
    }
  }

private:
  uint64_t mWarnedAbout;

protected:
  ~nsIDocument();
  nsPropertyTable* GetExtraPropertyTable(uint16_t aCategory);

  
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
    mCachedEncoder = nullptr;
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
  int32_t mCharacterSetSource;

  
  nsIDocument* mParentDocument;

  
  mozilla::dom::Element* mCachedRootElement;

  
  nsNodeInfoManager* mNodeInfoManager; 
  nsRefPtr<mozilla::css::Loader> mCSSLoader;
  nsRefPtr<mozilla::css::ImageLoader> mStyleImageLoader;
  nsRefPtr<nsHTMLStyleSheet> mAttrStyleSheet;

  
  
  
  
  nsAutoPtr<nsTHashtable<nsPtrHashKey<nsIContent> > > mFreezableElements;

  
  
  
  nsTHashtable<nsPtrHashKey<mozilla::dom::Link> > mLinksToUpdate;

  
  nsRefPtr<nsSMILAnimationController> mAnimationController;

  
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

  
  
  
  bool mLoadedAsInteractiveData;

  
  
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

  
  bool mIsBeingUsedAsImage;

  
  
  bool mIsSyntheticDocument;

  
  bool mHasLinksToUpdate;

  
  bool mNeedLayoutFlush;

  
  bool mNeedStyleFlush;

  
  bool mMayHaveDOMMutationObservers;

  
  
  
  nsCOMPtr<nsIScriptGlobalObject> mScriptGlobalObject;

  
  
  nsCOMPtr<nsIDocument> mOriginalDocument;

  
  
  uint32_t mBidiOptions;

  
  
  
  uint32_t mSandboxFlags;

  
  mozilla::directionality::Directionality mDirectionality;

  nsCString mContentLanguage;
private:
  nsCString mContentType;
protected:

  
  nsCOMPtr<nsISupports> mSecurityInfo;

  
  
  uint32_t mPartID;
  
  
  
  uint32_t mMarkedCCGeneration;

  nsIPresShell* mPresShell;

  nsCOMArray<nsINode> mSubtreeModifiedTargets;
  uint32_t            mSubtreeModifiedDepth;

  
  
  
  nsCOMPtr<nsIDocument> mDisplayDocument;

  uint32_t mEventsSuppressed;

  



  uint32_t mExternalScriptsBeingEvaluated;

  


  int32_t mFrameRequestCallbackCounter;

  
  
  nsPIDOMWindow *mWindow;

  nsCOMPtr<nsIDocumentEncoder> mCachedEncoder;

  struct FrameRequest {
    FrameRequest(nsIFrameRequestCallback* aCallback,
                 int32_t aHandle) :
      mCallback(aCallback),
      mHandle(aHandle)
    {}

    
    
    operator nsIFrameRequestCallback* const () const { return mCallback; }

    
    
    bool operator==(int32_t aHandle) const {
      return mHandle == aHandle;
    }
    bool operator<(int32_t aHandle) const {
      return mHandle < aHandle;
    }
    
    nsCOMPtr<nsIFrameRequestCallback> mCallback;
    int32_t mHandle;
  };

  nsTArray<FrameRequest> mFrameRequestCallbacks;

  
  
  nsIBFCacheEntry *mBFCacheEntry;

  
  nsString mBaseTarget;

  nsCOMPtr<nsIStructuredCloneContainer> mStateObjectContainer;
  nsCOMPtr<nsIVariant> mStateObjectCached;

  uint8_t mDefaultElementType;

  uint32_t mInSyncOperationCount;
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
    UpdateTarget(nullptr, nullptr);
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

class NS_STACK_CLASS nsAutoSyncOperation
{
public:
  nsAutoSyncOperation(nsIDocument* aDocument);
  ~nsAutoSyncOperation();
private:
  nsCOMArray<nsIDocument> mDocuments;
  uint32_t                mMicroTaskLevel;
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
                  DocumentFlavor aFlavor);



nsresult
NS_NewXBLDocument(nsIDOMDocument** aInstancePtrResult,
                  nsIURI* aDocumentURI,
                  nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal);

nsresult
NS_NewPluginDocument(nsIDocument** aInstancePtrResult);

inline nsIDocument*
nsINode::GetOwnerDocument() const
{
  nsIDocument* ownerDoc = OwnerDoc();

  return ownerDoc != this ? ownerDoc : nullptr;
}

inline nsINode*
nsINode::OwnerDocAsNode() const
{
  return OwnerDoc();
}

#endif 
