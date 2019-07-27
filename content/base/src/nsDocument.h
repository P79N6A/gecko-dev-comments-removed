









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
#include "nsIContentViewer.h"
#include "nsIDOMXPathNSResolver.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadContext.h"
#include "nsIProgressEventSink.h"
#include "nsISecurityEventSink.h"
#include "nsIChannelEventSink.h"
#include "imgIRequest.h"
#include "mozilla/EventListenerManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/DOMImplementation.h"
#include "mozilla/dom/StyleSheetList.h"
#include "nsIDOMTouchEvent.h"
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

namespace mozilla {
class EventChainPreVisitor;
namespace dom {
class UndoManager;
struct LifecycleCallbacks;
class CallbackFunction;
}
}













class nsIdentifierMapEntry : public nsStringHashKey
{
public:
  typedef mozilla::dom::Element Element;

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
  explicit CustomElementData(nsIAtom* aType);
  
  
  nsTArray<nsAutoPtr<CustomElementCallback>> mCallbackQueue;
  
  
  nsCOMPtr<nsIAtom> mType;
  
  int32_t mCurrentCallback;
  
  bool mElementIsBeingCreated;
  
  
  bool mCreatedCallbackInvoked;
  
  
  
  int32_t mAssociatedMicroTask;

  
  void RunCallbackQueue();
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

  virtual nsINode* GetParentObject() const MOZ_OVERRIDE
  {
    return mDocument;
  }

  virtual uint32_t Length() MOZ_OVERRIDE;
  virtual mozilla::CSSStyleSheet*
  IndexedGetter(uint32_t aIndex, bool& aFound) MOZ_OVERRIDE;

protected:
  virtual ~nsDOMStyleSheetList();

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

  class LoadgroupCallbacks MOZ_FINAL : public nsIInterfaceRequestor
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
    class _i##Shim MOZ_FINAL : public nsIInterfaceRequestor,                 \
                               public _i                                     \
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

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_SIZEOF_EXCLUDING_THIS

