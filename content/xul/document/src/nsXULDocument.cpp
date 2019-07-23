




























































#include "nsXULDocument.h"

#include "nsDOMError.h"
#include "nsIBoxObject.h"
#include "nsIChromeRegistry.h"
#include "nsIScrollableView.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIContentViewer.h"
#include "nsGUIEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMXULElement.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIRDFNode.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIStreamListener.h"
#include "nsITimer.h"
#include "nsIDocShell.h"
#include "nsGkAtoms.h"
#include "nsXMLContentSink.h"
#include "nsXULContentSink.h"
#include "nsXULContentUtils.h"
#include "nsIXULOverlayProvider.h"
#include "nsNetUtil.h"
#include "nsParserUtils.h"
#include "nsParserCIID.h"
#include "nsPIBoxObject.h"
#include "nsRDFCID.h"
#include "nsILocalStore.h"
#include "nsXPIDLString.h"
#include "nsPIDOMWindow.h"
#include "nsXULCommandDispatcher.h"
#include "nsXULDocument.h"
#include "nsXULElement.h"
#include "prlog.h"
#include "rdf.h"
#include "nsIFrame.h"
#include "nsIXBLService.h"
#include "nsCExternalHandlerService.h"
#include "nsMimeTypes.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIFocusController.h"
#include "nsContentList.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptSecurityManager.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsIParser.h"
#include "nsIParserService.h"
#include "nsICSSStyleSheet.h"
#include "nsIScriptError.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsEventDispatcher.h"
#include "nsContentErrors.h"
#include "nsIObserverService.h"
#include "nsNodeUtils.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIXULWindow.h"
#include "nsXULPopupManager.h"
#include "nsCCUncollectableMarker.h"
#include "nsURILoader.h"
#include "nsCSSFrameConstructor.h"






static NS_DEFINE_CID(kParserCID,                 NS_PARSER_CID);

static PRBool IsChromeURI(nsIURI* aURI)
{
    
    PRBool isChrome = PR_FALSE;
    if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome)
        return PR_TRUE;
    return PR_FALSE;
}






const nsForwardReference::Phase nsForwardReference::kPasses[] = {
    nsForwardReference::eConstruction,
    nsForwardReference::eHookup,
    nsForwardReference::eDone
};

const PRUint32 kMaxAttrNameLength = 512;
const PRUint32 kMaxAttributeLength = 4096;






PRInt32 nsXULDocument::gRefCnt = 0;

nsIRDFService* nsXULDocument::gRDFService;
nsIRDFResource* nsXULDocument::kNC_persist;
nsIRDFResource* nsXULDocument::kNC_attribute;
nsIRDFResource* nsXULDocument::kNC_value;

PRLogModuleInfo* nsXULDocument::gXULLog;



struct BroadcasterMapEntry : public PLDHashEntryHdr {
    nsIDOMElement*   mBroadcaster; 
    nsSmallVoidArray mListeners;   
};

struct BroadcastListener {
    nsWeakPtr mListener;
    nsCOMPtr<nsIAtom> mAttribute;
};

nsIContent*
nsRefMapEntry::GetFirstContent()
{
    return static_cast<nsIContent*>(mRefContentList.SafeElementAt(0));
}

void
nsRefMapEntry::AppendAll(nsCOMArray<nsIContent>* aElements)
{
    for (PRInt32 i = 0; i < mRefContentList.Count(); ++i) {
        aElements->AppendObject(static_cast<nsIContent*>(mRefContentList[i]));
    }
}

PRBool
nsRefMapEntry::AddContent(nsIContent* aContent)
{
    if (mRefContentList.IndexOf(aContent) >= 0)
        return PR_TRUE;
    return mRefContentList.AppendElement(aContent);
}

PRBool
nsRefMapEntry::RemoveContent(nsIContent* aContent)
{
    mRefContentList.RemoveElement(aContent);
    return mRefContentList.Count() == 0;
}






    
    

nsXULDocument::nsXULDocument(void)
    : nsXMLDocument("application/vnd.mozilla.xul+xml"),
      mDocDirection(Direction_Uninitialized),
      mDocLWTheme(Doc_Theme_Uninitialized),
      mState(eState_Master),
      mResolutionPhase(nsForwardReference::eStart)
{

    
    

    
    mCharacterSet.AssignLiteral("UTF-8");

    mDefaultElementType = kNameSpaceID_XUL;

    mDelayFrameLoaderInitialization = PR_TRUE;
}

nsXULDocument::~nsXULDocument()
{
    NS_ASSERTION(mNextSrcLoadWaiter == nsnull,
        "unreferenced document still waiting for script source to load?");

    
    
    mForwardReferences.Clear();

    
    if (mBroadcasterMap) {
        PL_DHashTableDestroy(mBroadcasterMap);
    }

    if (mLocalStore) {
        nsCOMPtr<nsIRDFRemoteDataSource> remote =
            do_QueryInterface(mLocalStore);
        if (remote)
            remote->Flush();
    }

    delete mTemplateBuilderTable;

    nsContentUtils::UnregisterPrefCallback("intl.uidirection.",
                                           nsXULDocument::DirectionChanged,
                                           this);

    if (--gRefCnt == 0) {
        NS_IF_RELEASE(gRDFService);

        NS_IF_RELEASE(kNC_persist);
        NS_IF_RELEASE(kNC_attribute);
        NS_IF_RELEASE(kNC_value);

        
        
        
        
        if (mDocumentURI)
            nsXULPrototypeCache::GetInstance()->RemoveFromFastLoadSet(mDocumentURI);
    }
}

nsresult
NS_NewXULDocument(nsIXULDocument** result)
{
    NS_PRECONDITION(result != nsnull, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    nsXULDocument* doc = new nsXULDocument();
    if (! doc)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(doc);

    nsresult rv;
    if (NS_FAILED(rv = doc->Init())) {
        NS_RELEASE(doc);
        return rv;
    }

    *result = doc;
    return NS_OK;
}







NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULDocument)

static PLDHashOperator
TraverseTemplateBuilders(nsISupports* aKey, nsIXULTemplateBuilder* aData,
                         void* aContext)
{
    nsCycleCollectionTraversalCallback *cb =
        static_cast<nsCycleCollectionTraversalCallback*>(aContext);

    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mTemplateBuilderTable key");
    cb->NoteXPCOMChild(aKey);
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mTemplateBuilderTable value");
    cb->NoteXPCOMChild(aData);

    return PL_DHASH_NEXT;
}

static PLDHashOperator
TraverseObservers(nsIURI* aKey, nsIObserver* aData, void* aContext)
{
    nsCycleCollectionTraversalCallback *cb =
        static_cast<nsCycleCollectionTraversalCallback*>(aContext);

    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mOverlayLoadObservers/mPendingOverlayLoadNotifications value");
    cb->NoteXPCOMChild(aData);

    return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXULDocument, nsXMLDocument)
    NS_ASSERTION(!nsCCUncollectableMarker::InGeneration(cb, tmp->GetMarkedCCGeneration()),
                 "Shouldn't traverse nsXULDocument!");
    
    

    
    
    if (tmp->mTemplateBuilderTable)
        tmp->mTemplateBuilderTable->EnumerateRead(TraverseTemplateBuilders, &cb);
        
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mCurrentPrototype,
                                                     nsIScriptGlobalObjectOwner)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mMasterPrototype,
                                                     nsIScriptGlobalObjectOwner)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mCommandDispatcher,
                                                     nsIDOMXULCommandDispatcher)

    PRUint32 i, count = tmp->mPrototypes.Length();
    for (i = 0; i < count; ++i) {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mPrototypes[i]");
        cb.NoteXPCOMChild(static_cast<nsIScriptGlobalObjectOwner*>(tmp->mPrototypes[i]));
    }

    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTooltipNode)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLocalStore)

    if (tmp->mOverlayLoadObservers.IsInitialized())
        tmp->mOverlayLoadObservers.EnumerateRead(TraverseObservers, &cb);
    if (tmp->mPendingOverlayLoadNotifications.IsInitialized())
        tmp->mPendingOverlayLoadNotifications.EnumerateRead(TraverseObservers, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsXULDocument, nsXMLDocument)
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTooltipNode)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(nsXULDocument, nsXMLDocument)
NS_IMPL_RELEASE_INHERITED(nsXULDocument, nsXMLDocument)



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsXULDocument)
    NS_DOCUMENT_INTERFACE_TABLE_BEGIN(nsXULDocument)
      NS_INTERFACE_TABLE_ENTRY(nsXULDocument, nsIXULDocument)
      NS_INTERFACE_TABLE_ENTRY(nsXULDocument, nsIDOMXULDocument)
      NS_INTERFACE_TABLE_ENTRY(nsXULDocument, nsIStreamLoaderObserver)
      NS_INTERFACE_TABLE_ENTRY(nsXULDocument, nsICSSLoaderObserver)
    NS_OFFSET_AND_INTERFACE_TABLE_END
    NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
    NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XULDocument)
NS_INTERFACE_MAP_END_INHERITING(nsXMLDocument)







void
nsXULDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
    NS_NOTREACHED("Reset");
}

void
nsXULDocument::ResetToURI(nsIURI* aURI, nsILoadGroup* aLoadGroup,
                          nsIPrincipal* aPrincipal)
{
    NS_NOTREACHED("ResetToURI");
}




NS_IMETHODIMP
nsXULDocument::GetContentType(nsAString& aContentType)
{
    aContentType.AssignLiteral("application/vnd.mozilla.xul+xml");
    return NS_OK;
}

void
nsXULDocument::SetContentType(const nsAString& aContentType)
{
    NS_ASSERTION(aContentType.EqualsLiteral("application/vnd.mozilla.xul+xml"),
                 "xul-documents always has content-type application/vnd.mozilla.xul+xml");
    
    
}



