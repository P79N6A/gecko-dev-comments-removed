








































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
#include "nsHashSets.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMNSDocumentStyle.h"
#include "nsStubDocumentObserver.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMNSEventTarget.h"
#include "nsIDOMStyleSheetList.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMEventTarget.h"
#include "nsIContent.h"
#include "nsIEventListenerManager.h"
#include "nsIDOM3Node.h"
#include "nsIDOMNodeSelector.h"
#include "nsIPrincipal.h"
#include "nsIParser.h"
#include "nsBindingManager.h"
#include "nsINodeInfo.h"
#include "nsHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"
#include "nsScriptLoader.h"
#include "nsIRadioGroupContainer.h"
#include "nsIScriptEventManager.h"
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
#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "nsIDocumentViewer.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadContext.h"
#include "nsIProgressEventSink.h"
#include "nsISecurityEventSink.h"
#include "nsIChannelEventSink.h"
#include "imgIRequest.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMTouchEvent.h"

#define XML_DECLARATION_BITS_DECLARATION_EXISTS   (1 << 0)
#define XML_DECLARATION_BITS_ENCODING_EXISTS      (1 << 1)
#define XML_DECLARATION_BITS_STANDALONE_EXISTS    (1 << 2)
#define XML_DECLARATION_BITS_STANDALONE_YES       (1 << 3)


class nsIEventListenerManager;
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
class nsXMLEventsManager;
class nsHTMLStyleSheet;
class nsHTMLCSSStyleSheet;













class nsIdentifierMapEntry : public nsStringHashKey
{
public:
  typedef mozilla::dom::Element Element;
  
  nsIdentifierMapEntry(const nsAString& aKey) :
    nsStringHashKey(&aKey), mNameContentList(nsnull)
  {
  }
  nsIdentifierMapEntry(const nsAString *aKey) :
    nsStringHashKey(aKey), mNameContentList(nsnull)
  {
  }
  nsIdentifierMapEntry(const nsIdentifierMapEntry& aOther) :
    nsStringHashKey(&aOther.GetKey())
  {
    NS_ERROR("Should never be called");
  }
  ~nsIdentifierMapEntry();

  void SetInvalidName();
  PRBool IsInvalidName();
  void AddNameElement(nsIDocument* aDocument, Element* aElement);
  void RemoveNameElement(Element* aElement);
  PRBool IsEmpty();
  nsBaseContentList* GetNameContentList() {
    return mNameContentList;
  }

  



  Element* GetIdElement();
  



  Element* GetImageIdElement();
  


  void AppendAllIdContent(nsCOMArray<nsIContent>* aElements);
  




  PRBool AddIdElement(Element* aElement);
  


  void RemoveIdElement(Element* aElement);
  



  void SetImageElement(Element* aElement);

  PRBool HasContentChangeCallback() { return mChangeCallbacks != nsnull; }
  void AddContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                void* aData, PRBool aForImage);
  void RemoveContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                void* aData, PRBool aForImage);

  void Traverse(nsCycleCollectionTraversalCallback* aCallback);

  void SetDocAllList(nsContentList* aContentList) { mDocAllList = aContentList; }
  nsContentList* GetDocAllList() { return mDocAllList; }

  struct ChangeCallback {
    nsIDocument::IDTargetObserver mCallback;
    void* mData;
    PRBool mForImage;
  };

  struct ChangeCallbackEntry : public PLDHashEntryHdr {
    typedef const ChangeCallback KeyType;
    typedef const ChangeCallback* KeyTypePointer;

    ChangeCallbackEntry(const ChangeCallback* key) :
      mKey(*key) { }
    ChangeCallbackEntry(const ChangeCallbackEntry& toCopy) :
      mKey(toCopy.mKey) { }

    KeyType GetKey() const { return mKey; }
    PRBool KeyEquals(KeyTypePointer aKey) const {
      return aKey->mCallback == mKey.mCallback &&
             aKey->mData == mKey.mData &&
             aKey->mForImage == mKey.mForImage;
    }

    static KeyTypePointer KeyToPointer(KeyType& aKey) { return &aKey; }
    static PLDHashNumber HashKey(KeyTypePointer aKey)
    {
      return (NS_PTR_TO_INT32(aKey->mCallback) >> 2) ^
             (NS_PTR_TO_INT32(aKey->mData));
    }
    enum { ALLOW_MEMMOVE = PR_TRUE };
    
    ChangeCallback mKey;
  };

