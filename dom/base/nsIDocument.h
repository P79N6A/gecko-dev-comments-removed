



#ifndef nsIDocument_h___
#define nsIDocument_h___

#include "mozFlushType.h"                
#include "nsAutoPtr.h"                   
#include "nsCOMArray.h"                  
#include "nsCRT.h"                       
#include "nsCompatibility.h"             
#include "nsCOMPtr.h"                    
#include "nsGkAtoms.h"                   
#include "nsIDocumentObserver.h"         
#include "nsILoadGroup.h"                
#include "nsINode.h"                     
#include "nsIScriptGlobalObject.h"       
#include "nsIServiceManager.h"
#include "nsIUUIDGenerator.h"
#include "nsPIDOMWindow.h"               
#include "nsPropertyTable.h"             
#include "nsTHashtable.h"                
#include "mozilla/net/ReferrerPolicy.h"  
#include "nsWeakReference.h"
#include "mozilla/dom/DocumentBinding.h"
#include "mozilla/WeakPtr.h"
#include "Units.h"
#include "nsExpirationTracker.h"
#include "nsClassHashtable.h"
#include "prclist.h"

class imgIRequest;
class nsAString;
class nsBindingManager;
class nsIDocShell;
class nsDocShell;
class nsDOMNavigationTiming;
class nsFrameLoader;
class nsHTMLCSSStyleSheet;
class nsHTMLDocument;
class nsHTMLStyleSheet;
class nsIAtom;
class nsIBFCacheEntry;
class nsIChannel;
class nsIContent;
class nsIContentSink;
class nsIDocShell;
class nsIDocumentEncoder;
class nsIDocumentObserver;
class nsIDOMDocument;
class nsIDOMDocumentFragment;
class nsIDOMDocumentType;
class nsIDOMElement;
class nsIDOMNodeFilter;
class nsIDOMNodeList;
class nsIHTMLCollection;
class nsILayoutHistoryState;
class nsILoadContext;
class nsIObjectLoadingContent;
class nsIObserver;
class nsIPresShell;
class nsIPrincipal;
class nsIRequest;
class nsIRunnable;
class nsIStreamListener;
class nsIStructuredCloneContainer;
class nsIStyleRule;
class nsIStyleSheet;
class nsIURI;
class nsIVariant;
class nsLocation;
class nsViewManager;
class nsPresContext;
class nsRange;
class nsScriptLoader;
class nsSMILAnimationController;
class nsStyleSet;
class nsTextNode;
class nsWindowSizes;
class nsSmallVoidArray;
class nsDOMCaretPosition;
class nsViewportInfo;
class nsIGlobalObject;
struct nsCSSSelectorList;

namespace mozilla {
class CSSStyleSheet;
class ErrorResult;
class EventStates;
class PendingPlayerTracker;
class SVGAttrAnimationRuleProcessor;

namespace css {
class Loader;
class ImageLoader;
} 

namespace gfx {
class VRHMDInfo;
} 

namespace dom {
class AnonymousContent;
class Attr;
class BoxObject;
class CDATASection;
class Comment;
struct CustomElementDefinition;
class DocumentFragment;
class DocumentTimeline;
class DocumentType;
class DOMImplementation;
class DOMStringList;
class Element;
struct ElementRegistrationOptions;
class Event;
class EventTarget;
class FontFaceSet;
class FrameRequestCallback;
class ImportManager;
class OverfillCallback;
class HTMLBodyElement;
struct LifecycleCallbackArgs;
class Link;
class MediaQueryList;
class GlobalObject;
class NodeFilter;
class NodeIterator;
class ProcessingInstruction;
class StyleSheetList;
class SVGDocument;
class Touch;
class TouchList;
class TreeWalker;
class UndoManager;
class XPathEvaluator;
class XPathExpression;
class XPathNSResolver;
class XPathResult;
template<typename> class OwningNonNull;
template<typename> class Sequence;

template<typename, typename> class CallbackObjectHolder;
typedef CallbackObjectHolder<NodeFilter, nsIDOMNodeFilter> NodeFilterHolder;

struct FullScreenOptions {
  FullScreenOptions();
  nsRefPtr<gfx::VRHMDInfo> mVRHMDDevice;
};

} 
} 

#define NS_IDOCUMENT_IID \
{ 0x0b78eabe, 0x8b94, 0x4ea1, \
  { 0x93, 0x31, 0x5d, 0x48, 0xe8, 0x3a, 0xda, 0x95 } }


enum DocumentFlavor {
  DocumentFlavorLegacyGuess, 
  DocumentFlavorHTML, 
  DocumentFlavorSVG, 
  DocumentFlavorPlain, 
};




#define NS_DOCUMENT_STATE_RTL_LOCALE              NS_DEFINE_EVENT_STATE_MACRO(0)

#define NS_DOCUMENT_STATE_WINDOW_INACTIVE         NS_DEFINE_EVENT_STATE_MACRO(1)


class nsContentList;

already_AddRefed<nsContentList>
NS_GetContentList(nsINode* aRootNode,
                  int32_t aMatchNameSpaceId,
                  const nsAString& aTagname);





class nsIDocument : public nsINode
{
  typedef mozilla::dom::GlobalObject GlobalObject;
public:
  typedef mozilla::net::ReferrerPolicy ReferrerPolicyEnum;
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

  



  virtual void SetChromeXHRDocURI(nsIURI* aURI) = 0;

  



  virtual void SetChromeXHRDocBaseURI(nsIURI* aURI) = 0;

  



  ReferrerPolicyEnum GetReferrerPolicy() const
  {
    return mReferrerPolicy;
  }

  


  uint32_t ReferrerPolicy() const
  {
    return GetReferrerPolicy();
  }

  


