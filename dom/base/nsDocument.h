









#ifndef nsDocument_h___
#define nsDocument_h___

#include "nsIDocument.h"

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCRT.h"
#include "nsWeakReference.h"
#include "nsWeakPtr.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsStubDocumentObserver.h"
#include "nsIScriptGlobalObject.h"
#include "nsIContent.h"
#include "nsIPrincipal.h"
#include "nsIParser.h"
#include "nsBindingManager.h"
#include "nsInterfaceHashtable.h"
#include "nsJSThingHashtable.h"
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
#include "nsIContentViewer.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadContext.h"
#include "nsIProgressEventSink.h"
#include "nsISecurityEventSink.h"
#include "nsIChannelEventSink.h"
#include "imgIRequest.h"
#include "mozilla/EventListenerManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PendingPlayerTracker.h"
#include "mozilla/dom/DOMImplementation.h"
#include "mozilla/dom/StyleSheetList.h"
#include "nsDataHashtable.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Attributes.h"
#include "nsIDOMXPathEvaluator.h"
#include "jsfriendapi.h"
#include "ImportManager.h"

#define XML_DECLARATION_BITS_DECLARATION_EXISTS   (1 << 0)
#define XML_DECLARATION_BITS_ENCODING_EXISTS      (1 << 1)
#define XML_DECLARATION_BITS_STANDALONE_EXISTS    (1 << 2)
#define XML_DECLARATION_BITS_STANDALONE_YES       (1 << 3)


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
class nsPointerLockPermissionRequest;
class nsISecurityConsoleMessage;
class nsPIBoxObject;

namespace mozilla {
class EventChainPreVisitor;
namespace dom {
class BoxObject;
class UndoManager;
struct LifecycleCallbacks;
class CallbackFunction;
}
}













class nsIdentifierMapEntry : public nsStringHashKey
{
public:
  typedef mozilla::dom::Element Element;
  typedef mozilla::net::ReferrerPolicy ReferrerPolicy;

  explicit nsIdentifierMapEntry(const nsAString& aKey) :
    nsStringHashKey(&aKey), mNameContentList(nullptr)
  {
  }
  explicit nsIdentifierMapEntry(const nsAString* aKey) :
    nsStringHashKey(aKey), mNameContentList(nullptr)
  {
  }
  nsIdentifierMapEntry(const nsIdentifierMapEntry& aOther) :
    nsStringHashKey(&aOther.GetKey())
  {
    NS_ERROR("Should never be called");
  }
  ~nsIdentifierMapEntry();

  void AddNameElement(nsINode* aDocument, Element* aElement);
  void RemoveNameElement(Element* aElement);
  bool IsEmpty();
  nsBaseContentList* GetNameContentList() {
    return mNameContentList;
  }
  bool HasNameElement() const {
    return mNameContentList && mNameContentList->Length() != 0;
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
  bool HasIdElementExposedAsHTMLDocumentProperty();

  bool HasContentChangeCallback() { return mChangeCallbacks != nullptr; }
  void AddContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                void* aData, bool aForImage);
  void RemoveContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                void* aData, bool aForImage);

  void Traverse(nsCycleCollectionTraversalCallback* aCallback);

  struct ChangeCallback {
    nsIDocument::IDTargetObserver mCallback;
    void* mData;
    bool mForImage;
  };

  struct ChangeCallbackEntry : public PLDHashEntryHdr {
    typedef const ChangeCallback KeyType;
    typedef const ChangeCallback* KeyTypePointer;

    explicit ChangeCallbackEntry(const ChangeCallback* aKey) :
      mKey(*aKey) { }
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

  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  void FireChangeCallbacks(Element* aOldElement, Element* aNewElement,
                           bool aImageOnly = false);

  
  
  nsSmallVoidArray mIdContentList;
  nsRefPtr<nsBaseContentList> mNameContentList;
  nsAutoPtr<nsTHashtable<ChangeCallbackEntry> > mChangeCallbacks;
  nsRefPtr<Element> mImageElement;
};

namespace mozilla {
namespace dom {

class CustomElementHashKey : public PLDHashEntryHdr
{
public:
  typedef CustomElementHashKey *KeyType;
  typedef const CustomElementHashKey *KeyTypePointer;

  CustomElementHashKey(int32_t aNamespaceID, nsIAtom *aAtom)
    : mNamespaceID(aNamespaceID),
      mAtom(aAtom)
  {}
  explicit CustomElementHashKey(const CustomElementHashKey* aKey)
    : mNamespaceID(aKey->mNamespaceID),
      mAtom(aKey->mAtom)
  {}
  ~CustomElementHashKey()
  {}

  KeyType GetKey() const { return const_cast<KeyType>(this); }
  bool KeyEquals(const KeyTypePointer aKey) const
  {
    MOZ_ASSERT(mNamespaceID != kNameSpaceID_Unknown,
               "This equals method is not transitive, nor symmetric. "
               "A key with a namespace of kNamespaceID_Unknown should "
               "not be stored in a hashtable.");
    return (kNameSpaceID_Unknown == aKey->mNamespaceID ||
            mNamespaceID == aKey->mNamespaceID) &&
           aKey->mAtom == mAtom;
  }