private:
  void FireChangeCallbacks(Element* aOldElement, Element* aNewElement,
                           PRBool aImageOnly = PR_FALSE);

  
  
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
    : mField(aField), mData(aData), mNext(nsnull)
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

  nsIStyleSheet* GetItemAt(PRUint32 aIndex);

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
  PRInt32       mLength;
  nsIDocument*  mDocument;
};

class nsOnloadBlocker : public nsIRequest
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
    mHaveShutDown = PR_TRUE;
  }

  PRBool HaveShutDown() const
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

    



    nsresult SetupViewer(nsIRequest* aRequest, nsIDocumentViewer** aViewer,
                         nsILoadGroup** aLoadGroup);

  private:
    nsRefPtr<nsDocument> mDisplayDocument;
    nsCOMPtr<nsIStreamListener> mTargetListener;
    nsCOMPtr<nsIURI> mURI;
  };
  friend class PendingLoad;

  class LoadgroupCallbacks : public nsIInterfaceRequestor
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
    class _i##Shim : public nsIInterfaceRequestor,                           \
                     public _i                                               \
    {                                                                        \
    public:                                                                  \
      _i##Shim(nsIInterfaceRequestor* aIfreq, _i* aRealPtr)                  \
        : mIfReq(aIfreq), mRealPtr(aRealPtr)                                 \
      {                                                                      \
        NS_ASSERTION(mIfReq, "Expected non-null here");                      \
        NS_ASSERTION(mRealPtr, "Expected non-null here");                    \
      }                                                                      \
      NS_DECL_ISUPPORTS                                                      \
      NS_FORWARD_NSIINTERFACEREQUESTOR(mIfReq->);                            \
      NS_FORWARD_##_allcaps(mRealPtr->);                                     \
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
  
  





  nsresult AddExternalResource(nsIURI* aURI, nsIDocumentViewer* aViewer,
                               nsILoadGroup* aLoadGroup,
                               nsIDocument* aDisplayDocument);
  
  nsClassHashtable<nsURIHashKey, ExternalResource> mMap;
  nsRefPtrHashtable<nsURIHashKey, PendingLoad> mPendingLoads;
  PRPackedBool mHaveShutDown;
};