  virtual void SetPrincipal(nsIPrincipal *aPrincipal) = 0;

  


  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup() const
  {
    nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);
    return group.forget();
  }

  





  nsIURI* GetDocBaseURI() const
  {
    if (mIsSrcdocDocument && mParentDocument) {
      return mParentDocument->GetDocBaseURI();
    }
    return mDocumentBaseURI ? mDocumentBaseURI : mDocumentURI;
  }
  virtual already_AddRefed<nsIURI> GetBaseURI(bool aTryUseXHRDocBaseURI = false) const override;

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

  


  bool GetHasMixedActiveContentLoaded()
  {
    return mHasMixedActiveContentLoaded;
  }

  


  void SetHasMixedActiveContentLoaded(bool aHasMixedActiveContentLoaded)
  {
    mHasMixedActiveContentLoaded = aHasMixedActiveContentLoaded;
  }

  


  bool GetHasMixedActiveContentBlocked()
  {
    return mHasMixedActiveContentBlocked;
  }

  


  void SetHasMixedActiveContentBlocked(bool aHasMixedActiveContentBlocked)
  {
    mHasMixedActiveContentBlocked = aHasMixedActiveContentBlocked;
  }

  


  bool GetHasMixedDisplayContentLoaded()
  {
    return mHasMixedDisplayContentLoaded;
  }

  


  void SetHasMixedDisplayContentLoaded(bool aHasMixedDisplayContentLoaded)
  {
    mHasMixedDisplayContentLoaded = aHasMixedDisplayContentLoaded;
  }

  


  bool GetHasMixedDisplayContentBlocked()
  {
    return mHasMixedDisplayContentBlocked;
  }

  


  void SetHasMixedDisplayContentBlocked(bool aHasMixedDisplayContentBlocked)
  {
    mHasMixedDisplayContentBlocked = aHasMixedDisplayContentBlocked;
  }

  


  bool GetHasTrackingContentBlocked()
  {
    return mHasTrackingContentBlocked;
  }

  


  void SetHasTrackingContentBlocked(bool aHasTrackingContentBlocked)
  {
    mHasTrackingContentBlocked = aHasTrackingContentBlocked;
  }

  


  bool GetHasTrackingContentLoaded()
  {
    return mHasTrackingContentLoaded;
  }

  


  void SetHasTrackingContentLoaded(bool aHasTrackingContentLoaded)
  {
    mHasTrackingContentLoaded = aHasTrackingContentLoaded;
  }

  



  uint32_t GetSandboxFlags() const
  {
    return mSandboxFlags;
  }

  



  void SetSandboxFlags(uint32_t sandboxFlags)
  {
    mSandboxFlags = sandboxFlags;
  }

  



  virtual void GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const = 0;
  virtual void SetHeaderData(nsIAtom* aheaderField, const nsAString& aData) = 0;

  






  virtual already_AddRefed<nsIPresShell> CreateShell(nsPresContext* aContext,
                                                     nsViewManager* aViewManager,
                                                     nsStyleSet* aStyleSet) = 0;
  virtual void DeleteShell() = 0;

  nsIPresShell* GetShell() const
  {
    return GetBFCacheEntry() ? nullptr : mPresShell;
  }

  void DisallowBFCaching()
  {
    NS_ASSERTION(!mBFCacheEntry, "We're already in the bfcache!");
    mBFCacheDisallowed = true;
  }

  bool IsBFCachingAllowed() const
  {
    return !mBFCacheDisallowed;
  }

  void SetBFCacheEntry(nsIBFCacheEntry* aEntry)
  {
    NS_ASSERTION(IsBFCachingAllowed() || !aEntry,
                 "You should have checked!");

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

  


  mozilla::dom::DocumentType* GetDoctype() const;

  


  Element* GetRootElement() const;

  virtual nsViewportInfo GetViewportInfo(const mozilla::ScreenIntSize& aDisplaySize) = 0;

  


  virtual bool WillIgnoreCharsetOverride() {
    return true;
  }

  


  bool IsSrcdocDocument() const {
    return mIsSrcdocDocument;
  }

  


  void SetIsSrcdocDocument(bool aIsSrcdocDocument) {
    mIsSrcdocDocument = aIsSrcdocDocument;
  }

  




  nsresult GetSrcdocData(nsAString& aSrcdocData);

  bool DidDocumentOpen() {
    return mDidDocumentOpen;
  }

  already_AddRefed<mozilla::dom::AnonymousContent>
  InsertAnonymousContent(mozilla::dom::Element& aElement,
                         mozilla::ErrorResult& aError);
  void RemoveAnonymousContent(mozilla::dom::AnonymousContent& aContent,
                              mozilla::ErrorResult& aError);
  nsTArray<nsRefPtr<mozilla::dom::AnonymousContent>>& GetAnonymousContents() {
    return mAnonymousContents;
  }

  nsresult GetId(nsAString& aId);

protected:
  virtual Element *GetRootElementInternal() const = 0;

private:
  class SelectorCacheKey
  {
    public:
      explicit SelectorCacheKey(const nsAString& aString) : mKey(aString)
      {
        MOZ_COUNT_CTOR(SelectorCacheKey);
      }

      nsString mKey;
      nsExpirationState mState;

      nsExpirationState* GetExpirationState() { return &mState; }

      ~SelectorCacheKey()
      {
        MOZ_COUNT_DTOR(SelectorCacheKey);
      }
  };

  class SelectorCacheKeyDeleter;

public:
  class SelectorCache final
    : public nsExpirationTracker<SelectorCacheKey, 4>
  {
    public:
      SelectorCache();

      
      void CacheList(const nsAString& aSelector, nsCSSSelectorList* aSelectorList);

      virtual void NotifyExpired(SelectorCacheKey* aSelector) override;

      
      
      
      
      
      bool GetList(const nsAString& aSelector, nsCSSSelectorList** aList)
      {
        return mTable.Get(aSelector, aList);
      }

      ~SelectorCache()
      {
        AgeAllGenerations();
      }

    private:
      nsClassHashtable<nsStringHashKey, nsCSSSelectorList> mTable;
  };

  SelectorCache& GetSelectorCache()
  {
    return mSelectorCache;
  }
  
  
  Element* GetHtmlElement() const;
  
  
  Element* GetHtmlChildElement(nsIAtom* aTag);
  
  
  mozilla::dom::HTMLBodyElement* GetBodyElement();
  
  
  Element* GetHeadElement() {
    return GetHtmlChildElement(nsGkAtoms::head);
  }
  
  




  



















  virtual void EnsureOnDemandBuiltInUASheet(mozilla::CSSStyleSheet* aSheet) = 0;

  





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

  enum additionalSheetType {
    eAgentSheet,
    eUserSheet,
    eAuthorSheet,
    SheetTypeCount
  };

  virtual nsresult LoadAdditionalStyleSheet(additionalSheetType aType, nsIURI* aSheetURI) = 0;
  virtual nsresult AddAdditionalStyleSheet(additionalSheetType aType, nsIStyleSheet* aSheet) = 0;
  virtual void RemoveAdditionalStyleSheet(additionalSheetType aType, nsIURI* sheetURI) = 0;
  virtual nsIStyleSheet* FirstAdditionalAuthorSheet() = 0;

  


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

  



  nsHTMLCSSStyleSheet* GetInlineStyleSheet() const {
    return mStyleAttrStyleSheet;
  }

  



  mozilla::SVGAttrAnimationRuleProcessor*
  GetSVGAttrAnimationRuleProcessor() const
  {
    return mSVGAttrAnimationRuleProcessor;
  }

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject) = 0;

  








  nsIScriptGlobalObject*
    GetScriptHandlingObject(bool& aHasHadScriptHandlingObject) const
  {
    aHasHadScriptHandlingObject = mHasHadScriptHandlingObject;
    return mScriptGlobalObject ? mScriptGlobalObject.get() :
                                 GetScriptHandlingObjectInternal();
  }
  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject) = 0;

  






  virtual nsIGlobalObject* GetScopeObject() const = 0;
  virtual void SetScopeObject(nsIGlobalObject* aGlobal) = 0;

  


  nsPIDOMWindow *GetWindow() const
  {
    return mWindow ? mWindow->GetOuterWindow() : GetWindowInternal();
  }

  bool IsInBackgroundWindow() const
  {
    nsPIDOMWindow* outer = mWindow ? mWindow->GetOuterWindow() : nullptr;
    return outer && outer->IsBackground();
  }
  
  




  nsPIDOMWindow* GetInnerWindow() const
  {
    return mRemovedFromDocShell ? nullptr : mWindow;
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

  










  virtual void AsyncRequestFullScreen(Element* aElement,
                                      mozilla::dom::FullScreenOptions& aOptions) = 0;

  






  virtual nsresult RemoteFrameFullscreenChanged(nsIDOMElement* aFrameElement,
                                                const nsAString& aNewOrigin) = 0;

  








   virtual nsresult RemoteFrameFullscreenReverted() = 0;

  




  virtual void RestorePreviousFullScreenState() = 0;

  


  virtual bool IsFullScreenDoc() = 0;

  



  virtual bool IsFullscreenLeaf() = 0;

  




  virtual nsIDocument* GetFullscreenRoot() = 0;

  



  virtual void SetFullscreenRoot(nsIDocument* aRoot) = 0;

  





  virtual void SetApprovedForFullscreen(bool aIsApproved) = 0;

  




















  static void ExitFullscreen(nsIDocument* aDocument, bool aRunAsync);

  virtual void RequestPointerLock(Element* aElement) = 0;

  static void UnlockPointer(nsIDocument* aDoc = nullptr);


  

  

  





  virtual void AddObserver(nsIDocumentObserver* aObserver) = 0;

  



  virtual bool RemoveObserver(nsIDocumentObserver* aObserver) = 0;

  
  
  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType) = 0;
  virtual void EndUpdate(nsUpdateType aUpdateType) = 0;
  virtual void BeginLoad() = 0;
  virtual void EndLoad() = 0;

  enum ReadyState { READYSTATE_UNINITIALIZED = 0, READYSTATE_LOADING = 1, READYSTATE_INTERACTIVE = 3, READYSTATE_COMPLETE = 4};
  virtual void SetReadyStateInternal(ReadyState rs) = 0;
  ReadyState GetReadyStateEnum()
  {
    return mReadyState;
  }

  
  
  virtual void ContentStateChanged(nsIContent* aContent,
                                   mozilla::EventStates aStateMask) = 0;

  
  
  
  virtual void DocumentStatesChanged(mozilla::EventStates aStateMask) = 0;

  
  
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

  



  virtual void SetContainer(nsDocShell* aContainer);

  


  virtual nsISupports* GetContainer() const;

  


  nsILoadContext* GetLoadContext() const;

  


  nsIDocShell* GetDocShell() const;

  





  virtual void SetXMLDeclaration(const char16_t *aVersion,
                                 const char16_t *aEncoding,
                                 const int32_t aStandalone) = 0;
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone) = 0;

  





  bool IsHTMLDocument() const
  {
    return mType == eHTML;
  }
  bool IsHTMLOrXHTML() const
  {
    return mType == eHTML || mType == eXHTML;
  }
  bool IsXMLDocument() const
  {
    return !IsHTMLDocument();
  }
  bool IsSVGDocument() const
  {
    return mType == eSVG;
  }
  bool IsXULDocument() const
  {
    return mType == eXUL;
  }
  bool IsUnstyledDocument()
  {
    return IsLoadedAsData() || IsLoadedAsInteractiveData();
  }
  bool LoadsFullXULStyleSheetUpFront()
  {
    return IsXULDocument() || AllowXULXBL();
  }

  virtual bool IsScriptEnabled() = 0;

  


  virtual nsresult CreateElem(const nsAString& aName, nsIAtom *aPrefix,
                              int32_t aNamespaceID,
                              nsIContent** aResult) = 0;

  






  nsISupports *GetSecurityInfo()
  {
    return mSecurityInfo;
  }

  



  virtual nsIChannel* GetFailedChannel() const = 0;

  



  virtual void SetFailedChannel(nsIChannel* aChannel) = 0;

  



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

  



  virtual void Sanitize() = 0;

  




  typedef bool (*nsSubDocEnumFunc)(nsIDocument *aDocument, void *aData);
  virtual void EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                     void *aData) = 0;

  
















  virtual bool CanSavePresentation(nsIRequest *aNewRequest) = 0;

  




  virtual void Destroy() = 0;

  





  virtual void RemovedFromDocShell() = 0;
  
  




  virtual already_AddRefed<nsILayoutHistoryState> GetLayoutHistoryState() const = 0;

  






  virtual void BlockOnload() = 0;
  





  virtual void UnblockOnload(bool aFireSync) = 0;

  void BlockDOMContentLoaded()
  {
    ++mBlockDOMContentLoaded;
  }

  virtual void UnblockDOMContentLoaded() = 0;

  











  virtual void OnPageShow(bool aPersisted,
                          mozilla::dom::EventTarget* aDispatchStartTarget) = 0;

  











  virtual void OnPageHide(bool aPersisted,
                          mozilla::dom::EventTarget* aDispatchStartTarget) = 0;

  



  


  virtual void AddStyleRelevantLink(mozilla::dom::Link* aLink) = 0;
  





  virtual void ForgetLink(mozilla::dom::Link* aLink) = 0;

  




  virtual void ClearBoxObjectFor(nsIContent *aContent) = 0;

  



  virtual already_AddRefed<mozilla::dom::BoxObject>
    GetBoxObjectFor(mozilla::dom::Element* aElement,
                    mozilla::ErrorResult& aRv) = 0;

  



  already_AddRefed<mozilla::dom::MediaQueryList>
    MatchMedia(const nsAString& aMediaQueryList);

  const PRCList* MediaQueryLists() const {
    return &mDOMMediaQueryLists;
  }

  


  nsCompatibility GetCompatibilityMode() const {
    return mCompatMode;
  }
  
  



  bool HaveFiredDOMTitleChange() const {
    return mHaveFiredTitleChange;
  }

  


  virtual Element*
    GetAnonymousElementByAttribute(nsIContent* aElement,
                                   nsIAtom* aAttrName,
                                   const nsAString& aAttrValue) const = 0;

  





  virtual Element* ElementFromPointHelper(float aX, float aY,
                                          bool aIgnoreRootScrollFrame,
                                          bool aFlushLayout) = 0;

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

  already_AddRefed<nsIDocumentEncoder> GetCachedEncoder();

  void SetCachedEncoder(already_AddRefed<nsIDocumentEncoder> aEncoder);

  
  virtual nsresult InitializeFrameLoader(nsFrameLoader* aLoader) = 0;
  
  
  virtual nsresult FinalizeFrameLoader(nsFrameLoader* aLoader, nsIRunnable* aFinalizer) = 0;
  
  virtual void TryCancelFrameLoaderInitialization(nsIDocShell* aShell) = 0;

  



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
                    !GetContainer() &&
                    !GetWindow(),
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

  



  bool IsVisibleConsideringAncestors() const;

  





  bool IsActive() const { return mDocumentContainer && !mRemovedFromDocShell; }

  






  bool IsCurrentActiveDocument() const
  {
    nsPIDOMWindow *inner = GetInnerWindow();
    return inner && inner->IsCurrentInnerWindow() && inner->GetDoc() == this;
  }

  





  void RegisterActivityObserver(nsISupports* aSupports);
  bool UnregisterActivityObserver(nsISupports* aSupports);
  
  typedef void (* ActivityObserverEnumerator)(nsISupports*, void*);
  void EnumerateActivityObservers(ActivityObserverEnumerator aEnumerator,
                                  void* aData);

  
  
  
  bool HasAnimationController()  { return !!mAnimationController; }

  
  
  
  virtual nsSMILAnimationController* GetAnimationController() = 0;

  
  
  
  
  virtual mozilla::PendingPlayerTracker* GetPendingPlayerTracker() = 0;

  
  
  
  virtual mozilla::PendingPlayerTracker* GetOrCreatePendingPlayerTracker() = 0;

  
  
  
  virtual void SetImagesNeedAnimating(bool aAnimating) = 0;

  enum SuppressionType {
    eAnimationsOnly = 0x1,

    
    
    eEvents = 0x3,
  };

  



  virtual void SuppressEventHandling(SuppressionType aWhat,
                                     uint32_t aIncrease = 1) = 0;

  




  virtual void UnsuppressEventHandlingAndFireEvents(SuppressionType aWhat,
                                                    bool aFireEvents) = 0;

  uint32_t EventHandlingSuppressed() const { return mEventsSuppressed; }
  uint32_t AnimationsPaused() const { return mAnimationsPaused; }

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

  



  virtual nsIDocument* GetTemplateContentsOwner() = 0;

  



  bool IsStaticDocument() { return mIsStaticDocument; }

  



  virtual already_AddRefed<nsIDocument>
  CreateStaticClone(nsIDocShell* aCloneContainer);

  



  nsIDocument* GetOriginalDocument()
  {
    MOZ_ASSERT(!mOriginalDocument || !mOriginalDocument->GetOriginalDocument());
    return mOriginalDocument;
  }

  











  virtual void PreloadPictureOpened() = 0;

  virtual void PreloadPictureClosed() = 0;

  virtual void PreloadPictureImageSource(const nsAString& aSrcsetAttr,
                                         const nsAString& aSizesAttr,
                                         const nsAString& aTypeAttr,
                                         const nsAString& aMediaAttr) = 0;

  






  virtual already_AddRefed<nsIURI>
    ResolvePreloadImage(nsIURI *aBaseURI,
                        const nsAString& aSrcAttr,
                        const nsAString& aSrcsetAttr,
                        const nsAString& aSizesAttr) = 0;
  





  virtual void MaybePreLoadImage(nsIURI* uri,
                                 const nsAString& aCrossOriginAttr,
                                 ReferrerPolicyEnum aReferrerPolicy) = 0;

  



  virtual void ForgetImagePreload(nsIURI* aURI) = 0;

  




  virtual void PreloadStyle(nsIURI* aURI, const nsAString& aCharset,
                            const nsAString& aCrossOriginAttr,
                            ReferrerPolicyEnum aReferrerPolicy) = 0;

  







  virtual nsresult LoadChromeSheetSync(nsIURI* aURI, bool aIsAgentSheet,
                                       mozilla::CSSStyleSheet** aSheet) = 0;

  






  virtual bool IsDocumentRightToLeft() { return false; }

  enum DocumentTheme {
    Doc_Theme_Uninitialized, 
    Doc_Theme_None,
    Doc_Theme_Neutral,
    Doc_Theme_Dark,
    Doc_Theme_Bright
  };

  



  void SetStateObject(nsIStructuredCloneContainer *scContainer);

  





  virtual int GetDocumentLWTheme() { return Doc_Theme_None; }

  




  virtual mozilla::EventStates GetDocumentState() = 0;

  virtual nsISupports* GetCurrentContentSink() = 0;

  





  virtual void RegisterHostObjectUri(const nsACString& aUri) = 0;
  virtual void UnregisterHostObjectUri(const nsACString& aUri) = 0;

  virtual void SetScrollToRef(nsIURI *aDocumentURI) = 0;
  virtual void ScrollToRef() = 0;
  virtual void ResetScrolledToRefAlready() = 0;
  virtual void SetChangeScrollPosWhenScrollingToRef(bool aValue) = 0;

  





  virtual Element* GetElementById(const nsAString& aElementId) = 0;

  




  virtual const nsSmallVoidArray* GetAllElementsForId(const nsAString& aElementId) const = 0;

  






  virtual Element* LookupImageElement(const nsAString& aElementId) = 0;

  virtual already_AddRefed<mozilla::dom::UndoManager> GetUndoManager() = 0;

  virtual mozilla::dom::DocumentTimeline* Timeline() = 0;

  typedef mozilla::dom::CallbackObjectHolder<
    mozilla::dom::FrameRequestCallback,
    nsIFrameRequestCallback> FrameRequestCallbackHolder;
  nsresult ScheduleFrameRequestCallback(const FrameRequestCallbackHolder& aCallback,
                                        int32_t *aHandle);
  void CancelFrameRequestCallback(int32_t aHandle);

  typedef nsTArray<FrameRequestCallbackHolder> FrameRequestCallbackList;
  



  void TakeFrameRequestCallbacks(FrameRequestCallbackList& aCallbacks);

  
  bool InUnlinkOrDeletion() { return mInUnlinkOrDeletion; }

  












  
  virtual nsresult AddImage(imgIRequest* aImage) = 0;
  
  
  enum { REQUEST_DISCARD = 0x1 };
  virtual nsresult RemoveImage(imgIRequest* aImage, uint32_t aFlags = 0) = 0;

  
  
  virtual nsresult SetImageLockingState(bool aLocked) = 0;

  virtual nsresult AddPlugin(nsIObjectLoadingContent* aPlugin) = 0;
  virtual void RemovePlugin(nsIObjectLoadingContent* aPlugin) = 0;
  virtual void GetPlugins(nsTArray<nsIObjectLoadingContent*>& aPlugins) = 0;

  virtual nsresult GetStateObject(nsIVariant** aResult) = 0;

  virtual nsDOMNavigationTiming* GetNavigationTiming() const = 0;

  virtual nsresult SetNavigationTiming(nsDOMNavigationTiming* aTiming) = 0;

  virtual Element* FindImageMap(const nsAString& aNormalizedMapName) = 0;

  
  void RegisterPendingLinkUpdate(mozilla::dom::Link* aLink);
  
  
  
  void UnregisterPendingLinkUpdate(mozilla::dom::Link* aElement);

  
  
  void FlushPendingLinkUpdates();