  static KeyTypePointer KeyToPointer(KeyType aKey) { return aKey; }
  static PLDHashNumber HashKey(const KeyTypePointer aKey)
  {
    return aKey->mAtom->hash();
  }
  enum { ALLOW_MEMMOVE = true };

private:
  int32_t mNamespaceID;
  nsCOMPtr<nsIAtom> mAtom;
};

struct LifecycleCallbackArgs
{
  nsString name;
  nsString oldValue;
  nsString newValue;
};

struct CustomElementData;

class CustomElementCallback
{
public:
  CustomElementCallback(Element* aThisObject,
                        nsIDocument::ElementCallbackType aCallbackType,
                        mozilla::dom::CallbackFunction* aCallback,
                        CustomElementData* aOwnerData);
  void Traverse(nsCycleCollectionTraversalCallback& aCb) const;
  void Call();
  void SetArgs(LifecycleCallbackArgs& aArgs)
  {
    MOZ_ASSERT(mType == nsIDocument::eAttributeChanged,
               "Arguments are only used by attribute changed callback.");
    mArgs = aArgs;
  }

private:
  
  nsRefPtr<mozilla::dom::Element> mThisObject;
  nsRefPtr<mozilla::dom::CallbackFunction> mCallback;
  
  nsIDocument::ElementCallbackType mType;
  
  
  LifecycleCallbackArgs mArgs;
  
  
  CustomElementData* mOwnerData;
};



struct CustomElementData
{
  NS_INLINE_DECL_REFCOUNTING(CustomElementData)

  explicit CustomElementData(nsIAtom* aType);
  
  
  nsTArray<nsAutoPtr<CustomElementCallback>> mCallbackQueue;
  
  
  nsCOMPtr<nsIAtom> mType;
  
  int32_t mCurrentCallback;
  
  bool mElementIsBeingCreated;
  
  
  bool mCreatedCallbackInvoked;
  
  
  
  int32_t mAssociatedMicroTask;

  
  void RunCallbackQueue();

private:
  virtual ~CustomElementData() {}
};



struct CustomElementDefinition
{
  CustomElementDefinition(JSObject* aPrototype,
                          nsIAtom* aType,
                          nsIAtom* aLocalName,
                          mozilla::dom::LifecycleCallbacks* aCallbacks,
                          uint32_t aNamespaceID,
                          uint32_t aDocOrder);

  
  JS::Heap<JSObject *> mPrototype;

  
  nsCOMPtr<nsIAtom> mType;

  
  nsCOMPtr<nsIAtom> mLocalName;

  
  nsAutoPtr<mozilla::dom::LifecycleCallbacks> mCallbacks;

  
  
  bool mElementIsBeingCreated;

  
  int32_t mNamespaceID;

  
  uint32_t mDocOrder;
};

class Registry : public nsISupports
{
public:
  friend class ::nsDocument;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Registry)

  Registry();

protected:
  virtual ~Registry();

  typedef nsClassHashtable<mozilla::dom::CustomElementHashKey,
                           mozilla::dom::CustomElementDefinition>
    DefinitionMap;
  typedef nsClassHashtable<mozilla::dom::CustomElementHashKey,
                           nsTArray<nsRefPtr<mozilla::dom::Element>>>
    CandidateMap;

  
  
  
  DefinitionMap mCustomDefinitions;

  
  
  
  CandidateMap mCandidatesMap;
};

} 
} 

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

class nsDOMStyleSheetList : public mozilla::dom::StyleSheetList,
                            public nsStubDocumentObserver
{
public:
  explicit nsDOMStyleSheetList(nsIDocument* aDocument);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOCUMENTOBSERVER_STYLESHEETADDED
  NS_DECL_NSIDOCUMENTOBSERVER_STYLESHEETREMOVED

  
  NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

  virtual nsINode* GetParentObject() const override
  {
    return mDocument;
  }

  virtual uint32_t Length() override;
  virtual mozilla::CSSStyleSheet*
  IndexedGetter(uint32_t aIndex, bool& aFound) override;

protected:
  virtual ~nsDOMStyleSheetList();

  int32_t       mLength;
  nsIDocument*  mDocument;
};

