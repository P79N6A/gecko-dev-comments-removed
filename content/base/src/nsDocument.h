








































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
#include "nsIDOM3Document.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMNSDocumentStyle.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMDocumentTraversal.h"
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
#include "nsIDOMDocumentEvent.h"
#include "nsIDOM3DocumentEvent.h"
#include "nsHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsIBoxObject.h"
#include "nsPIBoxObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"
#include "nsScriptLoader.h"
#include "nsICSSLoader.h"
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


#include "nsHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"

#include "nsStyleSet.h"
#include "nsXMLEventsManager.h"
#include "pldhash.h"
#include "nsAttrAndChildArray.h"
#include "nsDOMAttributeMap.h"
#include "nsPresShellIterator.h"
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
#ifdef MOZ_SMIL
class nsSMILAnimationController;
#endif 







class nsUint32ToContentHashEntry : public PLDHashEntryHdr
{
  public:
    typedef const PRUint32& KeyType;
    typedef const PRUint32* KeyTypePointer;

    nsUint32ToContentHashEntry(const KeyTypePointer key) :
      mValue(*key), mValOrHash(nsnull) { }
    nsUint32ToContentHashEntry(const nsUint32ToContentHashEntry& toCopy) :
      mValue(toCopy.mValue), mValOrHash(toCopy.mValOrHash)
    {
      
      
      
      const_cast<nsUint32ToContentHashEntry&>(toCopy).mValOrHash = nsnull;
      NS_ERROR("Copying not supported. Fasten your seat belt.");
    }
    ~nsUint32ToContentHashEntry() { Destroy(); }

    KeyType GetKey() const { return mValue; }

    PRBool KeyEquals(KeyTypePointer aKey) const { return mValue == *aKey; }

    static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
    static PLDHashNumber HashKey(KeyTypePointer aKey) { return *aKey; }
    enum { ALLOW_MEMMOVE = PR_TRUE };

    
    nsresult PutContent(nsIContent* aContent);

    void RemoveContent(nsIContent* aContent);

    struct Visitor {
      virtual void Visit(nsIContent* aContent) = 0;
    };
    void VisitContent(Visitor* aVisitor);

    PRBool IsEmpty() { return mValOrHash == nsnull; }

  private:
    typedef PRUptrdiff PtrBits;
    typedef nsTHashtable<nsISupportsHashKey> HashSet;
    
    HashSet* GetHashSet()
    {
      return (PtrBits(mValOrHash) & 0x1) ? nsnull : (HashSet*)mValOrHash;
    }
    
    nsIContent* GetContent()
    {
      return (PtrBits(mValOrHash) & 0x1)
             ? (nsIContent*)(PtrBits(mValOrHash) & ~0x1)
             : nsnull;
    }
    
    nsresult SetContent(nsIContent* aVal)
    {
      NS_IF_ADDREF(aVal);
      mValOrHash = (void*)(PtrBits(aVal) | 0x1);
      return NS_OK;
    }
    
    nsresult InitHashSet(HashSet** aSet);

    void Destroy();

  private:
    const PRUint32 mValue;
    
    void* mValOrHash;
};













class nsIdentifierMapEntry : public nsISupportsHashKey
{
public:
  nsIdentifierMapEntry(const nsISupports* aKey) :
    nsISupportsHashKey(aKey), mNameContentList(nsnull)
  {
  }
  nsIdentifierMapEntry(const nsIdentifierMapEntry& aOther) :
    nsISupportsHashKey(GetKey())
  {
    NS_ERROR("Should never be called");
  }
  ~nsIdentifierMapEntry();

  void SetInvalidName();
  PRBool IsInvalidName();
  void AddNameContent(nsIContent* aContent);
  void RemoveNameContent(nsIContent* aContent);
  PRBool HasNameContentList() {
    return mNameContentList != nsnull;
  }
  nsBaseContentList* GetNameContentList() {
    return mNameContentList;
  }
  nsresult CreateNameContentList();

  



  nsIContent* GetIdContent();
  


  void AppendAllIdContent(nsCOMArray<nsIContent>* aElements);
  