#define DEPRECATED_OPERATION(_op) e##_op,
  enum DeprecatedOperations {
#include "nsDeprecatedOperationList.h"
    eDeprecatedOperationCount
  };
#undef DEPRECATED_OPERATION
  bool HasWarnedAbout(DeprecatedOperations aOperation);
  void WarnOnceAbout(DeprecatedOperations aOperation, bool asError = false);

#define DOCUMENT_WARNING(_op) e##_op,
  enum DocumentWarnings {
#include "nsDocumentWarningList.h"
    eDocumentWarningCount
  };
#undef DOCUMENT_WARNING
  bool HasWarnedAbout(DocumentWarnings aWarning);
  void WarnOnceAbout(DocumentWarnings aWarning,
                     bool asError = false,
                     const char16_t **aParams = nullptr,
                     uint32_t aParamsLength = 0);

  virtual void PostVisibilityUpdateEvent() = 0;
  
  bool IsSyntheticDocument() const { return mIsSyntheticDocument; }

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

  
  
  
  
  virtual void DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const;
  
  
  
  virtual void DocAddSizeOfIncludingThis(nsWindowSizes* aWindowSizes) const;

  bool MayHaveDOMMutationObservers()
  {
    return mMayHaveDOMMutationObservers;
  }

  void SetMayHaveDOMMutationObservers()
  {
    mMayHaveDOMMutationObservers = true;
  }

  bool MayHaveAnimationObservers()
  {
    return mMayHaveAnimationObservers;
  }

  void SetMayHaveAnimationObservers()
  {
    mMayHaveAnimationObservers = true;
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

  bool CreatingStaticClone() const
  {
    return mCreatingStaticClone;
  }

  



  already_AddRefed<Element> CreateHTMLElement(nsIAtom* aTag);

  
  nsIGlobalObject* GetParentObject() const
  {
    return GetScopeObject();
  }
  static already_AddRefed<nsIDocument>
    Constructor(const GlobalObject& aGlobal,
                mozilla::ErrorResult& rv);
  virtual mozilla::dom::DOMImplementation*
    GetImplementation(mozilla::ErrorResult& rv) = 0;
  void GetURL(nsString& retval) const;
  void GetDocumentURI(nsString& retval) const;
  
  
  
  
  void GetDocumentURIFromJS(nsString& aDocumentURI) const;
  void GetCompatMode(nsString& retval) const;
  void GetCharacterSet(nsAString& retval) const;
  
  
  Element* GetDocumentElement() const
  {
    return GetRootElement();
  }

  enum ElementCallbackType {
    eCreated,
    eAttached,
    eDetached,
    eAttributeChanged
  };

  







  virtual nsresult RegisterUnresolvedElement(Element* aElement,
                                             nsIAtom* aTypeName = nullptr) = 0;
  virtual void EnqueueLifecycleCallback(ElementCallbackType aType,
                                        Element* aCustomElement,
                                        mozilla::dom::LifecycleCallbackArgs* aArgs = nullptr,
                                        mozilla::dom::CustomElementDefinition* aDefinition = nullptr) = 0;
  virtual void SetupCustomElement(Element* aElement,
                                  uint32_t aNamespaceID,
                                  const nsAString* aTypeExtension = nullptr) = 0;
  virtual void
    RegisterElement(JSContext* aCx, const nsAString& aName,
                    const mozilla::dom::ElementRegistrationOptions& aOptions,
                    JS::MutableHandle<JSObject*> aRetval,
                    mozilla::ErrorResult& rv) = 0;

  



  virtual void UseRegistryFromDocument(nsIDocument* aDocument) = 0;

  already_AddRefed<nsContentList>
  GetElementsByTagName(const nsAString& aTagName)
  {
    return NS_GetContentList(this, kNameSpaceID_Unknown, aTagName);
  }
  already_AddRefed<nsContentList>
    GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                           const nsAString& aLocalName,
                           mozilla::ErrorResult& aResult);
  already_AddRefed<nsContentList>
    GetElementsByClassName(const nsAString& aClasses);
  
  already_AddRefed<Element> CreateElement(const nsAString& aTagName,
                                          mozilla::ErrorResult& rv);
  already_AddRefed<Element> CreateElementNS(const nsAString& aNamespaceURI,
                                            const nsAString& aQualifiedName,
                                            mozilla::ErrorResult& rv);
  virtual already_AddRefed<Element> CreateElement(const nsAString& aTagName,
                                                  const nsAString& aTypeExtension,
                                                  mozilla::ErrorResult& rv) = 0;
  virtual already_AddRefed<Element> CreateElementNS(const nsAString& aNamespaceURI,
                                                    const nsAString& aQualifiedName,
                                                    const nsAString& aTypeExtension,
                                                    mozilla::ErrorResult& rv) = 0;
  already_AddRefed<mozilla::dom::DocumentFragment>
    CreateDocumentFragment() const;
  already_AddRefed<nsTextNode> CreateTextNode(const nsAString& aData) const;
  already_AddRefed<mozilla::dom::Comment>
    CreateComment(const nsAString& aData) const;
  already_AddRefed<mozilla::dom::ProcessingInstruction>
    CreateProcessingInstruction(const nsAString& target, const nsAString& data,
                                mozilla::ErrorResult& rv) const;
  already_AddRefed<nsINode>
    ImportNode(nsINode& aNode, bool aDeep, mozilla::ErrorResult& rv) const;
  nsINode* AdoptNode(nsINode& aNode, mozilla::ErrorResult& rv);
  already_AddRefed<mozilla::dom::Event>
    CreateEvent(const nsAString& aEventType, mozilla::ErrorResult& rv) const;
  already_AddRefed<nsRange> CreateRange(mozilla::ErrorResult& rv);
  already_AddRefed<mozilla::dom::NodeIterator>
    CreateNodeIterator(nsINode& aRoot, uint32_t aWhatToShow,
                       mozilla::dom::NodeFilter* aFilter,
                       mozilla::ErrorResult& rv) const;
  already_AddRefed<mozilla::dom::NodeIterator>
    CreateNodeIterator(nsINode& aRoot, uint32_t aWhatToShow,
                       const mozilla::dom::NodeFilterHolder& aFilter,
                       mozilla::ErrorResult& rv) const;
  already_AddRefed<mozilla::dom::TreeWalker>
    CreateTreeWalker(nsINode& aRoot, uint32_t aWhatToShow,
                     mozilla::dom::NodeFilter* aFilter, mozilla::ErrorResult& rv) const;
  already_AddRefed<mozilla::dom::TreeWalker>
    CreateTreeWalker(nsINode& aRoot, uint32_t aWhatToShow,
                     const mozilla::dom::NodeFilterHolder& aFilter,
                     mozilla::ErrorResult& rv) const;

  
  already_AddRefed<mozilla::dom::CDATASection>
    CreateCDATASection(const nsAString& aData, mozilla::ErrorResult& rv);
  already_AddRefed<mozilla::dom::Attr>
    CreateAttribute(const nsAString& aName, mozilla::ErrorResult& rv);
  already_AddRefed<mozilla::dom::Attr>
    CreateAttributeNS(const nsAString& aNamespaceURI,
                      const nsAString& aQualifiedName,
                      mozilla::ErrorResult& rv);
  void GetInputEncoding(nsAString& aInputEncoding);
  already_AddRefed<nsLocation> GetLocation() const;
  void GetReferrer(nsAString& aReferrer) const;
  void GetLastModified(nsAString& aLastModified) const;
  void GetReadyState(nsAString& aReadyState) const;
  
  
  
  virtual void GetTitle(nsString& aTitle) = 0;
  virtual void SetTitle(const nsAString& aTitle, mozilla::ErrorResult& rv) = 0;
  void GetDir(nsAString& aDirection) const;
  void SetDir(const nsAString& aDirection);
  nsIDOMWindow* GetDefaultView() const
  {
    return GetWindow();
  }
  Element* GetActiveElement();
  bool HasFocus(mozilla::ErrorResult& rv) const;
  
  bool MozSyntheticDocument() const
  {
    return IsSyntheticDocument();
  }
  Element* GetCurrentScript();
  void ReleaseCapture() const;
  virtual void MozSetImageElement(const nsAString& aImageElementId,
                                  Element* aElement) = 0;
  nsIURI* GetDocumentURIObject() const;
  
  virtual bool MozFullScreenEnabled() = 0;
  virtual Element* GetMozFullScreenElement(mozilla::ErrorResult& rv) = 0;
  bool MozFullScreen()
  {
    return IsFullScreenDoc();
  }
  void MozCancelFullScreen();
  Element* GetMozPointerLockElement();
  void MozExitPointerLock()
  {
    UnlockPointer(this);
  }
  bool Hidden() const
  {
    return mVisibilityState != mozilla::dom::VisibilityState::Visible;
  }
  bool MozHidden() 
  {
    WarnOnceAbout(ePrefixedVisibilityAPI);
    return Hidden();
  }
  mozilla::dom::VisibilityState VisibilityState()
  {
    return mVisibilityState;
  }
  mozilla::dom::VisibilityState MozVisibilityState()
  {
    WarnOnceAbout(ePrefixedVisibilityAPI);
    return VisibilityState();
  }
  virtual mozilla::dom::StyleSheetList* StyleSheets() = 0;
  void GetSelectedStyleSheetSet(nsAString& aSheetSet);
  virtual void SetSelectedStyleSheetSet(const nsAString& aSheetSet) = 0;
  virtual void GetLastStyleSheetSet(nsString& aSheetSet) = 0;
  void GetPreferredStyleSheetSet(nsAString& aSheetSet);
  virtual mozilla::dom::DOMStringList* StyleSheetSets() = 0;
  virtual void EnableStyleSheetsForSet(const nsAString& aSheetSet) = 0;
  Element* ElementFromPoint(float aX, float aY);

  








  already_AddRefed<nsDOMCaretPosition>
    CaretPositionFromPoint(float aX, float aY);

  
  nsINodeList* GetAnonymousNodes(Element& aElement);
  Element* GetAnonymousElementByAttribute(Element& aElement,
                                          const nsAString& aAttrName,
                                          const nsAString& aAttrValue);
  Element* GetBindingParent(nsINode& aNode);
  void LoadBindingDocument(const nsAString& aURI, mozilla::ErrorResult& rv);
  mozilla::dom::XPathExpression*
    CreateExpression(const nsAString& aExpression,
                     mozilla::dom::XPathNSResolver* aResolver,
                     mozilla::ErrorResult& rv);
  nsINode* CreateNSResolver(nsINode& aNodeResolver);
  already_AddRefed<mozilla::dom::XPathResult>
    Evaluate(JSContext* aCx, const nsAString& aExpression, nsINode& aContextNode,
             mozilla::dom::XPathNSResolver* aResolver, uint16_t aType,
             JS::Handle<JSObject*> aResult, mozilla::ErrorResult& rv);
  
  already_AddRefed<mozilla::dom::Touch>
    CreateTouch(nsIDOMWindow* aView, mozilla::dom::EventTarget* aTarget,
                int32_t aIdentifier, int32_t aPageX, int32_t aPageY,
                int32_t aScreenX, int32_t aScreenY, int32_t aClientX,
                int32_t aClientY, int32_t aRadiusX, int32_t aRadiusY,
                float aRotationAngle, float aForce);
  already_AddRefed<mozilla::dom::TouchList> CreateTouchList();
  already_AddRefed<mozilla::dom::TouchList>
    CreateTouchList(mozilla::dom::Touch& aTouch,
                    const mozilla::dom::Sequence<mozilla::dom::OwningNonNull<mozilla::dom::Touch> >& aTouches);
  already_AddRefed<mozilla::dom::TouchList>
    CreateTouchList(const mozilla::dom::Sequence<mozilla::dom::OwningNonNull<mozilla::dom::Touch> >& aTouches);

  void SetStyleSheetChangeEventsEnabled(bool aValue)
  {
    mStyleSheetChangeEventsEnabled = aValue;
  }

  bool StyleSheetChangeEventsEnabled() const
  {
    return mStyleSheetChangeEventsEnabled;
  }

  void ObsoleteSheet(nsIURI *aSheetURI, mozilla::ErrorResult& rv);

  void ObsoleteSheet(const nsAString& aSheetURI, mozilla::ErrorResult& rv);

  
  nsIHTMLCollection* Children();
  uint32_t ChildElementCount();

  virtual nsHTMLDocument* AsHTMLDocument() { return nullptr; }
  virtual mozilla::dom::SVGDocument* AsSVGDocument() { return nullptr; }

  
  
  virtual nsIDocument* MasterDocument() = 0;
  virtual void SetMasterDocument(nsIDocument* master) = 0;
  virtual bool IsMasterDocument() = 0;
  virtual mozilla::dom::ImportManager* ImportManager() = 0;
  
  virtual bool HasSubImportLink(nsINode* aLink) = 0;
  virtual uint32_t IndexOfSubImportLink(nsINode* aLink) = 0;
  virtual void AddSubImportLink(nsINode* aLink) = 0;
  virtual nsINode* GetSubImportLink(uint32_t aIdx) = 0;

  




  void AddBlockedTrackingNode(nsINode *node)
  {
    if (!node) {
      return;
    }

    nsWeakPtr weakNode = do_GetWeakReference(node);

    if (weakNode) {
      mBlockedTrackingNodes.AppendElement(weakNode);
    }
  }

  
  mozilla::dom::FontFaceSet* GetFonts(mozilla::ErrorResult& aRv);

  bool DidFireDOMContentLoaded() const { return mDidFireDOMContentLoaded; }