class nsOnloadBlocker final : public nsIRequest
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
    ~PendingLoad() {}

  public:
    explicit PendingLoad(nsDocument* aDisplayDocument) :
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

  class LoadgroupCallbacks final : public nsIInterfaceRequestor
  {
    ~LoadgroupCallbacks() {}
  public:
    explicit LoadgroupCallbacks(nsIInterfaceRequestor* aOtherCallbacks)
      : mCallbacks(aOtherCallbacks)
    {}
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR
  private:
    
    
    
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;

    
    
    
    
    
    
#define DECL_SHIM(_i, _allcaps)                                              \
    class _i##Shim final : public nsIInterfaceRequestor,                     \
                           public _i                                         \
    {                                                                        \
      ~_i##Shim() {}                                                         \
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

class CSPErrorQueue
{
  public:
    




    void Add(const char* aMessageName);
    void Flush(nsIDocument* aDocument);

    CSPErrorQueue()
    {
    }

    ~CSPErrorQueue()
    {
    }

  private:
    nsAutoTArray<const char*,5> mErrors;
};










class nsDocument : public nsIDocument,
                   public nsIDOMXMLDocument, 
                   public nsIDOMDocumentXBL,
                   public nsSupportsWeakReference,
                   public nsIScriptObjectPrincipal,
                   public nsIRadioGroupContainer,
                   public nsIApplicationCacheContainer,
                   public nsStubMutationObserver,
                   public nsIObserver,
                   public nsIDOMXPathEvaluator
{
  friend class nsIDocument;

public:
  typedef mozilla::dom::Element Element;
  using nsIDocument::GetElementsByTagName;
  typedef mozilla::net::ReferrerPolicy ReferrerPolicy;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_SIZEOF_EXCLUDING_THIS

  virtual void Reset(nsIChannel *aChannel, nsILoadGroup *aLoadGroup) override;
  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                          nsIPrincipal* aPrincipal) override;

  
  
  
  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     bool aReset = true,
                                     nsIContentSink* aContentSink = nullptr) override = 0;

  virtual void StopDocumentLoad() override;

  virtual void NotifyPossibleTitleChange(bool aBoundTitleElement) override;

  virtual void SetDocumentURI(nsIURI* aURI) override;

  virtual void SetChromeXHRDocURI(nsIURI* aURI) override;

  virtual void SetChromeXHRDocBaseURI(nsIURI* aURI) override;

  


  virtual void SetPrincipal(nsIPrincipal *aPrincipal) override;

  


  
  

  


  virtual void SetContentType(const nsAString& aContentType) override;

  virtual nsresult SetBaseURI(nsIURI* aURI) override;

  


  virtual void GetBaseTarget(nsAString &aBaseTarget) override;

  



  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID) override;

  


  virtual nsresult AddCharSetObserver(nsIObserver* aObserver) override;

  


  virtual void RemoveCharSetObserver(nsIObserver* aObserver) override;

  virtual Element* AddIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                       void* aData, bool aForImage) override;
  virtual void RemoveIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                      void* aData, bool aForImage) override;

  



  virtual void GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const override;
  virtual void SetHeaderData(nsIAtom* aheaderField,
                             const nsAString& aData) override;

  




  virtual already_AddRefed<nsIPresShell> CreateShell(nsPresContext* aContext,
                                                     nsViewManager* aViewManager,
                                                     nsStyleSet* aStyleSet) override;
  virtual void DeleteShell() override;

  virtual nsresult GetAllowPlugins(bool* aAllowPlugins) override;

  virtual already_AddRefed<mozilla::dom::UndoManager> GetUndoManager() override;

  static bool IsWebAnimationsEnabled(JSContext* aCx, JSObject* aObject);
  virtual mozilla::dom::DocumentTimeline* Timeline() override;

  virtual nsresult SetSubDocumentFor(Element* aContent,
                                     nsIDocument* aSubDoc) override;
  virtual nsIDocument* GetSubDocumentFor(nsIContent* aContent) const override;
  virtual Element* FindContentForSubDocument(nsIDocument *aDocument) const override;
  virtual Element* GetRootElementInternal() const override;

  virtual void EnsureOnDemandBuiltInUASheet(mozilla::CSSStyleSheet* aSheet) override;

  



  virtual int32_t GetNumberOfStyleSheets() const override;
  virtual nsIStyleSheet* GetStyleSheetAt(int32_t aIndex) const override;
  virtual int32_t GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const override;
  virtual void AddStyleSheet(nsIStyleSheet* aSheet) override;
  virtual void RemoveStyleSheet(nsIStyleSheet* aSheet) override;

  virtual void UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                                 nsCOMArray<nsIStyleSheet>& aNewSheets) override;
  virtual void AddStyleSheetToStyleSets(nsIStyleSheet* aSheet);
  virtual void RemoveStyleSheetFromStyleSets(nsIStyleSheet* aSheet);

  virtual void InsertStyleSheetAt(nsIStyleSheet* aSheet, int32_t aIndex) override;
  virtual void SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                            bool aApplicable) override;

  virtual nsresult LoadAdditionalStyleSheet(additionalSheetType aType, nsIURI* aSheetURI) override;
  virtual nsresult AddAdditionalStyleSheet(additionalSheetType aType, nsIStyleSheet* aSheet) override;
  virtual void RemoveAdditionalStyleSheet(additionalSheetType aType, nsIURI* sheetURI) override;
  virtual nsIStyleSheet* FirstAdditionalAuthorSheet() override;

  virtual nsIChannel* GetChannel() const override {
    return mChannel;
  }

  virtual nsIChannel* GetFailedChannel() const override {
    return mFailedChannel;
  }
  virtual void SetFailedChannel(nsIChannel* aChannel) override {
    mFailedChannel = aChannel;
  }

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject) override;

  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject) override;

  virtual nsIGlobalObject* GetScopeObject() const override;
  void SetScopeObject(nsIGlobalObject* aGlobal) override;
  


  virtual nsScriptLoader* ScriptLoader() override;

  


  virtual void AddToIdTable(Element* aElement, nsIAtom* aId) override;
  virtual void RemoveFromIdTable(Element* aElement, nsIAtom* aId) override;
  virtual void AddToNameTable(Element* aElement, nsIAtom* aName) override;
  virtual void RemoveFromNameTable(Element* aElement, nsIAtom* aName) override;

  




  virtual void AddObserver(nsIDocumentObserver* aObserver) override;

  



  virtual bool RemoveObserver(nsIDocumentObserver* aObserver) override;

  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType) override;
  virtual void EndUpdate(nsUpdateType aUpdateType) override;
  virtual void BeginLoad() override;
  virtual void EndLoad() override;

  virtual void SetReadyStateInternal(ReadyState rs) override;

  virtual void ContentStateChanged(nsIContent* aContent,
                                   mozilla::EventStates aStateMask)
                                     override;
  virtual void DocumentStatesChanged(
                 mozilla::EventStates aStateMask) override;

  virtual void StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule) override;
  virtual void StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) override;
  virtual void StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule) override;

  virtual void FlushPendingNotifications(mozFlushType aType) override;
  virtual void FlushExternalResources(mozFlushType aType) override;
  virtual void SetXMLDeclaration(const char16_t *aVersion,
                                 const char16_t *aEncoding,
                                 const int32_t aStandalone) override;
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone) override;
  virtual bool IsScriptEnabled() override;

  virtual void OnPageShow(bool aPersisted, mozilla::dom::EventTarget* aDispatchStartTarget) override;
  virtual void OnPageHide(bool aPersisted, mozilla::dom::EventTarget* aDispatchStartTarget) override;

  virtual void WillDispatchMutationEvent(nsINode* aTarget) override;
  virtual void MutationEventDispatched(nsINode* aTarget) override;

  
  virtual bool IsNodeOfType(uint32_t aFlags) const override;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const override;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const override;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const override;
  virtual uint32_t GetChildCount() const override;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) override;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  NS_IMETHOD WalkRadioGroup(const nsAString& aName,
                            nsIRadioVisitor* aVisitor,
                            bool aFlushContent) override;
  virtual void
    SetCurrentRadioButton(const nsAString& aName,
                          mozilla::dom::HTMLInputElement* aRadio) override;
  virtual mozilla::dom::HTMLInputElement*
    GetCurrentRadioButton(const nsAString& aName) override;
  NS_IMETHOD
    GetNextRadioButton(const nsAString& aName,
                       const bool aPrevious,
                       mozilla::dom::HTMLInputElement*  aFocusedRadio,
                       mozilla::dom::HTMLInputElement** aRadioOut) override;
  virtual void AddToRadioGroup(const nsAString& aName,
                               nsIFormControl* aRadio) override;
  virtual void RemoveFromRadioGroup(const nsAString& aName,
                                    nsIFormControl* aRadio) override;
  virtual uint32_t GetRequiredRadioCount(const nsAString& aName) const override;
  virtual void RadioRequiredWillChange(const nsAString& aName,
                                       bool aRequiredAdded) override;
  virtual bool GetValueMissingState(const nsAString& aName) const override;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue) override;

  
  nsRadioGroupStruct* GetRadioGroup(const nsAString& aName) const;
  nsRadioGroupStruct* GetOrCreateRadioGroup(const nsAString& aName);

  virtual nsViewportInfo GetViewportInfo(const mozilla::ScreenIntSize& aDisplaySize) override;

  



  void OnAppThemeChanged();

