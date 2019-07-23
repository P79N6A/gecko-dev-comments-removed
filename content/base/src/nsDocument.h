








































#ifndef nsDocument_h___
#define nsDocument_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCRT.h"
#include "nsIDocument.h"
#include "nsWeakReference.h"
#include "nsWeakPtr.h"
#include "nsVoidArray.h"
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
#include "nsIPrincipal.h"
#include "nsIParser.h"
#include "nsBindingManager.h"
#include "nsINodeInfo.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOM3DocumentEvent.h"
#include "nsCOMArray.h"
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


#include "nsHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"

#include "nsStyleSet.h"
#include "nsXMLEventsManager.h"
#include "pldhash.h"
#include "nsAttrAndChildArray.h"
#include "nsDOMAttributeMap.h"
#include "nsPresShellIterator.h"

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
struct PLEvent;
class nsChildContentList;

PR_BEGIN_EXTERN_C



typedef void*
(PR_CALLBACK EventHandlerFunc)(PLEvent* self);
typedef void
(PR_CALLBACK EventDestructorFunc)(PLEvent* self);
PR_END_EXTERN_C







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
      
      
      
      NS_CONST_CAST(nsUint32ToContentHashEntry&, toCopy).mValOrHash = nsnull;
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
    typedef unsigned long PtrBits;
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

  virtual void SetDocumentURI(nsIURI* aURI);
  
  


  virtual void SetPrincipal(nsIPrincipal *aPrincipal);

  


  
  

  


  virtual void SetContentType(const nsAString& aContentType);

  virtual nsresult SetBaseURI(nsIURI* aURI);

  


  virtual void GetBaseTarget(nsAString &aBaseTarget) const;
  virtual void SetBaseTarget(const nsAString &aBaseTarget);

  



  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID);

  


  virtual nsresult AddCharSetObserver(nsIObserver* aObserver);

  


  virtual void RemoveCharSetObserver(nsIObserver* aObserver);

  



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

  virtual nsIScriptGlobalObject* GetScopeObject();

  


  virtual nsPIDOMWindow *GetWindow();

  




  virtual nsPIDOMWindow *GetInnerWindow();

  


  virtual nsScriptLoader* GetScriptLoader();

  virtual void AddMutationObserver(nsIMutationObserver* aObserver);
  virtual void RemoveMutationObserver(nsIMutationObserver* aMutationObserver);

  




  virtual void AddObserver(nsIDocumentObserver* aObserver);

  



  virtual PRBool RemoveObserver(nsIDocumentObserver* aObserver);

  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType);
  virtual void EndUpdate(nsUpdateType aUpdateType);
  virtual void BeginLoad();
  virtual void EndLoad();
  virtual void ContentStatesChanged(nsIContent* aContent1,
                                    nsIContent* aContent2,
                                    PRInt32 aStateMask);

  virtual void AttributeWillChange(nsIContent* aChild,
                                   PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute);

  virtual void StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule);
  virtual void StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule);
  virtual void StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule);

  virtual void FlushPendingNotifications(mozFlushType aType);
  virtual void AddReference(void *aKey, nsISupports *aReference);
  virtual nsISupports *GetReference(void *aKey);
  virtual void RemoveReference(void *aKey);
  virtual nsIScriptEventManager* GetScriptEventManager();
  virtual void SetXMLDeclaration(const PRUnichar *aVersion,
                                 const PRUnichar *aEncoding,
                                 const PRInt32 aStandalone);
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone);
  virtual PRBool IsScriptEnabled();

  virtual void OnPageShow(PRBool aPersisted);
  virtual void OnPageHide(PRBool aPersisted);
  
  virtual void WillDispatchMutationEvent(nsINode* aTarget);
  virtual void MutationEventDispatched(nsINode* aTarget);
  virtual PRBool MutationEventBeingDispatched()
  {
    return (mSubtreeModifiedDepth > 0);
  }

  
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
  virtual nsIContent *GetChildAt(PRUint32 aIndex) const;
  virtual PRInt32 IndexOf(nsINode* aPossibleChild) const;
  virtual PRUint32 GetChildCount() const;
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult DispatchDOMEvent(nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus);
  virtual nsresult GetListenerManager(PRBool aCreateIfNotFound,
                                      nsIEventListenerManager** aResult);
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID);
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID);
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup);
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

  
  virtual nsIPrincipal* GetPrincipal();

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
  virtual NS_HIDDEN_(already_AddRefed<nsILayoutHistoryState>) GetLayoutHistoryState() const;

  virtual NS_HIDDEN_(void) BlockOnload();
  virtual NS_HIDDEN_(void) UnblockOnload(PRBool aFireSync);

  virtual NS_HIDDEN_(void) AddStyleRelevantLink(nsIContent* aContent, nsIURI* aURI);
  virtual NS_HIDDEN_(void) ForgetLink(nsIContent* aContent);
  virtual NS_HIDDEN_(void) NotifyURIVisitednessChanged(nsIURI* aURI);

  NS_HIDDEN_(void) ClearBoxObjectFor(nsIContent* aContent);

  virtual NS_HIDDEN_(nsresult) GetXBLChildNodesFor(nsIContent* aContent,
                                                   nsIDOMNodeList** aResult);
  virtual NS_HIDDEN_(nsresult) GetContentListFor(nsIContent* aContent,
                                                 nsIDOMNodeList** aResult);
  virtual NS_HIDDEN_(void) FlushSkinBindings();

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDocument, nsIDocument)

  


  static nsresult GetElementsByClassNameHelper(nsIContent* aContent,
                                               const nsAString& aClasses,
                                               nsIDOMNodeList** aReturn);