private:
  uint64_t mDeprecationWarnedAbout;
  uint64_t mDocWarningWarnedAbout;
  SelectorCache mSelectorCache;

protected:
  ~nsIDocument();
  nsPropertyTable* GetExtraPropertyTable(uint16_t aCategory);

  
  virtual nsPIDOMWindow *GetWindowInternal() const = 0;

  
  virtual nsIScriptGlobalObject* GetScriptHandlingObjectInternal() const = 0;

  
  virtual bool InternalAllowXULXBL() = 0;

  




  virtual void WillDispatchMutationEvent(nsINode* aTarget) = 0;
  virtual void MutationEventDispatched(nsINode* aTarget) = 0;
  friend class mozAutoSubtreeModified;

  virtual Element* GetNameSpaceElement() override
  {
    return GetRootElement();
  }

  void SetContentTypeInternal(const nsACString& aType);

  nsCString GetContentTypeInternal() const
  {
    return mContentType;
  }

  mozilla::dom::XPathEvaluator* XPathEvaluator();

  nsCString mReferrer;
  nsString mLastModified;

  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mOriginalURI;
  nsCOMPtr<nsIURI> mChromeXHRDocURI;
  nsCOMPtr<nsIURI> mDocumentBaseURI;
  nsCOMPtr<nsIURI> mChromeXHRDocBaseURI;

  nsWeakPtr mDocumentLoadGroup;

  bool mReferrerPolicySet;
  ReferrerPolicyEnum mReferrerPolicy;

  mozilla::WeakPtr<nsDocShell> mDocumentContainer;

  nsCString mCharacterSet;
  int32_t mCharacterSetSource;

  
  nsIDocument* mParentDocument;

  
  mozilla::dom::Element* mCachedRootElement;

  
  
  nsNodeInfoManager* mNodeInfoManager;
  nsRefPtr<mozilla::css::Loader> mCSSLoader;
  nsRefPtr<mozilla::css::ImageLoader> mStyleImageLoader;
  nsRefPtr<nsHTMLStyleSheet> mAttrStyleSheet;
  nsRefPtr<nsHTMLCSSStyleSheet> mStyleAttrStyleSheet;
  nsRefPtr<mozilla::SVGAttrAnimationRuleProcessor> mSVGAttrAnimationRuleProcessor;

  
  
  
  
  
  nsAutoPtr<nsTHashtable<nsPtrHashKey<nsISupports> > > mActivityObservers;

  
  
  
  nsTHashtable<nsPtrHashKey<mozilla::dom::Link> > mLinksToUpdate;

  
  nsRefPtr<nsSMILAnimationController> mAnimationController;

  
  nsPropertyTable mPropertyTable;
  nsTArray<nsAutoPtr<nsPropertyTable> > mExtraPropertyTables;

  
  nsCOMPtr<nsIHTMLCollection> mChildrenCollection;

  
  nsCompatibility mCompatMode;

  
  ReadyState mReadyState;

  
  mozilla::dom::VisibilityState mVisibilityState;

  
  bool mBidiEnabled;
  
  bool mMathMLEnabled;

  
  
  
  
  bool mIsInitialDocumentInWindow;

  
  
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

  
  bool mMayHaveAnimationObservers;

  
  bool mHasMixedActiveContentLoaded;

  
  bool mHasMixedActiveContentBlocked;

  
  bool mHasMixedDisplayContentLoaded;

  
  bool mHasMixedDisplayContentBlocked;

  
  bool mHasTrackingContentBlocked;

  
  bool mHasTrackingContentLoaded;

  
  bool mBFCacheDisallowed;

  
  
  bool mHaveInputEncoding;

  bool mHasHadDefaultView;

  
  bool mStyleSheetChangeEventsEnabled;

  
  bool mIsSrcdocDocument;

  
  
  
  bool mDidDocumentOpen;