  PRBool AddIdContent(nsIContent* aContent);
  



  PRBool RemoveIdContent(nsIContent* aContent);

  PRBool HasContentChangeCallback() { return mChangeCallbacks != nsnull; }
  void AddContentChangeCallback(nsIDocument::IDTargetObserver aCallback, void* aData);
  void RemoveContentChangeCallback(nsIDocument::IDTargetObserver aCallback, void* aData);

  void Traverse(nsCycleCollectionTraversalCallback* aCallback);

  void SetDocAllList(nsContentList* aContentList) { mDocAllList = aContentList; }
  nsContentList* GetDocAllList() { return mDocAllList; }

  struct ChangeCallback {
    nsIDocument::IDTargetObserver mCallback;
    void* mData;
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
             aKey->mData == mKey.mData;
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
  void FireChangeCallbacks(nsIContent* aOldContent, nsIContent* aNewContent);

  
  
  nsSmallVoidArray mIdContentList;
  
  nsBaseContentList *mNameContentList;
  nsRefPtr<nsContentList> mDocAllList;
  nsAutoPtr<nsTHashtable<ChangeCallbackEntry> > mChangeCallbacks;
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

  
  virtual void NodeWillBeDestroyed(const nsINode *aNode);
  virtual void StyleSheetAdded(nsIDocument *aDocument,
                               nsIStyleSheet* aStyleSheet,
                               PRBool aDocumentSheet);
  virtual void StyleSheetRemoved(nsIDocument *aDocument,
                                 nsIStyleSheet* aStyleSheet,
                                 PRBool aDocumentSheet);

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
                   public nsIDOMDocumentEvent,
                   public nsIDOM3DocumentEvent,
                   public nsIDOMNSDocumentStyle,
                   public nsIDOMDocumentView,
                   public nsIDOMDocumentRange,
                   public nsIDOMDocumentTraversal,
                   public nsIDOMDocumentXBL,
                   public nsIDOM3Document,
                   public nsSupportsWeakReference,
                   public nsIDOMEventTarget,
                   public nsIDOM3EventTarget,
                   public nsIDOMNSEventTarget,
                   public nsIScriptObjectPrincipal,
                   public nsIRadioGroupContainer,
                   public nsIDOMNodeSelector,
                   public nsIApplicationCacheContainer,
                   public nsIDOMXPathNSResolver,
                   public nsStubMutationObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

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

  


  virtual void GetBaseTarget(nsAString &aBaseTarget) const;
  virtual void SetBaseTarget(const nsAString &aBaseTarget);

  



  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID);

  


  virtual nsresult AddCharSetObserver(nsIObserver* aObserver);

  


  virtual void RemoveCharSetObserver(nsIObserver* aObserver);

  virtual nsIContent* AddIDTargetObserver(nsIAtom* aID,
                                          IDTargetObserver aObserver, void* aData);
  virtual void RemoveIDTargetObserver(nsIAtom* aID,
                                      IDTargetObserver aObserver, void* aData);

  



  virtual void GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const;
  virtual void SetHeaderData(nsIAtom* aheaderField,
                             const nsAString& aData);

  




  virtual nsresult CreateShell(nsPresContext* aContext,
                               nsIViewManager* aViewManager,
                               nsStyleSet* aStyleSet,
                               nsIPresShell** aInstancePtrResult);
  virtual PRBool DeleteShell(nsIPresShell* aShell);
  virtual nsIPresShell *GetPrimaryShell() const;

  virtual nsresult SetSubDocumentFor(nsIContent *aContent,
                                     nsIDocument* aSubDoc);
  virtual nsIDocument* GetSubDocumentFor(nsIContent *aContent) const;
  virtual nsIContent* FindContentForSubDocument(nsIDocument *aDocument) const;
  virtual nsIContent* GetRootContentInternal() const;

  



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

  



  virtual nsIHTMLCSSStyleSheet* GetInlineStyleSheet() const {
    return mStyleAttrStyleSheet;
  }
  
  




