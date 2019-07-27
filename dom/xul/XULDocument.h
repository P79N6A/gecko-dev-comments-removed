




#ifndef mozilla_dom_XULDocument_h
#define mozilla_dom_XULDocument_h

#include "nsCOMPtr.h"
#include "nsXULPrototypeDocument.h"
#include "nsXULPrototypeCache.h"
#include "nsTArray.h"

#include "mozilla/dom/XMLDocument.h"
#include "nsForwardReference.h"
#include "nsIContent.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULDocument.h"
#include "nsCOMArray.h"
#include "nsIURI.h"
#include "nsIXULDocument.h"
#include "nsScriptLoader.h"
#include "nsIStreamListener.h"
#include "nsICSSLoaderObserver.h"
#include "nsIXULStore.h"

#include "mozilla/Attributes.h"

#include "js/TracingAPI.h"
#include "js/TypeDecls.h"

class nsIRDFResource;
class nsIRDFService;
class nsPIWindowRoot;
#if 0 
class nsIObjectInputStream;
class nsIObjectOutputStream;
#else
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsXULElement.h"
#endif
#include "nsURIHashKey.h"
#include "nsInterfaceHashtable.h"

struct PRLogModuleInfo;

class nsRefMapEntry : public nsStringHashKey
{
public:
  explicit nsRefMapEntry(const nsAString& aKey) :
    nsStringHashKey(&aKey)
  {
  }
  explicit nsRefMapEntry(const nsAString* aKey) :
    nsStringHashKey(aKey)
  {
  }
  nsRefMapEntry(const nsRefMapEntry& aOther) :
    nsStringHashKey(&aOther.GetKey())
  {
    NS_ERROR("Should never be called");
  }

  mozilla::dom::Element* GetFirstElement();
  void AppendAll(nsCOMArray<nsIContent>* aElements);
  


  bool AddElement(mozilla::dom::Element* aElement);
  



  bool RemoveElement(mozilla::dom::Element* aElement);

private:
  nsSmallVoidArray mRefContentList;
};





namespace mozilla {
namespace dom {

class XULDocument final : public XMLDocument,
                          public nsIXULDocument,
                          public nsIDOMXULDocument,
                          public nsIStreamLoaderObserver,
                          public nsICSSLoaderObserver,
                          public nsIOffThreadScriptReceiver
{
public:
    XULDocument();

    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSISTREAMLOADEROBSERVER

    
    virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup) override;
    virtual void ResetToURI(nsIURI *aURI, nsILoadGroup* aLoadGroup,
                            nsIPrincipal* aPrincipal) override;

    virtual nsresult StartDocumentLoad(const char* aCommand,
                                       nsIChannel *channel,
                                       nsILoadGroup* aLoadGroup,
                                       nsISupports* aContainer,
                                       nsIStreamListener **aDocListener,
                                       bool aReset = true,
                                       nsIContentSink* aSink = nullptr) override;

    virtual void SetContentType(const nsAString& aContentType) override;

    virtual void EndLoad() override;

    
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE

    
    virtual void GetElementsForID(const nsAString& aID,
                                  nsCOMArray<nsIContent>& aElements) override;

    NS_IMETHOD AddSubtreeToDocument(nsIContent* aContent) override;
    NS_IMETHOD RemoveSubtreeFromDocument(nsIContent* aContent) override;
    NS_IMETHOD SetTemplateBuilderFor(nsIContent* aContent,
                                     nsIXULTemplateBuilder* aBuilder) override;
    NS_IMETHOD GetTemplateBuilderFor(nsIContent* aContent,
                                     nsIXULTemplateBuilder** aResult) override;
    NS_IMETHOD OnPrototypeLoadDone(bool aResumeWalk) override;
    bool OnDocumentParserError() override;

    
    virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

    
    NS_FORWARD_NSIDOMNODE_TO_NSINODE

    
    using nsDocument::CreateElement;
    using nsDocument::CreateElementNS;
    NS_FORWARD_NSIDOMDOCUMENT(XMLDocument::)
    