private:
  void AddOnDemandBuiltInUASheet(mozilla::CSSStyleSheet* aSheet);
  nsRadioGroupStruct* GetRadioGroupInternal(const nsAString& aName) const;
  void SendToConsole(nsCOMArray<nsISecurityConsoleMessage>& aMessages);

public:
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE_OVERRIDABLE

  
  NS_DECL_NSIDOMDOCUMENT

  
  NS_DECL_NSIDOMXMLDOCUMENT

  
  NS_DECL_NSIDOMDOCUMENTXBL

  
  virtual nsresult PreHandleEvent(
                     mozilla::EventChainPreVisitor& aVisitor) override;
  virtual mozilla::EventListenerManager*
    GetOrCreateListenerManager() override;
  virtual mozilla::EventListenerManager*
    GetExistingListenerManager() const override;

  
  virtual nsIPrincipal* GetPrincipal() override;

  
  NS_DECL_NSIAPPLICATIONCACHECONTAINER

  
  NS_DECL_NSIOBSERVER

  NS_DECL_NSIDOMXPATHEVALUATOR

  virtual nsresult Init();

  virtual nsresult CreateElem(const nsAString& aName, nsIAtom *aPrefix,
                              int32_t aNamespaceID,
                              nsIContent **aResult) override;

  virtual void Sanitize() override;

  virtual void EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                                 void *aData) override;

  virtual bool CanSavePresentation(nsIRequest *aNewRequest) override;
  virtual void Destroy() override;
  virtual void RemovedFromDocShell() override;
  virtual already_AddRefed<nsILayoutHistoryState> GetLayoutHistoryState() const override;

  virtual void BlockOnload() override;
  virtual void UnblockOnload(bool aFireSync) override;

  virtual void AddStyleRelevantLink(mozilla::dom::Link* aLink) override;
  virtual void ForgetLink(mozilla::dom::Link* aLink) override;

  virtual void ClearBoxObjectFor(nsIContent* aContent) override;

  virtual already_AddRefed<mozilla::dom::BoxObject>
  GetBoxObjectFor(mozilla::dom::Element* aElement,
                  mozilla::ErrorResult& aRv) override;

  virtual Element*
    GetAnonymousElementByAttribute(nsIContent* aElement,
                                   nsIAtom* aAttrName,
                                   const nsAString& aAttrValue) const override;

  virtual Element* ElementFromPointHelper(float aX, float aY,
                                                      bool aIgnoreRootScrollFrame,
                                                      bool aFlushLayout) override;

  virtual nsresult NodesFromRectHelper(float aX, float aY,
                                                   float aTopSize, float aRightSize,
                                                   float aBottomSize, float aLeftSize,
                                                   bool aIgnoreRootScrollFrame,
                                                   bool aFlushLayout,
                                                   nsIDOMNodeList** aReturn) override;

  virtual void FlushSkinBindings() override;

  virtual nsresult InitializeFrameLoader(nsFrameLoader* aLoader) override;
  virtual nsresult FinalizeFrameLoader(nsFrameLoader* aLoader, nsIRunnable* aFinalizer) override;
  virtual void TryCancelFrameLoaderInitialization(nsIDocShell* aShell) override;
  virtual nsIDocument*
    RequestExternalResource(nsIURI* aURI,
                            nsINode* aRequestingNode,
                            ExternalResourceLoad** aPendingLoad) override;
  virtual void
    EnumerateExternalResources(nsSubDocEnumFunc aCallback, void* aData) override;

  nsTArray<nsCString> mHostObjectURIs;

  
  
  nsSMILAnimationController* GetAnimationController() override;

  virtual mozilla::PendingPlayerTracker*
  GetPendingPlayerTracker() final override
  {
    return mPendingPlayerTracker;
  }

  virtual mozilla::PendingPlayerTracker*
  GetOrCreatePendingPlayerTracker() override;

  void SetImagesNeedAnimating(bool aAnimating) override;

  virtual void SuppressEventHandling(SuppressionType aWhat,
                                     uint32_t aIncrease) override;

  virtual void UnsuppressEventHandlingAndFireEvents(SuppressionType aWhat,
                                                    bool aFireEvents) override;

  void DecreaseEventSuppression() {
    MOZ_ASSERT(mEventsSuppressed);
    --mEventsSuppressed;
    MaybeRescheduleAnimationFrameNotifications();
  }

  void ResumeAnimations() {
    MOZ_ASSERT(mAnimationsPaused);
    --mAnimationsPaused;
    MaybeRescheduleAnimationFrameNotifications();
  }

  virtual nsIDocument* GetTemplateContentsOwner() override;

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

  virtual void PreloadPictureOpened() override;
  virtual void PreloadPictureClosed() override;

  virtual void
    PreloadPictureImageSource(const nsAString& aSrcsetAttr,
                              const nsAString& aSizesAttr,
                              const nsAString& aTypeAttr,
                              const nsAString& aMediaAttr) override;

  virtual already_AddRefed<nsIURI>
    ResolvePreloadImage(nsIURI *aBaseURI,
                        const nsAString& aSrcAttr,
                        const nsAString& aSrcsetAttr,
                        const nsAString& aSizesAttr) override;

  virtual void MaybePreLoadImage(nsIURI* uri,
                                 const nsAString &aCrossOriginAttr,
                                 ReferrerPolicy aReferrerPolicy) override;
  virtual void ForgetImagePreload(nsIURI* aURI) override;

  virtual void PreloadStyle(nsIURI* uri, const nsAString& charset,
                            const nsAString& aCrossOriginAttr,
                            ReferrerPolicy aReferrerPolicy) override;

  virtual nsresult LoadChromeSheetSync(nsIURI* uri, bool isAgentSheet,
                                       mozilla::CSSStyleSheet** sheet) override;

  virtual nsISupports* GetCurrentContentSink() override;

  virtual mozilla::EventStates GetDocumentState() override;

  virtual void RegisterHostObjectUri(const nsACString& aUri) override;
  virtual void UnregisterHostObjectUri(const nsACString& aUri) override;

  
  void AsyncBlockOnload();

  virtual void SetScrollToRef(nsIURI *aDocumentURI) override;
  virtual void ScrollToRef() override;
  virtual void ResetScrolledToRefAlready() override;
  virtual void SetChangeScrollPosWhenScrollingToRef(bool aValue) override;

  virtual Element *GetElementById(const nsAString& aElementId) override;
  virtual const nsSmallVoidArray* GetAllElementsForId(const nsAString& aElementId) const override;

  virtual Element *LookupImageElement(const nsAString& aElementId) override;
  virtual void MozSetImageElement(const nsAString& aImageElementId,
                                  Element* aElement) override;

  virtual nsresult AddImage(imgIRequest* aImage) override;
  virtual nsresult RemoveImage(imgIRequest* aImage, uint32_t aFlags) override;
  virtual nsresult SetImageLockingState(bool aLocked) override;

  
  
  virtual nsresult AddPlugin(nsIObjectLoadingContent* aPlugin) override;
  
  
  virtual void RemovePlugin(nsIObjectLoadingContent* aPlugin) override;
  
  
  virtual void GetPlugins(nsTArray<nsIObjectLoadingContent*>& aPlugins) override;

  virtual nsresult GetStateObject(nsIVariant** aResult) override;

  virtual nsDOMNavigationTiming* GetNavigationTiming() const override;
  virtual nsresult SetNavigationTiming(nsDOMNavigationTiming* aTiming) override;

  virtual Element* FindImageMap(const nsAString& aNormalizedMapName) override;

  virtual Element* GetFullScreenElement() override;
  virtual void AsyncRequestFullScreen(Element* aElement,
                                      mozilla::dom::FullScreenOptions& aOptions) override;
  virtual void RestorePreviousFullScreenState() override;
  virtual bool IsFullscreenLeaf() override;
  virtual bool IsFullScreenDoc() override;
  virtual void SetApprovedForFullscreen(bool aIsApproved) override;
  virtual nsresult RemoteFrameFullscreenChanged(nsIDOMElement* aFrameElement,
                                                const nsAString& aNewOrigin) override;

  virtual nsresult RemoteFrameFullscreenReverted() override;
  virtual nsIDocument* GetFullscreenRoot() override;
  virtual void SetFullscreenRoot(nsIDocument* aRoot) override;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  long BlockedTrackingNodeCount() const;

  
  
  
  
  
  
  
  already_AddRefed<nsSimpleContentList> BlockedTrackingNodes() const;

  static void ExitFullscreen(nsIDocument* aDoc);

  
  
  
  
  
  
  
  
  
  
  void RequestFullScreen(Element* aElement,
                         mozilla::dom::FullScreenOptions& aOptions,
                         bool aWasCallerChrome,
                         bool aNotifyOnOriginChange);

  
  
  void CleanupFullscreenState();

  
  
  
  
  nsresult AddFullscreenApprovedObserver();
  nsresult RemoveFullscreenApprovedObserver();

  
  
  
  bool FullScreenStackPush(Element* aElement);

  
  
  
  void FullScreenStackPop();

  
  Element* FullScreenStackTop();

  
  virtual bool MozFullScreenEnabled() override;
  virtual Element* GetMozFullScreenElement(mozilla::ErrorResult& rv) override;

  void RequestPointerLock(Element* aElement) override;
  bool ShouldLockPointer(Element* aElement, Element* aCurrentLock,
                         bool aNoFocusCheck = false);
  bool SetPointerLock(Element* aElement, int aCursorStyle);
  static void UnlockPointer(nsIDocument* aDoc = nullptr);

  
  
  void UpdateVisibilityState();
  
  virtual void PostVisibilityUpdateEvent() override;

  virtual void DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const override;
  

  virtual nsIDOMNode* AsDOMNode() override { return this; }

  virtual void EnqueueLifecycleCallback(nsIDocument::ElementCallbackType aType,
                                        Element* aCustomElement,
                                        mozilla::dom::LifecycleCallbackArgs* aArgs = nullptr,
                                        mozilla::dom::CustomElementDefinition* aDefinition = nullptr) override;

  static void ProcessTopElementQueue(bool aIsBaseQueue = false);

  void GetCustomPrototype(int32_t aNamespaceID,
                          nsIAtom* aAtom,
                          JS::MutableHandle<JSObject*> prototype)
  {
    if (!mRegistry) {
      prototype.set(nullptr);
      return;
    }

    mozilla::dom::CustomElementHashKey key(aNamespaceID, aAtom);
    mozilla::dom::CustomElementDefinition* definition;
    if (mRegistry->mCustomDefinitions.Get(&key, &definition)) {
      prototype.set(definition->mPrototype);
    } else {
      prototype.set(nullptr);
    }
  }

  static bool RegisterEnabled();

  virtual nsresult RegisterUnresolvedElement(mozilla::dom::Element* aElement,
                                             nsIAtom* aTypeName = nullptr) override;

  
  virtual mozilla::dom::DOMImplementation*
    GetImplementation(mozilla::ErrorResult& rv) override;
  virtual void
    RegisterElement(JSContext* aCx, const nsAString& aName,
                    const mozilla::dom::ElementRegistrationOptions& aOptions,
                    JS::MutableHandle<JSObject*> aRetval,
                    mozilla::ErrorResult& rv) override;
  virtual mozilla::dom::StyleSheetList* StyleSheets() override;
  virtual void SetSelectedStyleSheetSet(const nsAString& aSheetSet) override;
  virtual void GetLastStyleSheetSet(nsString& aSheetSet) override;
  virtual mozilla::dom::DOMStringList* StyleSheetSets() override;
  virtual void EnableStyleSheetsForSet(const nsAString& aSheetSet) override;
  using nsIDocument::CreateElement;
  using nsIDocument::CreateElementNS;
  virtual already_AddRefed<Element> CreateElement(const nsAString& aTagName,
                                                  const nsAString& aTypeExtension,
                                                  mozilla::ErrorResult& rv) override;
  virtual already_AddRefed<Element> CreateElementNS(const nsAString& aNamespaceURI,
                                                    const nsAString& aQualifiedName,
                                                    const nsAString& aTypeExtension,
                                                    mozilla::ErrorResult& rv) override;
  virtual void UseRegistryFromDocument(nsIDocument* aDocument) override;

  virtual nsIDocument* MasterDocument() override
  {
    return mMasterDocument ? mMasterDocument.get()
                           : this;
  }

  virtual void SetMasterDocument(nsIDocument* master) override
  {
    MOZ_ASSERT(master);
    mMasterDocument = master;
    UseRegistryFromDocument(mMasterDocument);
  }

  virtual bool IsMasterDocument() override
  {
    return !mMasterDocument;
  }

  virtual mozilla::dom::ImportManager* ImportManager() override
  {
    if (mImportManager) {
      MOZ_ASSERT(!mMasterDocument, "Only the master document has ImportManager set");
      return mImportManager.get();
    }

    if (mMasterDocument) {
      return mMasterDocument->ImportManager();
    }

    
    
    
    
    mImportManager = new mozilla::dom::ImportManager();
    return mImportManager.get();
  }

  virtual bool HasSubImportLink(nsINode* aLink) override
  {
    return mSubImportLinks.Contains(aLink);
  }

  virtual uint32_t IndexOfSubImportLink(nsINode* aLink) override
  {
    return mSubImportLinks.IndexOf(aLink);
  }

  virtual void AddSubImportLink(nsINode* aLink) override
  {
    mSubImportLinks.AppendElement(aLink);
  }

  virtual nsINode* GetSubImportLink(uint32_t aIdx) override
  {
    return aIdx < mSubImportLinks.Length() ? mSubImportLinks[aIdx].get()
                                           : nullptr;
  }

  virtual void UnblockDOMContentLoaded() override;

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
  
  virtual void GetTitle(nsString& aTitle) override;
  
  virtual void SetTitle(const nsAString& aTitle, mozilla::ErrorResult& rv) override;

  static void XPCOMShutdown();

  bool mIsTopLevelContentDocument:1;

  bool IsTopLevelContentDocument();

  void SetIsTopLevelContentDocument(bool aIsTopLevelContentDocument);

  js::ExpandoAndGeneration mExpandoAndGeneration;

