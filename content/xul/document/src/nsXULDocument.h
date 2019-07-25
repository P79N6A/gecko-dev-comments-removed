






































#ifndef nsXULDocument_h__
#define nsXULDocument_h__

#include "nsCOMPtr.h"
#include "nsXULPrototypeDocument.h"
#include "nsXULPrototypeCache.h"
#include "nsTArray.h"

#include "nsXMLDocument.h"
#include "nsForwardReference.h"
#include "nsIContent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULDocument.h"
#include "nsCOMArray.h"
#include "nsIURI.h"
#include "nsIXULDocument.h"
#include "nsScriptLoader.h"
#include "nsIStreamListener.h"
#include "nsICSSLoaderObserver.h"

class nsIRDFResource;
class nsIRDFService;
class nsPIWindowRoot;
#if 0 
class nsIObjectInputStream;
class nsIObjectOutputStream;
class nsIXULPrototypeScript;
#else
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsXULElement.h"
#endif
#include "nsURIHashKey.h"
#include "nsInterfaceHashtable.h"
 
struct JSObject;
struct PRLogModuleInfo;

class nsRefMapEntry : public nsStringHashKey
{
public:
  nsRefMapEntry(const nsAString& aKey) :
    nsStringHashKey(&aKey)
  {
  }
  nsRefMapEntry(const nsAString *aKey) :
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




class nsXULDocument : public nsXMLDocument,
                      public nsIXULDocument,
                      public nsIDOMXULDocument,
                      public nsIStreamLoaderObserver,
                      public nsICSSLoaderObserver
{
public:
    nsXULDocument();
    virtual ~nsXULDocument();

    
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSISTREAMLOADEROBSERVER

    
    virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup);
    virtual void ResetToURI(nsIURI *aURI, nsILoadGroup* aLoadGroup,
                            nsIPrincipal* aPrincipal);

    virtual nsresult StartDocumentLoad(const char* aCommand,
                                       nsIChannel *channel,
                                       nsILoadGroup* aLoadGroup,
                                       nsISupports* aContainer,
                                       nsIStreamListener **aDocListener,
                                       bool aReset = true,
                                       nsIContentSink* aSink = nsnull);

    virtual void SetContentType(const nsAString& aContentType);

    virtual void EndLoad();

    
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE

    
    virtual void GetElementsForID(const nsAString& aID,
                                  nsCOMArray<nsIContent>& aElements);

    NS_IMETHOD GetScriptGlobalObjectOwner(nsIScriptGlobalObjectOwner** aGlobalOwner);
    NS_IMETHOD AddSubtreeToDocument(nsIContent* aContent);
    NS_IMETHOD RemoveSubtreeFromDocument(nsIContent* aContent);
    NS_IMETHOD SetTemplateBuilderFor(nsIContent* aContent,
                                     nsIXULTemplateBuilder* aBuilder);
    NS_IMETHOD GetTemplateBuilderFor(nsIContent* aContent,
                                     nsIXULTemplateBuilder** aResult);
    NS_IMETHOD OnPrototypeLoadDone(bool aResumeWalk);
    bool OnDocumentParserError();

    
    NS_IMETHOD CloneNode(bool deep, nsIDOMNode **_retval);

    
    NS_IMETHOD GetContentType(nsAString& aContentType);

    
    NS_IMETHOD GetElementById(const nsAString& aId, nsIDOMElement** aReturn)
    {
        return nsDocument::GetElementById(aId, aReturn);
    }
    virtual mozilla::dom::Element* GetElementById(const nsAString & elementId);

    
    NS_DECL_NSIDOMXULDOCUMENT

    
    NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet* aSheet,
                                bool aWasAlternate,
                                nsresult aStatus);

    virtual void EndUpdate(nsUpdateType aUpdateType);

    virtual bool IsDocumentRightToLeft();

    virtual void ResetDocumentDirection();

    virtual int GetDocumentLWTheme();

    virtual void ResetDocumentLWTheme() { mDocLWTheme = Doc_Theme_Uninitialized; }

    static bool
    MatchAttribute(nsIContent* aContent,
                   PRInt32 aNameSpaceID,
                   nsIAtom* aAttrName,
                   void* aData);

    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULDocument, nsXMLDocument)

    virtual nsXPCClassInfo* GetClassInfo();