    using nsDocument::GetImplementation;
    using nsDocument::GetTitle;
    using nsDocument::SetTitle;
    using nsDocument::GetLastStyleSheetSet;
    using nsDocument::MozSetImageElement;
    using nsDocument::GetMozFullScreenElement;
    using nsIDocument::GetLocation;

    
    virtual Element* GetElementById(const nsAString & elementId) override;

    
    NS_DECL_NSIDOMXULDOCUMENT

    
    NS_IMETHOD StyleSheetLoaded(CSSStyleSheet* aSheet,
                                bool aWasAlternate,
                                nsresult aStatus) override;

    virtual void EndUpdate(nsUpdateType aUpdateType) override;

    virtual bool IsDocumentRightToLeft() override;

    virtual void ResetDocumentDirection() override;

    virtual int GetDocumentLWTheme() override;

    virtual void ResetDocumentLWTheme() override { mDocLWTheme = Doc_Theme_Uninitialized; }

    NS_IMETHOD OnScriptCompileComplete(JSScript* aScript, nsresult aStatus) override;

    static bool
    MatchAttribute(nsIContent* aContent,
                   int32_t aNameSpaceID,
                   nsIAtom* aAttrName,
                   void* aData);

    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULDocument, XMLDocument)

    void TraceProtos(JSTracer* aTrc, uint32_t aGCNumber);

    
    already_AddRefed<nsINode> GetPopupNode();
    void SetPopupNode(nsINode* aNode);
    already_AddRefed<nsINode> GetPopupRangeParent(ErrorResult& aRv);
    int32_t GetPopupRangeOffset(ErrorResult& aRv);
    already_AddRefed<nsINode> GetTooltipNode();
    void SetTooltipNode(nsINode* aNode) {  }
    nsIDOMXULCommandDispatcher* GetCommandDispatcher() const
    {
        return mCommandDispatcher;
    }
    int32_t GetWidth(ErrorResult& aRv);
    int32_t GetHeight(ErrorResult& aRv);
    already_AddRefed<nsINodeList>
      GetElementsByAttribute(const nsAString& aAttribute,
                             const nsAString& aValue);
    already_AddRefed<nsINodeList>
      GetElementsByAttributeNS(const nsAString& aNamespaceURI,
                               const nsAString& aAttribute,
                               const nsAString& aValue,
                               ErrorResult& aRv);
    void AddBroadcastListenerFor(Element& aBroadcaster, Element& aListener,
                                 const nsAString& aAttr, ErrorResult& aRv);
    void RemoveBroadcastListenerFor(Element& aBroadcaster, Element& aListener,
                                    const nsAString& aAttr);
    void Persist(const nsAString& aId, const nsAString& aAttr, ErrorResult& aRv)
    {
        aRv = Persist(aId, aAttr);
    }
    using nsDocument::GetBoxObjectFor;
    void LoadOverlay(const nsAString& aURL, nsIObserver* aObserver,
                     ErrorResult& aRv)
    {
        aRv = LoadOverlay(aURL, aObserver);
    }

protected:
    virtual ~XULDocument();

    
    friend nsresult
    (::NS_NewXULDocument(nsIXULDocument** aResult));

    nsresult Init(void) override;
    nsresult StartLayout(void);

    nsresult
    AddElementToRefMap(Element* aElement);
    void
    RemoveElementFromRefMap(Element* aElement);

    nsresult GetViewportSize(int32_t* aWidth, int32_t* aHeight);

    nsresult PrepareToLoad(nsISupports* aContainer,
                           const char* aCommand,
                           nsIChannel* aChannel,
                           nsILoadGroup* aLoadGroup,
                           nsIParser** aResult);

    nsresult
    PrepareToLoadPrototype(nsIURI* aURI,
                           const char* aCommand,
                           nsIPrincipal* aDocumentPrincipal,
                           nsIParser** aResult);