  virtual nsIScriptGlobalObject* GetScriptGlobalObject() const;
  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject);

  virtual nsIScriptGlobalObject*
    GetScriptHandlingObject(PRBool& aHasHadScriptHandlingObject) const;
  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject);

  virtual nsIScriptGlobalObject* GetScopeObject();

  


  virtual nsPIDOMWindow *GetWindow();

  




  virtual nsPIDOMWindow *GetInnerWindow();

  


  virtual nsScriptLoader* ScriptLoader();

  




  virtual void AddObserver(nsIDocumentObserver* aObserver);

  



  virtual PRBool RemoveObserver(nsIDocumentObserver* aObserver);

  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType);
  virtual void EndUpdate(nsUpdateType aUpdateType);
  virtual void BeginLoad();
  virtual void EndLoad();

  virtual void SetReadyStateInternal(ReadyState rs);
  virtual ReadyState GetReadyStateEnum();

  virtual void ContentStatesChanged(nsIContent* aContent1,
                                    nsIContent* aContent2,
                                    PRInt32 aStateMask);

  virtual void StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule);
  virtual void StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);
  virtual void StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule);

  virtual void FlushPendingNotifications(mozFlushType aType);
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

  
  nsresult GetRadioGroup(const nsAString& aName,
                         nsRadioGroupStruct **aRadioGroup);

  
  NS_DECL_NSIDOMNODE

  
  NS_DECL_NSIDOM3NODE

  
  NS_DECL_NSIDOMDOCUMENT

  
  NS_DECL_NSIDOM3DOCUMENT

  
  NS_DECL_NSIDOMXMLDOCUMENT

  
  NS_DECL_NSIDOMNSDOCUMENT

  
  NS_DECL_NSIDOMDOCUMENTEVENT

  
  NS_DECL_NSIDOM3DOCUMENTEVENT

  
  NS_DECL_NSIDOMDOCUMENTSTYLE

  
  NS_DECL_NSIDOMNSDOCUMENTSTYLE

  
  NS_DECL_NSIDOMDOCUMENTVIEW

  
  NS_DECL_NSIDOMDOCUMENTRANGE

  
  NS_DECL_NSIDOMDOCUMENTTRAVERSAL

  
  NS_DECL_NSIDOMDOCUMENTXBL

  
  NS_DECL_NSIDOMEVENTTARGET

  
  NS_DECL_NSIDOM3EVENTTARGET

  
  NS_DECL_NSIDOMNSEVENTTARGET

  
  NS_DECL_NSIDOMNODESELECTOR

  
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE

  
  virtual nsIPrincipal* GetPrincipal();

  
  NS_DECL_NSIAPPLICATIONCACHECONTAINER

  virtual nsresult Init();
  
  virtual nsresult AddXMLEventsContent(nsIContent * aXMLEventsElement);

  virtual nsresult CreateElem(nsIAtom *aName, nsIAtom *aPrefix,
                              PRInt32 aNamespaceID,
                              PRBool aDocumentDefaultType,
                              nsIContent **aResult);

  virtual NS_HIDDEN_(nsresult) Sanitize();

  virtual NS_HIDDEN_(void) EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                                 void *aData);

  virtual NS_HIDDEN_(PRBool) CanSavePresentation(nsIRequest *aNewRequest);
  virtual NS_HIDDEN_(void) Destroy();
  virtual NS_HIDDEN_(void) RemovedFromDocShell();
  virtual NS_HIDDEN_(already_AddRefed<nsILayoutHistoryState>) GetLayoutHistoryState() const;

  virtual NS_HIDDEN_(void) BlockOnload();
  virtual NS_HIDDEN_(void) UnblockOnload(PRBool aFireSync);

  virtual NS_HIDDEN_(void) AddStyleRelevantLink(nsIContent* aContent, nsIURI* aURI);
  virtual NS_HIDDEN_(void) ForgetLink(nsIContent* aContent);
  virtual NS_HIDDEN_(void) NotifyURIVisitednessChanged(nsIURI* aURI);

  NS_HIDDEN_(void) ClearBoxObjectFor(nsIContent* aContent);
  NS_IMETHOD GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult);

  virtual NS_HIDDEN_(nsresult) GetXBLChildNodesFor(nsIContent* aContent,
                                                   nsIDOMNodeList** aResult);
  virtual NS_HIDDEN_(nsresult) GetContentListFor(nsIContent* aContent,
                                                 nsIDOMNodeList** aResult);

  virtual NS_HIDDEN_(nsresult) ElementFromPointHelper(PRInt32 aX, PRInt32 aY,
                                                      PRBool aIgnoreRootScrollFrame,
                                                      PRBool aFlushLayout,
                                                      nsIDOMElement** aReturn);

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