nsresult
nsXULDocument::StartDocumentLoad(const char* aCommand, nsIChannel* aChannel,
                                 nsILoadGroup* aLoadGroup,
                                 nsISupports* aContainer,
                                 nsIStreamListener **aDocListener,
                                 PRBool aReset, nsIContentSink* aSink)
{
    
    
    mStillWalking = PR_TRUE;
    mMayStartLayout = PR_FALSE;
    mDocumentLoadGroup = do_GetWeakReference(aLoadGroup);

    mChannel = aChannel;

    mHaveInputEncoding = PR_TRUE;

    
    nsresult rv =
        NS_GetFinalChannelURI(aChannel, getter_AddRefs(mDocumentURI));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = ResetStylesheetsToURI(mDocumentURI);
    if (NS_FAILED(rv)) return rv;

    RetrieveRelevantHeaders(aChannel);

    
    
    nsXULPrototypeDocument* proto = IsChromeURI(mDocumentURI) ?
            nsXULPrototypeCache::GetInstance()->GetPrototype(mDocumentURI) :
            nsnull;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (proto) {
        
        
        
        
        
        PRBool loaded;
        rv = proto->AwaitLoadDone(this, &loaded);
        if (NS_FAILED(rv)) return rv;

        mMasterPrototype = mCurrentPrototype = proto;

        
        SetPrincipal(proto->DocumentPrincipal());

        
        
        
        
        *aDocListener = new CachedChromeStreamListener(this, loaded);
        if (! *aDocListener)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    else {
        PRBool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();
        PRBool fillXULCache = (useXULCache && IsChromeURI(mDocumentURI));


        
        

        nsCOMPtr<nsIParser> parser;
        rv = PrepareToLoad(aContainer, aCommand, aChannel, aLoadGroup,
                           getter_AddRefs(parser));
        if (NS_FAILED(rv)) return rv;

        
        
        
        mIsWritingFastLoad = useXULCache;

        nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(parser, &rv);
        NS_ASSERTION(NS_SUCCEEDED(rv), "parser doesn't support nsIStreamListener");
        if (NS_FAILED(rv)) return rv;

        *aDocListener = listener;

        parser->Parse(mDocumentURI);

        
        
        
        
        if (fillXULCache) {
            nsXULPrototypeCache::GetInstance()->PutPrototype(mCurrentPrototype);
        }
    }

    NS_IF_ADDREF(*aDocListener);
    return NS_OK;
}



void
nsXULDocument::EndLoad()
{
    
    if (!mCurrentPrototype)
        return;

    nsresult rv;

    
    

    nsCOMPtr<nsIURI> uri = mCurrentPrototype->GetURI();
    PRBool isChrome = IsChromeURI(uri);

    
    PRBool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();

    
    
    
    
    if (useXULCache && mIsWritingFastLoad && isChrome &&
        mMasterPrototype != mCurrentPrototype) {
        nsXULPrototypeCache::GetInstance()->WritePrototype(mCurrentPrototype);
    }

    if (isChrome) {
        nsCOMPtr<nsIXULOverlayProvider> reg =
            do_GetService(NS_CHROMEREGISTRY_CONTRACTID);

        if (reg) {
            nsCOMPtr<nsISimpleEnumerator> overlays;
            rv = reg->GetStyleOverlays(uri, getter_AddRefs(overlays));
            if (NS_FAILED(rv)) return;

            PRBool moreSheets;
            nsCOMPtr<nsISupports> next;
            nsCOMPtr<nsIURI> sheetURI;

            while (NS_SUCCEEDED(rv = overlays->HasMoreElements(&moreSheets)) &&
                   moreSheets) {
                overlays->GetNext(getter_AddRefs(next));

                sheetURI = do_QueryInterface(next);
                if (!sheetURI) {
                    NS_ERROR("Chrome registry handed me a non-nsIURI object!");
                    continue;
                }

                if (IsChromeURI(sheetURI)) {
                    mCurrentPrototype->AddStyleSheetReference(sheetURI);
                }
            }
        }

        if (useXULCache) {
            
            
            
            rv = mCurrentPrototype->NotifyLoadDone();
            if (NS_FAILED(rv)) return;
        }
    }

    OnPrototypeLoadDone(PR_TRUE);
}

NS_IMETHODIMP
nsXULDocument::OnPrototypeLoadDone(PRBool aResumeWalk)
{
    nsresult rv;

    
    rv = AddPrototypeSheets();
    if (NS_FAILED(rv)) return rv;

    rv = PrepareToWalk();
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to prepare for walk");
    if (NS_FAILED(rv)) return rv;

    if (aResumeWalk) {
        rv = ResumeWalk();
    }
    return rv;
}


PRBool
nsXULDocument::OnDocumentParserError()
{
  
  if (mCurrentPrototype && mMasterPrototype != mCurrentPrototype) {
    nsCOMPtr<nsIURI> uri = mCurrentPrototype->GetURI();
    if (IsChromeURI(uri)) {
      nsCOMPtr<nsIObserverService> os(
        do_GetService("@mozilla.org/observer-service;1"));
      if (os)
        os->NotifyObservers(uri, "xul-overlay-parsererror",
                            EmptyString().get());
    }

    return PR_FALSE;
  }

  return PR_TRUE;
}

static void
ClearBroadcasterMapEntry(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
    BroadcasterMapEntry* entry =
        static_cast<BroadcasterMapEntry*>(aEntry);
    for (PRInt32 i = entry->mListeners.Count() - 1; i >= 0; --i) {
        delete (BroadcastListener*)entry->mListeners[i];
    }

    
    
    entry->mListeners.~nsSmallVoidArray();
}

static PRBool
CanBroadcast(PRInt32 aNameSpaceID, nsIAtom* aAttribute)
{
    
    
    if (aNameSpaceID == kNameSpaceID_None) {
        if ((aAttribute == nsGkAtoms::id) ||
            (aAttribute == nsGkAtoms::ref) ||
            (aAttribute == nsGkAtoms::persist) ||
            (aAttribute == nsGkAtoms::command) ||
            (aAttribute == nsGkAtoms::observes)) {
            return PR_FALSE;
        }
    }
    return PR_TRUE;
}

struct nsAttrNameInfo
{
  nsAttrNameInfo(PRInt32 aNamespaceID, nsIAtom* aName, nsIAtom* aPrefix) :
    mNamespaceID(aNamespaceID), mName(aName), mPrefix(aPrefix) {}
  nsAttrNameInfo(const nsAttrNameInfo& aOther) :
    mNamespaceID(aOther.mNamespaceID), mName(aOther.mName),
    mPrefix(aOther.mPrefix) {}
  PRInt32           mNamespaceID;
  nsCOMPtr<nsIAtom> mName;
  nsCOMPtr<nsIAtom> mPrefix;
};

void
nsXULDocument::SynchronizeBroadcastListener(nsIDOMElement   *aBroadcaster,
                                            nsIDOMElement   *aListener,
                                            const nsAString &aAttr)
{
    if (!nsContentUtils::IsSafeToRunScript()) {
        nsDelayedBroadcastUpdate delayedUpdate(aBroadcaster, aListener,
                                               aAttr);
        mDelayedBroadcasters.AppendElement(delayedUpdate);
        MaybeBroadcast();
        return;
    }
    nsCOMPtr<nsIContent> broadcaster = do_QueryInterface(aBroadcaster);
    nsCOMPtr<nsIContent> listener = do_QueryInterface(aListener);
    PRBool notify = mInitialLayoutComplete || mHandlingDelayedBroadcasters;

    
    
    listener->SetScriptTypeID(broadcaster->GetScriptTypeID());

    if (aAttr.EqualsLiteral("*")) {
        PRUint32 count = broadcaster->GetAttrCount();
        nsTArray<nsAttrNameInfo> attributes(count);
        for (PRUint32 i = 0; i < count; ++i) {
            const nsAttrName* attrName = broadcaster->GetAttrNameAt(i);
            PRInt32 nameSpaceID = attrName->NamespaceID();
            nsIAtom* name = attrName->LocalName();

            
            if (! CanBroadcast(nameSpaceID, name))
                continue;

            attributes.AppendElement(nsAttrNameInfo(nameSpaceID, name,
                                                    attrName->GetPrefix()));
        }

        count = attributes.Length();
        while (count-- > 0) {
            PRInt32 nameSpaceID = attributes[count].mNamespaceID;
            nsIAtom* name = attributes[count].mName;
            nsAutoString value;
            if (broadcaster->GetAttr(nameSpaceID, name, value)) {
              listener->SetAttr(nameSpaceID, name, attributes[count].mPrefix,
                                value, notify);
            }

#if 0
            
            
            
            
            
            ExecuteOnBroadcastHandlerFor(broadcaster, aListener, name);
#endif
        }
    }
    else {
        
        nsCOMPtr<nsIAtom> name = do_GetAtom(aAttr);

        nsAutoString value;
        if (broadcaster->GetAttr(kNameSpaceID_None, name, value)) {
            listener->SetAttr(kNameSpaceID_None, name, value, notify);
        } else {
            listener->UnsetAttr(kNameSpaceID_None, name, notify);
        }

#if 0
        
        
        
        
        ExecuteOnBroadcastHandlerFor(broadcaster, aListener, name);
#endif
    }
}

NS_IMETHODIMP
nsXULDocument::AddBroadcastListenerFor(nsIDOMElement* aBroadcaster,
                                       nsIDOMElement* aListener,
                                       const nsAString& aAttr)
{
    NS_ENSURE_ARG(aBroadcaster && aListener);
    
    nsresult rv =
        nsContentUtils::CheckSameOrigin(static_cast<nsDocument *>(this),
                                        aBroadcaster);

    if (NS_FAILED(rv)) {
        return rv;
    }

    rv = nsContentUtils::CheckSameOrigin(static_cast<nsDocument *>(this),
                                         aListener);

    if (NS_FAILED(rv)) {
        return rv;
    }

    static PLDHashTableOps gOps = {
        PL_DHashAllocTable,
        PL_DHashFreeTable,
        PL_DHashVoidPtrKeyStub,
        PL_DHashMatchEntryStub,
        PL_DHashMoveEntryStub,
        ClearBroadcasterMapEntry,
        PL_DHashFinalizeStub,
        nsnull
    };

    if (! mBroadcasterMap) {
        mBroadcasterMap =
            PL_NewDHashTable(&gOps, nsnull, sizeof(BroadcasterMapEntry),
                             PL_DHASH_MIN_SIZE);

        if (! mBroadcasterMap)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    BroadcasterMapEntry* entry =
        static_cast<BroadcasterMapEntry*>
                   (PL_DHashTableOperate(mBroadcasterMap, aBroadcaster,
                                            PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_FREE(entry)) {
        entry =
            static_cast<BroadcasterMapEntry*>
                       (PL_DHashTableOperate(mBroadcasterMap, aBroadcaster,
                                                PL_DHASH_ADD));

        if (! entry)
            return NS_ERROR_OUT_OF_MEMORY;

        entry->mBroadcaster = aBroadcaster;

        
        
        new (&entry->mListeners) nsSmallVoidArray();
    }

    
    nsCOMPtr<nsIAtom> attr = do_GetAtom(aAttr);

    BroadcastListener* bl;
    for (PRInt32 i = entry->mListeners.Count() - 1; i >= 0; --i) {
        bl = static_cast<BroadcastListener*>(entry->mListeners[i]);

        nsCOMPtr<nsIDOMElement> blListener = do_QueryReferent(bl->mListener);

        if ((blListener == aListener) && (bl->mAttribute == attr))
            return NS_OK;
    }

    bl = new BroadcastListener;
    if (! bl)
        return NS_ERROR_OUT_OF_MEMORY;

    bl->mListener  = do_GetWeakReference(aListener);
    bl->mAttribute = attr;

    entry->mListeners.AppendElement(bl);

    SynchronizeBroadcastListener(aBroadcaster, aListener, aAttr);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::RemoveBroadcastListenerFor(nsIDOMElement* aBroadcaster,
                                          nsIDOMElement* aListener,
                                          const nsAString& aAttr)
{
    
    
    if (! mBroadcasterMap)
        return NS_OK;

    BroadcasterMapEntry* entry =
        static_cast<BroadcasterMapEntry*>
                   (PL_DHashTableOperate(mBroadcasterMap, aBroadcaster,
                                            PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
        nsCOMPtr<nsIAtom> attr = do_GetAtom(aAttr);
        for (PRInt32 i = entry->mListeners.Count() - 1; i >= 0; --i) {
            BroadcastListener* bl =
                static_cast<BroadcastListener*>(entry->mListeners[i]);

            nsCOMPtr<nsIDOMElement> blListener = do_QueryReferent(bl->mListener);

            if ((blListener == aListener) && (bl->mAttribute == attr)) {
                entry->mListeners.RemoveElementAt(i);
                delete bl;

                if (entry->mListeners.Count() == 0)
                    PL_DHashTableOperate(mBroadcasterMap, aBroadcaster,
                                         PL_DHASH_REMOVE);

                break;
            }
        }
    }

    return NS_OK;
}

nsresult
nsXULDocument::ExecuteOnBroadcastHandlerFor(nsIContent* aBroadcaster,
                                            nsIDOMElement* aListener,
                                            nsIAtom* aAttr)
{
    
    
    

    nsCOMPtr<nsIContent> listener = do_QueryInterface(aListener);
    PRUint32 count = listener->GetChildCount();
    for (PRUint32 i = 0; i < count; ++i) {
        
        
        
        
        nsIContent *child = listener->GetChildAt(i);

        if (!child->NodeInfo()->Equals(nsGkAtoms::observes, kNameSpaceID_XUL))
            continue;

        
        nsAutoString listeningToID;
        child->GetAttr(kNameSpaceID_None, nsGkAtoms::element, listeningToID);

        nsAutoString broadcasterID;
        aBroadcaster->GetAttr(kNameSpaceID_None, nsGkAtoms::id, broadcasterID);

        if (listeningToID != broadcasterID)
            continue;

        
        
        nsAutoString listeningToAttribute;
        child->GetAttr(kNameSpaceID_None, nsGkAtoms::attribute,
                       listeningToAttribute);

        if (!aAttr->Equals(listeningToAttribute) &&
            !listeningToAttribute.EqualsLiteral("*")) {
            continue;
        }

        
        
        nsEvent event(PR_TRUE, NS_XUL_BROADCAST);

        nsPresShellIterator iter(this);
        nsCOMPtr<nsIPresShell> shell;
        while ((shell = iter.GetNextShell())) {

            nsCOMPtr<nsPresContext> aPresContext = shell->GetPresContext();

            
            nsEventStatus status = nsEventStatus_eIgnore;
            nsEventDispatcher::Dispatch(child, aPresContext, &event, nsnull,
                                        &status);
        }
    }

    return NS_OK;
}

void
nsXULDocument::AttributeWillChange(nsIDocument* aDocument,
                                   nsIContent* aContent, PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute, PRInt32 aModType)
{
    NS_ABORT_IF_FALSE(aContent, "Null content!");
    NS_PRECONDITION(aAttribute, "Must have an attribute that's changing!");

    
    
    if (aAttribute == nsGkAtoms::ref ||
        (aAttribute == nsGkAtoms::id && !aContent->GetIDAttributeName())) {
        RemoveElementFromRefMap(aContent);
    }
    
    nsXMLDocument::AttributeWillChange(aDocument, aContent, aNameSpaceID,
                                       aAttribute, aModType);
}

void
nsXULDocument::AttributeChanged(nsIDocument* aDocument,
                                nsIContent* aElement, PRInt32 aNameSpaceID,
                                nsIAtom* aAttribute, PRInt32 aModType,
                                PRUint32 aStateMask)
{
    NS_ASSERTION(aDocument == this, "unexpected doc");

    
    nsXMLDocument::AttributeChanged(aDocument, aElement, aNameSpaceID,
            aAttribute, aModType, aStateMask);

    
    
    if (aAttribute == nsGkAtoms::ref ||
        (aAttribute == nsGkAtoms::id && !aElement->GetIDAttributeName())) {
        AddElementToRefMap(aElement);
    }
    
    nsresult rv;

    
    nsCOMPtr<nsIDOMElement> domele = do_QueryInterface(aElement);
    if (domele && mBroadcasterMap &&
        CanBroadcast(aNameSpaceID, aAttribute)) {
        BroadcasterMapEntry* entry =
            static_cast<BroadcasterMapEntry*>
                       (PL_DHashTableOperate(mBroadcasterMap, domele.get(),
                                                PL_DHASH_LOOKUP));

        if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
            
            nsAutoString value;
            PRBool attrSet = aElement->GetAttr(kNameSpaceID_None, aAttribute, value);

            PRInt32 i;
            for (i = entry->mListeners.Count() - 1; i >= 0; --i) {
                BroadcastListener* bl =
                    static_cast<BroadcastListener*>(entry->mListeners[i]);

                if ((bl->mAttribute == aAttribute) ||
                    (bl->mAttribute == nsGkAtoms::_asterix)) {
                    nsCOMPtr<nsIDOMElement> listenerEl
                        = do_QueryReferent(bl->mListener);
                    nsCOMPtr<nsIContent> l = do_QueryInterface(listenerEl);
                    if (l) {
                        nsAutoString currentValue;
                        PRBool hasAttr = l->GetAttr(kNameSpaceID_None,
                                                    aAttribute,
                                                    currentValue);
                        
                        
                        
                        
                        PRBool needsAttrChange =
                            attrSet != hasAttr || !value.Equals(currentValue);
                        nsDelayedBroadcastUpdate delayedUpdate(domele,
                                                               listenerEl,
                                                               aAttribute,
                                                               value,
                                                               attrSet,
                                                               needsAttrChange);

                        PRUint32 index =
                            mDelayedAttrChangeBroadcasts.IndexOf(delayedUpdate,
                                0, nsDelayedBroadcastUpdate::Comparator());
                        if (index != mDelayedAttrChangeBroadcasts.NoIndex) {
                            if (mHandlingDelayedAttrChange) {
                                NS_WARNING("Broadcasting loop!");
                                continue;
                            }
                            mDelayedAttrChangeBroadcasts.RemoveElementAt(index);
                        }

                        mDelayedAttrChangeBroadcasts.AppendElement(delayedUpdate);
                    }
                }
            }
        }
    }

    
    PRBool listener, resolved;
    CheckBroadcasterHookup(aElement, &listener, &resolved);

    
    
    
    nsAutoString persist;
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::persist, persist);
    if (!persist.IsEmpty()) {
        nsAutoString attr;
        rv = aAttribute->ToString(attr);
        if (NS_FAILED(rv)) return;

        
        if (persist.Find(attr) >= 0) {
            rv = Persist(aElement, kNameSpaceID_None, aAttribute);
            if (NS_FAILED(rv)) return;
        }
    }
}

void
nsXULDocument::ContentAppended(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               PRInt32 aNewIndexInContainer)
{
    NS_ASSERTION(aDocument == this, "unexpected doc");
    
    
    PRUint32 count = aContainer->GetChildCount();

    nsresult rv = NS_OK;
    for (PRUint32 i = aNewIndexInContainer; i < count && NS_SUCCEEDED(rv);
         ++i) {
        rv = AddSubtreeToDocument(aContainer->GetChildAt(i));
    }

    nsXMLDocument::ContentAppended(aDocument, aContainer, aNewIndexInContainer);
}

void
nsXULDocument::ContentInserted(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer)
{
    NS_ASSERTION(aDocument == this, "unexpected doc");

    AddSubtreeToDocument(aChild);

    nsXMLDocument::ContentInserted(aDocument, aContainer, aChild, aIndexInContainer);
}

void
nsXULDocument::ContentRemoved(nsIDocument* aDocument,
                              nsIContent* aContainer,
                              nsIContent* aChild,
                              PRInt32 aIndexInContainer)
{
    NS_ASSERTION(aDocument == this, "unexpected doc");

    RemoveSubtreeFromDocument(aChild);

    nsXMLDocument::ContentRemoved(aDocument, aContainer, aChild, aIndexInContainer);
}






NS_IMETHODIMP
nsXULDocument::AddElementForID(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    UpdateIdTableEntry(aElement);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetElementsForID(const nsAString& aID,
                                nsCOMArray<nsIContent>& aElements)
{
    aElements.Clear();

    nsCOMPtr<nsIAtom> atom = do_GetAtom(aID);
    if (!atom)
        return NS_ERROR_OUT_OF_MEMORY;
    nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(atom);
    if (entry) {
        entry->AppendAllIdContent(&aElements);
    }
    nsRefMapEntry *refEntry = mRefMap.GetEntry(atom);
    if (refEntry) {
        refEntry->AppendAll(&aElements);
    }
    return NS_OK;
}

nsresult
nsXULDocument::AddForwardReference(nsForwardReference* aRef)
{
    if (mResolutionPhase < aRef->GetPhase()) {
        if (!mForwardReferences.AppendElement(aRef)) {
            delete aRef;
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
    else {
        NS_ERROR("forward references have already been resolved");
        delete aRef;
    }

    return NS_OK;
}

nsresult
nsXULDocument::ResolveForwardReferences()
{
    if (mResolutionPhase == nsForwardReference::eDone)
        return NS_OK;

    NS_ASSERTION(mResolutionPhase == nsForwardReference::eStart,
                 "nested ResolveForwardReferences()");
        
    
    
    
    
    

    const nsForwardReference::Phase* pass = nsForwardReference::kPasses;
    while ((mResolutionPhase = *pass) != nsForwardReference::eDone) {
        PRUint32 previous = 0;
        while (mForwardReferences.Length() &&
               mForwardReferences.Length() != previous) {
            previous = mForwardReferences.Length();

            for (PRUint32 i = 0; i < mForwardReferences.Length(); ++i) {
                nsForwardReference* fwdref = mForwardReferences[i];

                if (fwdref->GetPhase() == *pass) {
                    nsForwardReference::Result result = fwdref->Resolve();

                    switch (result) {
                    case nsForwardReference::eResolve_Succeeded:
                    case nsForwardReference::eResolve_Error:
                        mForwardReferences.RemoveElementAt(i);

                        
                        --i;
                        break;

                    case nsForwardReference::eResolve_Later:
                        
                        ;
                    }

                    if (mResolutionPhase == nsForwardReference::eStart) {
                        
                        
                        
                        return NS_OK;
                    }
                }
            }
        }

        ++pass;
    }

    mForwardReferences.Clear();
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetScriptGlobalObjectOwner(nsIScriptGlobalObjectOwner** aGlobalOwner)
{
    NS_IF_ADDREF(*aGlobalOwner = mMasterPrototype);
    return NS_OK;
}






NS_IMETHODIMP
nsXULDocument::GetElementsByAttribute(const nsAString& aAttribute,
                                      const nsAString& aValue,
                                      nsIDOMNodeList** aReturn)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));
    NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);
    void* attrValue = new nsString(aValue);
    NS_ENSURE_TRUE(attrValue, NS_ERROR_OUT_OF_MEMORY);
    nsContentList *list = new nsContentList(this,
                                            MatchAttribute,
                                            nsContentUtils::DestroyMatchString,
                                            attrValue,
                                            PR_TRUE,
                                            attrAtom,
                                            kNameSpaceID_Unknown);
    NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);
    
    NS_ADDREF(*aReturn = list);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetElementsByAttributeNS(const nsAString& aNamespaceURI,
                                        const nsAString& aAttribute,
                                        const nsAString& aValue,
                                        nsIDOMNodeList** aReturn)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));
    NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);
    void* attrValue = new nsString(aValue);
    NS_ENSURE_TRUE(attrValue, NS_ERROR_OUT_OF_MEMORY);

    PRInt32 nameSpaceId = kNameSpaceID_Wildcard;
    if (!aNamespaceURI.EqualsLiteral("*")) {
      nsresult rv =
        nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                              nameSpaceId);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsContentList *list = new nsContentList(this,
                                            MatchAttribute,
                                            nsContentUtils::DestroyMatchString,
                                            attrValue,
                                            PR_TRUE,
                                            attrAtom,
                                            nameSpaceId);
    NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aReturn = list);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::Persist(const nsAString& aID,
                       const nsAString& aAttr)
{
    
    
    if (mApplyingPersistedAttrs)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsIDOMElement> domelement;
    rv = GetElementById(aID, getter_AddRefs(domelement));
    if (NS_FAILED(rv)) return rv;

    if (! domelement)
        return NS_OK;

    nsCOMPtr<nsIContent> element = do_QueryInterface(domelement);
    NS_ASSERTION(element != nsnull, "null ptr");
    if (! element)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIAtom> tag;
    PRInt32 nameSpaceID;

    nsCOMPtr<nsINodeInfo> ni = element->GetExistingAttrNameFromQName(aAttr);
    if (ni) {
        tag = ni->NameAtom();
        nameSpaceID = ni->NamespaceID();
    }
    else {
        
        nsIParserService *parserService = nsContentUtils::GetParserService();
        NS_ASSERTION(parserService, "Running scripts during shutdown?");

        const PRUnichar *colon;
        rv = parserService->CheckQName(PromiseFlatString(aAttr), PR_TRUE, &colon);
        if (NS_FAILED(rv)) {
            
            return NS_ERROR_INVALID_ARG;
        }

        if (colon) {
            
            return NS_ERROR_NOT_IMPLEMENTED;
        }

        tag = do_GetAtom(aAttr);
        NS_ENSURE_TRUE(tag, NS_ERROR_OUT_OF_MEMORY);

        nameSpaceID = kNameSpaceID_None;
    }

    rv = Persist(element, nameSpaceID, tag);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


PRBool
nsXULDocument::IsCapabilityEnabled(const char* aCapabilityLabel)
{
    nsresult rv;

    
    PRBool enabled = PR_FALSE;
    rv = NodePrincipal()->IsCapabilityEnabled(aCapabilityLabel, nsnull, &enabled);
    if (NS_FAILED(rv))
        return PR_FALSE;
 
    return enabled;
}


nsresult
nsXULDocument::Persist(nsIContent* aElement, PRInt32 aNameSpaceID,
                       nsIAtom* aAttribute)
{
    
    if (!IsCapabilityEnabled("UniversalBrowserWrite"))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    if (!mLocalStore)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsIRDFResource> element;
    rv = nsXULContentUtils::GetElementResource(aElement, getter_AddRefs(element));
    if (NS_FAILED(rv)) return rv;

    
    if (! element)
        return NS_OK;

    
    
    const char* attrstr;
    rv = aAttribute->GetUTF8String(&attrstr);
    if (NS_FAILED(rv)) return rv;

    
    
    
    if (!attrstr || strlen(attrstr) > kMaxAttrNameLength) {
        NS_WARNING("Can't persist, Attribute name too long");
        return NS_ERROR_ILLEGAL_VALUE;
    }

    nsCOMPtr<nsIRDFResource> attr;
    rv = gRDFService->GetResource(nsDependentCString(attrstr),
                                  getter_AddRefs(attr));
    if (NS_FAILED(rv)) return rv;

    
    nsAutoString valuestr;
    aElement->GetAttr(kNameSpaceID_None, aAttribute, valuestr);

    
    
    
    if (valuestr.Length() > kMaxAttributeLength) {
        NS_WARNING("Truncating persisted attribute value");
        valuestr.Truncate(kMaxAttributeLength);
    }

    
    nsCOMPtr<nsIRDFNode> oldvalue;
    rv = mLocalStore->GetTarget(element, attr, PR_TRUE, getter_AddRefs(oldvalue));
    if (NS_FAILED(rv)) return rv;

    if (oldvalue && valuestr.IsEmpty()) {
        
        
        rv = mLocalStore->Unassert(element, attr, oldvalue);
    }
    else {
        
        
        nsCOMPtr<nsIRDFLiteral> newvalue;
        rv = gRDFService->GetLiteral(valuestr.get(), getter_AddRefs(newvalue));
        if (NS_FAILED(rv)) return rv;

        if (oldvalue) {
            if (oldvalue != newvalue)
                rv = mLocalStore->Change(element, attr, oldvalue, newvalue);
            else
                rv = NS_OK;
        }
        else {
            rv = mLocalStore->Assert(element, attr, newvalue, PR_TRUE);
        }
    }

    if (NS_FAILED(rv)) return rv;

    
    
    {
        nsCAutoString docurl;
        rv = mDocumentURI->GetSpec(docurl);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> doc;
        rv = gRDFService->GetResource(docurl, getter_AddRefs(doc));
        if (NS_FAILED(rv)) return rv;

        PRBool hasAssertion;
        rv = mLocalStore->HasAssertion(doc, kNC_persist, element, PR_TRUE, &hasAssertion);
        if (NS_FAILED(rv)) return rv;

        if (! hasAssertion) {
            rv = mLocalStore->Assert(doc, kNC_persist, element, PR_TRUE);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}


nsresult
nsXULDocument::GetViewportSize(PRInt32* aWidth,
                               PRInt32* aHeight)
{
    *aWidth = *aHeight = 0;

    FlushPendingNotifications(Flush_Layout);

    nsIPresShell *shell = GetPrimaryShell();
    NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

    nsIFrame* frame = shell->GetRootFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

    nsSize size = frame->GetSize();

    *aWidth = nsPresContext::AppUnitsToIntCSSPixels(size.width);
    *aHeight = nsPresContext::AppUnitsToIntCSSPixels(size.height);

    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetWidth(PRInt32* aWidth)
{
    NS_ENSURE_ARG_POINTER(aWidth);

    PRInt32 height;
    return GetViewportSize(aWidth, &height);
}

NS_IMETHODIMP
nsXULDocument::GetHeight(PRInt32* aHeight)
{
    NS_ENSURE_ARG_POINTER(aHeight);

    PRInt32 width;
    return GetViewportSize(&width, aHeight);
}






NS_IMETHODIMP
nsXULDocument::GetPopupNode(nsIDOMNode** aNode)
{
    
    nsresult rv = TrustedGetPopupNode(aNode); 

    if (NS_SUCCEEDED(rv) && *aNode && !nsContentUtils::CanCallerAccess(*aNode)) {
        NS_RELEASE(*aNode);
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    return rv;
}

NS_IMETHODIMP
nsXULDocument::TrustedGetPopupNode(nsIDOMNode** aNode)
{
    
    nsCOMPtr<nsIFocusController> focusController;
    GetFocusController(getter_AddRefs(focusController));
    NS_ENSURE_TRUE(focusController, NS_ERROR_FAILURE);

    
    return focusController->GetPopupNode(aNode); 
}

NS_IMETHODIMP
nsXULDocument::SetPopupNode(nsIDOMNode* aNode)
{
    nsresult rv;

    
    nsCOMPtr<nsIFocusController> focusController;
    GetFocusController(getter_AddRefs(focusController));
    NS_ENSURE_TRUE(focusController, NS_ERROR_FAILURE);
    
    rv = focusController->SetPopupNode(aNode);

    return rv;
}



NS_IMETHODIMP
nsXULDocument::GetPopupRangeParent(nsIDOMNode** aRangeParent)
{
    NS_ENSURE_ARG_POINTER(aRangeParent);
    *aRangeParent = nsnull;

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (!pm)
        return NS_ERROR_FAILURE;

    PRInt32 offset;
    pm->GetMouseLocation(aRangeParent, &offset);

    if (*aRangeParent && !nsContentUtils::CanCallerAccess(*aRangeParent)) {
        NS_RELEASE(*aRangeParent);
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    return NS_OK;
}



NS_IMETHODIMP
nsXULDocument::GetPopupRangeOffset(PRInt32* aRangeOffset)
{
    NS_ENSURE_ARG_POINTER(aRangeOffset);

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (!pm)
        return NS_ERROR_FAILURE;

    PRInt32 offset;
    nsCOMPtr<nsIDOMNode> parent;
    pm->GetMouseLocation(getter_AddRefs(parent), &offset);

    if (parent && !nsContentUtils::CanCallerAccess(parent))
        return NS_ERROR_DOM_SECURITY_ERR;

    *aRangeOffset = offset;
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetTooltipNode(nsIDOMNode** aNode)
{
    if (mTooltipNode && !nsContentUtils::CanCallerAccess(mTooltipNode)) {
        return NS_ERROR_DOM_SECURITY_ERR;
    }
    *aNode = mTooltipNode;
    NS_IF_ADDREF(*aNode);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::TrustedGetTooltipNode(nsIDOMNode** aNode)
{
    NS_IF_ADDREF(*aNode = mTooltipNode);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetTooltipNode(nsIDOMNode* aNode)
{
    mTooltipNode = aNode;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetCommandDispatcher(nsIDOMXULCommandDispatcher** aTracker)
{
    *aTracker = mCommandDispatcher;
    NS_IF_ADDREF(*aTracker);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetElementById(const nsAString& aId,
                              nsIDOMElement** aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);
    *aReturn = nsnull;

    nsCOMPtr<nsIAtom> atom = do_GetAtom(aId);
    if (!atom)
        return NS_ERROR_OUT_OF_MEMORY;

    if (!CheckGetElementByIdArg(atom))
        return NS_OK;

    nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(atom);
    if (entry) {
        nsIContent* content = entry->GetIdContent();
        if (content)
            return CallQueryInterface(content, aReturn);
    }
    nsRefMapEntry* refEntry = mRefMap.GetEntry(atom);
    if (refEntry) {
        NS_ASSERTION(refEntry->GetFirstContent(),
                     "nsRefMapEntries should have nonempty content lists");
        return CallQueryInterface(refEntry->GetFirstContent(), aReturn);
    }
    return NS_OK;
}

nsresult
nsXULDocument::AddElementToDocumentPre(nsIContent* aElement)
{
    
    
    nsresult rv;

    
    
    
    UpdateIdTableEntry(aElement);
    rv = AddElementToRefMap(aElement);
    if (NS_FAILED(rv)) return rv;

    
    
    
    if (aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::commandupdater,
                              nsGkAtoms::_true, eCaseMatters)) {
        rv = nsXULContentUtils::SetCommandUpdater(this, aElement);
        if (NS_FAILED(rv)) return rv;
    }

    
    
    PRBool listener, resolved;
    rv = CheckBroadcasterHookup(aElement, &listener, &resolved);
    if (NS_FAILED(rv)) return rv;

    
    
    if (listener && !resolved && (mResolutionPhase != nsForwardReference::eDone)) {
        BroadcasterHookup* hookup = new BroadcasterHookup(this, aElement);
        if (! hookup)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = AddForwardReference(hookup);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsXULDocument::AddElementToDocumentPost(nsIContent* aElement)
{
    
    if (aElement->NodeInfo()->Equals(nsGkAtoms::keyset, kNameSpaceID_XUL)) {
        
        nsCOMPtr<nsIXBLService> xblService(do_GetService("@mozilla.org/xbl;1"));
        if (xblService) {
            nsCOMPtr<nsPIDOMEventTarget> piTarget(do_QueryInterface(aElement));
            xblService->AttachGlobalKeyHandler(piTarget);
        }
    }

    
    PRBool needsHookup;
    nsresult rv = CheckTemplateBuilderHookup(aElement, &needsHookup);
    if (NS_FAILED(rv))
        return rv;

    if (needsHookup) {
        if (mResolutionPhase == nsForwardReference::eDone) {
            rv = CreateTemplateBuilder(aElement);
            if (NS_FAILED(rv))
                return rv;
        }
        else {
            TemplateBuilderHookup* hookup = new TemplateBuilderHookup(aElement);
            if (! hookup)
                return NS_ERROR_OUT_OF_MEMORY;

            rv = AddForwardReference(hookup);
            if (NS_FAILED(rv))
                return rv;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::AddSubtreeToDocument(nsIContent* aElement)
{
    NS_ASSERTION(aElement->GetCurrentDoc() == this, "Element not in doc!");
    
    if (!aElement->IsNodeOfType(nsINode::eELEMENT)) {
        return NS_OK;
    }

    
    nsresult rv = AddElementToDocumentPre(aElement);
    if (NS_FAILED(rv)) return rv;

    
    PRUint32 count = aElement->GetChildCount();

    while (count-- > 0) {
        rv = AddSubtreeToDocument(aElement->GetChildAt(count));
        if (NS_FAILED(rv))
            return rv;
    }

    
    return AddElementToDocumentPost(aElement);
}

NS_IMETHODIMP
nsXULDocument::RemoveSubtreeFromDocument(nsIContent* aElement)
{
    
    if (!aElement->IsNodeOfType(nsINode::eELEMENT)) {
        return NS_OK;
    }

    
    
    nsresult rv;

    if (aElement->NodeInfo()->Equals(nsGkAtoms::keyset, kNameSpaceID_XUL)) {
        nsCOMPtr<nsIXBLService> xblService(do_GetService("@mozilla.org/xbl;1"));
        if (xblService) {
            nsCOMPtr<nsPIDOMEventTarget> piTarget(do_QueryInterface(aElement));
            xblService->DetachGlobalKeyHandler(piTarget);
        }
    }

    
    PRUint32 count = aElement->GetChildCount();

    while (count-- > 0) {
        rv = RemoveSubtreeFromDocument(aElement->GetChildAt(count));
        if (NS_FAILED(rv))
            return rv;
    }

    
    
    
    RemoveElementFromRefMap(aElement);
    RemoveFromIdTable(aElement);

    
    
    if (aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::commandupdater,
                              nsGkAtoms::_true, eCaseMatters)) {
        nsCOMPtr<nsIDOMElement> domelement = do_QueryInterface(aElement);
        NS_ASSERTION(domelement != nsnull, "not a DOM element");
        if (! domelement)
            return NS_ERROR_UNEXPECTED;

        rv = mCommandDispatcher->RemoveCommandUpdater(domelement);
        if (NS_FAILED(rv)) return rv;
    }

    
    
    nsCOMPtr<nsIDOMElement> broadcaster, listener;
    nsAutoString attribute, broadcasterID;
    rv = FindBroadcaster(aElement, getter_AddRefs(listener),
                         broadcasterID, attribute, getter_AddRefs(broadcaster));
    if (rv == NS_FINDBROADCASTER_FOUND) {
        RemoveBroadcastListenerFor(broadcaster, listener, attribute);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetTemplateBuilderFor(nsIContent* aContent,
                                     nsIXULTemplateBuilder* aBuilder)
{
    if (! mTemplateBuilderTable) {
        mTemplateBuilderTable = new BuilderTable;
        if (! mTemplateBuilderTable || !mTemplateBuilderTable->Init()) {
            mTemplateBuilderTable = nsnull;
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    if (aBuilder) {
        mTemplateBuilderTable->Put(aContent, aBuilder);
    }
    else {
        mTemplateBuilderTable->Remove(aContent);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetTemplateBuilderFor(nsIContent* aContent,
                                     nsIXULTemplateBuilder** aResult)
{
    if (mTemplateBuilderTable) {
        mTemplateBuilderTable->Get(aContent, aResult);
    }
    else
        *aResult = nsnull;

    return NS_OK;
}

static void
GetRefMapAttribute(nsIContent* aElement, nsAutoString* aValue)
{
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::ref, *aValue);
    if (aValue->IsEmpty() && !aElement->GetIDAttributeName()) {
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::id, *aValue);
    }
}

nsresult
nsXULDocument::AddElementToRefMap(nsIContent* aElement)
{
    
    
    nsAutoString value;
    GetRefMapAttribute(aElement, &value);
    if (!value.IsEmpty()) {
        nsCOMPtr<nsIAtom> atom = do_GetAtom(value);
        if (!atom)
            return NS_ERROR_OUT_OF_MEMORY;
        nsRefMapEntry *entry = mRefMap.PutEntry(atom);
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;
        if (!entry->AddContent(aElement))
            return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

void
nsXULDocument::RemoveElementFromRefMap(nsIContent* aElement)
{
    
    nsAutoString value;
    GetRefMapAttribute(aElement, &value);
    if (!value.IsEmpty()) {
        nsCOMPtr<nsIAtom> atom = do_GetAtom(value);
        if (!atom)
            return;
        nsRefMapEntry *entry = mRefMap.GetEntry(atom);
        if (!entry)
            return;
        if (entry->RemoveContent(aElement)) {
            mRefMap.RemoveEntry(atom);
        }
    }
}






NS_IMETHODIMP
nsXULDocument::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
    
    *aReturn = nsnull;
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}







nsresult
nsXULDocument::Init()
{
    mRefMap.Init();

    nsresult rv = nsXMLDocument::Init();
    NS_ENSURE_SUCCESS(rv, rv);

    
    mCommandDispatcher = new nsXULCommandDispatcher(this);
    NS_ENSURE_TRUE(mCommandDispatcher, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    mLocalStore = do_GetService(NS_LOCALSTORE_CONTRACTID);

    if (gRefCnt++ == 0) {
        
        
        rv = CallGetService("@mozilla.org/rdf/rdf-service;1", &gRDFService);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF Service");
        if (NS_FAILED(rv)) return rv;

        gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "persist"),
                                 &kNC_persist);
        gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "attribute"),
                                 &kNC_attribute);
        gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "value"),
                                 &kNC_value);

        
        
        
        nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
        if (!cache) {
          NS_ERROR("Could not instantiate nsXULPrototypeCache");
          return NS_ERROR_FAILURE;
        }
    }

    nsContentUtils::RegisterPrefCallback("intl.uidirection.",
                                         nsXULDocument::DirectionChanged,
                                         this);

#ifdef PR_LOGGING
    if (! gXULLog)
        gXULLog = PR_NewLogModule("nsXULDocument");
#endif

    return NS_OK;
}


nsresult
nsXULDocument::StartLayout(void)
{
    nsPresShellIterator iter(this);
    nsCOMPtr<nsIPresShell> shell;
    while ((shell = iter.GetNextShell())) {

        
        nsPresContext *cx = shell->GetPresContext();
        NS_ASSERTION(cx != nsnull, "no pres context");
        if (! cx)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsISupports> container = cx->GetContainer();
        NS_ASSERTION(container != nsnull, "pres context has no container");
        if (! container)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
        NS_ASSERTION(docShell != nsnull, "container is not a docshell");
        if (! docShell)
            return NS_ERROR_UNEXPECTED;

        
        
        
        
        
        
        nsresult rv = NS_OK;
        nsIViewManager* vm = shell->GetViewManager();
        if (vm) {
            nsCOMPtr<nsIContentViewer> contentViewer;
            rv = docShell->GetContentViewer(getter_AddRefs(contentViewer));
            if (NS_SUCCEEDED(rv) && (contentViewer != nsnull)) {
                PRBool enabled;
                contentViewer->GetEnableRendering(&enabled);
                if (enabled) {
                    vm->EnableRefresh(NS_VMREFRESH_IMMEDIATE);
                }
            }
        }

        mMayStartLayout = PR_TRUE;

        
        
        
        nsRect r = cx->GetVisibleArea();
        
        
        nsCOMPtr<nsIPresShell> shellGrip = shell;
        rv = shell->InitialReflow(r.width, r.height);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}


PRBool
nsXULDocument::MatchAttribute(nsIContent* aContent,
                              PRInt32 aNamespaceID,
                              nsIAtom* aAttrName,
                              void* aData)
{
    NS_PRECONDITION(aContent, "Must have content node to work with!");
    nsString* attrValue = static_cast<nsString*>(aData);
    if (aNamespaceID != kNameSpaceID_Unknown &&
        aNamespaceID != kNameSpaceID_Wildcard) {
        return attrValue->EqualsLiteral("*") ?
            aContent->HasAttr(aNamespaceID, aAttrName) :
            aContent->AttrValueIs(aNamespaceID, aAttrName, *attrValue,
                                  eCaseMatters);
    }

    

    PRUint32 count = aContent->GetAttrCount();
    for (PRUint32 i = 0; i < count; ++i) {
        const nsAttrName* name = aContent->GetAttrNameAt(i);
        PRBool nameMatch;
        if (name->IsAtom()) {
            nameMatch = name->Atom() == aAttrName;
        } else if (aNamespaceID == kNameSpaceID_Wildcard) {
            nameMatch = name->NodeInfo()->Equals(aAttrName);
        } else {
            nameMatch = name->NodeInfo()->QualifiedNameEquals(aAttrName);
        }

        if (nameMatch) {
            return attrValue->EqualsLiteral("*") ||
                aContent->AttrValueIs(name->NamespaceID(), name->LocalName(),
                                      *attrValue, eCaseMatters);
        }
    }

    return PR_FALSE;
}

nsresult
nsXULDocument::PrepareToLoad(nsISupports* aContainer,
                             const char* aCommand,
                             nsIChannel* aChannel,
                             nsILoadGroup* aLoadGroup,
                             nsIParser** aResult)
{
    
    nsCOMPtr<nsIPrincipal> principal;
    nsContentUtils::GetSecurityManager()->
        GetChannelPrincipal(aChannel, getter_AddRefs(principal));
    return PrepareToLoadPrototype(mDocumentURI, aCommand, principal, aResult);
}


nsresult
nsXULDocument::PrepareToLoadPrototype(nsIURI* aURI, const char* aCommand,
                                      nsIPrincipal* aDocumentPrincipal,
                                      nsIParser** aResult)
{
    nsresult rv;

    
    rv = NS_NewXULPrototypeDocument(getter_AddRefs(mCurrentPrototype));
    if (NS_FAILED(rv)) return rv;

    rv = mCurrentPrototype->InitPrincipal(aURI, aDocumentPrincipal);
    if (NS_FAILED(rv)) {
        mCurrentPrototype = nsnull;
        return rv;
    }    

    
    if (! mMasterPrototype) {
        mMasterPrototype = mCurrentPrototype;
        
        SetPrincipal(aDocumentPrincipal);
    }

    
    
    nsRefPtr<XULContentSinkImpl> sink = new XULContentSinkImpl();
    if (!sink) return NS_ERROR_OUT_OF_MEMORY;

    rv = sink->Init(this, mCurrentPrototype);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to initialize datasource sink");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIParser> parser = do_CreateInstance(kParserCID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create parser");
    if (NS_FAILED(rv)) return rv;

    parser->SetCommand(nsCRT::strcmp(aCommand, "view-source") ? eViewNormal :
                       eViewSource);

    parser->SetDocumentCharset(NS_LITERAL_CSTRING("UTF-8"),
                               kCharsetFromDocTypeDefault);
    parser->SetContentSink(sink); 

    *aResult = parser;
    NS_ADDREF(*aResult);
    return NS_OK;
}


nsresult
nsXULDocument::ApplyPersistentAttributes()
{
    
    if (!IsCapabilityEnabled("UniversalBrowserRead"))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    if (!mLocalStore)
        return NS_OK;

    mApplyingPersistedAttrs = PR_TRUE;
    ApplyPersistentAttributesInternal();
    mApplyingPersistedAttrs = PR_FALSE;

    return NS_OK;
}


nsresult 
nsXULDocument::ApplyPersistentAttributesInternal()
{
    nsCOMArray<nsIContent> elements;

    nsCAutoString docurl;
    mDocumentURI->GetSpec(docurl);

    nsCOMPtr<nsIRDFResource> doc;
    gRDFService->GetResource(docurl, getter_AddRefs(doc));

    nsCOMPtr<nsISimpleEnumerator> persisted;
    mLocalStore->GetTargets(doc, kNC_persist, PR_TRUE, getter_AddRefs(persisted));

    while (1) {
        PRBool hasmore = PR_FALSE;
        persisted->HasMoreElements(&hasmore);
        if (! hasmore)
            break;

        nsCOMPtr<nsISupports> isupports;
        persisted->GetNext(getter_AddRefs(isupports));

        nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(isupports);
        if (! resource) {
            NS_WARNING("expected element to be a resource");
            continue;
        }

        const char *uri;
        resource->GetValueConst(&uri);
        if (! uri)
            continue;

        nsAutoString id;
        nsXULContentUtils::MakeElementID(this, nsDependentCString(uri), id);

        if (id.IsEmpty())
            continue;

        
        GetElementsForID(id, elements);

        if (!elements.Count())
            continue;

        ApplyPersistentAttributesToElements(resource, elements);
    }

    return NS_OK;
}


nsresult
nsXULDocument::ApplyPersistentAttributesToElements(nsIRDFResource* aResource,
                                                   nsCOMArray<nsIContent>& aElements)
{
    nsresult rv;

    nsCOMPtr<nsISimpleEnumerator> attrs;
    rv = mLocalStore->ArcLabelsOut(aResource, getter_AddRefs(attrs));
    if (NS_FAILED(rv)) return rv;

    while (1) {
        PRBool hasmore;
        rv = attrs->HasMoreElements(&hasmore);
        if (NS_FAILED(rv)) return rv;

        if (! hasmore)
            break;

        nsCOMPtr<nsISupports> isupports;
        rv = attrs->GetNext(getter_AddRefs(isupports));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);
        if (! property) {
            NS_WARNING("expected a resource");
            continue;
        }

        const char* attrname;
        rv = property->GetValueConst(&attrname);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIAtom> attr = do_GetAtom(attrname);
        if (! attr)
            return NS_ERROR_OUT_OF_MEMORY;

        

        nsCOMPtr<nsIRDFNode> node;
        rv = mLocalStore->GetTarget(aResource, property, PR_TRUE,
                                    getter_AddRefs(node));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFLiteral> literal = do_QueryInterface(node);
        if (! literal) {
            NS_WARNING("expected a literal");
            continue;
        }

        const PRUnichar* value;
        rv = literal->GetValueConst(&value);
        if (NS_FAILED(rv)) return rv;

        nsDependentString wrapper(value);

        PRUint32 cnt = aElements.Count();

        for (PRInt32 i = PRInt32(cnt) - 1; i >= 0; --i) {
            nsCOMPtr<nsIContent> element = aElements.SafeObjectAt(i);
            if (!element)
                continue;

            rv = element->SetAttr( kNameSpaceID_None,
                                  attr,
                                  wrapper,
                                  PR_TRUE);
        }
    }

    return NS_OK;
}






nsXULDocument::ContextStack::ContextStack()
    : mTop(nsnull), mDepth(0)
{
}

nsXULDocument::ContextStack::~ContextStack()
{
    while (mTop) {
        Entry* doomed = mTop;
        mTop = mTop->mNext;
        NS_IF_RELEASE(doomed->mElement);
        delete doomed;
    }
}

nsresult
nsXULDocument::ContextStack::Push(nsXULPrototypeElement* aPrototype,
                                  nsIContent* aElement)
{
    Entry* entry = new Entry;
    if (! entry)
        return NS_ERROR_OUT_OF_MEMORY;

    entry->mPrototype = aPrototype;
    entry->mElement   = aElement;
    NS_IF_ADDREF(entry->mElement);
    entry->mIndex     = 0;

    entry->mNext = mTop;
    mTop = entry;

    ++mDepth;
    return NS_OK;
}

nsresult
nsXULDocument::ContextStack::Pop()
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    Entry* doomed = mTop;
    mTop = mTop->mNext;
    --mDepth;

    NS_IF_RELEASE(doomed->mElement);
    delete doomed;
    return NS_OK;
}

nsresult
nsXULDocument::ContextStack::Peek(nsXULPrototypeElement** aPrototype,
                                           nsIContent** aElement,
                                           PRInt32* aIndex)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    *aPrototype = mTop->mPrototype;
    *aElement   = mTop->mElement;
    NS_IF_ADDREF(*aElement);
    *aIndex     = mTop->mIndex;

    return NS_OK;
}


nsresult
nsXULDocument::ContextStack::SetTopIndex(PRInt32 aIndex)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    mTop->mIndex = aIndex;
    return NS_OK;
}


PRBool
nsXULDocument::ContextStack::IsInsideXULTemplate()
{
    if (mDepth) {
        for (nsIContent* element = mTop->mElement; element;
             element = element->GetParent()) {

            if (element->NodeInfo()->Equals(nsGkAtoms::_template,
                                            kNameSpaceID_XUL)) {
                return PR_TRUE;
            }
        }
    }
    return PR_FALSE;
}







nsresult
nsXULDocument::PrepareToWalk()
{
    
    nsresult rv;

    
    
    mPrototypes.AppendElement(mCurrentPrototype);

    
    
    nsXULPrototypeElement* proto = mCurrentPrototype->GetRootElement();

    if (! proto) {
#ifdef PR_LOGGING
        if (PR_LOG_TEST(gXULLog, PR_LOG_ERROR)) {
            nsCOMPtr<nsIURI> url = mCurrentPrototype->GetURI();

            nsCAutoString urlspec;
            rv = url->GetSpec(urlspec);
            if (NS_FAILED(rv)) return rv;

            PR_LOG(gXULLog, PR_LOG_ERROR,
                   ("xul: error parsing '%s'", urlspec.get()));
        }
#endif

        return NS_OK;
    }

    PRUint32 piInsertionPoint = 0;
    if (mState != eState_Master) {
        piInsertionPoint = IndexOf(GetRootContent());
        NS_ASSERTION(piInsertionPoint >= 0,
                     "No root content when preparing to walk overlay!");
    }

    const nsTArray<nsRefPtr<nsXULPrototypePI> >& processingInstructions =
        mCurrentPrototype->GetProcessingInstructions();

    PRUint32 total = processingInstructions.Length();
    for (PRUint32 i = 0; i < total; ++i) {
        rv = CreateAndInsertPI(processingInstructions[i],
                               this, piInsertionPoint + i);
        if (NS_FAILED(rv)) return rv;
    }

    
    rv = AddChromeOverlays();
    if (NS_FAILED(rv)) return rv;

    
    
    nsCOMPtr<nsIContent> root;

    if (mState == eState_Master) {
        
        rv = CreateElementFromPrototype(proto, getter_AddRefs(root));
        if (NS_FAILED(rv)) return rv;

        rv = AppendChildTo(root, PR_FALSE);
        if (NS_FAILED(rv)) return rv;
        
        
        UpdateIdTableEntry(root);
        rv = AddElementToRefMap(root);
        if (NS_FAILED(rv)) return rv;

        
        
        BlockOnload();
    }

    
    
    
    
    
    
    
    NS_ASSERTION(mContextStack.Depth() == 0, "something's on the context stack already");
    if (mContextStack.Depth() != 0)
        return NS_ERROR_UNEXPECTED;

    rv = mContextStack.Push(proto, root);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsXULDocument::CreateAndInsertPI(const nsXULPrototypePI* aProtoPI,
                                 nsINode* aParent, PRUint32 aIndex)
{
    NS_PRECONDITION(aProtoPI, "null ptr");
    NS_PRECONDITION(aParent, "null ptr");

    nsresult rv;
    nsCOMPtr<nsIContent> node;

    rv = NS_NewXMLProcessingInstruction(getter_AddRefs(node),
                                        mNodeInfoManager,
                                        aProtoPI->mTarget,
                                        aProtoPI->mData);
    if (NS_FAILED(rv)) return rv;

    if (aProtoPI->mTarget.EqualsLiteral("xml-stylesheet")) {
        rv = InsertXMLStylesheetPI(aProtoPI, aParent, aIndex, node);
    } else if (aProtoPI->mTarget.EqualsLiteral("xul-overlay")) {
        rv = InsertXULOverlayPI(aProtoPI, aParent, aIndex, node);
    } else {
        
        rv = aParent->InsertChildAt(node, aIndex, PR_FALSE);
    }

    return rv;
}

nsresult
nsXULDocument::InsertXMLStylesheetPI(const nsXULPrototypePI* aProtoPI,
                                     nsINode* aParent,
                                     PRUint32 aIndex,
                                     nsIContent* aPINode)
{
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(aPINode));
    NS_ASSERTION(ssle, "passed XML Stylesheet node does not "
                       "implement nsIStyleSheetLinkingElement!");

    nsresult rv;

    ssle->InitStyleLinkElement(PR_FALSE);
    
    
    ssle->SetEnableUpdates(PR_FALSE);
    ssle->OverrideBaseURI(mCurrentPrototype->GetURI());

    rv = aParent->InsertChildAt(aPINode, aIndex, PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    ssle->SetEnableUpdates(PR_TRUE);

    
    
    PRBool willNotify;
    PRBool isAlternate;
    rv = ssle->UpdateStyleSheet(this, &willNotify, &isAlternate);
    if (NS_SUCCEEDED(rv) && willNotify && !isAlternate) {
        ++mPendingSheets;
    }

    
    
    
    if (rv == NS_ERROR_OUT_OF_MEMORY) {
        return rv;
    }
    
    return NS_OK;
}

nsresult
nsXULDocument::InsertXULOverlayPI(const nsXULPrototypePI* aProtoPI,
                                  nsINode* aParent,
                                  PRUint32 aIndex,
                                  nsIContent* aPINode)
{
    nsresult rv;

    rv = aParent->InsertChildAt(aPINode, aIndex, PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    
    if (!nsContentUtils::InProlog(aPINode)) {
        return NS_OK;
    }

    nsAutoString href;
    nsParserUtils::GetQuotedAttributeValue(aProtoPI->mData,
                                           nsGkAtoms::href,
                                           href);

    
    if (href.IsEmpty()) {
        return NS_OK;
    }

    
    nsCOMPtr<nsIURI> uri;

    rv = NS_NewURI(getter_AddRefs(uri), href, nsnull,
                   mCurrentPrototype->GetURI());
    if (NS_SUCCEEDED(rv)) {
        
        
        
        
        
        
        rv = mUnloadedOverlays.InsertObjectAt(uri, 0);
    } else if (rv == NS_ERROR_MALFORMED_URI) {
        
        
        rv = NS_OK;
    }

    return rv;
}

nsresult
nsXULDocument::AddChromeOverlays()
{
    nsresult rv;

    nsCOMPtr<nsIURI> docUri = mCurrentPrototype->GetURI();

    
    if (!IsChromeURI(docUri)) return NS_OK;

    nsCOMPtr<nsIXULOverlayProvider> chromeReg(do_GetService(NS_CHROMEREGISTRY_CONTRACTID));
    
    
    NS_ENSURE_TRUE(chromeReg, NS_OK);

    nsCOMPtr<nsISimpleEnumerator> overlays;
    rv = chromeReg->GetXULOverlays(docUri, getter_AddRefs(overlays));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool moreOverlays;
    nsCOMPtr<nsISupports> next;
    nsCOMPtr<nsIURI> uri;

    while (NS_SUCCEEDED(rv = overlays->HasMoreElements(&moreOverlays)) &&
           moreOverlays) {

        rv = overlays->GetNext(getter_AddRefs(next));
        if (NS_FAILED(rv) || !next) break;

        uri = do_QueryInterface(next);
        if (!uri) {
            NS_ERROR("Chrome registry handed me a non-nsIURI object!");
            continue;
        }

        
        rv = mUnloadedOverlays.InsertObjectAt(uri, 0);
        if (NS_FAILED(rv)) break;
    }

    return rv;
}

NS_IMETHODIMP
nsXULDocument::LoadOverlay(const nsAString& aURL, nsIObserver* aObserver)
{
    nsresult rv;

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), aURL, nsnull);
    if (NS_FAILED(rv)) return rv;

    if (aObserver) {
        nsIObserver* obs = nsnull;
        NS_ENSURE_TRUE(mOverlayLoadObservers.IsInitialized() || mOverlayLoadObservers.Init(), 
                       NS_ERROR_OUT_OF_MEMORY);
        
        obs = mOverlayLoadObservers.GetWeak(uri);

        if (obs) {
            
            
            return NS_ERROR_FAILURE;
        }
        mOverlayLoadObservers.Put(uri, aObserver);
    }
    PRBool shouldReturn, failureFromContent;
    rv = LoadOverlayInternal(uri, PR_TRUE, &shouldReturn, &failureFromContent);
    if (NS_FAILED(rv) && mOverlayLoadObservers.IsInitialized())
        mOverlayLoadObservers.Remove(uri); 
    return rv;
}

nsresult
nsXULDocument::LoadOverlayInternal(nsIURI* aURI, PRBool aIsDynamic,
                                   PRBool* aShouldReturn,
                                   PRBool* aFailureFromContent)
{
    nsresult rv;

    *aShouldReturn = PR_FALSE;
    *aFailureFromContent = PR_FALSE;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
        nsCAutoString urlspec;
        aURI->GetSpec(urlspec);

        PR_LOG(gXULLog, PR_LOG_DEBUG,
                ("xul: loading overlay %s", urlspec.get()));
    }
#endif

    if (aIsDynamic)
        mResolutionPhase = nsForwardReference::eStart;

    
    
    

    if (!IsChromeURI(mDocumentURI)) {
        
        rv = NodePrincipal()->CheckMayLoad(aURI, PR_TRUE);
        if (NS_FAILED(rv)) {
            *aFailureFromContent = PR_TRUE;
            return rv;
        }
    }

    
    
    PRBool overlayIsChrome = IsChromeURI(aURI);
    mCurrentPrototype = overlayIsChrome ?
        nsXULPrototypeCache::GetInstance()->GetPrototype(aURI) : nsnull;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    PRBool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();
    if (aIsDynamic)
        mIsWritingFastLoad = useXULCache;

    if (useXULCache && mCurrentPrototype) {
        PRBool loaded;
        rv = mCurrentPrototype->AwaitLoadDone(this, &loaded);
        if (NS_FAILED(rv)) return rv;

        if (! loaded) {
            
            
            
            
            *aShouldReturn = PR_TRUE;
            return NS_OK;
        }

        PR_LOG(gXULLog, PR_LOG_DEBUG, ("xul: overlay was cached"));

        
        
        
        return OnPrototypeLoadDone(aIsDynamic);
    }
    else {
        
        PR_LOG(gXULLog, PR_LOG_DEBUG, ("xul: overlay was not cached"));

        
        
        
        nsCOMPtr<nsIParser> parser;
        rv = PrepareToLoadPrototype(aURI, "view", nsnull, getter_AddRefs(parser));
        if (NS_FAILED(rv)) return rv;

        
        
        
        mIsWritingFastLoad = useXULCache;

        nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(parser);
        if (! listener)
            return NS_ERROR_UNEXPECTED;

        
        
        
        ParserObserver* parserObserver =
            new ParserObserver(this, mCurrentPrototype);
        if (! parserObserver)
            return NS_ERROR_OUT_OF_MEMORY;

        NS_ADDREF(parserObserver);
        parser->Parse(aURI, parserObserver);
        NS_RELEASE(parserObserver);

        nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);
        rv = NS_OpenURI(listener, nsnull, aURI, nsnull, group);
        if (NS_FAILED(rv)) {
            
            mCurrentPrototype = nsnull;

            
            
            parser->Terminate();

            
            
            
            ReportMissingOverlay(aURI);
            
            
            *aFailureFromContent = PR_TRUE;
            return rv;
        }

        
        
        
        
        
        if (useXULCache && overlayIsChrome) {
            nsXULPrototypeCache::GetInstance()->PutPrototype(mCurrentPrototype);
        }

        
        
        
        
        if (!aIsDynamic)
            *aShouldReturn = PR_TRUE;
    }
    return NS_OK;
}

static PLDHashOperator
FirePendingMergeNotification(nsIURI* aKey, nsCOMPtr<nsIObserver>& aObserver, void* aClosure)
{
    aObserver->Observe(aKey, "xul-overlay-merged", EmptyString().get());

    typedef nsInterfaceHashtable<nsURIHashKey,nsIObserver> table;
    table* observers = static_cast<table*>(aClosure);
    observers->Remove(aKey);

    return PL_DHASH_REMOVE;
}

nsresult
nsXULDocument::ResumeWalk()
{
    
    
    
    
    
    
    
    
    
    nsresult rv;
    nsCOMPtr<nsIURI> overlayURI =
        mCurrentPrototype ? mCurrentPrototype->GetURI() : nsnull;

    while (1) {
        

        while (mContextStack.Depth() > 0) {
            
            
            
            
            nsXULPrototypeElement* proto;
            nsCOMPtr<nsIContent> element;
            PRInt32 indx; 
                          
            rv = mContextStack.Peek(&proto, getter_AddRefs(element), &indx);
            if (NS_FAILED(rv)) return rv;

            if (indx >= (PRInt32)proto->mChildren.Length()) {
                if (element) {
                    
                    
                    
                    
                    
                    if (mState == eState_Master) {
                        AddElementToDocumentPost(element);

                        if (element->NodeInfo()->Equals(nsGkAtoms::style,
                                                        kNameSpaceID_XHTML) ||
                            element->NodeInfo()->Equals(nsGkAtoms::style,
                                                        kNameSpaceID_SVG)) {
                            
                            
                            nsCOMPtr<nsIStyleSheetLinkingElement> ssle =
                                do_QueryInterface(element);
                            NS_ASSERTION(ssle, "<html:style> doesn't implement "
                                               "nsIStyleSheetLinkingElement?");
                            PRBool willNotify;
                            PRBool isAlternate;
                            ssle->UpdateStyleSheet(nsnull, &willNotify,
                                                   &isAlternate);
                        }
                    }

#ifdef MOZ_XTF
                    if (element->GetNameSpaceID() > kNameSpaceID_LastBuiltin) {
                        element->DoneAddingChildren(PR_FALSE);
                    }
#endif
                }
                
                
                mContextStack.Pop();
                continue;
            }

            
            
            nsXULPrototypeNode* childproto = proto->mChildren[indx];
            mContextStack.SetTopIndex(++indx);

            
            
            
            
            
            PRBool processingOverlayHookupNodes = (mState == eState_Overlay) && 
                                                  (mContextStack.Depth() == 1);

            NS_ASSERTION(element || processingOverlayHookupNodes,
                         "no element on context stack");

            switch (childproto->mType) {
            case nsXULPrototypeNode::eType_Element: {
                
                nsXULPrototypeElement* protoele =
                    static_cast<nsXULPrototypeElement*>(childproto);

                nsCOMPtr<nsIContent> child;

                if (!processingOverlayHookupNodes) {
                    rv = CreateElementFromPrototype(protoele,
                                                    getter_AddRefs(child));
                    if (NS_FAILED(rv)) return rv;

                    
                    rv = element->AppendChildTo(child, PR_FALSE);
                    if (NS_FAILED(rv)) return rv;

                    
                    
                    
                    
                    if (mState == eState_Master)
                        AddElementToDocumentPre(child);
                }
                else {
                    
                    
                    
                    
                    
                    rv = CreateOverlayElement(protoele, getter_AddRefs(child));
                    if (NS_FAILED(rv)) return rv;
                }

                
                
                if (protoele->mChildren.Length() > 0) {
                    rv = mContextStack.Push(protoele, child);
                    if (NS_FAILED(rv)) return rv;
                }
                else {
                    if (mState == eState_Master) {
                        
                        
                        
                        AddElementToDocumentPost(child);
                    }
#ifdef MOZ_XTF
                    if (child &&
                        child->GetNameSpaceID() > kNameSpaceID_LastBuiltin) {
                        child->DoneAddingChildren(PR_FALSE);
                    }
#endif
                }
            }
            break;

            case nsXULPrototypeNode::eType_Script: {
                
                
                nsXULPrototypeScript* scriptproto =
                    static_cast<nsXULPrototypeScript*>(childproto);

                if (scriptproto->mSrcURI) {
                    
                    
                    
                    
                    PRBool blocked;
                    rv = LoadScript(scriptproto, &blocked);
                    

                    if (NS_SUCCEEDED(rv) && blocked)
                        return NS_OK;
                }
                else if (scriptproto->mScriptObject.mObject) {
                    
                    rv = ExecuteScript(scriptproto);
                    if (NS_FAILED(rv)) return rv;
                }
            }
            break;

            case nsXULPrototypeNode::eType_Text: {
                

                if (!processingOverlayHookupNodes) {
                    
                    

                    nsCOMPtr<nsIContent> text;
                    rv = NS_NewTextNode(getter_AddRefs(text),
                                        mNodeInfoManager);
                    NS_ENSURE_SUCCESS(rv, rv);

                    nsXULPrototypeText* textproto =
                        static_cast<nsXULPrototypeText*>(childproto);
                    text->SetText(textproto->mValue, PR_FALSE);

                    rv = element->AppendChildTo(text, PR_FALSE);
                    NS_ENSURE_SUCCESS(rv, rv);
                }
            }
            break;

            case nsXULPrototypeNode::eType_PI: {
                nsXULPrototypePI* piProto =
                    static_cast<nsXULPrototypePI*>(childproto);

                
                

                if (piProto->mTarget.EqualsLiteral("xml-stylesheet") ||
                    piProto->mTarget.EqualsLiteral("xul-overlay")) {

                    const PRUnichar* params[] = { piProto->mTarget.get() };

                    nsContentUtils::ReportToConsole(
                                        nsContentUtils::eXUL_PROPERTIES,
                                        "PINotInProlog",
                                        params, NS_ARRAY_LENGTH(params),
                                        overlayURI,
                                        EmptyString(), 
                                        0, 
                                        0, 
                                        nsIScriptError::warningFlag,
                                        "XUL Document");
                }

                nsIContent* parent = processingOverlayHookupNodes ?
                    GetRootContent() : element.get();

                if (parent) {
                    
                    rv = CreateAndInsertPI(piProto, parent,
                                           parent->GetChildCount());
                    NS_ENSURE_SUCCESS(rv, rv);
                }
            }
            break;

            default:
                NS_NOTREACHED("Unexpected nsXULPrototypeNode::Type value");
            }
        }

        
        
        

        
        mState = eState_Overlay;

        
        PRUint32 count = mUnloadedOverlays.Count();
        if (! count)
            break;

        nsCOMPtr<nsIURI> uri = mUnloadedOverlays[count-1];
        mUnloadedOverlays.RemoveObjectAt(count-1);

        PRBool shouldReturn, failureFromContent;
        rv = LoadOverlayInternal(uri, PR_FALSE, &shouldReturn,
                                 &failureFromContent);
        if (failureFromContent)
            
            
            
            continue;
        if (NS_FAILED(rv))
            return rv;
        if (mOverlayLoadObservers.IsInitialized()) {
            nsIObserver *obs = mOverlayLoadObservers.GetWeak(overlayURI);
            if (obs) {
                
                
                
                
                if (!mOverlayLoadObservers.GetWeak(uri))
                    mOverlayLoadObservers.Put(uri, obs);
                mOverlayLoadObservers.Remove(overlayURI);
            }
        }
        if (shouldReturn)
            return NS_OK;
        overlayURI.swap(uri);
    }

    
    
    rv = ResolveForwardReferences();
    if (NS_FAILED(rv)) return rv;

    ApplyPersistentAttributes();

    mStillWalking = PR_FALSE;
    if (mPendingSheets == 0) {
        rv = DoneWalking();
    }
    return rv;
}

nsresult
nsXULDocument::DoneWalking()
{
    NS_PRECONDITION(mPendingSheets == 0, "there are sheets to be loaded");
    NS_PRECONDITION(!mStillWalking, "walk not done");

    
    

    PRUint32 count = mOverlaySheets.Count();
    for (PRUint32 i = 0; i < count; ++i) {
        AddStyleSheet(mOverlaySheets[i]);
    }
    mOverlaySheets.Clear();

    if (!mDocumentLoaded) {
        
        
        
        
        
        
        
        mDocumentLoaded = PR_TRUE;

        NotifyPossibleTitleChange(PR_FALSE);

        
        
        
        nsCOMPtr<nsISupports> container = GetContainer();
        nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(container);
        if (item) {
            nsCOMPtr<nsIDocShellTreeOwner> owner;
            item->GetTreeOwner(getter_AddRefs(owner));
            nsCOMPtr<nsIXULWindow> xulWin = do_GetInterface(owner);
            if (xulWin) {
                nsCOMPtr<nsIDocShell> xulWinShell;
                xulWin->GetDocShell(getter_AddRefs(xulWinShell));
                if (SameCOMIdentity(xulWinShell, container)) {
                    
                    xulWin->ApplyChromeFlags();
                }
            }
        }

        StartLayout();

        if (mIsWritingFastLoad && IsChromeURI(mDocumentURI))
            nsXULPrototypeCache::GetInstance()->WritePrototype(mMasterPrototype);

        NS_ASSERTION(mDelayFrameLoaderInitialization,
                     "mDelayFrameLoaderInitialization should be true!");
        mDelayFrameLoaderInitialization = PR_FALSE;
        NS_WARN_IF_FALSE(mUpdateNestLevel == 0,
                         "Constructing XUL document in middle of an update?");
        if (mUpdateNestLevel == 0) {
            MaybeInitializeFinalizeFrameLoaders();
        }

        NS_DOCUMENT_NOTIFY_OBSERVERS(EndLoad, (this));

        
        
        DispatchContentLoadedEvents();

        mInitialLayoutComplete = PR_TRUE;

        
        
        if (mPendingOverlayLoadNotifications.IsInitialized())
            mPendingOverlayLoadNotifications.Enumerate(FirePendingMergeNotification, (void*)&mOverlayLoadObservers);
    }
    else {
        if (mOverlayLoadObservers.IsInitialized()) {
            nsCOMPtr<nsIURI> overlayURI = mCurrentPrototype->GetURI();
            nsCOMPtr<nsIObserver> obs;
            if (mInitialLayoutComplete) {
                
                mOverlayLoadObservers.Get(overlayURI, getter_AddRefs(obs));
                if (obs)
                    obs->Observe(overlayURI, "xul-overlay-merged", EmptyString().get());
                mOverlayLoadObservers.Remove(overlayURI);
            }
            else {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                

                NS_ENSURE_TRUE(mPendingOverlayLoadNotifications.IsInitialized() || mPendingOverlayLoadNotifications.Init(), 
                               NS_ERROR_OUT_OF_MEMORY);
                
                mPendingOverlayLoadNotifications.Get(overlayURI, getter_AddRefs(obs));
                if (!obs) {
                    mOverlayLoadObservers.Get(overlayURI, getter_AddRefs(obs));
                    NS_ASSERTION(obs, "null overlay load observer?");
                    mPendingOverlayLoadNotifications.Put(overlayURI, obs);
                }
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::StyleSheetLoaded(nsICSSStyleSheet* aSheet,
                                PRBool aWasAlternate,
                                nsresult aStatus)
{
    if (!aWasAlternate) {
        

        NS_ASSERTION(mPendingSheets > 0,
            "Unexpected StyleSheetLoaded notification");

        --mPendingSheets;

        if (!mStillWalking && mPendingSheets == 0) {
            return DoneWalking();
        }
    }

    return NS_OK;
}

void
nsXULDocument::MaybeBroadcast()
{
    
    if (mUpdateNestLevel == 0 &&
        (mDelayedAttrChangeBroadcasts.Length() ||
         mDelayedBroadcasters.Length())) {
        if (!nsContentUtils::IsSafeToRunScript()) {
            if (!mInDestructor) {
                nsContentUtils::AddScriptRunner(
                  NS_NEW_RUNNABLE_METHOD(nsXULDocument, this, MaybeBroadcast));
            }
            return;
        }
        if (!mHandlingDelayedAttrChange) {
            mHandlingDelayedAttrChange = PR_TRUE;
            for (PRUint32 i = 0; i < mDelayedAttrChangeBroadcasts.Length(); ++i) {
                nsIAtom* attrName = mDelayedAttrChangeBroadcasts[i].mAttrName;
                if (mDelayedAttrChangeBroadcasts[i].mNeedsAttrChange) {
                    nsCOMPtr<nsIContent> listener =
                        do_QueryInterface(mDelayedAttrChangeBroadcasts[i].mListener);
                    nsString value = mDelayedAttrChangeBroadcasts[i].mAttr;
                    if (mDelayedAttrChangeBroadcasts[i].mSetAttr) {
                        listener->SetAttr(kNameSpaceID_None, attrName, value,
                                          PR_TRUE);
                    } else {
                        listener->UnsetAttr(kNameSpaceID_None, attrName,
                                            PR_TRUE);
                    }
                }
                nsCOMPtr<nsIContent> broadcaster =
                    do_QueryInterface(mDelayedAttrChangeBroadcasts[i].mBroadcaster);
                ExecuteOnBroadcastHandlerFor(broadcaster,
                                             mDelayedAttrChangeBroadcasts[i].mListener,
                                             attrName);
            }
            mDelayedAttrChangeBroadcasts.Clear();
            mHandlingDelayedAttrChange = PR_FALSE;
        }

        PRUint32 length = mDelayedBroadcasters.Length();
        if (length) {
            PRBool oldValue = mHandlingDelayedBroadcasters;
            mHandlingDelayedBroadcasters = PR_TRUE;
            nsTArray<nsDelayedBroadcastUpdate> delayedBroadcasters;
            mDelayedBroadcasters.SwapElements(delayedBroadcasters);
            for (PRUint32 i = 0; i < length; ++i) {
                SynchronizeBroadcastListener(delayedBroadcasters[i].mBroadcaster,
                                             delayedBroadcasters[i].mListener,
                                             delayedBroadcasters[i].mAttr);
            }
            mHandlingDelayedBroadcasters = oldValue;
        }
    }
}

void
nsXULDocument::EndUpdate(nsUpdateType aUpdateType)
{
    nsXMLDocument::EndUpdate(aUpdateType);

    MaybeBroadcast();
}

void
nsXULDocument::ReportMissingOverlay(nsIURI* aURI)
{
    NS_PRECONDITION(aURI, "Must have a URI");
    
    nsCAutoString spec;
    aURI->GetSpec(spec);

    NS_ConvertUTF8toUTF16 utfSpec(spec);
    const PRUnichar* params[] = { utfSpec.get() };

    nsContentUtils::ReportToConsole(nsContentUtils::eXUL_PROPERTIES,
                                    "MissingOverlay",
                                    params, NS_ARRAY_LENGTH(params),
                                    mDocumentURI,
                                    EmptyString(), 
                                    0, 
                                    0, 
                                    nsIScriptError::warningFlag,
                                    "XUL Document");
}

nsresult
nsXULDocument::LoadScript(nsXULPrototypeScript* aScriptProto, PRBool* aBlock)
{
    
    nsresult rv;

    PRBool isChromeDoc = IsChromeURI(mDocumentURI);

    if (isChromeDoc && aScriptProto->mScriptObject.mObject) {
        rv = ExecuteScript(aScriptProto);

        
        *aBlock = PR_FALSE;
        return NS_OK;
    }

    
    
    
    PRBool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();

    if (isChromeDoc && useXULCache) {
        PRUint32 fetchedLang = nsIProgrammingLanguage::UNKNOWN;
        void *newScriptObject =
            nsXULPrototypeCache::GetInstance()->GetScript(
                                   aScriptProto->mSrcURI,
                                   &fetchedLang);
        if (newScriptObject) {
            
            
            if (aScriptProto->mScriptObject.mLangID != fetchedLang) {
                NS_ERROR("XUL cache gave me an incorrect script language");
                return NS_ERROR_UNEXPECTED;
            }
            aScriptProto->Set(newScriptObject);
        }

        if (aScriptProto->mScriptObject.mObject) {
            rv = ExecuteScript(aScriptProto);

            
            *aBlock = PR_FALSE;
            return NS_OK;
        }
    }

    
    
    rv = nsScriptLoader::ShouldLoadScript(
                            this,
                            static_cast<nsIDocument*>(this),
                            aScriptProto->mSrcURI,
                            NS_LITERAL_STRING("application/x-javascript"));
    if (NS_FAILED(rv)) {
      *aBlock = PR_FALSE;
      return rv;
    }

    
    
    NS_ASSERTION(!mCurrentScriptProto,
                 "still loading a script when starting another load?");
    mCurrentScriptProto = aScriptProto;

    if (aScriptProto->mSrcLoading) {
        
        
        mNextSrcLoadWaiter = aScriptProto->mSrcLoadWaiters;
        aScriptProto->mSrcLoadWaiters = this;
        NS_ADDREF_THIS();
    }
    else {
        nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);

        
        nsCOMPtr<nsIStreamLoader> loader;
        rv = NS_NewStreamLoader(getter_AddRefs(loader), aScriptProto->mSrcURI,
                                this, nsnull, group);
        if (NS_FAILED(rv)) {
            mCurrentScriptProto = nsnull;
            return rv;
        }

        aScriptProto->mSrcLoading = PR_TRUE;
    }

    
    *aBlock = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::OnStreamComplete(nsIStreamLoader* aLoader,
                                nsISupports* context,
                                nsresult aStatus,
                                PRUint32 stringLen,
                                const PRUint8* string)
{
    nsCOMPtr<nsIRequest> request;
    aLoader->GetRequest(getter_AddRefs(request));
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);

#ifdef DEBUG
    
    if (NS_FAILED(aStatus)) {
        if (channel) {
            nsCOMPtr<nsIURI> uri;
            channel->GetURI(getter_AddRefs(uri));
            if (uri) {
                nsCAutoString uriSpec;
                uri->GetSpec(uriSpec);
                printf("Failed to load %s\n", uriSpec.get());
            }
        }
    }
#endif

    
    
    
    
    nsresult rv;

    NS_ASSERTION(mCurrentScriptProto && mCurrentScriptProto->mSrcLoading,
                 "script source not loading on unichar stream complete?");
    if (!mCurrentScriptProto) {
        
        return NS_OK;
    }

    
    
    
    nsXULPrototypeScript* scriptProto = mCurrentScriptProto;
    mCurrentScriptProto = nsnull;

    
    
    
    scriptProto->mSrcLoading = PR_FALSE;

    if (NS_SUCCEEDED(aStatus)) {
        
        
        
        
        
        
        
        nsCOMPtr<nsIURI> uri = scriptProto->mSrcURI;

        

        nsString stringStr;
        rv = nsScriptLoader::ConvertToUTF16(channel, string, stringLen,
                                            EmptyString(), this, stringStr);
        if (NS_SUCCEEDED(rv)) {
            rv = scriptProto->Compile(stringStr.get(), stringStr.Length(),
                                      uri, 1, this, mCurrentPrototype);
        }

        aStatus = rv;
        if (NS_SUCCEEDED(rv)) {
            if (nsScriptLoader::ShouldExecuteScript(this, channel)) {
                rv = ExecuteScript(scriptProto);
            }

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            PRBool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();
  
            if (useXULCache && IsChromeURI(mDocumentURI)) {
                nsXULPrototypeCache::GetInstance()->PutScript(
                                   scriptProto->mSrcURI,
                                   scriptProto->mScriptObject.mLangID,
                                   scriptProto->mScriptObject.mObject);
            }

            if (mIsWritingFastLoad && mCurrentPrototype != mMasterPrototype) {
                
                
                
                
                
                
                
                
                
                
                
                nsIScriptGlobalObject* global =
                    mCurrentPrototype->GetScriptGlobalObject();

                NS_ASSERTION(global != nsnull, "master prototype w/o global?!");
                if (global) {
                    PRUint32 stid = scriptProto->mScriptObject.mLangID;
                    nsIScriptContext *scriptContext = \
                          global->GetScriptContext(stid);
                    NS_ASSERTION(scriptContext != nsnull,
                                 "Failed to get script context for language");
                    if (scriptContext)
                        scriptProto->SerializeOutOfLine(nsnull, global);
                }
            }
        }
        
    }

    rv = ResumeWalk();

    
    
    nsXULDocument** docp = &scriptProto->mSrcLoadWaiters;

    
    
    nsXULDocument* doc;
    while ((doc = *docp) != nsnull) {
        NS_ASSERTION(doc->mCurrentScriptProto == scriptProto,
                     "waiting for wrong script to load?");
        doc->mCurrentScriptProto = nsnull;

        
        *docp = doc->mNextSrcLoadWaiter;
        doc->mNextSrcLoadWaiter = nsnull;

        
        if (NS_SUCCEEDED(aStatus) && scriptProto->mScriptObject.mObject &&
            nsScriptLoader::ShouldExecuteScript(doc, channel)) {
            doc->ExecuteScript(scriptProto);
        }
        doc->ResumeWalk();
        NS_RELEASE(doc);
    }

    return rv;
}


nsresult
nsXULDocument::ExecuteScript(nsIScriptContext * aContext, void * aScriptObject)
{
    NS_PRECONDITION(aScriptObject != nsnull && aContext != nsnull, "null ptr");
    if (! aScriptObject || ! aContext)
        return NS_ERROR_NULL_POINTER;

    NS_ENSURE_TRUE(mScriptGlobalObject, NS_ERROR_NOT_INITIALIZED);

    
    nsresult rv;
    void *global = mScriptGlobalObject->GetScriptGlobal(
                                            aContext->GetScriptTypeID());
    rv = aContext->ExecuteScript(aScriptObject,
                                 global,
                                 nsnull, nsnull);

    return rv;
}

nsresult
nsXULDocument::ExecuteScript(nsXULPrototypeScript *aScript)
{
    NS_PRECONDITION(aScript != nsnull, "null ptr");
    NS_ENSURE_TRUE(aScript, NS_ERROR_NULL_POINTER);
    NS_ENSURE_TRUE(mScriptGlobalObject, NS_ERROR_NOT_INITIALIZED);
    PRUint32 stid = aScript->mScriptObject.mLangID;

    nsresult rv;
    rv = mScriptGlobalObject->EnsureScriptEnvironment(stid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIScriptContext> context =
      mScriptGlobalObject->GetScriptContext(stid);
    
    NS_ENSURE_TRUE(context != nsnull, NS_ERROR_UNEXPECTED);

    if (aScript->mScriptObject.mObject)
        rv = ExecuteScript(context, aScript->mScriptObject.mObject);
    else
        rv = NS_ERROR_UNEXPECTED;
    return rv;
}


nsresult
nsXULDocument::CreateElementFromPrototype(nsXULPrototypeElement* aPrototype,
                                          nsIContent** aResult)
{
    
    NS_PRECONDITION(aPrototype != nsnull, "null ptr");
    if (! aPrototype)
        return NS_ERROR_NULL_POINTER;

    *aResult = nsnull;
    nsresult rv = NS_OK;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_NOTICE)) {
        nsAutoString tagstr;
        aPrototype->mNodeInfo->GetQualifiedName(tagstr);

        nsCAutoString tagstrC;
        tagstrC.AssignWithConversion(tagstr);
        PR_LOG(gXULLog, PR_LOG_NOTICE,
               ("xul: creating <%s> from prototype",
                tagstrC.get()));
    }
#endif

    nsCOMPtr<nsIContent> result;

    if (aPrototype->mNodeInfo->NamespaceEquals(kNameSpaceID_XUL)) {
        
        
        rv = nsXULElement::Create(aPrototype, this, PR_TRUE, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        
        
        
        
        nsCOMPtr<nsINodeInfo> newNodeInfo;
        newNodeInfo = mNodeInfoManager->GetNodeInfo(aPrototype->mNodeInfo->NameAtom(),
                                                    aPrototype->mNodeInfo->GetPrefixAtom(),
                                                    aPrototype->mNodeInfo->NamespaceID());
        if (!newNodeInfo) return NS_ERROR_OUT_OF_MEMORY;
        rv = NS_NewElement(getter_AddRefs(result), newNodeInfo->NamespaceID(),
                           newNodeInfo, PR_FALSE);
        if (NS_FAILED(rv)) return rv;

#ifdef MOZ_XTF
        if (result && newNodeInfo->NamespaceID() > kNameSpaceID_LastBuiltin) {
            result->BeginAddingChildren();
        }
#endif

        rv = AddAttributes(aPrototype, result);
        if (NS_FAILED(rv)) return rv;
    }

    result.swap(*aResult);

    return NS_OK;
}

nsresult
nsXULDocument::CreateOverlayElement(nsXULPrototypeElement* aPrototype,
                                    nsIContent** aResult)
{
    nsresult rv;

    nsCOMPtr<nsIContent> element;
    rv = CreateElementFromPrototype(aPrototype, getter_AddRefs(element));
    if (NS_FAILED(rv)) return rv;

    OverlayForwardReference* fwdref =
        new OverlayForwardReference(this, element);
    if (! fwdref)
        return NS_ERROR_OUT_OF_MEMORY;

    
    rv = AddForwardReference(fwdref);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*aResult = element);
    return NS_OK;
}

nsresult
nsXULDocument::AddAttributes(nsXULPrototypeElement* aPrototype,
                             nsIContent* aElement)
{
    nsresult rv;

    for (PRUint32 i = 0; i < aPrototype->mNumAttributes; ++i) {
        nsXULPrototypeAttribute* protoattr = &(aPrototype->mAttributes[i]);
        nsAutoString  valueStr;
        protoattr->mValue.ToString(valueStr);

        rv = aElement->SetAttr(protoattr->mName.NamespaceID(),
                               protoattr->mName.LocalName(),
                               protoattr->mName.GetPrefix(),
                               valueStr,
                               PR_FALSE);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
nsXULDocument::CheckTemplateBuilderHookup(nsIContent* aElement,
                                          PRBool* aNeedsHookup)
{
    
    
    
    
    
    
    
    nsCOMPtr<nsIDOMXULElement> xulElement = do_QueryInterface(aElement);
    if (xulElement) {
        nsCOMPtr<nsIRDFCompositeDataSource> ds;
        xulElement->GetDatabase(getter_AddRefs(ds));
        if (ds) {
            *aNeedsHookup = PR_FALSE;
            return NS_OK;
        }
    }

    
    
    *aNeedsHookup = aElement->HasAttr(kNameSpaceID_None,
                                      nsGkAtoms::datasources);
    return NS_OK;
}

 nsresult
nsXULDocument::CreateTemplateBuilder(nsIContent* aElement)
{
    
    PRBool isTreeBuilder = PR_FALSE;

    
    
    nsIDocument *document = aElement->GetCurrentDoc();
    NS_ENSURE_TRUE(document, NS_OK);

    PRInt32 nameSpaceID;
    nsIAtom* baseTag = document->BindingManager()->
      ResolveTag(aElement, &nameSpaceID);

    if ((nameSpaceID == kNameSpaceID_XUL) && (baseTag == nsGkAtoms::tree)) {
        
        
        
        

        nsAutoString flags;
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::flags, flags);
        if (flags.Find(NS_LITERAL_STRING("dont-build-content")) >= 0) {
            isTreeBuilder = PR_TRUE;
        }
    }

    if (isTreeBuilder) {
        
        nsCOMPtr<nsIXULTemplateBuilder> builder =
            do_CreateInstance("@mozilla.org/xul/xul-tree-builder;1");

        if (! builder)
            return NS_ERROR_FAILURE;

        builder->Init(aElement);

        
        
        nsCOMPtr<nsIContent> bodyContent;
        nsXULContentUtils::FindChildByTag(aElement, kNameSpaceID_XUL,
                                          nsGkAtoms::treechildren,
                                          getter_AddRefs(bodyContent));

        if (! bodyContent) {
            nsresult rv = document->CreateElem(nsGkAtoms::treechildren,
                                               nsnull, kNameSpaceID_XUL,
                                               PR_FALSE,
                                               getter_AddRefs(bodyContent));
            NS_ENSURE_SUCCESS(rv, rv);

            aElement->AppendChildTo(bodyContent, PR_FALSE);
        }
    }
    else {
        
        nsCOMPtr<nsIXULTemplateBuilder> builder
            = do_CreateInstance("@mozilla.org/xul/xul-template-builder;1");

        if (! builder)
            return NS_ERROR_FAILURE;

        builder->Init(aElement);
        builder->CreateContents(aElement, PR_FALSE);
    }

    return NS_OK;
}


nsresult
nsXULDocument::AddPrototypeSheets()
{
    nsresult rv;

    const nsCOMArray<nsIURI>& sheets = mCurrentPrototype->GetStyleSheetReferences();

    for (PRInt32 i = 0; i < sheets.Count(); i++) {
        nsCOMPtr<nsIURI> uri = sheets[i];

        nsCOMPtr<nsICSSStyleSheet> incompleteSheet;
        rv = CSSLoader()->LoadSheet(uri,
                                    mCurrentPrototype->DocumentPrincipal(),
                                    EmptyCString(), this,
                                    getter_AddRefs(incompleteSheet));

        
        
        
        if (NS_SUCCEEDED(rv)) {
            ++mPendingSheets;
            if (!mOverlaySheets.AppendObject(incompleteSheet)) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
    }

    return NS_OK;
}







nsForwardReference::Result
nsXULDocument::OverlayForwardReference::Resolve()
{
    
    
    nsresult rv;
    nsCOMPtr<nsIContent> target;

    nsIPresShell *shell = mDocument->GetPrimaryShell();
    PRBool notify = shell && shell->DidInitialReflow();

    nsAutoString id;
    mOverlay->GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);
    if (id.IsEmpty()) {
        
        
        nsIContent* root = mDocument->GetRootContent();
        if (!root) {
            return eResolve_Error;
        }

        rv = mDocument->InsertElement(root, mOverlay, notify);
        if (NS_FAILED(rv)) return eResolve_Error;

        target = mOverlay;
    }
    else {
        
        
        nsCOMPtr<nsIDOMElement> domtarget;
        rv = mDocument->GetElementById(id, getter_AddRefs(domtarget));
        if (NS_FAILED(rv)) return eResolve_Error;

        
        
        target = do_QueryInterface(domtarget);
        NS_ASSERTION(!domtarget || target, "not an nsIContent");
        if (!target)
            return eResolve_Later;

        
        
        
        
        PRUint32 oldDefLang = target->GetScriptTypeID();
        target->SetScriptTypeID(mOverlay->GetScriptTypeID());
        rv = Merge(target, mOverlay, notify);
        target->SetScriptTypeID(oldDefLang);
        if (NS_FAILED(rv)) return eResolve_Error;
    }

    
    if (!notify && target->GetCurrentDoc() == mDocument) {
        
        
        
        rv = mDocument->AddSubtreeToDocument(target);
        if (NS_FAILED(rv)) return eResolve_Error;
    }

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_NOTICE)) {
        nsCAutoString idC;
        idC.AssignWithConversion(id);
        PR_LOG(gXULLog, PR_LOG_NOTICE,
               ("xul: overlay resolved '%s'",
                idC.get()));
    }
#endif

    mResolved = PR_TRUE;
    return eResolve_Succeeded;
}



nsresult
nsXULDocument::OverlayForwardReference::Merge(nsIContent* aTargetNode,
                                              nsIContent* aOverlayNode, 
                                              PRBool aNotify)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv;

    
    
    PRUint32 i;
    const nsAttrName* name;
    for (i = 0; (name = aOverlayNode->GetAttrNameAt(i)); ++i) {
        
        if (name->Equals(nsGkAtoms::id))
            continue;

        
        if (!aNotify) {
            if (aTargetNode->NodeInfo()->Equals(nsGkAtoms::observes,
                                                kNameSpaceID_XUL))
                continue;

            if (name->Equals(nsGkAtoms::observes) &&
                aTargetNode->HasAttr(kNameSpaceID_None, nsGkAtoms::observes))
                continue;

            if (name->Equals(nsGkAtoms::command) &&
                aTargetNode->HasAttr(kNameSpaceID_None, nsGkAtoms::command) &&
                !aTargetNode->NodeInfo()->Equals(nsGkAtoms::key,
                                                 kNameSpaceID_XUL) &&
                !aTargetNode->NodeInfo()->Equals(nsGkAtoms::menuitem,
                                                 kNameSpaceID_XUL))
                continue;
        }

        PRInt32 nameSpaceID = name->NamespaceID();
        nsIAtom* attr = name->LocalName();
        nsIAtom* prefix = name->GetPrefix();

        nsAutoString value;
        aOverlayNode->GetAttr(nameSpaceID, attr, value);

        
        
        if (attr == nsGkAtoms::removeelement &&
            value.EqualsLiteral("true")) {

            rv = RemoveElement(aTargetNode->GetParent(), aTargetNode);
            if (NS_FAILED(rv)) return rv;

            return NS_OK;
        }

        rv = aTargetNode->SetAttr(nameSpaceID, attr, prefix, value, aNotify);
        if (NS_FAILED(rv)) return rv;
    }


    
    
    
    
    
    

    PRUint32 childCount = aOverlayNode->GetChildCount();

    
    
    nsCOMPtr<nsIContent> currContent;

    for (i = 0; i < childCount; ++i) {
        currContent = aOverlayNode->GetChildAt(0);

        nsAutoString id;
        currContent->GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);

        nsCOMPtr<nsIDOMElement> nodeInDocument;
        if (!id.IsEmpty()) {
            nsCOMPtr<nsIDOMDocument> domDocument(
                        do_QueryInterface(aTargetNode->GetDocument()));
            if (!domDocument) return NS_ERROR_FAILURE;

            rv = domDocument->GetElementById(id, getter_AddRefs(nodeInDocument));
            if (NS_FAILED(rv)) return rv;
        }

        
        
        
        
        
        if (nodeInDocument) {
            
            
            
            

            nsCOMPtr<nsIDOMNode> nodeParent;
            rv = nodeInDocument->GetParentNode(getter_AddRefs(nodeParent));
            if (NS_FAILED(rv)) return rv;
            nsCOMPtr<nsIDOMElement> elementParent(do_QueryInterface(nodeParent));

            nsAutoString parentID;
            elementParent->GetAttribute(NS_LITERAL_STRING("id"), parentID);
            if (aTargetNode->AttrValueIs(kNameSpaceID_None, nsGkAtoms::id,
                                         parentID, eCaseMatters)) {
                
                nsCOMPtr<nsIContent> childDocumentContent(do_QueryInterface(nodeInDocument));
                rv = Merge(childDocumentContent, currContent, aNotify);
                if (NS_FAILED(rv)) return rv;
                rv = aOverlayNode->RemoveChildAt(0, PR_FALSE);
                if (NS_FAILED(rv)) return rv;

                continue;
            }
        }

        rv = aOverlayNode->RemoveChildAt(0, PR_FALSE);
        if (NS_FAILED(rv)) return rv;

        rv = InsertElement(aTargetNode, currContent, aNotify);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}



nsXULDocument::OverlayForwardReference::~OverlayForwardReference()
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_WARNING) && !mResolved) {
        nsAutoString id;
        mOverlay->GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);

        nsCAutoString idC;
        idC.AssignWithConversion(id);
        PR_LOG(gXULLog, PR_LOG_WARNING,
               ("xul: overlay failed to resolve '%s'",
                idC.get()));
    }
#endif
}







nsForwardReference::Result
nsXULDocument::BroadcasterHookup::Resolve()
{
    nsresult rv;

    PRBool listener;
    rv = mDocument->CheckBroadcasterHookup(mObservesElement, &listener, &mResolved);
    if (NS_FAILED(rv)) return eResolve_Error;

    return mResolved ? eResolve_Succeeded : eResolve_Later;
}


nsXULDocument::BroadcasterHookup::~BroadcasterHookup()
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_WARNING) && !mResolved) {
        
        nsresult rv;

        nsIAtom *tag = mObservesElement->Tag();

        nsAutoString broadcasterID;
        nsAutoString attribute;

        if (tag == nsGkAtoms::observes) {
            mObservesElement->GetAttr(kNameSpaceID_None, nsGkAtoms::element, broadcasterID);
            mObservesElement->GetAttr(kNameSpaceID_None, nsGkAtoms::attribute, attribute);
        }
        else {
            mObservesElement->GetAttr(kNameSpaceID_None, nsGkAtoms::observes, broadcasterID);
            attribute.AssignLiteral("*");
        }

        nsAutoString tagStr;
        rv = tag->ToString(tagStr);
        if (NS_FAILED(rv)) return;

        nsCAutoString tagstrC, attributeC,broadcasteridC;
        tagstrC.AssignWithConversion(tagStr);
        attributeC.AssignWithConversion(attribute);
        broadcasteridC.AssignWithConversion(broadcasterID);
        PR_LOG(gXULLog, PR_LOG_WARNING,
               ("xul: broadcaster hookup failed <%s attribute='%s'> to %s",
                tagstrC.get(),
                attributeC.get(),
                broadcasteridC.get()));
    }
#endif
}