    nsresult 
    LoadOverlayInternal(nsIURI* aURI, bool aIsDynamic, bool* aShouldReturn,
                        bool* aFailureFromContent);

    nsresult ApplyPersistentAttributes();
    nsresult ApplyPersistentAttributesInternal();
    nsresult ApplyPersistentAttributesToElements(const nsAString &aID,
                                                 nsCOMArray<nsIContent>& aElements);

    nsresult
    AddElementToDocumentPre(Element* aElement);

    nsresult
    AddElementToDocumentPost(Element* aElement);

    nsresult
    ExecuteOnBroadcastHandlerFor(Element* aBroadcaster,
                                 Element* aListener,
                                 nsIAtom* aAttr);

    nsresult
    BroadcastAttributeChangeFromOverlay(nsIContent* aNode,
                                        int32_t aNameSpaceID,
                                        nsIAtom* aAttribute,
                                        nsIAtom* aPrefix,
                                        const nsAString& aValue);

    already_AddRefed<nsPIWindowRoot> GetWindowRoot();

    static void DirectionChanged(const char* aPrefName, void* aData);

    
    static int32_t gRefCnt;

    static nsIAtom** kIdentityAttrs[];

    static nsIRDFService* gRDFService;
    static nsIRDFResource* kNC_persist;
    static nsIRDFResource* kNC_attribute;
    static nsIRDFResource* kNC_value;

    static PRLogModuleInfo* gXULLog;

    nsresult
    Persist(nsIContent* aElement, int32_t aNameSpaceID, nsIAtom* aAttribute);

    virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

    
    
    
    
    
    
    

    XULDocument*             mNextSrcLoadWaiter;  

    
    
    nsTHashtable<nsRefMapEntry> mRefMap;
    nsCOMPtr<nsIXULStore>       mLocalStore;
    bool                        mApplyingPersistedAttrs;
    bool                        mIsWritingFastLoad;
    bool                        mDocumentLoaded;
    






    bool                       mStillWalking;

    


    bool                           mRestrictPersistence;
    nsTHashtable<nsStringHashKey>  mPersistenceIds;

    



    nsTArray<nsRefPtr<CSSStyleSheet>> mOverlaySheets;

    nsCOMPtr<nsIDOMXULCommandDispatcher>     mCommandDispatcher; 

    
    
    typedef nsInterfaceHashtable<nsISupportsHashKey, nsIXULTemplateBuilder>
        BuilderTable;
    BuilderTable* mTemplateBuilderTable;

    uint32_t mPendingSheets;

    



    DocumentTheme                         mDocLWTheme;

    



    class ContextStack {
    protected:
        struct Entry {
            nsXULPrototypeElement* mPrototype;
            nsIContent*            mElement;
            int32_t                mIndex;
            Entry*                 mNext;
        };

        Entry* mTop;
        int32_t mDepth;

    public:
        ContextStack();
        ~ContextStack();

        int32_t Depth() { return mDepth; }

        nsresult Push(nsXULPrototypeElement* aPrototype, nsIContent* aElement);
        nsresult Pop();
        nsresult Peek(nsXULPrototypeElement** aPrototype, nsIContent** aElement, int32_t* aIndex);

        nsresult SetTopIndex(int32_t aIndex);
    };

    friend class ContextStack;
    ContextStack mContextStack;

    enum State { eState_Master, eState_Overlay };
    State mState;

    










    nsTArray<nsCOMPtr<nsIURI> > mUnloadedOverlays;

    




    nsresult LoadScript(nsXULPrototypeScript *aScriptProto, bool* aBlock);

    



    nsresult ExecuteScript(nsXULPrototypeScript *aScript);

    



    nsresult CreateElementFromPrototype(nsXULPrototypeElement* aPrototype,
                                        Element** aResult,
                                        bool aIsRoot);

    



    nsresult CreateOverlayElement(nsXULPrototypeElement* aPrototype,
                                  Element** aResult);

    