#ifdef MOZ_SMIL
  nsSMILAnimationController* GetAnimationController();
#endif 

  virtual void SuppressEventHandling(PRUint32 aIncrease);

  virtual void UnsuppressEventHandlingAndFireEvents(PRBool aFireEvents);
  
  void DecreaseEventSuppression() { --mEventsSuppressed; }

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDocument,
                                                         nsIDocument)

  



  static nsresult GetElementsByClassNameHelper(nsINode* aRootNode,
                                               const nsAString& aClasses,
                                               nsIDOMNodeList** aReturn);

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
protected:
  friend class nsNodeUtils;
  void RegisterNamedItems(nsIContent *aContent);
  void UnregisterNamedItems(nsIContent *aContent);
  void UpdateNameTableEntry(nsIContent *aContent);
  void UpdateIdTableEntry(nsIContent *aContent);
  void RemoveFromNameTable(nsIContent *aContent);
  void RemoveFromIdTable(nsIContent *aContent);

  




  static PRBool CheckGetElementByIdArg(const nsIAtom* aId);
  nsIdentifierMapEntry* GetElementByIdInternal(nsIAtom* aID);

  void DispatchContentLoadedEvents();

  void RetrieveRelevantHeaders(nsIChannel *aChannel);

  static PRBool TryChannelCharset(nsIChannel *aChannel,
                                  PRInt32& aCharsetSource,
                                  nsACString& aCharset);

  void UpdateLinkMap();
  
  
  void DestroyLinkMap();

  
  void RefreshLinkHrefs();

  nsIContent* GetFirstBaseNodeWithHref();
  nsresult SetFirstBaseNodeWithHref(nsIContent *node);

  
  
  nsIContent* GetHtmlContent();
  
  
  nsIContent* GetHtmlChildContent(nsIAtom* aTag);
  
  
  nsIContent* GetBodyContent() {
    return GetHtmlChildContent(nsGkAtoms::body);
  }
  
  
  nsIContent* GetHeadContent() {
    return GetHtmlChildContent(nsGkAtoms::head);
  }

  
  
  nsIContent* GetTitleContent(PRUint32 aNodeType);
  
  
  
  void GetTitleFromElement(PRUint32 aNodeType, nsAString& aTitle);

  nsresult doCreateShell(nsPresContext* aContext,
                         nsIViewManager* aViewManager, nsStyleSet* aStyleSet,
                         nsCompatibility aCompatMode,
                         nsIPresShell** aInstancePtrResult);

  nsresult ResetStylesheetsToURI(nsIURI* aURI);
  virtual nsStyleSet::sheetType GetAttrSheetType();
  void FillStyleSet(nsStyleSet* aStyleSet);

  
  PRBool IsSafeToFlush() const;
  
  virtual PRInt32 GetDefaultNamespaceID() const
  {
    return kNameSpaceID_None;
  }

  void DispatchPageTransition(nsPIDOMEventTarget* aDispatchTarget,
                              const nsAString& aType,
                              PRBool aPersisted);

  
  static PRBool MatchClassNames(nsIContent* aContent, PRInt32 aNamespaceID,
                                nsIAtom* aAtom, void* aData);

  static void DestroyClassNameArray(void* aData);

#define NS_DOCUMENT_NOTIFY_OBSERVERS(func_, params_)                  \
  NS_OBSERVER_ARRAY_NOTIFY_OBSERVERS(mObservers, nsIDocumentObserver, \
                                     func_, params_);
  