class nsDocument : public nsIDocument,
                   public nsIDOMXMLDocument, 
                   public nsIDOMNSDocument,
                   public nsIDOMNSDocumentStyle,
                   public nsIDOMDocumentXBL,
                   public nsSupportsWeakReference,
                   public nsIDOMEventTarget,
                   public nsIDOM3EventTarget,
                   public nsIDOMNSEventTarget,
                   public nsIScriptObjectPrincipal,
                   public nsIRadioGroupContainer_MOZILLA_2_0_BRANCH,
                   public nsIApplicationCacheContainer,
                   public nsStubMutationObserver,
                   public nsIDOMDocumentTouch
{
public:
  typedef mozilla::dom::Element Element;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  using nsINode::GetScriptTypeID;

  virtual void Reset(nsIChannel *aChannel, nsILoadGroup *aLoadGroup);
  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                          nsIPrincipal* aPrincipal);

  
  
  
  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     PRBool aReset = PR_TRUE,
                                     nsIContentSink* aContentSink = nsnull) = 0;

  virtual void StopDocumentLoad();

  virtual void NotifyPossibleTitleChange(PRBool aBoundTitleElement);

  virtual void SetDocumentURI(nsIURI* aURI);
  
  


  virtual void SetPrincipal(nsIPrincipal *aPrincipal);

  


  
  

  


  virtual void SetContentType(const nsAString& aContentType);

  virtual nsresult SetBaseURI(nsIURI* aURI);

  


  virtual void GetBaseTarget(nsAString &aBaseTarget);

  



  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID);

  


  virtual nsresult AddCharSetObserver(nsIObserver* aObserver);

  


  virtual void RemoveCharSetObserver(nsIObserver* aObserver);

  virtual Element* AddIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                       void* aData, PRBool aForImage);
  virtual void RemoveIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                      void* aData, PRBool aForImage);

  



  virtual void GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const;
  virtual void SetHeaderData(nsIAtom* aheaderField,
                             const nsAString& aData);

  




  virtual nsresult CreateShell(nsPresContext* aContext,
                               nsIViewManager* aViewManager,
                               nsStyleSet* aStyleSet,
                               nsIPresShell** aInstancePtrResult);
  virtual void DeleteShell();

  virtual nsresult SetSubDocumentFor(nsIContent *aContent,
                                     nsIDocument* aSubDoc);
  virtual nsIDocument* GetSubDocumentFor(nsIContent *aContent) const;
  virtual nsIContent* FindContentForSubDocument(nsIDocument *aDocument) const;
  virtual Element* GetRootElementInternal() const;

  



  virtual PRInt32 GetNumberOfStyleSheets() const;
  virtual nsIStyleSheet* GetStyleSheetAt(PRInt32 aIndex) const;
  virtual PRInt32 GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const;
  virtual void AddStyleSheet(nsIStyleSheet* aSheet);
  virtual void RemoveStyleSheet(nsIStyleSheet* aSheet);

  virtual void UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                                 nsCOMArray<nsIStyleSheet>& aNewSheets);
  virtual void AddStyleSheetToStyleSets(nsIStyleSheet* aSheet);
  virtual void RemoveStyleSheetFromStyleSets(nsIStyleSheet* aSheet);

  virtual void InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex);
  virtual void SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                            PRBool aApplicable);

  virtual PRInt32 GetNumberOfCatalogStyleSheets() const;
  virtual nsIStyleSheet* GetCatalogStyleSheetAt(PRInt32 aIndex) const;
  virtual void AddCatalogStyleSheet(nsIStyleSheet* aSheet);
  virtual void EnsureCatalogStyleSheet(const char *aStyleSheetURI);

  virtual nsIChannel* GetChannel() const {
    return mChannel;
  }

  



  virtual nsHTMLStyleSheet* GetAttributeStyleSheet() const {
    return mAttrStyleSheet;
  }

  



  virtual nsHTMLCSSStyleSheet* GetInlineStyleSheet() const {
    return mStyleAttrStyleSheet;
  }
  
  




  virtual nsIScriptGlobalObject* GetScriptGlobalObject() const;
  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject);

  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject);

  virtual nsIScriptGlobalObject* GetScopeObject();

  


  virtual nsScriptLoader* ScriptLoader();

  


  virtual void AddToIdTable(Element* aElement, nsIAtom* aId);
  virtual void RemoveFromIdTable(Element* aElement, nsIAtom* aId);
  virtual void AddToNameTable(Element* aElement, nsIAtom* aName);
  virtual void RemoveFromNameTable(Element* aElement, nsIAtom* aName);

  




  virtual void AddObserver(nsIDocumentObserver* aObserver);

  



  virtual PRBool RemoveObserver(nsIDocumentObserver* aObserver);

  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType);
  virtual void EndUpdate(nsUpdateType aUpdateType);
  virtual void BeginLoad();
  virtual void EndLoad();

  virtual void SetReadyStateInternal(ReadyState rs);
  virtual ReadyState GetReadyStateEnum();

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
  virtual nsIScriptEventManager* GetScriptEventManager();
  virtual void SetXMLDeclaration(const PRUnichar *aVersion,
                                 const PRUnichar *aEncoding,
                                 const PRInt32 aStandalone);
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone);
  virtual PRBool IsScriptEnabled();

  virtual void OnPageShow(PRBool aPersisted, nsIDOMEventTarget* aDispatchStartTarget);
  virtual void OnPageHide(PRBool aPersisted, nsIDOMEventTarget* aDispatchStartTarget);
  
  virtual void WillDispatchMutationEvent(nsINode* aTarget);
  virtual void MutationEventDispatched(nsINode* aTarget);

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
  virtual nsIContent *GetChildAt(PRUint32 aIndex) const;
  virtual nsIContent * const * GetChildArray(PRUint32* aChildCount) const;
  virtual PRInt32 IndexOf(nsINode* aPossibleChild) const;
  virtual PRUint32 GetChildCount() const;
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent = PR_TRUE);
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult DispatchDOMEvent(nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus);
  virtual nsIEventListenerManager* GetListenerManager(PRBool aCreateIfNotFound);
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID);
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID);
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup);
  virtual nsIScriptContext* GetContextForEventHandlers(nsresult* aRv)
  {
    return nsContentUtils::GetContextForEventHandlers(this, aRv);
  }
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  virtual PRBool IsEqualNode(nsINode* aOther);
  virtual void GetTextContent(nsAString &aTextContent);

  
  NS_IMETHOD WalkRadioGroup(const nsAString& aName,
                            nsIRadioVisitor* aVisitor,
                            PRBool aFlushContent);
  NS_IMETHOD SetCurrentRadioButton(const nsAString& aName,
                                   nsIDOMHTMLInputElement* aRadio);
  NS_IMETHOD GetCurrentRadioButton(const nsAString& aName,
                                   nsIDOMHTMLInputElement** aRadio);
  NS_IMETHOD GetPositionInGroup(nsIDOMHTMLInputElement *aRadio,
                                PRInt32 *aPositionIndex,
                                PRInt32 *aItemsInGroup);
  NS_IMETHOD GetNextRadioButton(const nsAString& aName,
                                const PRBool aPrevious,
                                nsIDOMHTMLInputElement*  aFocusedRadio,
                                nsIDOMHTMLInputElement** aRadioOut);
  NS_IMETHOD AddToRadioGroup(const nsAString& aName,
                             nsIFormControl* aRadio);
  NS_IMETHOD RemoveFromRadioGroup(const nsAString& aName,
                                  nsIFormControl* aRadio);
  virtual PRUint32 GetRequiredRadioCount(const nsAString& aName) const;
  virtual void RadioRequiredChanged(const nsAString& aName,
                                    nsIFormControl* aRadio);
  virtual bool GetValueMissingState(const nsAString& aName) const;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue);

  
  nsresult GetRadioGroup(const nsAString& aName,
                         nsRadioGroupStruct **aRadioGroup);

  
  NS_DECL_NSIDOMNODE

  
  NS_DECL_NSIDOMDOCUMENT

  
  NS_DECL_NSIDOMXMLDOCUMENT

  
  NS_DECL_NSIDOMNSDOCUMENT

  
  NS_DECL_NSIDOMDOCUMENTSTYLE

  
  NS_DECL_NSIDOMNSDOCUMENTSTYLE

  
  NS_DECL_NSIDOMDOCUMENTXBL

  
  NS_DECL_NSIDOMEVENTTARGET

  
  NS_DECL_NSIDOM3EVENTTARGET

  
  NS_DECL_NSIDOMNSEVENTTARGET

  
  virtual nsIPrincipal* GetPrincipal();

  
  NS_DECL_NSIAPPLICATIONCACHECONTAINER

  
  NS_DECL_NSIDOMDOCUMENTTOUCH

  virtual nsresult Init();
  
  virtual nsresult AddXMLEventsContent(nsIContent * aXMLEventsElement);

  virtual nsresult CreateElem(const nsAString& aName, nsIAtom *aPrefix,
                              PRInt32 aNamespaceID,
                              PRBool aDocumentDefaultType,
                              nsIContent **aResult);

  nsresult CreateElement(const nsAString& aTagName,
                         nsIContent** aReturn);
  nsresult CreateElementNS(const nsAString& aNamespaceURI,
                           const nsAString& aQualifiedName,
                           nsIContent** aReturn);

  nsresult CreateTextNode(const nsAString& aData, nsIContent** aReturn);

  virtual NS_HIDDEN_(nsresult) Sanitize();

  virtual NS_HIDDEN_(void) EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                                 void *aData);

  virtual NS_HIDDEN_(PRBool) CanSavePresentation(nsIRequest *aNewRequest);
  virtual NS_HIDDEN_(void) Destroy();
  virtual NS_HIDDEN_(void) RemovedFromDocShell();
  virtual NS_HIDDEN_(already_AddRefed<nsILayoutHistoryState>) GetLayoutHistoryState() const;

  virtual NS_HIDDEN_(void) BlockOnload();
  virtual NS_HIDDEN_(void) UnblockOnload(PRBool aFireSync);

  virtual NS_HIDDEN_(void) AddStyleRelevantLink(mozilla::dom::Link* aLink);
  virtual NS_HIDDEN_(void) ForgetLink(mozilla::dom::Link* aLink);

  NS_HIDDEN_(void) ClearBoxObjectFor(nsIContent* aContent);
  NS_IMETHOD GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult);

  virtual NS_HIDDEN_(nsresult) GetXBLChildNodesFor(nsIContent* aContent,
                                                   nsIDOMNodeList** aResult);
  virtual NS_HIDDEN_(nsresult) GetContentListFor(nsIContent* aContent,
                                                 nsIDOMNodeList** aResult);

  virtual NS_HIDDEN_(nsresult) ElementFromPointHelper(float aX, float aY,
                                                      PRBool aIgnoreRootScrollFrame,
                                                      PRBool aFlushLayout,
                                                      nsIDOMElement** aReturn);

  virtual NS_HIDDEN_(nsresult) NodesFromRectHelper(float aX, float aY,
                                                   float aTopSize, float aRightSize,
                                                   float aBottomSize, float aLeftSize,
                                                   PRBool aIgnoreRootScrollFrame,
                                                   PRBool aFlushLayout,
                                                   nsIDOMNodeList** aReturn);

  virtual NS_HIDDEN_(void) FlushSkinBindings();

  virtual NS_HIDDEN_(nsresult) InitializeFrameLoader(nsFrameLoader* aLoader);
  virtual NS_HIDDEN_(nsresult) FinalizeFrameLoader(nsFrameLoader* aLoader);
  virtual NS_HIDDEN_(void) TryCancelFrameLoaderInitialization(nsIDocShell* aShell);
  virtual NS_HIDDEN_(PRBool) FrameLoaderScheduledToBeFinalized(nsIDocShell* aShell);
  virtual NS_HIDDEN_(nsIDocument*)
    RequestExternalResource(nsIURI* aURI,
                            nsINode* aRequestingNode,
                            ExternalResourceLoad** aPendingLoad);
  virtual NS_HIDDEN_(void)
    EnumerateExternalResources(nsSubDocEnumFunc aCallback, void* aData);

  nsTArray<nsCString> mFileDataUris;