#ifdef MOZ_EME
  bool ContainsEMEContent();
#endif

  bool ContainsMSEContent();

protected:
  already_AddRefed<nsIPresShell> doCreateShell(nsPresContext* aContext,
                                               nsViewManager* aViewManager,
                                               nsStyleSet* aStyleSet,
                                               nsCompatibility aCompatMode);

  void RemoveDocStyleSheetsFromStyleSets();
  void RemoveStyleSheetsFromStyleSets(nsCOMArray<nsIStyleSheet>& aSheets, 
                                      nsStyleSet::sheetType aType);
  void ResetStylesheetsToURI(nsIURI* aURI);
  void FillStyleSet(nsStyleSet* aStyleSet);

  
  bool IsSafeToFlush() const;

  void DispatchPageTransition(mozilla::dom::EventTarget* aDispatchTarget,
                              const nsAString& aType,
                              bool aPersisted);

  virtual nsPIDOMWindow *GetWindowInternal() const override;
  virtual nsIScriptGlobalObject* GetScriptHandlingObjectInternal() const override;
  virtual bool InternalAllowXULXBL() override;

#define NS_DOCUMENT_NOTIFY_OBSERVERS(func_, params_)                        \
  NS_OBSERVER_ARRAY_NOTIFY_XPCOM_OBSERVERS(mObservers, nsIDocumentObserver, \
                                           func_, params_);

