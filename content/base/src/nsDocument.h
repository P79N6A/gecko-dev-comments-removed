








#ifndef nsDocument_h___
#define nsDocument_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCRT.h"
#include "nsIDocument.h"
#include "nsWeakReference.h"
#include "nsWeakPtr.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsStubDocumentObserver.h"
#include "nsIDOMStyleSheetList.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMEventTarget.h"
#include "nsIContent.h"
#include "nsEventListenerManager.h"
#include "nsIDOMNodeSelector.h"
#include "nsIPrincipal.h"
#include "nsIParser.h"
#include "nsBindingManager.h"
#include "nsINodeInfo.h"
#include "nsInterfaceHashtable.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"
#include "nsScriptLoader.h"
#include "nsIRadioGroupContainer.h"
#include "nsILayoutHistoryState.h"
#include "nsIRequest.h"
#include "nsILoadGroup.h"
#include "nsTObserverArray.h"
#include "nsStubMutationObserver.h"
#include "nsIChannel.h"
#include "nsCycleCollectionParticipant.h"
#include "nsContentList.h"
#include "nsGkAtoms.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheContainer.h"
#include "nsStyleSet.h"
#include "pldhash.h"
#include "nsAttrAndChildArray.h"
#include "nsDOMAttributeMap.h"
#include "nsThreadUtils.h"
#include "nsIContentViewer.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadContext.h"
#include "nsIProgressEventSink.h"
#include "nsISecurityEventSink.h"
#include "nsIChannelEventSink.h"
#include "nsIDocumentRegister.h"
#include "imgIRequest.h"
#include "mozilla/dom/DOMImplementation.h"
#include "nsIDOMTouchEvent.h"
#include "nsIInlineEventHandlers.h"
#include "nsDataHashtable.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"

#define XML_DECLARATION_BITS_DECLARATION_EXISTS   (1 << 0)
#define XML_DECLARATION_BITS_ENCODING_EXISTS      (1 << 1)
#define XML_DECLARATION_BITS_STANDALONE_EXISTS    (1 << 2)
#define XML_DECLARATION_BITS_STANDALONE_YES       (1 << 3)


class nsEventListenerManager;
class nsDOMStyleSheetList;
class nsDOMStyleSheetSetList;
class nsIOutputStream;
class nsDocument;
class nsIDTD;
class nsIRadioVisitor;
class nsIFormControl;
struct nsRadioGroupStruct;
class nsOnloadBlocker;
class nsUnblockOnloadEvent;
class nsChildContentList;
class nsHTMLStyleSheet;
class nsHTMLCSSStyleSheet;
class nsDOMNavigationTiming;
class nsWindowSizes;
class nsHtml5TreeOpExecutor;
class nsDocumentOnStack;

namespace mozilla {
namespace dom {
class UndoManager;
}
}













class nsIdentifierMapEntry : public nsStringHashKey
{
public:
  typedef mozilla::dom::Element Element;
  
  nsIdentifierMapEntry(const nsAString& aKey) :
    nsStringHashKey(&aKey), mNameContentList(nullptr)
  {
  }
  nsIdentifierMapEntry(const nsAString *aKey) :
    nsStringHashKey(aKey), mNameContentList(nullptr)
  {
  }
  nsIdentifierMapEntry(const nsIdentifierMapEntry& aOther) :
    nsStringHashKey(&aOther.GetKey())
  {
    NS_ERROR("Should never be called");
  }
  ~nsIdentifierMapEntry();

  void SetInvalidName();
  bool IsInvalidName();
  void AddNameElement(nsIDocument* aDocument, Element* aElement);
  void RemoveNameElement(Element* aElement);
  bool IsEmpty();
  nsBaseContentList* GetNameContentList() {
    return mNameContentList;
  }

  



  Element* GetIdElement();
  


  const nsSmallVoidArray* GetIdElements() const {
    return &mIdContentList;
  }
  



  Element* GetImageIdElement();
  


  void AppendAllIdContent(nsCOMArray<nsIContent>* aElements);
  




  bool AddIdElement(Element* aElement);
  


  void RemoveIdElement(Element* aElement);
  



  void SetImageElement(Element* aElement);

  bool HasContentChangeCallback() { return mChangeCallbacks != nullptr; }
  void AddContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                void* aData, bool aForImage);
  void RemoveContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                void* aData, bool aForImage);

  void Traverse(nsCycleCollectionTraversalCallback* aCallback);

  void SetDocAllList(nsContentList* aContentList) { mDocAllList = aContentList; }
  nsContentList* GetDocAllList() { return mDocAllList; }

  struct ChangeCallback {
    nsIDocument::IDTargetObserver mCallback;
    void* mData;
    bool mForImage;
  };

  struct ChangeCallbackEntry : public PLDHashEntryHdr {
    typedef const ChangeCallback KeyType;
    typedef const ChangeCallback* KeyTypePointer;

    ChangeCallbackEntry(const ChangeCallback* key) :
      mKey(*key) { }
    ChangeCallbackEntry(const ChangeCallbackEntry& toCopy) :
      mKey(toCopy.mKey) { }

    KeyType GetKey() const { return mKey; }
    bool KeyEquals(KeyTypePointer aKey) const {
      return aKey->mCallback == mKey.mCallback &&
             aKey->mData == mKey.mData &&
             aKey->mForImage == mKey.mForImage;
    }

    static KeyTypePointer KeyToPointer(KeyType& aKey) { return &aKey; }
    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      return mozilla::HashGeneric(aKey->mCallback, aKey->mData);
    }
    enum { ALLOW_MEMMOVE = true };
    
    ChangeCallback mKey;
  };

  static size_t SizeOfExcludingThis(nsIdentifierMapEntry* aEntry,
                                    nsMallocSizeOfFun aMallocSizeOf,
                                    void* aArg);