nsForwardReference::Result
nsXULDocument::TemplateBuilderHookup::Resolve()
{
    PRBool needsHookup;
    nsresult rv = CheckTemplateBuilderHookup(mElement, &needsHookup);
    if (NS_FAILED(rv))
        return eResolve_Error;

    if (needsHookup) {
        rv = CreateTemplateBuilder(mElement);
        if (NS_FAILED(rv))
            return eResolve_Error;
    }

    return eResolve_Succeeded;
}




nsresult
nsXULDocument::FindBroadcaster(nsIContent* aElement,
                               nsIDOMElement** aListener,
                               nsString& aBroadcasterID,
                               nsString& aAttribute,
                               nsIDOMElement** aBroadcaster)
{
    NS_ASSERTION(aElement->IsNodeOfType(nsINode::eELEMENT),
                 "Only pass elements into FindBroadcaster!");

    nsresult rv;
    nsINodeInfo *ni = aElement->NodeInfo();
    *aListener = nsnull;
    *aBroadcaster = nsnull;

    if (ni->Equals(nsGkAtoms::observes, kNameSpaceID_XUL)) {
        
        
        
        
        
        nsIContent* parent = aElement->GetParent();
        if (!parent) {
             
            return NS_FINDBROADCASTER_NOT_FOUND;
        }

        
        
        if (parent->NodeInfo()->Equals(nsGkAtoms::overlay,
                                       kNameSpaceID_XUL)) {
            return NS_FINDBROADCASTER_AWAIT_OVERLAYS;
        }

        if (NS_FAILED(CallQueryInterface(parent, aListener)))
            *aListener = nsnull;

        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::element, aBroadcasterID);
        if (aBroadcasterID.IsEmpty()) {
            return NS_FINDBROADCASTER_NOT_FOUND;
        }
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::attribute, aAttribute);
    }
    else {
        
        
        
        
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::observes, aBroadcasterID);

        
        if (aBroadcasterID.IsEmpty()) {
            
            aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::command, aBroadcasterID);
            if (!aBroadcasterID.IsEmpty()) {
                
                
                

                if (ni->Equals(nsGkAtoms::menuitem, kNameSpaceID_XUL) ||
                    ni->Equals(nsGkAtoms::key, kNameSpaceID_XUL)) {
                return NS_FINDBROADCASTER_NOT_FOUND;
              }
            }
            else {
              return NS_FINDBROADCASTER_NOT_FOUND;
            }
        }

        if (NS_FAILED(CallQueryInterface(aElement, aListener)))
            *aListener = nsnull;

        aAttribute.AssignLiteral("*");
    }

    
    NS_ENSURE_TRUE(*aListener, NS_ERROR_UNEXPECTED);

    
    rv = GetElementById(aBroadcasterID, aBroadcaster);
    if (NS_FAILED(rv)) return rv;

    
    
    
    if (! *aBroadcaster) {
        return NS_FINDBROADCASTER_AWAIT_OVERLAYS;
    }

    return NS_FINDBROADCASTER_FOUND;
}