#ifdef DEBUG
  void VerifyRootContentState();
#endif

  nsDocument(const char* aContentType);
  virtual ~nsDocument();

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

  
  
  
  nsCOMPtr<nsIScriptGlobalObject> mScriptGlobalObject;
  
  
  nsPIDOMWindow *mWindow;

  
  
  
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
  
  PRPackedBool mHasHadScriptHandlingObject:1;

  
  
  PRPackedBool mMayHaveTitleElement:1;

  PRPackedBool mHasWarnedAboutBoxObjects:1;

  PRPackedBool mDelayFrameLoaderInitialization:1;

  PRPackedBool mSynchronousDOMContentLoaded:1;

  
  
  PRPackedBool mHaveInputEncoding:1;

  PRPackedBool mInXBLUpdate:1;

  PRUint8 mXMLDeclarationBits;

  PRUint8 mDefaultElementType;

  nsInterfaceHashtable<nsVoidPtrHashKey, nsPIBoxObject> *mBoxObjectTable;

  
  nsCOMPtr<nsIChannel> mChannel;
  nsRefPtr<nsHTMLStyleSheet> mAttrStyleSheet;
  nsCOMPtr<nsIHTMLCSSStyleSheet> mStyleAttrStyleSheet;
  nsRefPtr<nsXMLEventsManager> mXMLEventsManager;

  nsCOMPtr<nsIScriptEventManager> mScriptEventManager;

  nsString mBaseTarget;

  
  PRUint32 mUpdateNestLevel;

  
  
  nsCOMPtr<nsIApplicationCache> mApplicationCache;

  nsCOMPtr<nsIContent> mFirstBaseNodeWithHref;

private:
  friend class nsUnblockOnloadEvent;

  void PostUnblockOnloadEvent();
  void DoUnblockOnload();

  



  already_AddRefed<nsIDOMElement>
    CheckAncestryAndGetFrame(nsIDocument* aDocument) const;

  
  
  
  void EnableStyleSheetsForSetInternal(const nsAString& aSheetSet,
                                       PRBool aUpdateCSSLoader);

  
  nsDocument(const nsDocument& aOther);
  nsDocument& operator=(const nsDocument& aOther);

  nsCOMPtr<nsISupports> mXPathEvaluatorTearoff;

  
  
  
  
  nsCOMPtr<nsILayoutHistoryState> mLayoutHistoryState;

  PRUint32 mOnloadBlockCount;
  nsCOMPtr<nsIRequest> mOnloadBlocker;
  ReadyState mReadyState;
  
  
  nsTHashtable<nsUint32ToContentHashEntry> mLinkMap;
  
  nsCOMArray<nsIURI> mVisitednessChangedURIs;

  
  nsString mLastStyleSheetSet;

  nsTArray<nsRefPtr<nsFrameLoader> > mInitializableFrameLoaders;
  nsTArray<nsRefPtr<nsFrameLoader> > mFinalizableFrameLoaders;
  nsRefPtr<nsRunnableMethod<nsDocument> > mFrameLoaderRunner;

  nsRevocableEventPtr<nsNonOwningRunnableMethod<nsDocument> >
    mPendingTitleChangeEvent;

  nsExternalResourceMap mExternalResourceMap;

  
  nsCOMArray<imgIRequest> mPreloadingImages;

  nsCOMPtr<nsIDOMDOMImplementation> mDOMImplementation;

#ifdef MOZ_SMIL
  nsAutoPtr<nsSMILAnimationController> mAnimationController;
#endif 

#ifdef DEBUG
protected:
  PRBool mWillReparent;
#endif
};

#define NS_DOCUMENT_INTERFACE_TABLE_BEGIN(_class)                             \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMDocument, nsDocument)      \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMNSDocument, nsDocument)    \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMDocumentEvent, nsDocument) \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMDocumentView, nsDocument)  \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMDocumentTraversal,         \
                                     nsDocument)                              \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMEventTarget, nsDocument)   \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMNode, nsDocument)          \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOM3Node, nsDocument)         \
  NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOM3Document, nsDocument)

#endif 