#ifdef MOZ_SMIL
  
  
  nsSMILAnimationController* GetAnimationController();
#endif 

  void SetImagesNeedAnimating(PRBool aAnimating);

  virtual void SuppressEventHandling(PRUint32 aIncrease);

  virtual void UnsuppressEventHandlingAndFireEvents(PRBool aFireEvents);
  
  void DecreaseEventSuppression() { --mEventsSuppressed; }

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDocument,
                                                         nsIDocument)

  void DoNotifyPossibleTitleChange();

  nsExternalResourceMap& ExternalResourceMap()
  {
    return mExternalResourceMap;
  }

  void SetLoadedAsData(PRBool aLoadedAsData) { mLoadedAsData = aLoadedAsData; }

  nsresult CloneDocHelper(nsDocument* clone) const;

  void MaybeInitializeFinalizeFrameLoaders();

  void MaybeEndOutermostXBLUpdate();

  virtual void MaybePreLoadImage(nsIURI* uri);

  virtual void PreloadStyle(nsIURI* uri, const nsAString& charset);

  virtual nsresult LoadChromeSheetSync(nsIURI* uri, PRBool isAgentSheet,
                                       nsCSSStyleSheet** sheet);

  virtual nsISupports* GetCurrentContentSink();

  virtual nsEventStates GetDocumentState();

  virtual void RegisterFileDataUri(const nsACString& aUri);
  virtual void UnregisterFileDataUri(const nsACString& aUri);

  
  void AsyncBlockOnload();

  virtual void SetScrollToRef(nsIURI *aDocumentURI);
  virtual void ScrollToRef();
  virtual void ResetScrolledToRefAlready();
  virtual void SetChangeScrollPosWhenScrollingToRef(PRBool aValue);

  already_AddRefed<nsContentList>
  GetElementsByTagName(const nsAString& aTagName) {
    return NS_GetContentList(this, kNameSpaceID_Unknown, aTagName);
  }
  already_AddRefed<nsContentList>
    GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                           const nsAString& aLocalName);

  virtual Element *GetElementById(const nsAString& aElementId);

  virtual Element *LookupImageElement(const nsAString& aElementId);

  virtual NS_HIDDEN_(nsresult) AddImage(imgIRequest* aImage);
  virtual NS_HIDDEN_(nsresult) RemoveImage(imgIRequest* aImage);
  virtual NS_HIDDEN_(nsresult) SetImageLockingState(PRBool aLocked);

  virtual nsresult GetStateObject(nsIVariant** aResult);