nsresult
nsXULDocument::CheckBroadcasterHookup(nsIContent* aElement,
                                      PRBool* aNeedsHookup,
                                      PRBool* aDidResolve)
{
    
    
    
    nsresult rv;

    *aDidResolve = PR_FALSE;

    nsCOMPtr<nsIDOMElement> listener;
    nsAutoString broadcasterID;
    nsAutoString attribute;
    nsCOMPtr<nsIDOMElement> broadcaster;

    rv = FindBroadcaster(aElement, getter_AddRefs(listener),
                         broadcasterID, attribute, getter_AddRefs(broadcaster));
    switch (rv) {
        case NS_FINDBROADCASTER_NOT_FOUND:
            *aNeedsHookup = PR_FALSE;
            return NS_OK;
        case NS_FINDBROADCASTER_AWAIT_OVERLAYS:
            *aNeedsHookup = PR_TRUE;
            return NS_OK;
        case NS_FINDBROADCASTER_FOUND:
            break;
        default:
            return rv;
    }

    rv = AddBroadcastListenerFor(broadcaster, listener, attribute);
    if (NS_FAILED(rv)) return rv;

#ifdef PR_LOGGING
    
    if (PR_LOG_TEST(gXULLog, PR_LOG_NOTICE)) {
        nsCOMPtr<nsIContent> content =
            do_QueryInterface(listener);

        NS_ASSERTION(content != nsnull, "not an nsIContent");
        if (! content)
            return rv;

        nsAutoString tagStr;
        rv = content->Tag()->ToString(tagStr);
        if (NS_FAILED(rv)) return rv;

        nsCAutoString tagstrC, attributeC,broadcasteridC;
        tagstrC.AssignWithConversion(tagStr);
        attributeC.AssignWithConversion(attribute);
        broadcasteridC.AssignWithConversion(broadcasterID);
        PR_LOG(gXULLog, PR_LOG_NOTICE,
               ("xul: broadcaster hookup <%s attribute='%s'> to %s",
                tagstrC.get(),
                attributeC.get(),
                broadcasteridC.get()));
    }
#endif

    *aNeedsHookup = PR_FALSE;
    *aDidResolve = PR_TRUE;
    return NS_OK;
}