#ifdef DEBUG
  void VerifyRootContentState();
#endif

  explicit nsDocument(const char* aContentType);
  virtual ~nsDocument();

  void EnsureOnloadBlocker();

  void NotifyStyleSheetApplicableStateChanged();

  nsTArray<nsIObserver*> mCharSetObservers;

  PLDHashTable *mSubDocuments;

  
  nsAttrAndChildArray mChildren;

  
  
  nsCOMPtr<nsIParser> mParser;

  
  
  
  nsWeakPtr mWeakSink;

  nsCOMArray<nsIStyleSheet> mStyleSheets;
  nsCOMArray<nsIStyleSheet> mOnDemandBuiltInUASheets;
  nsCOMArray<nsIStyleSheet> mAdditionalSheets[SheetTypeCount];

  
  nsTObserverArray<nsIDocumentObserver*> mObservers;

  
  
  nsRefPtr<mozilla::PendingPlayerTracker> mPendingPlayerTracker;

  
  
  
  nsWeakPtr mScopeObject;

  
  
  
  nsTArray<nsWeakPtr> mFullScreenStack;

  
  
  nsWeakPtr mFullscreenRoot;

private:
  
  
  
  
  
  
  static mozilla::Maybe<nsTArray<nsRefPtr<mozilla::dom::CustomElementData>>> sProcessingStack;

  
  
  static bool sProcessingBaseElementQueue;

  static bool CustomElementConstructor(JSContext* aCx, unsigned aArgc, JS::Value* aVp);