protected:
  friend class nsNodeUtils;

  




  inline PRBool CheckGetElementByIdArg(const nsAString& aId)
  {
    if (aId.IsEmpty()) {
      ReportEmptyGetElementByIdArg();
      return PR_FALSE;
    }
    return PR_TRUE;
  }

  void ReportEmptyGetElementByIdArg();

  void DispatchContentLoadedEvents();

  void RetrieveRelevantHeaders(nsIChannel *aChannel);

  static PRBool TryChannelCharset(nsIChannel *aChannel,
                                  PRInt32& aCharsetSource,
                                  nsACString& aCharset);

  
  
  void DestroyElementMaps();

  
  void RefreshLinkHrefs();

  nsIContent* GetFirstBaseNodeWithHref();
  nsresult SetFirstBaseNodeWithHref(nsIContent *node);

  
  
  nsIContent* GetTitleContent(PRUint32 aNodeType);
  
  
  
  void GetTitleFromElement(PRUint32 aNodeType, nsAString& aTitle);

  nsresult doCreateShell(nsPresContext* aContext,
                         nsIViewManager* aViewManager, nsStyleSet* aStyleSet,
                         nsCompatibility aCompatMode,
                         nsIPresShell** aInstancePtrResult);

  nsresult ResetStylesheetsToURI(nsIURI* aURI);
  void FillStyleSet(nsStyleSet* aStyleSet);

  
  PRBool IsSafeToFlush() const;
  
  virtual PRInt32 GetDefaultNamespaceID() const
  {
    return kNameSpaceID_None;
  }

  void DispatchPageTransition(nsPIDOMEventTarget* aDispatchTarget,
                              const nsAString& aType,
                              PRBool aPersisted);

  virtual nsPIDOMWindow *GetWindowInternal() const;
  virtual nsPIDOMWindow *GetInnerWindowInternal();
  virtual nsIScriptGlobalObject* GetScriptHandlingObjectInternal() const;
  virtual PRBool InternalAllowXULXBL();