private:
  void FireChangeCallbacks(Element* aOldElement, Element* aNewElement,
                           bool aImageOnly = false);

  
  
  nsSmallVoidArray mIdContentList;
  nsRefPtr<nsBaseContentList> mNameContentList;
  nsRefPtr<nsContentList> mDocAllList;
  nsAutoPtr<nsTHashtable<ChangeCallbackEntry> > mChangeCallbacks;
  nsRefPtr<Element> mImageElement;
};

class nsDocHeaderData
{
public:
  nsDocHeaderData(nsIAtom* aField, const nsAString& aData)
    : mField(aField), mData(aData), mNext(nullptr)
  {
  }

  ~nsDocHeaderData(void)
  {
    delete mNext;
  }

  nsCOMPtr<nsIAtom> mField;
  nsString          mData;
  nsDocHeaderData*  mNext;
};

class nsDOMStyleSheetList : public nsIDOMStyleSheetList,
                            public nsStubDocumentObserver
{
public:
  nsDOMStyleSheetList(nsIDocument *aDocument);
  virtual ~nsDOMStyleSheetList();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMSTYLESHEETLIST

  
  NS_DECL_NSIDOCUMENTOBSERVER_STYLESHEETADDED
  NS_DECL_NSIDOCUMENTOBSERVER_STYLESHEETREMOVED

  
  NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

  nsIStyleSheet* GetItemAt(uint32_t aIndex);

  static nsDOMStyleSheetList* FromSupports(nsISupports* aSupports)
  {
    nsIDOMStyleSheetList* list = static_cast<nsIDOMStyleSheetList*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMStyleSheetList> list_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(list_qi == list, "Uh, fix QI!");
    }
#endif
    return static_cast<nsDOMStyleSheetList*>(list);
  }

protected:
  int32_t       mLength;
  nsIDocument*  mDocument;
};

class nsOnloadBlocker MOZ_FINAL : public nsIRequest
{
public:
  nsOnloadBlocker() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUEST

private:
  ~nsOnloadBlocker() {}
};

class nsExternalResourceMap
{
public:
  typedef nsIDocument::ExternalResourceLoad ExternalResourceLoad;
  nsExternalResourceMap();

  



  nsIDocument* RequestResource(nsIURI* aURI,
                               nsINode* aRequestingNode,
                               nsDocument* aDisplayDocument,
                               ExternalResourceLoad** aPendingLoad);

  



  void EnumerateResources(nsIDocument::nsSubDocEnumFunc aCallback, void* aData);

  


  void Traverse(nsCycleCollectionTraversalCallback* aCallback) const;

  



  void Shutdown()
  {
    mPendingLoads.Clear();
    mMap.Clear();
    mHaveShutDown = true;
  }

