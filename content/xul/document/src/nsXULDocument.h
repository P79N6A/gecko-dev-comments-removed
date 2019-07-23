






































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
class nsIXULPrototypeCache;
class nsIFocusController;
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

class nsRefMapEntry : public nsISupportsHashKey
{
public:
  nsRefMapEntry(const nsISupports* aKey) :
    nsISupportsHashKey(aKey)
  {
  }
  nsRefMapEntry(const nsRefMapEntry& aOther) :
    nsISupportsHashKey(GetKey())
  {
    NS_ERROR("Should never be called");
  }

  nsIContent* GetFirstContent();
  void AppendAll(nsCOMArray<nsIContent>* aElements);
  


  PRBool AddContent(nsIContent* aContent);
  



  PRBool RemoveContent(nsIContent* aContent);

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
                                       PRBool aReset = PR_TRUE,
                                       nsIContentSink* aSink = nsnull);

    virtual void SetContentType(const nsAString& aContentType);

    virtual void EndLoad();

    
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE

    
    NS_IMETHOD AddElementForID(nsIContent* aElement);
    NS_IMETHOD GetElementsForID(const nsAString& aID,
                                nsCOMArray<nsIContent>& aElements);

    NS_IMETHOD GetScriptGlobalObjectOwner(nsIScriptGlobalObjectOwner** aGlobalOwner);
    NS_IMETHOD AddSubtreeToDocument(nsIContent* aElement);
    NS_IMETHOD RemoveSubtreeFromDocument(nsIContent* aElement);
    NS_IMETHOD SetTemplateBuilderFor(nsIContent* aContent,
                                     nsIXULTemplateBuilder* aBuilder);
    NS_IMETHOD GetTemplateBuilderFor(nsIContent* aContent,
                                     nsIXULTemplateBuilder** aResult);
    NS_IMETHOD OnPrototypeLoadDone(PRBool aResumeWalk);
    PRBool OnDocumentParserError();

    
    NS_IMETHOD CloneNode(PRBool deep, nsIDOMNode **_retval);

    
    NS_IMETHOD GetElementById(const nsAString & elementId,
                              nsIDOMElement **_retval); 

    
    NS_DECL_NSIDOMXULDOCUMENT

    
    NS_IMETHOD GetContentType(nsAString& aContentType);

    
    NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet* aSheet,
                                PRBool aWasAlternate,
                                nsresult aStatus);

    virtual void EndUpdate(nsUpdateType aUpdateType);

    virtual PRBool IsDocumentRightToLeft();

    virtual void ResetDocumentDirection() { mDocDirection = Direction_Uninitialized; }

    virtual int GetDocumentLWTheme();

    virtual void ResetDocumentLWTheme() { mDocLWTheme = Doc_Theme_Uninitialized; }

    static PRBool
    MatchAttribute(nsIContent* aContent,
                   PRInt32 aNameSpaceID,
                   nsIAtom* aAttrName,
                   void* aData);

    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULDocument, nsXMLDocument)