#define NS_DOCUMENT_NOTIFY_OBSERVERS(func_, params_)                        \
  NS_OBSERVER_ARRAY_NOTIFY_XPCOM_OBSERVERS(mObservers, nsIDocumentObserver, \
                                           func_, params_);
  
#ifdef DEBUG
  void VerifyRootContentState();
#endif

  nsDocument(const char* aContentType);
  virtual ~nsDocument();

  void EnsureOnloadBlocker();

  nsCString mReferrer;
  nsString mLastModified;

  nsTArray<nsIObserver*> mCharSetObservers;

  PLDHashTable *mSubDocuments;

  
  nsAttrAndChildArray mChildren;

  
  
  nsCOMPtr<nsIParser> mParser;

  
  
  
  nsWeakPtr mWeakSink;

  nsCOMArray<nsIStyleSheet> mStyleSheets;
  nsCOMArray<nsIStyleSheet> mCatalogSheets;

  
  nsTObserverArray<nsIDocumentObserver*> mObservers;

  
  
  
  nsWeakPtr mScriptObject;

  
  
  
  nsWeakPtr mScopeObject;

  nsCOMPtr<nsIEventListenerManager> mListenerManager;
  nsCOMPtr<nsIDOMStyleSheetList> mDOMStyleSheets;
  nsRefPtr<nsDOMStyleSheetSetList> mStyleSheetSetList;
  nsRefPtr<nsScriptLoader> mScriptLoader;
  nsDocHeaderData* mHeaderData;
  






  nsTHashtable<nsIdentifierMapEntry> mIdentifierMap;

  nsClassHashtable<nsStringHashKey, nsRadioGroupStruct> mRadioGroups;

  
  PRPackedBool mIsGoingAway:1;
  
  PRPackedBool mInDestructor:1;

  
  
  PRPackedBool mMayHaveTitleElement:1;

  PRPackedBool mHasWarnedAboutBoxObjects:1;

  PRPackedBool mDelayFrameLoaderInitialization:1;

  PRPackedBool mSynchronousDOMContentLoaded:1;

  
  
  PRPackedBool mHaveInputEncoding:1;

  PRPackedBool mInXBLUpdate:1;

  
  
  
  PRPackedBool mLoadedAsInteractiveData:1;

  
  PRPackedBool mLockingImages:1;

  
  PRPackedBool mAnimatingImages:1;

  PRUint8 mXMLDeclarationBits;

  PRUint8 mDefaultElementType;

  nsInterfaceHashtable<nsVoidPtrHashKey, nsPIBoxObject> *mBoxObjectTable;

  
  nsCOMPtr<nsIChannel> mChannel;
  nsRefPtr<nsHTMLStyleSheet> mAttrStyleSheet;
  nsRefPtr<nsHTMLCSSStyleSheet> mStyleAttrStyleSheet;
  nsRefPtr<nsXMLEventsManager> mXMLEventsManager;

  nsCOMPtr<nsIScriptEventManager> mScriptEventManager;

  
  PRUint32 mUpdateNestLevel;

  
  
  nsCOMPtr<nsIApplicationCache> mApplicationCache;

  nsCOMPtr<nsIContent> mFirstBaseNodeWithHref;

  nsEventStates mDocumentState;
  nsEventStates mGotDocumentState;