#ifdef DEBUG
  



  bool mIsLinkUpdateRegistrationsForbidden;
#endif

  enum Type {
    eUnknown, 
    eHTML,
    eXHTML,
    eGenericXML,
    eSVG,
    eXUL
  };

  uint8_t mType;

  uint8_t mDefaultElementType;

  enum {
    eTriUnset = 0,
    eTriFalse,
    eTriTrue
  };

  uint8_t mAllowXULXBL;

  
  
  
  nsCOMPtr<nsIScriptGlobalObject> mScriptGlobalObject;

  
  
  nsCOMPtr<nsIDocument> mOriginalDocument;

  
  
  uint32_t mBidiOptions;

  
  
  
  uint32_t mSandboxFlags;

  nsCString mContentLanguage;

  
  nsCOMPtr<nsIChannel> mChannel;
private:
  nsCString mContentType;
  nsString mId;
protected:

  
  nsCOMPtr<nsISupports> mSecurityInfo;

  
  
  nsCOMPtr<nsIChannel> mFailedChannel;

  
  
  uint32_t mPartID;
  
  
  
  uint32_t mMarkedCCGeneration;

  nsIPresShell* mPresShell;

  nsCOMArray<nsINode> mSubtreeModifiedTargets;
  uint32_t            mSubtreeModifiedDepth;

  
  
  
  nsCOMPtr<nsIDocument> mDisplayDocument;

  uint32_t mEventsSuppressed;

  uint32_t mAnimationsPaused;

  



  uint32_t mExternalScriptsBeingEvaluated;

  


  int32_t mFrameRequestCallbackCounter;

  
  
  
  
  
  nsTArray<nsWeakPtr> mBlockedTrackingNodes;

  
  
  nsPIDOMWindow *mWindow;

  nsCOMPtr<nsIDocumentEncoder> mCachedEncoder;

  struct FrameRequest;

  nsTArray<FrameRequest> mFrameRequestCallbacks;

  
  
  nsIBFCacheEntry *mBFCacheEntry;

  
  nsString mBaseTarget;

  nsCOMPtr<nsIStructuredCloneContainer> mStateObjectContainer;
  nsCOMPtr<nsIVariant> mStateObjectCached;

  uint32_t mInSyncOperationCount;

  nsRefPtr<mozilla::dom::XPathEvaluator> mXPathEvaluator;

  nsTArray<nsRefPtr<mozilla::dom::AnonymousContent>> mAnonymousContents;

  uint32_t mBlockDOMContentLoaded;
  bool mDidFireDOMContentLoaded:1;

  
  PRCList mDOMMediaQueryLists;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocument, NS_IDOCUMENT_IID)