public:
  static void ProcessBaseElementQueue();

  
  
  
  virtual void SetupCustomElement(Element* aElement,
                                  uint32_t aNamespaceID,
                                  const nsAString* aTypeExtension) override;

  static bool IsWebComponentsEnabled(JSContext* aCx, JSObject* aObject);

  
  nsRefPtr<mozilla::dom::Registry> mRegistry;

  nsRefPtr<mozilla::EventListenerManager> mListenerManager;
  nsRefPtr<mozilla::dom::StyleSheetList> mDOMStyleSheets;
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

  
  
  bool mInFlush:1;

  
  
  bool mParserAborted:1;

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mIsApprovedForFullscreen:1;

  
  
  
  bool mHasFullscreenApprovedObserver:1;

  friend class nsPointerLockPermissionRequest;
  friend class nsCallRequestFullScreen;
  
  
  bool mAllowRelocking:1;

  bool mAsyncFullscreenPending:1;

  
  
  
  
  bool mObservingAppThemeChanged:1;

  
  
  bool mSSApplicableStateNotificationPending:1;

  uint32_t mCancelledPointerLockRequests;

  uint8_t mXMLDeclarationBits;

  nsInterfaceHashtable<nsPtrHashKey<nsIContent>, nsPIBoxObject> *mBoxObjectTable;

  
  
  nsCOMPtr<nsIDocument> mTemplateContentsOwner;

  
  uint32_t mUpdateNestLevel;

  
  
  nsCOMPtr<nsIApplicationCache> mApplicationCache;

  nsCOMPtr<nsIContent> mFirstBaseNodeWithHref;

  mozilla::EventStates mDocumentState;
  mozilla::EventStates mGotDocumentState;

  nsRefPtr<nsDOMNavigationTiming> mTiming;