  bool HaveShutDown() const
  {
    return mHaveShutDown;
  }

  
  struct ExternalResource
  {
    ~ExternalResource();
    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsIContentViewer> mViewer;
    nsCOMPtr<nsILoadGroup> mLoadGroup;
  };

  
  void HideViewers();

  
  void ShowViewers();

protected:
  class PendingLoad : public ExternalResourceLoad,
                      public nsIStreamListener
  {
  public:
    PendingLoad(nsDocument* aDisplayDocument) :
      mDisplayDocument(aDisplayDocument)
    {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    



    nsresult StartLoad(nsIURI* aURI, nsINode* aRequestingNode);

    



    nsresult SetupViewer(nsIRequest* aRequest, nsIContentViewer** aViewer,
                         nsILoadGroup** aLoadGroup);

  private:
    nsRefPtr<nsDocument> mDisplayDocument;
    nsCOMPtr<nsIStreamListener> mTargetListener;
    nsCOMPtr<nsIURI> mURI;
  };
  friend class PendingLoad;

  class LoadgroupCallbacks MOZ_FINAL : public nsIInterfaceRequestor
  {
  public:
    LoadgroupCallbacks(nsIInterfaceRequestor* aOtherCallbacks)
      : mCallbacks(aOtherCallbacks)
    {}
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR
  private:
    
    
    
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;

    
    
    
    
    
    
#define DECL_SHIM(_i, _allcaps)                                              \
    class _i##Shim MOZ_FINAL : public nsIInterfaceRequestor,                 \
                               public _i                                     \
    {                                                                        \
    public:                                                                  \
      _i##Shim(nsIInterfaceRequestor* aIfreq, _i* aRealPtr)                  \
        : mIfReq(aIfreq), mRealPtr(aRealPtr)                                 \
      {                                                                      \
        NS_ASSERTION(mIfReq, "Expected non-null here");                      \
        NS_ASSERTION(mRealPtr, "Expected non-null here");                    \
      }                                                                      \
      NS_DECL_ISUPPORTS                                                      \
      NS_FORWARD_NSIINTERFACEREQUESTOR(mIfReq->)                             \
      NS_FORWARD_##_allcaps(mRealPtr->)                                      \
    private:                                                                 \
      nsCOMPtr<nsIInterfaceRequestor> mIfReq;                                \
      nsCOMPtr<_i> mRealPtr;                                                 \
    };

    DECL_SHIM(nsILoadContext, NSILOADCONTEXT)
    DECL_SHIM(nsIProgressEventSink, NSIPROGRESSEVENTSINK)
    DECL_SHIM(nsIChannelEventSink, NSICHANNELEVENTSINK)
    DECL_SHIM(nsISecurityEventSink, NSISECURITYEVENTSINK)
    DECL_SHIM(nsIApplicationCacheContainer, NSIAPPLICATIONCACHECONTAINER)
#undef DECL_SHIM
  };
  
  





  nsresult AddExternalResource(nsIURI* aURI, nsIContentViewer* aViewer,
                               nsILoadGroup* aLoadGroup,
                               nsIDocument* aDisplayDocument);
  
  nsClassHashtable<nsURIHashKey, ExternalResource> mMap;
  nsRefPtrHashtable<nsURIHashKey, PendingLoad> mPendingLoads;
  bool mHaveShutDown;
};










class nsDocument : public nsIDocument,
                   public nsIDOMXMLDocument, 
                   public nsIDOMDocumentXBL,
                   public nsSupportsWeakReference,
                   public nsIScriptObjectPrincipal,
                   public nsIRadioGroupContainer,
                   public nsIApplicationCacheContainer,
                   public nsStubMutationObserver,
                   public nsIDOMDocumentTouch,
                   public nsIInlineEventHandlers,
                   public nsIDocumentRegister,
                   public nsIObserver
{
public:
  typedef mozilla::dom::Element Element;
  using nsIDocument::GetElementsByTagName;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_SIZEOF_EXCLUDING_THIS

  virtual void Reset(nsIChannel *aChannel, nsILoadGroup *aLoadGroup);
  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                          nsIPrincipal* aPrincipal);

  
  
  
  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     bool aReset = true,
                                     nsIContentSink* aContentSink = nullptr) = 0;

  virtual void StopDocumentLoad();

  virtual void NotifyPossibleTitleChange(bool aBoundTitleElement);

  virtual void SetDocumentURI(nsIURI* aURI);

  


  virtual void SetPrincipal(nsIPrincipal *aPrincipal);

  


  
  

  


  virtual void SetContentType(const nsAString& aContentType);

  virtual nsresult SetBaseURI(nsIURI* aURI);

  


  virtual void GetBaseTarget(nsAString &aBaseTarget);

  



  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID);

  


  virtual nsresult AddCharSetObserver(nsIObserver* aObserver);

  


  virtual void RemoveCharSetObserver(nsIObserver* aObserver);

  virtual Element* AddIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                       void* aData, bool aForImage);
  virtual void RemoveIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                      void* aData, bool aForImage);

  



  virtual void GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const;
  virtual void SetHeaderData(nsIAtom* aheaderField,
                             const nsAString& aData);

  




  virtual nsresult CreateShell(nsPresContext* aContext,
                               nsViewManager* aViewManager,
                               nsStyleSet* aStyleSet,
                               nsIPresShell** aInstancePtrResult);
  virtual void DeleteShell();

  virtual nsresult GetAllowPlugins(bool* aAllowPlugins);

  virtual already_AddRefed<mozilla::dom::UndoManager> GetUndoManager();

  virtual nsresult SetSubDocumentFor(Element* aContent,
                                     nsIDocument* aSubDoc);
  virtual nsIDocument* GetSubDocumentFor(nsIContent* aContent) const;
  virtual Element* FindContentForSubDocument(nsIDocument *aDocument) const;
  virtual Element* GetRootElementInternal() const;

  



  virtual int32_t GetNumberOfStyleSheets() const;
  virtual nsIStyleSheet* GetStyleSheetAt(int32_t aIndex) const;
  virtual int32_t GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const;
  virtual void AddStyleSheet(nsIStyleSheet* aSheet);
  virtual void RemoveStyleSheet(nsIStyleSheet* aSheet);

  virtual void UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                                 nsCOMArray<nsIStyleSheet>& aNewSheets);
  virtual void AddStyleSheetToStyleSets(nsIStyleSheet* aSheet);
  virtual void RemoveStyleSheetFromStyleSets(nsIStyleSheet* aSheet);

  virtual void InsertStyleSheetAt(nsIStyleSheet* aSheet, int32_t aIndex);
  virtual void SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                            bool aApplicable);

  virtual int32_t GetNumberOfCatalogStyleSheets() const;
  virtual nsIStyleSheet* GetCatalogStyleSheetAt(int32_t aIndex) const;
  virtual void AddCatalogStyleSheet(nsCSSStyleSheet* aSheet);
  virtual void EnsureCatalogStyleSheet(const char *aStyleSheetURI);

  virtual nsresult LoadAdditionalStyleSheet(additionalSheetType aType, nsIURI* aSheetURI);
  virtual void RemoveAdditionalStyleSheet(additionalSheetType aType, nsIURI* sheetURI);
  virtual nsIStyleSheet* FirstAdditionalAuthorSheet();

  virtual nsIChannel* GetChannel() const {
    return mChannel;
  }

  



  virtual nsHTMLCSSStyleSheet* GetInlineStyleSheet() const {
    return mStyleAttrStyleSheet;
  }
  
  




  virtual nsIScriptGlobalObject* GetScriptGlobalObject() const;
  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject);

  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject);

  virtual nsIScriptGlobalObject* GetScopeObject() const;

  


  virtual nsScriptLoader* ScriptLoader();

  


  virtual void AddToIdTable(Element* aElement, nsIAtom* aId);
  virtual void RemoveFromIdTable(Element* aElement, nsIAtom* aId);
  virtual void AddToNameTable(Element* aElement, nsIAtom* aName);
  virtual void RemoveFromNameTable(Element* aElement, nsIAtom* aName);

  




  virtual void AddObserver(nsIDocumentObserver* aObserver);

  



  virtual bool RemoveObserver(nsIDocumentObserver* aObserver);

  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType);
  virtual void EndUpdate(nsUpdateType aUpdateType);
  virtual void BeginLoad();
  virtual void EndLoad();

  virtual void SetReadyStateInternal(ReadyState rs);

  virtual void ContentStateChanged(nsIContent* aContent,
                                   nsEventStates aStateMask);
  virtual void DocumentStatesChanged(nsEventStates aStateMask);

  virtual void StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule);
  virtual void StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);
  virtual void StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule);

  virtual void FlushPendingNotifications(mozFlushType aType);
  virtual void FlushExternalResources(mozFlushType aType);
  virtual void SetXMLDeclaration(const PRUnichar *aVersion,
                                 const PRUnichar *aEncoding,
                                 const int32_t aStandalone);
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone);
  virtual bool IsScriptEnabled();

  virtual void OnPageShow(bool aPersisted, nsIDOMEventTarget* aDispatchStartTarget);
  virtual void OnPageHide(bool aPersisted, nsIDOMEventTarget* aDispatchStartTarget);
  
  virtual void WillDispatchMutationEvent(nsINode* aTarget);
  virtual void MutationEventDispatched(nsINode* aTarget);

  
  virtual bool IsNodeOfType(uint32_t aFlags) const;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const MOZ_OVERRIDE;
  virtual uint32_t GetChildCount() const;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify);
  virtual nsresult AppendChildTo(nsIContent* aKid, bool aNotify);
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  NS_IMETHOD WalkRadioGroup(const nsAString& aName,
                            nsIRadioVisitor* aVisitor,
                            bool aFlushContent);
  virtual void SetCurrentRadioButton(const nsAString& aName,
                                     nsIDOMHTMLInputElement* aRadio);
  virtual nsIDOMHTMLInputElement* GetCurrentRadioButton(const nsAString& aName);
  NS_IMETHOD GetNextRadioButton(const nsAString& aName,
                                const bool aPrevious,
                                nsIDOMHTMLInputElement*  aFocusedRadio,
                                nsIDOMHTMLInputElement** aRadioOut);
  virtual void AddToRadioGroup(const nsAString& aName,
                               nsIFormControl* aRadio);
  virtual void RemoveFromRadioGroup(const nsAString& aName,
                                    nsIFormControl* aRadio);
  virtual uint32_t GetRequiredRadioCount(const nsAString& aName) const;
  virtual void RadioRequiredChanged(const nsAString& aName,
                                    nsIFormControl* aRadio);
  virtual bool GetValueMissingState(const nsAString& aName) const;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue);

  
  nsRadioGroupStruct* GetRadioGroup(const nsAString& aName) const;
  nsRadioGroupStruct* GetOrCreateRadioGroup(const nsAString& aName);

  virtual nsViewportInfo GetViewportInfo(uint32_t aDisplayWidth,
                                         uint32_t aDisplayHeight);