protected:

  




  static PRBool CheckGetElementByIdArg(const nsAString& aId);

  void DispatchContentLoadedEvents();

  void RetrieveRelevantHeaders(nsIChannel *aChannel);

  static PRBool TryChannelCharset(nsIChannel *aChannel,
                                  PRInt32& aCharsetSource,
                                  nsACString& aCharset);
  
  void UpdateLinkMap();
  
  
  void DestroyLinkMap();

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

  
  void DispatchEventToWindow(nsEvent *aEvent);

  
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

  nsVoidArray mCharSetObservers;

  PLDHashTable *mSubDocuments;

  
  nsAttrAndChildArray mChildren;

  
  
  nsCOMPtr<nsIParser> mParser;

  nsCOMArray<nsIStyleSheet> mStyleSheets;
  nsCOMArray<nsIStyleSheet> mCatalogSheets;

  
  nsTObserverArray<nsIDocumentObserver> mObservers;

  
  
  
  nsCOMPtr<nsIScriptGlobalObject> mScriptGlobalObject;

  
  
  
  nsWeakPtr mScopeObject;

  nsCOMPtr<nsIEventListenerManager> mListenerManager;
  nsCOMPtr<nsIDOMStyleSheetList> mDOMStyleSheets;
  nsRefPtr<nsDOMStyleSheetSetList> mStyleSheetSetList;
  nsRefPtr<nsScriptLoader> mScriptLoader;
  nsDocHeaderData* mHeaderData;

  nsHashtable mRadioGroups;

  
  PRPackedBool mIsGoingAway:1;
  
  PRPackedBool mInDestructor:1;
  
  PRPackedBool mVisible:1;

  PRUint8 mXMLDeclarationBits;

  PRUint8 mDefaultElementType;

  nsInterfaceHashtable<nsISupportsHashKey, nsPIBoxObject> *mBoxObjectTable;
  nsInterfaceHashtable<nsVoidPtrHashKey, nsISupports> *mContentWrapperHash;

  
  nsCOMPtr<nsIChannel> mChannel;
  nsRefPtr<nsHTMLStyleSheet> mAttrStyleSheet;
  nsCOMPtr<nsIHTMLCSSStyleSheet> mStyleAttrStyleSheet;
  nsRefPtr<nsXMLEventsManager> mXMLEventsManager;

  nsCOMPtr<nsIScriptEventManager> mScriptEventManager;

  nsString mBaseTarget;

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
  
  
  nsTHashtable<nsUint32ToContentHashEntry> mLinkMap;
  
  nsCOMArray<nsIURI> mVisitednessChangedURIs;

  
  nsString mLastStyleSheetSet;

  nsCOMArray<nsINode> mSubtreeModifiedTargets;
  PRUint32            mSubtreeModifiedDepth;

  
  PRUint32 mUpdateNestLevel;
};


#endif 