nsresult
nsXULDocument::InsertElement(nsIContent* aParent, nsIContent* aChild, PRBool aNotify)
{
    
    
    nsresult rv;

    nsAutoString posStr;
    PRBool wasInserted = PR_FALSE;

    
    aChild->GetAttr(kNameSpaceID_None, nsGkAtoms::insertafter, posStr);
    PRBool isInsertAfter = PR_TRUE;

    if (posStr.IsEmpty()) {
        aChild->GetAttr(kNameSpaceID_None, nsGkAtoms::insertbefore, posStr);
        isInsertAfter = PR_FALSE;
    }

    if (!posStr.IsEmpty()) {
        nsCOMPtr<nsIDOMDocument> domDocument(
               do_QueryInterface(aParent->GetDocument()));
        if (!domDocument) return NS_ERROR_FAILURE;

        nsCOMPtr<nsIDOMElement> domElement;

        char* str = ToNewCString(posStr);
        char* rest;
        char* token = nsCRT::strtok(str, ", ", &rest);

        while (token) {
            rv = domDocument->GetElementById(NS_ConvertASCIItoUTF16(token),
                                             getter_AddRefs(domElement));
            if (domElement)
                break;

            token = nsCRT::strtok(rest, ", ", &rest);
        }
        nsMemory::Free(str);
        if (NS_FAILED(rv))
            return rv;

        if (domElement) {
            nsCOMPtr<nsIContent> content(do_QueryInterface(domElement));
            NS_ASSERTION(content != nsnull, "null ptr");
            if (!content)
                return NS_ERROR_UNEXPECTED;

            PRInt32 pos = aParent->IndexOf(content);

            if (pos != -1) {
                pos = isInsertAfter ? pos + 1 : pos;
                rv = aParent->InsertChildAt(aChild, pos, aNotify);
                if (NS_FAILED(rv))
                    return rv;

                wasInserted = PR_TRUE;
            }
        }
    }

    if (!wasInserted) {

        aChild->GetAttr(kNameSpaceID_None, nsGkAtoms::position, posStr);
        if (!posStr.IsEmpty()) {
            
            PRInt32 pos = posStr.ToInteger(reinterpret_cast<PRInt32*>(&rv));
            
            
            
            
            if (NS_SUCCEEDED(rv) && pos > 0 &&
                PRUint32(pos - 1) <= aParent->GetChildCount()) {
                rv = aParent->InsertChildAt(aChild, pos - 1, aNotify);
                if (NS_SUCCEEDED(rv))
                    wasInserted = PR_TRUE;
                
                
                
                
            }
        }
    }

    if (! wasInserted) {
        rv = aParent->AppendChildTo(aChild, aNotify);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

nsresult
nsXULDocument::RemoveElement(nsIContent* aParent, nsIContent* aChild)
{
    PRInt32 nodeOffset = aParent->IndexOf(aChild);

    return aParent->RemoveChildAt(nodeOffset, PR_TRUE);
}






nsXULDocument::CachedChromeStreamListener::CachedChromeStreamListener(nsXULDocument* aDocument, PRBool aProtoLoaded)
    : mDocument(aDocument),
      mProtoLoaded(aProtoLoaded)
{
    NS_ADDREF(mDocument);
}


nsXULDocument::CachedChromeStreamListener::~CachedChromeStreamListener()
{
    NS_RELEASE(mDocument);
}


NS_IMPL_ISUPPORTS2(nsXULDocument::CachedChromeStreamListener,
                   nsIRequestObserver, nsIStreamListener)

NS_IMETHODIMP
nsXULDocument::CachedChromeStreamListener::OnStartRequest(nsIRequest *request,
                                                          nsISupports* acontext)
{
    return NS_ERROR_PARSED_DATA_CACHED;
}


NS_IMETHODIMP
nsXULDocument::CachedChromeStreamListener::OnStopRequest(nsIRequest *request,
                                                         nsISupports* aContext,
                                                         nsresult aStatus)
{
    if (! mProtoLoaded)
        return NS_OK;

    return mDocument->OnPrototypeLoadDone(PR_TRUE);
}


NS_IMETHODIMP
nsXULDocument::CachedChromeStreamListener::OnDataAvailable(nsIRequest *request,
                                                           nsISupports* aContext,
                                                           nsIInputStream* aInStr,
                                                           PRUint32 aSourceOffset,
                                                           PRUint32 aCount)
{
    NS_NOTREACHED("CachedChromeStream doesn't receive data");
    return NS_ERROR_UNEXPECTED;
}






nsXULDocument::ParserObserver::ParserObserver(nsXULDocument* aDocument,
                                              nsXULPrototypeDocument* aPrototype)
    : mDocument(aDocument), mPrototype(aPrototype)
{
}

nsXULDocument::ParserObserver::~ParserObserver()
{
}

NS_IMPL_ISUPPORTS1(nsXULDocument::ParserObserver, nsIRequestObserver)

NS_IMETHODIMP
nsXULDocument::ParserObserver::OnStartRequest(nsIRequest *request,
                                              nsISupports* aContext)
{
    
    if (mPrototype) {
        nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
        nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
        if (channel && secMan) {
            nsCOMPtr<nsIPrincipal> principal;
            secMan->GetChannelPrincipal(channel, getter_AddRefs(principal));

            
            mPrototype->SetDocumentPrincipal(principal);            
        }

        
        mPrototype = nsnull;
    }
        
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::ParserObserver::OnStopRequest(nsIRequest *request,
                                             nsISupports* aContext,
                                             nsresult aStatus)
{
    nsresult rv = NS_OK;

    if (NS_FAILED(aStatus)) {
        
        
        nsCOMPtr<nsIChannel> aChannel = do_QueryInterface(request);
        if (aChannel) {
            nsCOMPtr<nsIURI> uri;
            aChannel->GetOriginalURI(getter_AddRefs(uri));
            if (uri) {
                mDocument->ReportMissingOverlay(uri);
            }
        }

        rv = mDocument->ResumeWalk();
    }

    
    
    
    mDocument = nsnull;

    return rv;
}

void
nsXULDocument::GetFocusController(nsIFocusController** aFocusController)
{
    nsCOMPtr<nsIInterfaceRequestor> ir = do_QueryReferent(mDocumentContainer);
    nsCOMPtr<nsPIDOMWindow> windowPrivate = do_GetInterface(ir);
    if (windowPrivate) {
        NS_IF_ADDREF(*aFocusController = windowPrivate->GetRootFocusController());
    } else
        *aFocusController = nsnull;
}

PRBool
nsXULDocument::IsDocumentRightToLeft()
{
    if (mDocDirection == Direction_Uninitialized) {
        mDocDirection = Direction_LeftToRight; 

        
        
        nsIContent* content = GetRootContent();
        if (content) {
            static nsIContent::AttrValuesArray strings[] =
                {&nsGkAtoms::ltr, &nsGkAtoms::rtl, nsnull};
            switch (content->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::localedir,
                                             strings, eCaseMatters)) {
                case 0: mDocDirection = Direction_LeftToRight; return PR_FALSE;
                case 1: mDocDirection = Direction_RightToLeft; return PR_TRUE;
                default: break;
            }
        }

        
        
        nsCOMPtr<nsIXULChromeRegistry> reg =
            do_GetService(NS_CHROMEREGISTRY_CONTRACTID);
        if (reg) {
            nsCAutoString package;
            PRBool isChrome;
            if (NS_SUCCEEDED(mDocumentURI->SchemeIs("chrome", &isChrome)) &&
                isChrome) {
                mDocumentURI->GetHostPort(package);
            }
            else {
                
                
                PRBool isAbout, isResource;
                if (NS_SUCCEEDED(mDocumentURI->SchemeIs("about", &isAbout)) &&
                    isAbout) {
                    package.AssignLiteral("global");
                }
                else if (NS_SUCCEEDED(mDocumentURI->SchemeIs("resource", &isResource)) &&
                    isResource) {
                    package.AssignLiteral("global");
                }
                else {
                    return PR_FALSE;
                }
            }

            nsCAutoString locale;
            reg->GetSelectedLocale(package, locale);
            if (locale.Length() >= 2) {
                
                
                
                
                nsCAutoString prefString =
                    NS_LITERAL_CSTRING("intl.uidirection.") + locale;
                nsAdoptingCString dir = nsContentUtils::GetCharPref(prefString.get());
                if (dir.IsEmpty()) {
                    PRInt32 hyphen = prefString.FindChar('-');
                    if (hyphen >= 1) {
                        nsCAutoString shortPref(Substring(prefString, 0, hyphen));
                        dir = nsContentUtils::GetCharPref(shortPref.get());
                    }
                }

                mDocDirection = dir.EqualsLiteral("rtl") ?
                                Direction_RightToLeft : Direction_LeftToRight;
            }
        }
    }

    return (mDocDirection == Direction_RightToLeft);
}

int
nsXULDocument::DirectionChanged(const char* aPrefName, void* aData)
{
  
  
  
  nsXULDocument* doc = (nsXULDocument *)aData;
  if (doc)
      doc->ResetDocumentDirection();

  nsIPresShell *shell = doc->GetPrimaryShell();
  if (shell) {
      shell->FrameConstructor()->
          PostRestyleEvent(doc->GetRootContent(), eReStyle_Self, NS_STYLE_HINT_NONE);
  }

  return 0;
}

int
nsXULDocument::GetDocumentLWTheme()
{
    if (mDocLWTheme == Doc_Theme_Uninitialized) {
        mDocLWTheme = Doc_Theme_None; 

        nsIContent* content = GetRootContent();
        nsAutoString hasLWTheme;
        if (content &&
            content->GetAttr(kNameSpaceID_None, nsGkAtoms::lwtheme, hasLWTheme) &&
            !(hasLWTheme.IsEmpty()) &&
            hasLWTheme.EqualsLiteral("true")) {
            mDocLWTheme = Doc_Theme_Neutral;
            nsAutoString lwTheme;
            content->GetAttr(kNameSpaceID_None, nsGkAtoms::lwthemetextcolor, lwTheme);
            if (!(lwTheme.IsEmpty())) {
                if (lwTheme.EqualsLiteral("dark"))
                    mDocLWTheme = Doc_Theme_Dark;
                else if (lwTheme.EqualsLiteral("bright"))
                    mDocLWTheme = Doc_Theme_Bright;
            }
        }
    }
    return mDocLWTheme;
}

NS_IMETHODIMP
nsXULDocument::GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult)
{
    return nsDocument::GetBoxObjectFor(aElement, aResult);
}