private:
  nsRadioGroupStruct* GetRadioGroupInternal(const nsAString& aName) const;

public:
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE_OVERRIDABLE

  
  NS_DECL_NSIDOMDOCUMENT

  
  NS_DECL_NSIDOMXMLDOCUMENT

  
  NS_DECL_NSIDOMDOCUMENTXBL

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsEventListenerManager*
    GetListenerManager(bool aCreateIfNotFound);

  
  virtual nsIPrincipal* GetPrincipal();

  
  NS_DECL_NSIAPPLICATIONCACHECONTAINER

  
  NS_DECL_NSITOUCHEVENTRECEIVER

  
  NS_DECL_NSIDOMDOCUMENTTOUCH

  
  NS_DECL_NSIINLINEEVENTHANDLERS

  
  NS_DECL_NSIDOCUMENTREGISTER

  
  NS_DECL_NSIOBSERVER

  virtual nsresult Init();

  virtual nsresult CreateElem(const nsAString& aName, nsIAtom *aPrefix,
                              int32_t aNamespaceID,
                              nsIContent **aResult);

  nsresult CreateTextNode(const nsAString& aData, nsIContent** aReturn);

  virtual NS_HIDDEN_(nsresult) Sanitize();

  virtual NS_HIDDEN_(void) EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                                 void *aData);

  virtual NS_HIDDEN_(bool) CanSavePresentation(nsIRequest *aNewRequest);
  virtual NS_HIDDEN_(void) Destroy();
  virtual NS_HIDDEN_(void) RemovedFromDocShell();
  virtual NS_HIDDEN_(already_AddRefed<nsILayoutHistoryState>) GetLayoutHistoryState() const;

  virtual NS_HIDDEN_(void) BlockOnload();
  virtual NS_HIDDEN_(void) UnblockOnload(bool aFireSync);

  virtual NS_HIDDEN_(void) AddStyleRelevantLink(mozilla::dom::Link* aLink);
  virtual NS_HIDDEN_(void) ForgetLink(mozilla::dom::Link* aLink);

  NS_HIDDEN_(void) ClearBoxObjectFor(nsIContent* aContent);
  NS_IMETHOD GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult);

  virtual NS_HIDDEN_(nsresult) GetXBLChildNodesFor(nsIContent* aContent,
                                                   nsIDOMNodeList** aResult);
  virtual NS_HIDDEN_(nsresult) GetContentListFor(nsIContent* aContent,
                                                 nsIDOMNodeList** aResult);
  virtual NS_HIDDEN_(Element*)
    GetAnonymousElementByAttribute(nsIContent* aElement,
                                   nsIAtom* aAttrName,
                                   const nsAString& aAttrValue) const;

  virtual NS_HIDDEN_(Element*) ElementFromPointHelper(float aX, float aY,
                                                      bool aIgnoreRootScrollFrame,
                                                      bool aFlushLayout);

  virtual NS_HIDDEN_(nsresult) NodesFromRectHelper(float aX, float aY,
                                                   float aTopSize, float aRightSize,
                                                   float aBottomSize, float aLeftSize,
                                                   bool aIgnoreRootScrollFrame,
                                                   bool aFlushLayout,
                                                   nsIDOMNodeList** aReturn);

  virtual NS_HIDDEN_(void) FlushSkinBindings();

  virtual NS_HIDDEN_(nsresult) InitializeFrameLoader(nsFrameLoader* aLoader);
  virtual NS_HIDDEN_(nsresult) FinalizeFrameLoader(nsFrameLoader* aLoader);
  virtual NS_HIDDEN_(void) TryCancelFrameLoaderInitialization(nsIDocShell* aShell);
  virtual NS_HIDDEN_(bool) FrameLoaderScheduledToBeFinalized(nsIDocShell* aShell);
  virtual NS_HIDDEN_(nsIDocument*)
    RequestExternalResource(nsIURI* aURI,
                            nsINode* aRequestingNode,
                            ExternalResourceLoad** aPendingLoad);
  virtual NS_HIDDEN_(void)
    EnumerateExternalResources(nsSubDocEnumFunc aCallback, void* aData);

  nsTArray<nsCString> mHostObjectURIs;

  
  
  nsSMILAnimationController* GetAnimationController();

  void SetImagesNeedAnimating(bool aAnimating);

  virtual void SuppressEventHandling(uint32_t aIncrease);

  virtual void UnsuppressEventHandlingAndFireEvents(bool aFireEvents);
  
  void DecreaseEventSuppression() {
    --mEventsSuppressed;
    MaybeRescheduleAnimationFrameNotifications();
  }

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDocument,
                                                                   nsIDocument)

  void DoNotifyPossibleTitleChange();

  nsExternalResourceMap& ExternalResourceMap()
  {
    return mExternalResourceMap;
  }

  void SetLoadedAsData(bool aLoadedAsData) { mLoadedAsData = aLoadedAsData; }
  void SetLoadedAsInteractiveData(bool aLoadedAsInteractiveData)
  {
    mLoadedAsInteractiveData = aLoadedAsInteractiveData;
  }

  nsresult CloneDocHelper(nsDocument* clone) const;

  void MaybeInitializeFinalizeFrameLoaders();

  void MaybeEndOutermostXBLUpdate();

  virtual void MaybePreLoadImage(nsIURI* uri,
                                 const nsAString &aCrossOriginAttr);

  virtual void PreloadStyle(nsIURI* uri, const nsAString& charset,
                            const nsAString& aCrossOriginAttr);

  virtual nsresult LoadChromeSheetSync(nsIURI* uri, bool isAgentSheet,
                                       nsCSSStyleSheet** sheet);

  virtual nsISupports* GetCurrentContentSink();

  virtual nsEventStates GetDocumentState();

  virtual void RegisterHostObjectUri(const nsACString& aUri);
  virtual void UnregisterHostObjectUri(const nsACString& aUri);

  
  void AsyncBlockOnload();

  virtual void SetScrollToRef(nsIURI *aDocumentURI);
  virtual void ScrollToRef();
  virtual void ResetScrolledToRefAlready();
  virtual void SetChangeScrollPosWhenScrollingToRef(bool aValue);

  virtual Element *GetElementById(const nsAString& aElementId);
  virtual const nsSmallVoidArray* GetAllElementsForId(const nsAString& aElementId) const;

  virtual Element *LookupImageElement(const nsAString& aElementId);
  virtual void MozSetImageElement(const nsAString& aImageElementId,
                                  Element* aElement);

  virtual NS_HIDDEN_(nsresult) AddImage(imgIRequest* aImage);
  virtual NS_HIDDEN_(nsresult) RemoveImage(imgIRequest* aImage, uint32_t aFlags);
  virtual NS_HIDDEN_(nsresult) SetImageLockingState(bool aLocked);

  
  
  virtual nsresult AddPlugin(nsIObjectLoadingContent* aPlugin);
  
  
  virtual void RemovePlugin(nsIObjectLoadingContent* aPlugin);
  
  
  virtual void GetPlugins(nsTArray<nsIObjectLoadingContent*>& aPlugins);

  virtual nsresult GetStateObject(nsIVariant** aResult);

  virtual nsDOMNavigationTiming* GetNavigationTiming() const;
  virtual nsresult SetNavigationTiming(nsDOMNavigationTiming* aTiming);

  virtual Element* FindImageMap(const nsAString& aNormalizedMapName);

  virtual void NotifyAudioAvailableListener();

  bool HasAudioAvailableListeners()
  {
    return mHasAudioAvailableListener;
  }

  virtual Element* GetFullScreenElement();
  virtual void AsyncRequestFullScreen(Element* aElement);
  virtual void RestorePreviousFullScreenState();
  virtual bool IsFullscreenLeaf();
  virtual bool IsFullScreenDoc();
  virtual void SetApprovedForFullscreen(bool aIsApproved);
  virtual nsresult RemoteFrameFullscreenChanged(nsIDOMElement* aFrameElement,
                                                const nsAString& aNewOrigin);

  virtual nsresult RemoteFrameFullscreenReverted();
  virtual nsIDocument* GetFullscreenRoot();
  virtual void SetFullscreenRoot(nsIDocument* aRoot);

  static void ExitFullscreen(nsIDocument* aDoc);

  
  
  
  
  
  
  
  
  
  
  void RequestFullScreen(Element* aElement,
                         bool aWasCallerChrome,
                         bool aNotifyOnOriginChange);

  
  
  void CleanupFullscreenState();

  
  
  
  
  nsresult AddFullscreenApprovedObserver();
  nsresult RemoveFullscreenApprovedObserver();

  
  
  
  bool FullScreenStackPush(Element* aElement);

  
  
  
  void FullScreenStackPop();

  
  Element* FullScreenStackTop();

  
  virtual bool MozFullScreenEnabled();
  virtual Element* GetMozFullScreenElement(mozilla::ErrorResult& rv);

  void RequestPointerLock(Element* aElement);
  bool ShouldLockPointer(Element* aElement);
  bool SetPointerLock(Element* aElement, int aCursorStyle);
  static void UnlockPointer();

  
  
  void UpdateVisibilityState();
  
  virtual void PostVisibilityUpdateEvent();

  virtual void DocSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const;
  

  virtual nsIDOMNode* AsDOMNode() { return this; }

  JSObject* GetCustomPrototype(const nsAString& aElementName)
  {
    JSObject* prototype = nullptr;
    mCustomPrototypes.Get(aElementName, &prototype);
    return prototype;
  }

  static bool RegisterEnabled();

  
  virtual mozilla::dom::DOMImplementation*
    GetImplementation(mozilla::ErrorResult& rv);
  virtual JSObject*
  Register(JSContext* aCx, const nsAString& aName,
           const mozilla::dom::ElementRegistrationOptions& aOptions,
           mozilla::ErrorResult& rv);
  virtual nsIDOMStyleSheetList* StyleSheets();
  virtual void SetSelectedStyleSheetSet(const nsAString& aSheetSet);
  virtual void GetLastStyleSheetSet(nsString& aSheetSet);
  virtual nsIDOMDOMStringList* StyleSheetSets();
  virtual void EnableStyleSheetsForSet(const nsAString& aSheetSet);