private:
  friend class nsUnblockOnloadEvent;
  
  mozilla::dom::VisibilityState GetVisibilityState() const;
  void NotifyStyleSheetAdded(nsIStyleSheet* aSheet, bool aDocumentSheet);
  void NotifyStyleSheetRemoved(nsIStyleSheet* aSheet, bool aDocumentSheet);

  void PostUnblockOnloadEvent();
  void DoUnblockOnload();

  nsresult CheckFrameOptions();
  bool IsLoopDocument(nsIChannel* aChannel);
  nsresult InitCSP(nsIChannel* aChannel);

  void FlushCSPWebConsoleErrorQueue()
  {
    mCSPWebConsoleErrorQueue.Flush(this);
  }

  







  nsIContent* GetContentInThisDocument(nsIFrame* aFrame) const;

  
  
  
  void EnableStyleSheetsForSetInternal(const nsAString& aSheetSet,
                                       bool aUpdateCSSLoader);

  
  void RevokeAnimationFrameNotifications();
  
  
  void MaybeRescheduleAnimationFrameNotifications();

  
  nsDocument(const nsDocument& aOther);
  nsDocument& operator=(const nsDocument& aOther);

  
  
  
  
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
  nsTArray<nsCOMPtr<nsIRunnable> > mFrameLoaderFinalizers;
  nsRefPtr<nsRunnableMethod<nsDocument> > mFrameLoaderRunner;

  nsRevocableEventPtr<nsRunnableMethod<nsDocument, void, false> >
    mPendingTitleChangeEvent;

  nsExternalResourceMap mExternalResourceMap;

  
  
  
  
  nsRefPtrHashtable<nsURIHashKey, imgIRequest> mPreloadingImages;

  
  int32_t mPreloadPictureDepth;

  
  nsString mPreloadPictureFoundSource;

  nsRefPtr<mozilla::dom::DOMImplementation> mDOMImplementation;

  nsRefPtr<nsContentList> mImageMaps;

  nsCString mScrollToRef;
  uint8_t mScrolledToRefAlready : 1;
  uint8_t mChangeScrollPosWhenScrollingToRef : 1;

  
  nsDataHashtable< nsPtrHashKey<imgIRequest>, uint32_t> mImageTracker;

  
  nsTHashtable< nsPtrHashKey<nsIObjectLoadingContent> > mPlugins;

  nsRefPtr<mozilla::dom::UndoManager> mUndoManager;

  nsRefPtr<mozilla::dom::DocumentTimeline> mDocumentTimeline;

  enum ViewportType {
    DisplayWidthHeight,
    DisplayWidthHeightNoZoom,
    Specified,
    Unknown
  };

  ViewportType mViewportType;

  
  
  bool mValidWidth, mValidHeight;
  mozilla::LayoutDeviceToScreenScale mScaleMinFloat;
  mozilla::LayoutDeviceToScreenScale mScaleMaxFloat;
  mozilla::LayoutDeviceToScreenScale mScaleFloat;
  mozilla::CSSToLayoutDeviceScale mPixelRatio;
  bool mAutoSize, mAllowZoom, mAllowDoubleTapZoom, mValidScaleFloat, mValidMaxScale, mScaleStrEmpty, mWidthStrEmpty;
  mozilla::CSSSize mViewportSize;

  nsrefcnt mStackRefCnt;
  bool mNeedsReleaseAfterStackRefCntRelease;

  CSPErrorQueue mCSPWebConsoleErrorQueue;

  nsCOMPtr<nsIDocument> mMasterDocument;
  nsRefPtr<mozilla::dom::ImportManager> mImportManager;
  nsTArray<nsCOMPtr<nsINode> > mSubImportLinks;

  
  
  bool mMaybeServiceWorkerControlled;

#ifdef DEBUG
public:
  bool mWillReparent;
#endif
};

class nsDocumentOnStack
{
public:
  explicit nsDocumentOnStack(nsDocument* aDoc) : mDoc(aDoc)
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

#endif 