protected:
    
    friend nsresult
    NS_NewXULDocument(nsIXULDocument** aResult);

    nsresult Init(void);
    nsresult StartLayout(void);

    nsresult
    AddElementToRefMap(mozilla::dom::Element* aElement);
    void
    RemoveElementFromRefMap(mozilla::dom::Element* aElement);

    nsresult GetViewportSize(PRInt32* aWidth, PRInt32* aHeight);

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
    nsresult ApplyPersistentAttributesToElements(nsIRDFResource* aResource,
                                                 nsCOMArray<nsIContent>& aElements);

    nsresult
    AddElementToDocumentPre(mozilla::dom::Element* aElement);

    nsresult
    AddElementToDocumentPost(mozilla::dom::Element* aElement);

    nsresult
    ExecuteOnBroadcastHandlerFor(nsIContent* aBroadcaster,
                                 nsIDOMElement* aListener,
                                 nsIAtom* aAttr);

    nsresult
    BroadcastAttributeChangeFromOverlay(nsIContent* aNode,
                                        PRInt32 aNameSpaceID,
                                        nsIAtom* aAttribute,
                                        nsIAtom* aPrefix,
                                        const nsAString& aValue);

    already_AddRefed<nsPIWindowRoot> GetWindowRoot();

    PRInt32 GetDefaultNamespaceID() const
    {
        return kNameSpaceID_XUL;
    }

    static NS_HIDDEN_(int) DirectionChanged(const char* aPrefName, void* aData);

    
    static PRInt32 gRefCnt;

    static nsIAtom** kIdentityAttrs[];

    static nsIRDFService* gRDFService;
    static nsIRDFResource* kNC_persist;
    static nsIRDFResource* kNC_attribute;
    static nsIRDFResource* kNC_value;

    static nsXULPrototypeCache* gXULCache;

    static PRLogModuleInfo* gXULLog;

    bool
    IsCapabilityEnabled(const char* aCapabilityLabel);

    nsresult
    Persist(nsIContent* aElement, PRInt32 aNameSpaceID, nsIAtom* aAttribute);

    
    
    
    
    
    
    

    nsXULDocument*             mNextSrcLoadWaiter;  

    
    
    nsTHashtable<nsRefMapEntry> mRefMap;
    nsCOMPtr<nsIRDFDataSource> mLocalStore;
    bool                       mApplyingPersistedAttrs;
    bool                       mIsWritingFastLoad;
    bool                       mDocumentLoaded;
    






    bool                       mStillWalking;

    



    nsTArray<nsRefPtr<nsCSSStyleSheet> > mOverlaySheets;

    nsCOMPtr<nsIDOMXULCommandDispatcher>     mCommandDispatcher; 

    
    
    typedef nsInterfaceHashtable<nsISupportsHashKey, nsIXULTemplateBuilder>
        BuilderTable;
    BuilderTable* mTemplateBuilderTable;

    PRUint32 mPendingSheets;

    



    DocumentTheme                         mDocLWTheme;

    



    class ContextStack {
    protected:
        struct Entry {
            nsXULPrototypeElement* mPrototype;
            nsIContent*            mElement;
            PRInt32                mIndex;
            Entry*                 mNext;
        };

        Entry* mTop;
        PRInt32 mDepth;

    public:
        ContextStack();
        ~ContextStack();

        PRInt32 Depth() { return mDepth; }

        nsresult Push(nsXULPrototypeElement* aPrototype, nsIContent* aElement);
        nsresult Pop();
        nsresult Peek(nsXULPrototypeElement** aPrototype, nsIContent** aElement, PRInt32* aIndex);

        nsresult SetTopIndex(PRInt32 aIndex);

        bool IsInsideXULTemplate();
    };

    friend class ContextStack;
    ContextStack mContextStack;

    enum State { eState_Master, eState_Overlay };
    State mState;

    










    nsCOMArray<nsIURI> mUnloadedOverlays;

    




    nsresult LoadScript(nsXULPrototypeScript *aScriptProto, bool* aBlock);

    



    nsresult ExecuteScript(nsIScriptContext *aContext, JSScript* aScriptObject);

    



    nsresult ExecuteScript(nsXULPrototypeScript *aScript);

    



    nsresult CreateElementFromPrototype(nsXULPrototypeElement* aPrototype,
                                        mozilla::dom::Element** aResult);

    



    nsresult CreateOverlayElement(nsXULPrototypeElement* aPrototype,
                                  mozilla::dom::Element** aResult);

    


    nsresult AddAttributes(nsXULPrototypeElement* aPrototype, nsIContent* aElement);

    





    nsXULPrototypeScript* mCurrentScriptProto;

    


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
        nsXULDocument* mDocument;              
        nsRefPtr<mozilla::dom::Element> mObservesElement; 
        bool mResolved;

    public:
        BroadcasterHookup(nsXULDocument* aDocument,
                          mozilla::dom::Element* aObservesElement)
            : mDocument(aDocument),
              mObservesElement(aObservesElement),
              mResolved(false)
        {
        }

        virtual ~BroadcasterHookup();

        virtual Phase GetPhase() { return eHookup; }
        virtual Result Resolve();
    };

    friend class BroadcasterHookup;


    


    class OverlayForwardReference : public nsForwardReference
    {
    protected:
        nsXULDocument* mDocument;      
        nsCOMPtr<nsIContent> mOverlay; 
        bool mResolved;

        nsresult Merge(nsIContent* aTargetNode, nsIContent* aOverlayNode, bool aNotify);

    public:
        OverlayForwardReference(nsXULDocument* aDocument, nsIContent* aOverlay)
            : mDocument(aDocument), mOverlay(aOverlay), mResolved(false) {}

        virtual ~OverlayForwardReference();

        virtual Phase GetPhase() { return eConstruction; }
        virtual Result Resolve();
    };

    friend class OverlayForwardReference;

    class TemplateBuilderHookup : public nsForwardReference
    {
    protected:
        nsCOMPtr<nsIContent> mElement; 

    public:
        TemplateBuilderHookup(nsIContent* aElement)
            : mElement(aElement) {}

        virtual Phase GetPhase() { return eHookup; }
        virtual Result Resolve();
    };

    friend class TemplateBuilderHookup;

    
    
    
    
    nsresult
    FindBroadcaster(mozilla::dom::Element* aElement,
                    nsIDOMElement** aListener,
                    nsString& aBroadcasterID,
                    nsString& aAttribute,
                    nsIDOMElement** aBroadcaster);

    nsresult
    CheckBroadcasterHookup(mozilla::dom::Element* aElement,
                           bool* aNeedsHookup,
                           bool* aDidResolve);

    void
    SynchronizeBroadcastListener(nsIDOMElement   *aBroadcaster,
                                 nsIDOMElement   *aListener,
                                 const nsAString &aAttr);

    static
    nsresult
    InsertElement(nsIContent* aParent, nsIContent* aChild, bool aNotify);

    static 
    nsresult
    RemoveElement(nsIContent* aParent, nsIContent* aChild);

    



    nsRefPtr<nsXULPrototypeDocument> mCurrentPrototype;

    



    nsRefPtr<nsXULPrototypeDocument> mMasterPrototype;

    



    nsTArray< nsRefPtr<nsXULPrototypeDocument> > mPrototypes;

    


    nsresult PrepareToWalk();

    



    nsresult
    CreateAndInsertPI(const nsXULPrototypePI* aProtoPI,
                      nsINode* aParent, PRUint32 aIndex);

    








    nsresult
    InsertXMLStylesheetPI(const nsXULPrototypePI* aProtoPI,
                          nsINode* aParent,
                          PRUint32 aIndex,
                          nsIContent* aPINode);

    



    nsresult
    InsertXULOverlayPI(const nsXULPrototypePI* aProtoPI,
                       nsINode* aParent,
                       PRUint32 aIndex,
                       nsIContent* aPINode);

    



    nsresult AddChromeOverlays();

    



    nsresult ResumeWalk();

    




    nsresult DoneWalking();

    



    void ReportMissingOverlay(nsIURI* aURI);

    class CachedChromeStreamListener : public nsIStreamListener {
    protected:
        nsXULDocument* mDocument;
        bool           mProtoLoaded;

        virtual ~CachedChromeStreamListener();

    public:
        CachedChromeStreamListener(nsXULDocument* aDocument,
                                   bool aProtoLoaded);

        NS_DECL_ISUPPORTS
        NS_DECL_NSIREQUESTOBSERVER
        NS_DECL_NSISTREAMLISTENER
    };

    friend class CachedChromeStreamListener;


    class ParserObserver : public nsIRequestObserver {
    protected:
        nsRefPtr<nsXULDocument> mDocument;
        nsRefPtr<nsXULPrototypeDocument> mPrototype;
        virtual ~ParserObserver();

    public:
        ParserObserver(nsXULDocument* aDocument,
                       nsXULPrototypeDocument* aPrototype);

        NS_DECL_ISUPPORTS
        NS_DECL_NSIREQUESTOBSERVER
    };

    friend class ParserObserver;

    


    PLDHashTable* mBroadcasterMap;

    nsInterfaceHashtable<nsURIHashKey,nsIObserver> mOverlayLoadObservers;
    nsInterfaceHashtable<nsURIHashKey,nsIObserver> mPendingOverlayLoadNotifications;
    
    bool mInitialLayoutComplete;

    class nsDelayedBroadcastUpdate
    {
    public:
      nsDelayedBroadcastUpdate(nsIDOMElement* aBroadcaster,
                               nsIDOMElement* aListener,
                               const nsAString &aAttr)
      : mBroadcaster(aBroadcaster), mListener(aListener), mAttr(aAttr),
        mSetAttr(false), mNeedsAttrChange(false) {}

      nsDelayedBroadcastUpdate(nsIDOMElement* aBroadcaster,
                               nsIDOMElement* aListener,
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

      nsCOMPtr<nsIDOMElement> mBroadcaster;
      nsCOMPtr<nsIDOMElement> mListener;
      
      
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

#endif