protected:
  friend class nsNodeUtils;
  friend class nsDocumentOnStack;

  void IncreaseStackRefCnt()
  {
    ++mStackRefCnt;
  }

  void DecreaseStackRefCnt()
  {
    if (--mStackRefCnt == 0 && mNeedsReleaseAfterStackRefCntRelease) {
      mNeedsReleaseAfterStackRefCntRelease = false;
      NS_RELEASE_THIS();
    }
  }

  
  
  
  
  
  
  
  bool IsFullScreenEnabled(bool aIsCallerChrome, bool aLogFailure);

  




  inline bool CheckGetElementByIdArg(const nsAString& aId)
  {
    if (aId.IsEmpty()) {
      ReportEmptyGetElementByIdArg();
      return false;
    }
    return true;
  }

  void ReportEmptyGetElementByIdArg();

  void DispatchContentLoadedEvents();

  void RetrieveRelevantHeaders(nsIChannel *aChannel);

  void TryChannelCharset(nsIChannel *aChannel,
                         int32_t& aCharsetSource,
                         nsACString& aCharset,
                         nsHtml5TreeOpExecutor* aExecutor);

  
  
  void DestroyElementMaps();

  
  void RefreshLinkHrefs();

  nsIContent* GetFirstBaseNodeWithHref();
  nsresult SetFirstBaseNodeWithHref(nsIContent *node);

  
  
  nsIContent* GetTitleContent(uint32_t aNodeType);
  
  
  
  void GetTitleFromElement(uint32_t aNodeType, nsAString& aTitle);