  virtual void Reset(nsIChannel *aChannel, nsILoadGroup *aLoadGroup) MOZ_OVERRIDE;
  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                          nsIPrincipal* aPrincipal) MOZ_OVERRIDE;

  
  
  
  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     bool aReset = true,
                                     nsIContentSink* aContentSink = nullptr) MOZ_OVERRIDE = 0;

  virtual void StopDocumentLoad() MOZ_OVERRIDE;

  virtual void NotifyPossibleTitleChange(bool aBoundTitleElement) MOZ_OVERRIDE;

  virtual void SetDocumentURI(nsIURI* aURI) MOZ_OVERRIDE;

  virtual void SetChromeXHRDocURI(nsIURI* aURI) MOZ_OVERRIDE;

  virtual void SetChromeXHRDocBaseURI(nsIURI* aURI) MOZ_OVERRIDE;

  


  virtual void SetPrincipal(nsIPrincipal *aPrincipal) MOZ_OVERRIDE;

  


  
  

  


  virtual void SetContentType(const nsAString& aContentType) MOZ_OVERRIDE;

  virtual nsresult SetBaseURI(nsIURI* aURI) MOZ_OVERRIDE;

  


  virtual void GetBaseTarget(nsAString &aBaseTarget) MOZ_OVERRIDE;

  



  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID) MOZ_OVERRIDE;

  


  virtual nsresult AddCharSetObserver(nsIObserver* aObserver) MOZ_OVERRIDE;

  


  virtual void RemoveCharSetObserver(nsIObserver* aObserver) MOZ_OVERRIDE;

  virtual Element* AddIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                       void* aData, bool aForImage) MOZ_OVERRIDE;
  virtual void RemoveIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                      void* aData, bool aForImage) MOZ_OVERRIDE;

  



  virtual void GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const MOZ_OVERRIDE;
  virtual void SetHeaderData(nsIAtom* aheaderField,
                             const nsAString& aData) MOZ_OVERRIDE;

  




  virtual already_AddRefed<nsIPresShell> CreateShell(nsPresContext* aContext,
                                                     nsViewManager* aViewManager,
                                                     nsStyleSet* aStyleSet) MOZ_OVERRIDE;
  virtual void DeleteShell() MOZ_OVERRIDE;

  virtual nsresult GetAllowPlugins(bool* aAllowPlugins) MOZ_OVERRIDE;

  virtual already_AddRefed<mozilla::dom::UndoManager> GetUndoManager() MOZ_OVERRIDE;

  virtual mozilla::dom::AnimationTimeline* Timeline() MOZ_OVERRIDE;

  virtual nsresult SetSubDocumentFor(Element* aContent,
                                     nsIDocument* aSubDoc) MOZ_OVERRIDE;
  virtual nsIDocument* GetSubDocumentFor(nsIContent* aContent) const MOZ_OVERRIDE;
  virtual Element* FindContentForSubDocument(nsIDocument *aDocument) const MOZ_OVERRIDE;
  virtual Element* GetRootElementInternal() const MOZ_OVERRIDE;

  virtual void EnsureOnDemandBuiltInUASheet(mozilla::CSSStyleSheet* aSheet) MOZ_OVERRIDE;

  



  virtual int32_t GetNumberOfStyleSheets() const MOZ_OVERRIDE;
  virtual nsIStyleSheet* GetStyleSheetAt(int32_t aIndex) const MOZ_OVERRIDE;
  virtual int32_t GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const MOZ_OVERRIDE;
  virtual void AddStyleSheet(nsIStyleSheet* aSheet) MOZ_OVERRIDE;
  virtual void RemoveStyleSheet(nsIStyleSheet* aSheet) MOZ_OVERRIDE;

  virtual void UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                                 nsCOMArray<nsIStyleSheet>& aNewSheets) MOZ_OVERRIDE;
  virtual void AddStyleSheetToStyleSets(nsIStyleSheet* aSheet);
  virtual void RemoveStyleSheetFromStyleSets(nsIStyleSheet* aSheet);

  virtual void InsertStyleSheetAt(nsIStyleSheet* aSheet, int32_t aIndex) MOZ_OVERRIDE;
  virtual void SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                            bool aApplicable) MOZ_OVERRIDE;

  virtual nsresult LoadAdditionalStyleSheet(additionalSheetType aType, nsIURI* aSheetURI) MOZ_OVERRIDE;
  virtual nsresult AddAdditionalStyleSheet(additionalSheetType aType, nsIStyleSheet* aSheet) MOZ_OVERRIDE;
  virtual void RemoveAdditionalStyleSheet(additionalSheetType aType, nsIURI* sheetURI) MOZ_OVERRIDE;
  virtual nsIStyleSheet* FirstAdditionalAuthorSheet() MOZ_OVERRIDE;

  virtual nsIChannel* GetChannel() const MOZ_OVERRIDE {
    return mChannel;
  }

  virtual nsIChannel* GetFailedChannel() const MOZ_OVERRIDE {
    return mFailedChannel;
  }
  virtual void SetFailedChannel(nsIChannel* aChannel) MOZ_OVERRIDE {
    mFailedChannel = aChannel;
  }

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject) MOZ_OVERRIDE;

  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject) MOZ_OVERRIDE;

  virtual nsIGlobalObject* GetScopeObject() const MOZ_OVERRIDE;
  void SetScopeObject(nsIGlobalObject* aGlobal) MOZ_OVERRIDE;
  


  virtual nsScriptLoader* ScriptLoader() MOZ_OVERRIDE;

  


  virtual void AddToIdTable(Element* aElement, nsIAtom* aId) MOZ_OVERRIDE;
  virtual void RemoveFromIdTable(Element* aElement, nsIAtom* aId) MOZ_OVERRIDE;
  virtual void AddToNameTable(Element* aElement, nsIAtom* aName) MOZ_OVERRIDE;
  virtual void RemoveFromNameTable(Element* aElement, nsIAtom* aName) MOZ_OVERRIDE;

  




  virtual void AddObserver(nsIDocumentObserver* aObserver) MOZ_OVERRIDE;

  



  virtual bool RemoveObserver(nsIDocumentObserver* aObserver) MOZ_OVERRIDE;

  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType) MOZ_OVERRIDE;
  virtual void EndUpdate(nsUpdateType aUpdateType) MOZ_OVERRIDE;
  virtual void BeginLoad() MOZ_OVERRIDE;
  virtual void EndLoad() MOZ_OVERRIDE;

  virtual void SetReadyStateInternal(ReadyState rs) MOZ_OVERRIDE;

  virtual void ContentStateChanged(nsIContent* aContent,
                                   mozilla::EventStates aStateMask)
                                     MOZ_OVERRIDE;
  virtual void DocumentStatesChanged(
                 mozilla::EventStates aStateMask) MOZ_OVERRIDE;

  virtual void StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule) MOZ_OVERRIDE;
  virtual void StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) MOZ_OVERRIDE;
  virtual void StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule) MOZ_OVERRIDE;

  virtual void FlushPendingNotifications(mozFlushType aType) MOZ_OVERRIDE;
  virtual void FlushExternalResources(mozFlushType aType) MOZ_OVERRIDE;
  virtual void SetXMLDeclaration(const char16_t *aVersion,
                                 const char16_t *aEncoding,
                                 const int32_t aStandalone) MOZ_OVERRIDE;
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone) MOZ_OVERRIDE;
  virtual bool IsScriptEnabled() MOZ_OVERRIDE;

  virtual void OnPageShow(bool aPersisted, mozilla::dom::EventTarget* aDispatchStartTarget) MOZ_OVERRIDE;
  virtual void OnPageHide(bool aPersisted, mozilla::dom::EventTarget* aDispatchStartTarget) MOZ_OVERRIDE;

  virtual void WillDispatchMutationEvent(nsINode* aTarget) MOZ_OVERRIDE;
  virtual void MutationEventDispatched(nsINode* aTarget) MOZ_OVERRIDE;

  
  virtual bool IsNodeOfType(uint32_t aFlags) const MOZ_OVERRIDE;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const MOZ_OVERRIDE;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const MOZ_OVERRIDE;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const MOZ_OVERRIDE;
  virtual uint32_t GetChildCount() const MOZ_OVERRIDE;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) MOZ_OVERRIDE;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) MOZ_OVERRIDE;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  NS_IMETHOD WalkRadioGroup(const nsAString& aName,
                            nsIRadioVisitor* aVisitor,
                            bool aFlushContent) MOZ_OVERRIDE;
  virtual void
    SetCurrentRadioButton(const nsAString& aName,
                          mozilla::dom::HTMLInputElement* aRadio) MOZ_OVERRIDE;
  virtual mozilla::dom::HTMLInputElement*
    GetCurrentRadioButton(const nsAString& aName) MOZ_OVERRIDE;
  NS_IMETHOD
    GetNextRadioButton(const nsAString& aName,
                       const bool aPrevious,
                       mozilla::dom::HTMLInputElement*  aFocusedRadio,
                       mozilla::dom::HTMLInputElement** aRadioOut) MOZ_OVERRIDE;
  virtual void AddToRadioGroup(const nsAString& aName,
                               nsIFormControl* aRadio) MOZ_OVERRIDE;
  virtual void RemoveFromRadioGroup(const nsAString& aName,
                                    nsIFormControl* aRadio) MOZ_OVERRIDE;
  virtual uint32_t GetRequiredRadioCount(const nsAString& aName) const MOZ_OVERRIDE;
  virtual void RadioRequiredChanged(const nsAString& aName,
                                    nsIFormControl* aRadio) MOZ_OVERRIDE;
  virtual bool GetValueMissingState(const nsAString& aName) const MOZ_OVERRIDE;
  virtual void SetValueMissingState(const nsAString& aName, bool aValue) MOZ_OVERRIDE;

  
  nsRadioGroupStruct* GetRadioGroup(const nsAString& aName) const;
  nsRadioGroupStruct* GetOrCreateRadioGroup(const nsAString& aName);

  virtual nsViewportInfo GetViewportInfo(const mozilla::ScreenIntSize& aDisplaySize) MOZ_OVERRIDE;

  



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
                     mozilla::EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;
  virtual mozilla::EventListenerManager*
    GetOrCreateListenerManager() MOZ_OVERRIDE;
  virtual mozilla::EventListenerManager*
    GetExistingListenerManager() const MOZ_OVERRIDE;

  
  virtual nsIPrincipal* GetPrincipal() MOZ_OVERRIDE;

  
  NS_DECL_NSIAPPLICATIONCACHECONTAINER

  
  NS_DECL_NSIOBSERVER

  NS_DECL_NSIDOMXPATHEVALUATOR

  virtual nsresult Init();

  virtual nsresult CreateElem(const nsAString& aName, nsIAtom *aPrefix,
                              int32_t aNamespaceID,
                              nsIContent **aResult) MOZ_OVERRIDE;

  virtual void Sanitize();

  virtual void EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                                 void *aData);

  virtual bool CanSavePresentation(nsIRequest *aNewRequest);
  virtual void Destroy();
  virtual void RemovedFromDocShell();
  virtual already_AddRefed<nsILayoutHistoryState> GetLayoutHistoryState() const;

  virtual void BlockOnload();
  virtual void UnblockOnload(bool aFireSync);

  virtual void AddStyleRelevantLink(mozilla::dom::Link* aLink);
  virtual void ForgetLink(mozilla::dom::Link* aLink);

  void ClearBoxObjectFor(nsIContent* aContent);
  already_AddRefed<nsIBoxObject> GetBoxObjectFor(mozilla::dom::Element* aElement,
                                                 mozilla::ErrorResult& aRv) MOZ_OVERRIDE;

  virtual Element*
    GetAnonymousElementByAttribute(nsIContent* aElement,
                                   nsIAtom* aAttrName,
                                   const nsAString& aAttrValue) const;

  virtual Element* ElementFromPointHelper(float aX, float aY,
                                                      bool aIgnoreRootScrollFrame,
                                                      bool aFlushLayout);

  virtual nsresult NodesFromRectHelper(float aX, float aY,
                                                   float aTopSize, float aRightSize,
                                                   float aBottomSize, float aLeftSize,
                                                   bool aIgnoreRootScrollFrame,
                                                   bool aFlushLayout,
                                                   nsIDOMNodeList** aReturn);

  virtual void FlushSkinBindings();

  virtual nsresult InitializeFrameLoader(nsFrameLoader* aLoader);
  virtual nsresult FinalizeFrameLoader(nsFrameLoader* aLoader);
  virtual void TryCancelFrameLoaderInitialization(nsIDocShell* aShell);
  virtual bool FrameLoaderScheduledToBeFinalized(nsIDocShell* aShell);
  virtual nsIDocument*
    RequestExternalResource(nsIURI* aURI,
                            nsINode* aRequestingNode,
                            ExternalResourceLoad** aPendingLoad);
  virtual void
    EnumerateExternalResources(nsSubDocEnumFunc aCallback, void* aData);

  nsTArray<nsCString> mHostObjectURIs;

  
  
  nsSMILAnimationController* GetAnimationController() MOZ_OVERRIDE;

  void SetImagesNeedAnimating(bool aAnimating) MOZ_OVERRIDE;

  virtual void SuppressEventHandling(SuppressionType aWhat,
                                     uint32_t aIncrease) MOZ_OVERRIDE;

  virtual void UnsuppressEventHandlingAndFireEvents(SuppressionType aWhat,
                                                    bool aFireEvents) MOZ_OVERRIDE;

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

  virtual nsIDocument* GetTemplateContentsOwner() MOZ_OVERRIDE;

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
                                 const nsAString &aCrossOriginAttr) MOZ_OVERRIDE;

  virtual void PreloadStyle(nsIURI* uri, const nsAString& charset,
                            const nsAString& aCrossOriginAttr) MOZ_OVERRIDE;

  virtual nsresult LoadChromeSheetSync(nsIURI* uri, bool isAgentSheet,
                                       mozilla::CSSStyleSheet** sheet) MOZ_OVERRIDE;

  virtual nsISupports* GetCurrentContentSink() MOZ_OVERRIDE;

  virtual mozilla::EventStates GetDocumentState() MOZ_OVERRIDE;

  virtual void RegisterHostObjectUri(const nsACString& aUri) MOZ_OVERRIDE;
  virtual void UnregisterHostObjectUri(const nsACString& aUri) MOZ_OVERRIDE;

  
  void AsyncBlockOnload();

  virtual void SetScrollToRef(nsIURI *aDocumentURI) MOZ_OVERRIDE;
  virtual void ScrollToRef() MOZ_OVERRIDE;
  virtual void ResetScrolledToRefAlready() MOZ_OVERRIDE;
  virtual void SetChangeScrollPosWhenScrollingToRef(bool aValue) MOZ_OVERRIDE;

  virtual Element *GetElementById(const nsAString& aElementId) MOZ_OVERRIDE;
  virtual const nsSmallVoidArray* GetAllElementsForId(const nsAString& aElementId) const MOZ_OVERRIDE;

  virtual Element *LookupImageElement(const nsAString& aElementId) MOZ_OVERRIDE;
  virtual void MozSetImageElement(const nsAString& aImageElementId,
                                  Element* aElement) MOZ_OVERRIDE;

  virtual nsresult AddImage(imgIRequest* aImage);
  virtual nsresult RemoveImage(imgIRequest* aImage, uint32_t aFlags);
  virtual nsresult SetImageLockingState(bool aLocked);

  
  
  virtual nsresult AddPlugin(nsIObjectLoadingContent* aPlugin) MOZ_OVERRIDE;
  
  
  virtual void RemovePlugin(nsIObjectLoadingContent* aPlugin) MOZ_OVERRIDE;
  
  
  virtual void GetPlugins(nsTArray<nsIObjectLoadingContent*>& aPlugins) MOZ_OVERRIDE;

  virtual nsresult GetStateObject(nsIVariant** aResult) MOZ_OVERRIDE;

  virtual nsDOMNavigationTiming* GetNavigationTiming() const MOZ_OVERRIDE;
  virtual nsresult SetNavigationTiming(nsDOMNavigationTiming* aTiming) MOZ_OVERRIDE;

  virtual Element* FindImageMap(const nsAString& aNormalizedMapName) MOZ_OVERRIDE;

  virtual Element* GetFullScreenElement() MOZ_OVERRIDE;
  virtual void AsyncRequestFullScreen(Element* aElement) MOZ_OVERRIDE;
  virtual void RestorePreviousFullScreenState() MOZ_OVERRIDE;
  virtual bool IsFullscreenLeaf() MOZ_OVERRIDE;
  virtual bool IsFullScreenDoc() MOZ_OVERRIDE;
  virtual void SetApprovedForFullscreen(bool aIsApproved) MOZ_OVERRIDE;
  virtual nsresult RemoteFrameFullscreenChanged(nsIDOMElement* aFrameElement,
                                                const nsAString& aNewOrigin) MOZ_OVERRIDE;

  virtual nsresult RemoteFrameFullscreenReverted() MOZ_OVERRIDE;
  virtual nsIDocument* GetFullscreenRoot() MOZ_OVERRIDE;
  virtual void SetFullscreenRoot(nsIDocument* aRoot) MOZ_OVERRIDE;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  long BlockedTrackingNodeCount() const;

  
  
  
  
  
  
  
  already_AddRefed<nsSimpleContentList> BlockedTrackingNodes() const;

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

  
  virtual bool MozFullScreenEnabled() MOZ_OVERRIDE;
  virtual Element* GetMozFullScreenElement(mozilla::ErrorResult& rv) MOZ_OVERRIDE;

  void RequestPointerLock(Element* aElement) MOZ_OVERRIDE;
  bool ShouldLockPointer(Element* aElement, Element* aCurrentLock,
                         bool aNoFocusCheck = false);
  bool SetPointerLock(Element* aElement, int aCursorStyle);
  static void UnlockPointer(nsIDocument* aDoc = nullptr);

  
  
  void UpdateVisibilityState();
  
  virtual void PostVisibilityUpdateEvent() MOZ_OVERRIDE;

  virtual void DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const MOZ_OVERRIDE;
  

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  virtual void EnqueueLifecycleCallback(nsIDocument::ElementCallbackType aType,
                                        Element* aCustomElement,
                                        mozilla::dom::LifecycleCallbackArgs* aArgs = nullptr,
                                        mozilla::dom::CustomElementDefinition* aDefinition = nullptr) MOZ_OVERRIDE;

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
                                             nsIAtom* aTypeName = nullptr) MOZ_OVERRIDE;

  
  virtual mozilla::dom::DOMImplementation*
    GetImplementation(mozilla::ErrorResult& rv) MOZ_OVERRIDE;
  virtual void
    RegisterElement(JSContext* aCx, const nsAString& aName,
                    const mozilla::dom::ElementRegistrationOptions& aOptions,
                    JS::MutableHandle<JSObject*> aRetval,
                    mozilla::ErrorResult& rv) MOZ_OVERRIDE;
  virtual mozilla::dom::StyleSheetList* StyleSheets() MOZ_OVERRIDE;
  virtual void SetSelectedStyleSheetSet(const nsAString& aSheetSet) MOZ_OVERRIDE;
  virtual void GetLastStyleSheetSet(nsString& aSheetSet) MOZ_OVERRIDE;
  virtual mozilla::dom::DOMStringList* StyleSheetSets() MOZ_OVERRIDE;
  virtual void EnableStyleSheetsForSet(const nsAString& aSheetSet) MOZ_OVERRIDE;
  using nsIDocument::CreateElement;
  using nsIDocument::CreateElementNS;
  virtual already_AddRefed<Element> CreateElement(const nsAString& aTagName,
                                                  const nsAString& aTypeExtension,
                                                  mozilla::ErrorResult& rv) MOZ_OVERRIDE;
  virtual already_AddRefed<Element> CreateElementNS(const nsAString& aNamespaceURI,
                                                    const nsAString& aQualifiedName,
                                                    const nsAString& aTypeExtension,
                                                    mozilla::ErrorResult& rv) MOZ_OVERRIDE;
  virtual void UseRegistryFromDocument(nsIDocument* aDocument) MOZ_OVERRIDE;

  virtual already_AddRefed<nsIDocument> MasterDocument()
  {
    return mMasterDocument ? (nsCOMPtr<nsIDocument>(mMasterDocument)).forget()
                           : (nsCOMPtr<nsIDocument>(this)).forget();
  }

  virtual void SetMasterDocument(nsIDocument* master)
  {
    mMasterDocument = master;
  }

  virtual bool IsMasterDocument()
  {
    return !mMasterDocument;
  }

  virtual already_AddRefed<mozilla::dom::ImportManager> ImportManager()
  {
    if (mImportManager) {
      MOZ_ASSERT(!mMasterDocument, "Only the master document has ImportManager set");
      return nsRefPtr<mozilla::dom::ImportManager>(mImportManager).forget();
    }

    if (mMasterDocument) {
      return mMasterDocument->ImportManager();
    }

    
    
    
    
    mImportManager = new mozilla::dom::ImportManager();
    return nsRefPtr<mozilla::dom::ImportManager>(mImportManager).forget();
  }

  virtual void UnblockDOMContentLoaded() MOZ_OVERRIDE;

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
  
  virtual void GetTitle(nsString& aTitle) MOZ_OVERRIDE;
  
  virtual void SetTitle(const nsAString& aTitle, mozilla::ErrorResult& rv) MOZ_OVERRIDE;

  static void XPCOMShutdown();

  bool mIsTopLevelContentDocument:1;

  bool IsTopLevelContentDocument();

  void SetIsTopLevelContentDocument(bool aIsTopLevelContentDocument);

  js::ExpandoAndGeneration mExpandoAndGeneration;

#ifdef MOZ_EME
  bool ContainsEMEContent();
#endif

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

  virtual nsPIDOMWindow *GetWindowInternal() const MOZ_OVERRIDE;
  virtual nsIScriptGlobalObject* GetScriptHandlingObjectInternal() const MOZ_OVERRIDE;
  virtual bool InternalAllowXULXBL() MOZ_OVERRIDE;

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

  
  
  
  nsWeakPtr mScopeObject;

  
  
  
  nsTArray<nsWeakPtr> mFullScreenStack;

  
  
  nsWeakPtr mFullscreenRoot;

private:
  
  
  
  
  
  
  static mozilla::Maybe<nsTArray<mozilla::dom::CustomElementData*>> sProcessingStack;

  
  
  static bool sProcessingBaseElementQueue;

  static bool CustomElementConstructor(JSContext* aCx, unsigned aArgc, JS::Value* aVp);

public:
  static void ProcessBaseElementQueue();

  
  virtual void SwizzleCustomElement(Element* aElement,
                                    const nsAString& aTypeExtension,
                                    uint32_t aNamespaceID,
                                    mozilla::ErrorResult& rv);

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

  nsRefPtr<mozilla::dom::AnimationTimeline> mAnimationTimeline;

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