private:
  friend class nsUnblockOnloadEvent;

  void PostUnblockOnloadEvent();
  void DoUnblockOnload();

  nsresult CheckFrameOptions();
  nsresult InitCSP();

  



  already_AddRefed<nsIDOMElement>
    CheckAncestryAndGetFrame(nsIDocument* aDocument) const;

  
  
  
  void EnableStyleSheetsForSetInternal(const nsAString& aSheetSet,
                                       PRBool aUpdateCSSLoader);

  
  void RevokeAnimationFrameNotifications();
  
  void RescheduleAnimationFrameNotifications();

  
  nsDocument(const nsDocument& aOther);
  nsDocument& operator=(const nsDocument& aOther);

  nsCOMPtr<nsISupports> mXPathEvaluatorTearoff;

  
  
  
  
  nsCOMPtr<nsILayoutHistoryState> mLayoutHistoryState;

  
  PRUint32 mOnloadBlockCount;
  
  PRUint32 mAsyncOnloadBlockCount;
  nsCOMPtr<nsIRequest> mOnloadBlocker;
  ReadyState mReadyState;

  
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

  nsCOMPtr<nsIDOMDOMImplementation> mDOMImplementation;

  nsCString mScrollToRef;
  PRUint8 mScrolledToRefAlready : 1;
  PRUint8 mChangeScrollPosWhenScrollingToRef : 1;

  
  nsDataHashtable< nsPtrHashKey<imgIRequest>, PRUint32> mImageTracker;

#ifdef DEBUG
protected:
  PRBool mWillReparent;
#endif
};

#define NS_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                             \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMDocument, nsDocument)      \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMNSDocument, nsDocument)    \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMEventTarget, nsDocument)   \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMNode, nsDocument)

#endif 