public:
  
  virtual void GetTitle(nsString& aTitle);
  
  virtual void SetTitle(const nsAString& aTitle, mozilla::ErrorResult& rv);

protected:
  nsresult doCreateShell(nsPresContext* aContext,
                         nsViewManager* aViewManager, nsStyleSet* aStyleSet,
                         nsCompatibility aCompatMode,
                         nsIPresShell** aInstancePtrResult);

  void RemoveDocStyleSheetsFromStyleSets();
  void RemoveStyleSheetsFromStyleSets(nsCOMArray<nsIStyleSheet>& aSheets, 
                                      nsStyleSet::sheetType aType);
  nsresult ResetStylesheetsToURI(nsIURI* aURI);
  void FillStyleSet(nsStyleSet* aStyleSet);

  
  bool IsSafeToFlush() const;
  
  void DispatchPageTransition(nsIDOMEventTarget* aDispatchTarget,
                              const nsAString& aType,
                              bool aPersisted);

  virtual nsPIDOMWindow *GetWindowInternal() const;
  virtual nsPIDOMWindow *GetInnerWindowInternal();
  virtual nsIScriptGlobalObject* GetScriptHandlingObjectInternal() const;
  virtual bool InternalAllowXULXBL();

#define NS_DOCUMENT_NOTIFY_OBSERVERS(func_, params_)                        \
  NS_OBSERVER_ARRAY_NOTIFY_XPCOM_OBSERVERS(mObservers, nsIDocumentObserver, \
                                           func_, params_);
  