    nsresult AddAttributes(nsXULPrototypeElement* aPrototype, nsIContent* aElement);

    





    nsXULPrototypeScript* mCurrentScriptProto;

    



    bool mOffThreadCompiling;

    



    char16_t* mOffThreadCompileStringBuf;
    size_t mOffThreadCompileStringLength;

    


    static nsresult
    CheckTemplateBuilderHookup(nsIContent* aElement, bool* aNeedsHookup);

    


    static nsresult
    CreateTemplateBuilder(nsIContent* aElement);

    



    nsresult AddPrototypeSheets();


protected:
    






    




    nsTArray<nsAutoPtr<nsForwardReference> > mForwardReferences;

    
    nsForwardReference::Phase mResolutionPhase;

    


    nsresult AddForwardReference(nsForwardReference* aRef);

    


    nsresult ResolveForwardReferences();

    


    class BroadcasterHookup : public nsForwardReference
    {
    protected:
        XULDocument* mDocument;              
        nsRefPtr<Element> mObservesElement; 
        bool mResolved;

    public:
        BroadcasterHookup(XULDocument* aDocument,
                          Element* aObservesElement)
            : mDocument(aDocument),
              mObservesElement(aObservesElement),
              mResolved(false)
        {
        }

        virtual ~BroadcasterHookup();

        virtual Phase GetPhase() override { return eHookup; }
        virtual Result Resolve() override;
    };

    friend class BroadcasterHookup;


    


    class OverlayForwardReference : public nsForwardReference
    {
    protected:
        XULDocument* mDocument;      
        nsCOMPtr<nsIContent> mOverlay; 
        bool mResolved;

        nsresult Merge(nsIContent* aTargetNode, nsIContent* aOverlayNode, bool aNotify);

    public:
        OverlayForwardReference(XULDocument* aDocument, nsIContent* aOverlay)
            : mDocument(aDocument), mOverlay(aOverlay), mResolved(false) {}

        virtual ~OverlayForwardReference();

        virtual Phase GetPhase() override { return eConstruction; }
        virtual Result Resolve() override;
    };

    friend class OverlayForwardReference;

    class TemplateBuilderHookup : public nsForwardReference
    {
    protected:
        nsCOMPtr<nsIContent> mElement; 

    public:
        explicit TemplateBuilderHookup(nsIContent* aElement)
            : mElement(aElement) {}

        virtual Phase GetPhase() override { return eHookup; }
        virtual Result Resolve() override;
    };

    friend class TemplateBuilderHookup;

    
    
    
    
    nsresult
    FindBroadcaster(Element* aElement,
                    Element** aListener,
                    nsString& aBroadcasterID,
                    nsString& aAttribute,
                    Element** aBroadcaster);

    nsresult
    CheckBroadcasterHookup(Element* aElement,
                           bool* aNeedsHookup,
                           bool* aDidResolve);

    void
    SynchronizeBroadcastListener(Element *aBroadcaster,
                                 Element *aListener,
                                 const nsAString &aAttr);

    static
    nsresult
    InsertElement(nsINode* aParent, nsIContent* aChild, bool aNotify);

    static 
    nsresult
    RemoveElement(nsINode* aParent, nsINode* aChild);

    



    nsRefPtr<nsXULPrototypeDocument> mCurrentPrototype;

    



    nsRefPtr<nsXULPrototypeDocument> mMasterPrototype;

    



    nsTArray< nsRefPtr<nsXULPrototypeDocument> > mPrototypes;

    


    nsresult PrepareToWalk();

    



    nsresult
    CreateAndInsertPI(const nsXULPrototypePI* aProtoPI,
                      nsINode* aParent, uint32_t aIndex);

    








    nsresult
    InsertXMLStylesheetPI(const nsXULPrototypePI* aProtoPI,
                          nsINode* aParent,
                          uint32_t aIndex,
                          nsIContent* aPINode);

    