class MOZ_STACK_CLASS mozAutoSubtreeModified
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

class MOZ_STACK_CLASS nsAutoSyncOperation
{
public:
  explicit nsAutoSyncOperation(nsIDocument* aDocument);
  ~nsAutoSyncOperation();
private:
  nsCOMArray<nsIDocument> mDocuments;
  uint32_t                mMicroTaskLevel;
};


nsresult
NS_NewHTMLDocument(nsIDocument** aInstancePtrResult, bool aLoadedAsData = false);

nsresult
NS_NewXMLDocument(nsIDocument** aInstancePtrResult, bool aLoadedAsData = false,
                  bool aIsPlainDocument = false);

nsresult
NS_NewSVGDocument(nsIDocument** aInstancePtrResult);

nsresult
NS_NewImageDocument(nsIDocument** aInstancePtrResult);

nsresult
NS_NewVideoDocument(nsIDocument** aInstancePtrResult);




nsresult
NS_NewDOMDocument(nsIDOMDocument** aInstancePtrResult,
                  const nsAString& aNamespaceURI, 
                  const nsAString& aQualifiedName, 
                  nsIDOMDocumentType* aDoctype,
                  nsIURI* aDocumentURI,
                  nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal,
                  bool aLoadedAsData,
                  nsIGlobalObject* aEventObject,
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

inline mozilla::dom::ParentObject
nsINode::GetParentObject() const
{
  mozilla::dom::ParentObject p(OwnerDoc());
    
    
  p.mUseXBLScope = IsInAnonymousSubtree() && !IsAnonymousContentInSVGUseSubtree();
  return p;
}

#endif 