#ifdef DEBUG
  void VerifyRootContentState();
#endif

  nsDocument(const char* aContentType);
  virtual ~nsDocument();

  void EnsureOnloadBlocker();

  nsTArray<nsIObserver*> mCharSetObservers;

  PLDHashTable *mSubDocuments;

  
  nsAttrAndChildArray mChildren;

  
  
  nsCOMPtr<nsIParser> mParser;

  
  
  
  nsWeakPtr mWeakSink;

  nsCOMArray<nsIStyleSheet> mStyleSheets;
  nsCOMArray<nsIStyleSheet> mCatalogSheets;
  nsCOMArray<nsIStyleSheet> mAdditionalSheets[SheetTypeCount];

  
  nsTObserverArray<nsIDocumentObserver*> mObservers;

  
  
  
  nsWeakPtr mScriptObject;

  
  
  
  nsWeakPtr mScopeObject;

  
  
  static nsWeakPtr sPendingPointerLockDoc;

  
  
  
  static nsWeakPtr sPendingPointerLockElement;

  
  
  
  nsTArray<nsWeakPtr> mFullScreenStack;

  
  
  nsWeakPtr mFullscreenRoot;

  
  
  nsDataHashtable<nsStringHashKey, JSObject*> mCustomPrototypes;

  nsRefPtr<nsEventListenerManager> mListenerManager;
  nsCOMPtr<nsIDOMStyleSheetList> mDOMStyleSheets;
  nsRefPtr<nsDOMStyleSheetSetList> mStyleSheetSetList;
  nsRefPtr<nsScriptLoader> mScriptLoader;
  nsDocHeaderData* mHeaderData;
  






  nsTHashtable<nsIdentifierMapEntry> mIdentifierMap;

  nsClassHashtable<nsStringHashKey, nsRadioGroupStruct> mRadioGroups;

  
  mozilla::TimeStamp mLoadingTimeStamp;

  
  bool mIsGoingAway:1;
  
  bool mInDestructor:1;

  
  
  bool mMayHaveTitleElement:1;

  bool mHasWarnedAboutBoxObjects:1;

  bool mDelayFrameLoaderInitialization:1;

  bool mSynchronousDOMContentLoaded:1;

  bool mInXBLUpdate:1;

  
  bool mLockingImages:1;

  
  bool mAnimatingImages:1;

  
  
  bool mHasAudioAvailableListener:1;

  
  
  bool mInFlush:1;

  
  
  bool mParserAborted:1;

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mIsApprovedForFullscreen:1;

  
  
  
  bool mHasFullscreenApprovedObserver:1;

  uint8_t mXMLDeclarationBits;

  nsInterfaceHashtable<nsPtrHashKey<nsIContent>, nsPIBoxObject> *mBoxObjectTable;

  
  nsCOMPtr<nsIChannel> mChannel;
  nsRefPtr<nsHTMLCSSStyleSheet> mStyleAttrStyleSheet;

  
  uint32_t mUpdateNestLevel;

  
  
  nsCOMPtr<nsIApplicationCache> mApplicationCache;

  nsCOMPtr<nsIContent> mFirstBaseNodeWithHref;

  nsEventStates mDocumentState;
  nsEventStates mGotDocumentState;

  nsRefPtr<nsDOMNavigationTiming> mTiming;