protected:
    
    friend nsresult
    NS_NewXULDocument(nsIXULDocument** aResult);

    nsresult Init(void);
    nsresult StartLayout(void);

    nsresult
    AddElementToRefMap(nsIContent* aElement);
    void
    RemoveElementFromRefMap(nsIContent* aElement);

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
    LoadOverlayInternal(nsIURI* aURI, PRBool aIsDynamic, PRBool* aShouldReturn,
                        PRBool* aFailureFromContent);

    nsresult ApplyPersistentAttributes();
    nsresult ApplyPersistentAttributesInternal();
    nsresult ApplyPersistentAttributesToElements(nsIRDFResource* aResource,
                                                 nsCOMArray<nsIContent>& aElements);

    nsresult
    AddElementToDocumentPre(nsIContent* aElement);

    nsresult
    AddElementToDocumentPost(nsIContent* aElement);

    nsresult
    ExecuteOnBroadcastHandlerFor(nsIContent* aBroadcaster,
                                 nsIDOMElement* aListener,
                                 nsIAtom* aAttr);

    void GetFocusController(nsIFocusController** aFocusController);

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

    PRBool
    IsCapabilityEnabled(const char* aCapabilityLabel);

    nsresult
    Persist(nsIContent* aElement, PRInt32 aNameSpaceID, nsIAtom* aAttribute);

    
    
    
    
    
    
    

    nsXULDocument*             mNextSrcLoadWaiter;  

    
    
    nsTHashtable<nsRefMapEntry> mRefMap;
    nsCOMPtr<nsIRDFDataSource> mLocalStore;
    PRPackedBool               mApplyingPersistedAttrs;
    PRPackedBool               mIsWritingFastLoad;
    PRPackedBool               mDocumentLoaded;
    






    PRPackedBool               mStillWalking;

    



    nsCOMArray<nsICSSStyleSheet> mOverlaySheets;

    nsCOMPtr<nsIDOMXULCommandDispatcher>     mCommandDispatcher; 

    
    
    typedef nsInterfaceHashtable<nsISupportsHashKey, nsIXULTemplateBuilder>
        BuilderTable;
    BuilderTable* mTemplateBuilderTable;

    PRUint32 mPendingSheets;

    
















    nsCOMPtr<nsIDOMNode>    mTooltipNode;          

    


    enum DocumentDirection {
      Direction_Uninitialized, 
      Direction_LeftToRight,
      Direction_RightToLeft
    };

    DocumentDirection               mDocDirection;

    



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

        PRBool IsInsideXULTemplate();
    };

    friend class ContextStack;
    ContextStack mContextStack;

    enum State { eState_Master, eState_Overlay };
    State mState;

    










    nsCOMArray<nsIURI> mUnloadedOverlays;

    




    nsresult LoadScript(nsXULPrototypeScript *aScriptProto, PRBool* aBlock);

    



    nsresult ExecuteScript(nsIScriptContext *aContext, void* aScriptObject);

    



    nsresult ExecuteScript(nsXULPrototypeScript *aScript);

    



    nsresult CreateElementFromPrototype(nsXULPrototypeElement* aPrototype,
                                        nsIContent** aResult);

    



    nsresult CreateOverlayElement(nsXULPrototypeElement* aPrototype, nsIContent** aResult);

    


    nsresult AddAttributes(nsXULPrototypeElement* aPrototype, nsIContent* aElement);

    





    nsXULPrototypeScript* mCurrentScriptProto;

    


    static nsresult
    CheckTemplateBuilderHookup(nsIContent* aElement, PRBool* aNeedsHookup);

    


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
        nsCOMPtr<nsIContent> mObservesElement; 
        PRBool mResolved;

    public:
        BroadcasterHookup(nsXULDocument* aDocument,
                          nsIContent* aObservesElement)
            : mDocument(aDocument),
              mObservesElement(aObservesElement),
              mResolved(PR_FALSE)
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
        PRBool mResolved;

        nsresult Merge(nsIContent* aTargetNode, nsIContent* aOverlayNode, PRBool aNotify);

    public:
        OverlayForwardReference(nsXULDocument* aDocument, nsIContent* aOverlay)
            : mDocument(aDocument), mOverlay(aOverlay), mResolved(PR_FALSE) {}

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
    FindBroadcaster(nsIContent* aElement,
                    nsIDOMElement** aListener,
                    nsString& aBroadcasterID,
                    nsString& aAttribute,
                    nsIDOMElement** aBroadcaster);

    nsresult
    CheckBroadcasterHookup(nsIContent* aElement,
                           PRBool* aNeedsHookup,
                           PRBool* aDidResolve);

    void
    SynchronizeBroadcastListener(nsIDOMElement   *aBroadcaster,
                                 nsIDOMElement   *aListener,
                                 const nsAString &aAttr);

    static
    nsresult
    InsertElement(nsIContent* aParent, nsIContent* aChild, PRBool aNotify);

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
    
#if defined(DEBUG_waterson) || defined(DEBUG_hyatt)
    
    nsTime mLoadStart;
#endif

    class CachedChromeStreamListener : public nsIStreamListener {
    protected:
        nsXULDocument* mDocument;
        PRPackedBool   mProtoLoaded;

        virtual ~CachedChromeStreamListener();

    public:
        CachedChromeStreamListener(nsXULDocument* aDocument,
                                   PRBool aProtoLoaded);

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
    
    PRBool mInitialLayoutComplete;

    class nsDelayedBroadcastUpdate
    {
    public:
      nsDelayedBroadcastUpdate(nsIDOMElement* aBroadcaster,
                               nsIDOMElement* aListener,
                               const nsAString &aAttr)
      : mBroadcaster(aBroadcaster), mListener(aListener), mAttr(aAttr),
        mSetAttr(PR_FALSE), mNeedsAttrChange(PR_FALSE) {}

      nsDelayedBroadcastUpdate(nsIDOMElement* aBroadcaster,
                               nsIDOMElement* aListener,
                               nsIAtom* aAttrName,
                               const nsAString &aAttr,
                               PRBool aSetAttr,
                               PRBool aNeedsAttrChange)
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
      PRPackedBool            mSetAttr;
      PRPackedBool            mNeedsAttrChange;

      class Comparator {
        public:
          static PRBool Equals(const nsDelayedBroadcastUpdate& a, const nsDelayedBroadcastUpdate& b) {
            return a.mBroadcaster == b.mBroadcaster && a.mListener == b.mListener && a.mAttrName == b.mAttrName;
          }
      };
    };

    nsTArray<nsDelayedBroadcastUpdate> mDelayedBroadcasters;
    nsTArray<nsDelayedBroadcastUpdate> mDelayedAttrChangeBroadcasts;
    PRPackedBool                       mHandlingDelayedAttrChange;
    PRPackedBool                       mHandlingDelayedBroadcasters;

    void MaybeBroadcast();
private:
    

};

#endif