    nsresult
    InsertXULOverlayPI(const nsXULPrototypePI* aProtoPI,
                       nsINode* aParent,
                       uint32_t aIndex,
                       nsIContent* aPINode);

    



    nsresult AddChromeOverlays();

    



    nsresult ResumeWalk();

    




    nsresult DoneWalking();

    



    void ReportMissingOverlay(nsIURI* aURI);

    class CachedChromeStreamListener : public nsIStreamListener {
    protected:
        nsRefPtr<XULDocument> mDocument;
        bool mProtoLoaded;

        virtual ~CachedChromeStreamListener();

    public:
        CachedChromeStreamListener(XULDocument* aDocument,
                                   bool aProtoLoaded);

        NS_DECL_ISUPPORTS
        NS_DECL_NSIREQUESTOBSERVER
        NS_DECL_NSISTREAMLISTENER
    };

    friend class CachedChromeStreamListener;


    class ParserObserver : public nsIRequestObserver {
    protected:
        nsRefPtr<XULDocument> mDocument;
        nsRefPtr<nsXULPrototypeDocument> mPrototype;
        virtual ~ParserObserver();

    public:
        ParserObserver(XULDocument* aDocument,
                       nsXULPrototypeDocument* aPrototype);

        NS_DECL_ISUPPORTS
        NS_DECL_NSIREQUESTOBSERVER
    };

    friend class ParserObserver;

    


    PLDHashTable* mBroadcasterMap;

    nsAutoPtr<nsInterfaceHashtable<nsURIHashKey,nsIObserver> > mOverlayLoadObservers;
    nsAutoPtr<nsInterfaceHashtable<nsURIHashKey,nsIObserver> > mPendingOverlayLoadNotifications;

    bool mInitialLayoutComplete;

    class nsDelayedBroadcastUpdate
    {
    public:
      nsDelayedBroadcastUpdate(Element* aBroadcaster,
                               Element* aListener,
                               const nsAString &aAttr)
      : mBroadcaster(aBroadcaster), mListener(aListener), mAttr(aAttr),
        mSetAttr(false), mNeedsAttrChange(false) {}

      nsDelayedBroadcastUpdate(Element* aBroadcaster,
                               Element* aListener,
                               nsIAtom* aAttrName,
                               const nsAString &aAttr,
                               bool aSetAttr,
                               bool aNeedsAttrChange)
      : mBroadcaster(aBroadcaster), mListener(aListener), mAttr(aAttr),
        mAttrName(aAttrName), mSetAttr(aSetAttr),
        mNeedsAttrChange(aNeedsAttrChange) {}

      nsDelayedBroadcastUpdate(const nsDelayedBroadcastUpdate& aOther)
      : mBroadcaster(aOther.mBroadcaster), mListener(aOther.mListener),
        mAttr(aOther.mAttr), mAttrName(aOther.mAttrName),
        mSetAttr(aOther.mSetAttr), mNeedsAttrChange(aOther.mNeedsAttrChange) {}

      nsCOMPtr<Element>       mBroadcaster;
      nsCOMPtr<Element>       mListener;
      
      
      nsString                mAttr;
      nsCOMPtr<nsIAtom>       mAttrName;
      bool                    mSetAttr;
      bool                    mNeedsAttrChange;

      class Comparator {
        public:
          static bool Equals(const nsDelayedBroadcastUpdate& a, const nsDelayedBroadcastUpdate& b) {
            return a.mBroadcaster == b.mBroadcaster && a.mListener == b.mListener && a.mAttrName == b.mAttrName;
          }
      };
    };

    nsTArray<nsDelayedBroadcastUpdate> mDelayedBroadcasters;
    nsTArray<nsDelayedBroadcastUpdate> mDelayedAttrChangeBroadcasts;
    bool                               mHandlingDelayedAttrChange;
    bool                               mHandlingDelayedBroadcasters;

    void MaybeBroadcast();
private:
    

};

} 
} 

#endif