private:
  friend class nsUnblockOnloadEvent;
  
  mozilla::dom::VisibilityState GetVisibilityState() const;

  void PostUnblockOnloadEvent();
  void DoUnblockOnload();

  nsresult CheckFrameOptions();
  nsresult InitCSP(nsIChannel* aChannel);

  
  
  
  
  
  static nsresult SetPendingPointerLockRequest(Element* aElement);

  
  static void ClearPendingPointerLockRequest(bool aDispatchErrorEvents);

  







  nsIContent* GetContentInThisDocument(nsIFrame* aFrame) const;

  
  
  
  void EnableStyleSheetsForSetInternal(const nsAString& aSheetSet,
                                       bool aUpdateCSSLoader);

  
  void RevokeAnimationFrameNotifications();
  
  
  void MaybeRescheduleAnimationFrameNotifications();

  
  nsDocument(const nsDocument& aOther);
  nsDocument& operator=(const nsDocument& aOther);

  nsCOMPtr<nsISupports> mXPathEvaluatorTearoff;

  
  
  
  
  nsCOMPtr<nsILayoutHistoryState> mLayoutHistoryState;

  
  uint32_t mOnloadBlockCount;
  
  uint32_t mAsyncOnloadBlockCount;
  nsCOMPtr<nsIRequest> mOnloadBlocker;

  
  nsTHashtable<nsPtrHashKey<mozilla::dom::Link> > mStyledLinks;
#ifdef DEBUG
  
  
  
  bool mStyledLinksCleared;
#endif

  
  nsString mLastStyleSheetSet;

  nsTArray<nsRefPtr<nsFrameLoader> > mInitializableFrameLoaders;
  nsTArray<nsRefPtr<nsFrameLoader> > mFinalizableFrameLoaders;
  nsRefPtr<nsRunnableMethod<nsDocument> > mFrameLoaderRunner;

  nsRevocableEventPtr<nsRunnableMethod<nsDocument, void, false> >
    mPendingTitleChangeEvent;

  nsExternalResourceMap mExternalResourceMap;

  
  nsCOMArray<imgIRequest> mPreloadingImages;

  nsRefPtr<mozilla::dom::DOMImplementation> mDOMImplementation;

  nsRefPtr<nsContentList> mImageMaps;

  nsCString mScrollToRef;
  uint8_t mScrolledToRefAlready : 1;
  uint8_t mChangeScrollPosWhenScrollingToRef : 1;

  
  nsDataHashtable< nsPtrHashKey<imgIRequest>, uint32_t> mImageTracker;

  
  nsTHashtable< nsPtrHashKey<nsIObjectLoadingContent> > mPlugins;

  nsRefPtr<mozilla::dom::UndoManager> mUndoManager;

  enum ViewportType {
    DisplayWidthHeight,
    Specified,
    Unknown
  };

  ViewportType mViewportType;

  
  
  bool mValidWidth, mValidHeight;
  float mScaleMinFloat, mScaleMaxFloat, mScaleFloat, mPixelRatio;
  bool mAutoSize, mAllowZoom, mValidScaleFloat, mValidMaxScale, mScaleStrEmpty, mWidthStrEmpty;
  uint32_t mViewportWidth, mViewportHeight;

  nsrefcnt mStackRefCnt;
  bool mNeedsReleaseAfterStackRefCntRelease;

#ifdef DEBUG
protected:
  bool mWillReparent;
#endif
};

class nsDocumentOnStack
{
public:
  nsDocumentOnStack(nsDocument* aDoc) : mDoc(aDoc)
  {
    mDoc->IncreaseStackRefCnt();
  }
  ~nsDocumentOnStack()
  {
    mDoc->DecreaseStackRefCnt();
  }
private:
  nsDocument* mDoc;
};

#define NS_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                             \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMDocument, nsDocument)      \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMEventTarget, nsDocument)   \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMNode, nsDocument)

#endif 
