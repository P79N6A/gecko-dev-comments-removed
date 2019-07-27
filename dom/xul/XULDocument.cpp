






















#include "mozilla/ArrayUtils.h"

#include "XULDocument.h"

#include "nsError.h"
#include "nsIBoxObject.h"
#include "nsIChromeRegistry.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsIContentViewer.h"
#include "nsIDOMXULElement.h"
#include "nsIStreamListener.h"
#include "nsITimer.h"
#include "nsDocShell.h"
#include "nsGkAtoms.h"
#include "nsXMLContentSink.h"
#include "nsXULContentSink.h"
#include "nsXULContentUtils.h"
#include "nsIXULOverlayProvider.h"
#include "nsIStringEnumerator.h"
#include "nsNetUtil.h"
#include "nsParserCIID.h"
#include "nsPIBoxObject.h"
#include "mozilla/dom/BoxObject.h"
#include "nsXPIDLString.h"
#include "nsPIDOMWindow.h"
#include "nsPIWindowRoot.h"
#include "nsXULCommandDispatcher.h"
#include "nsXULElement.h"
#include "prlog.h"
#include "rdf.h"
#include "nsIFrame.h"
#include "nsXBLService.h"
#include "nsCExternalHandlerService.h"
#include "nsMimeTypes.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsContentList.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsIParser.h"
#include "nsCharsetSource.h"
#include "nsIParserService.h"
#include "mozilla/CSSStyleSheet.h"
#include "mozilla/css/Loader.h"
#include "nsIScriptError.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsIObserverService.h"
#include "nsNodeUtils.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIXULWindow.h"
#include "nsXULPopupManager.h"
#include "nsCCUncollectableMarker.h"
#include "nsURILoader.h"
#include "mozilla/AddonPathService.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/NodeInfoInlines.h"
#include "mozilla/dom/ProcessingInstruction.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/XULDocumentBinding.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/LoadInfo.h"
#include "mozilla/Preferences.h"
#include "nsTextNode.h"
#include "nsJSUtils.h"
#include "mozilla/dom/URL.h"
#include "nsIContentPolicy.h"

using namespace mozilla;
using namespace mozilla::dom;






static NS_DEFINE_CID(kParserCID,                 NS_PARSER_CID);

static bool IsOverlayAllowed(nsIURI* aURI)
{
    bool canOverlay = false;
    if (NS_SUCCEEDED(aURI->SchemeIs("about", &canOverlay)) && canOverlay)
        return true;
    if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &canOverlay)) && canOverlay)
        return true;
    return false;
}






const nsForwardReference::Phase nsForwardReference::kPasses[] = {
    nsForwardReference::eConstruction,
    nsForwardReference::eHookup,
    nsForwardReference::eDone
};






int32_t XULDocument::gRefCnt = 0;

PRLogModuleInfo* XULDocument::gXULLog;



struct BroadcasterMapEntry : public PLDHashEntryHdr {
    Element*         mBroadcaster; 
    nsSmallVoidArray mListeners;   
};

struct BroadcastListener {
    nsWeakPtr mListener;
    nsCOMPtr<nsIAtom> mAttribute;
};

Element*
nsRefMapEntry::GetFirstElement()
{
    return static_cast<Element*>(mRefContentList.SafeElementAt(0));
}

void
nsRefMapEntry::AppendAll(nsCOMArray<nsIContent>* aElements)
{
    for (int32_t i = 0; i < mRefContentList.Count(); ++i) {
        aElements->AppendObject(static_cast<nsIContent*>(mRefContentList[i]));
    }
}

bool
nsRefMapEntry::AddElement(Element* aElement)
{
    if (mRefContentList.IndexOf(aElement) >= 0)
        return true;
    return mRefContentList.AppendElement(aElement);
}

bool
nsRefMapEntry::RemoveElement(Element* aElement)
{
    mRefContentList.RemoveElement(aElement);
    return mRefContentList.Count() == 0;
}






namespace mozilla {
namespace dom {

XULDocument::XULDocument(void)
    : XMLDocument("application/vnd.mozilla.xul+xml"),
      mDocLWTheme(Doc_Theme_Uninitialized),
      mState(eState_Master),
      mResolutionPhase(nsForwardReference::eStart)
{
    
    

    
    mCharacterSet.AssignLiteral("UTF-8");

    mDefaultElementType = kNameSpaceID_XUL;
    mType = eXUL;

    mDelayFrameLoaderInitialization = true;

    mAllowXULXBL = eTriTrue;
}

XULDocument::~XULDocument()
{
    NS_ASSERTION(mNextSrcLoadWaiter == nullptr,
        "unreferenced document still waiting for script source to load?");

    
    
    mForwardReferences.Clear();
    
    
    mPersistenceIds.Clear();

    
    if (mBroadcasterMap) {
        PL_DHashTableDestroy(mBroadcasterMap);
    }

    delete mTemplateBuilderTable;

    Preferences::UnregisterCallback(XULDocument::DirectionChanged,
                                    "intl.uidirection.", this);

    if (mOffThreadCompileStringBuf) {
      js_free(mOffThreadCompileStringBuf);
    }
}

} 
} 

nsresult
NS_NewXULDocument(nsIXULDocument** result)
{
    NS_PRECONDITION(result != nullptr, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    XULDocument* doc = new XULDocument();
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


namespace mozilla {
namespace dom {






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

NS_IMPL_CYCLE_COLLECTION_CLASS(XULDocument)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(XULDocument, XMLDocument)
    NS_ASSERTION(!nsCCUncollectableMarker::InGeneration(cb, tmp->GetMarkedCCGeneration()),
                 "Shouldn't traverse XULDocument!");
    
    

    
    
    if (tmp->mTemplateBuilderTable)
        tmp->mTemplateBuilderTable->EnumerateRead(TraverseTemplateBuilders, &cb);

    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCurrentPrototype)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMasterPrototype)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCommandDispatcher)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPrototypes);
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mLocalStore)

    if (tmp->mOverlayLoadObservers) {
        tmp->mOverlayLoadObservers->EnumerateRead(TraverseObservers, &cb);
    }
    if (tmp->mPendingOverlayLoadNotifications) {
        tmp->mPendingOverlayLoadNotifications->EnumerateRead(TraverseObservers, &cb);
    }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(XULDocument, XMLDocument)
    delete tmp->mTemplateBuilderTable;
    tmp->mTemplateBuilderTable = nullptr;

    NS_IMPL_CYCLE_COLLECTION_UNLINK(mCommandDispatcher)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mLocalStore)
    
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(XULDocument, XMLDocument)
NS_IMPL_RELEASE_INHERITED(XULDocument, XMLDocument)



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(XULDocument)
    NS_INTERFACE_TABLE_INHERITED(XULDocument, nsIXULDocument,
                                 nsIDOMXULDocument, nsIStreamLoaderObserver,
                                 nsICSSLoaderObserver, nsIOffThreadScriptReceiver)
NS_INTERFACE_TABLE_TAIL_INHERITING(XMLDocument)







void
XULDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
    NS_NOTREACHED("Reset");
}

void
XULDocument::ResetToURI(nsIURI* aURI, nsILoadGroup* aLoadGroup,
                        nsIPrincipal* aPrincipal)
{
    NS_NOTREACHED("ResetToURI");
}

void
XULDocument::SetContentType(const nsAString& aContentType)
{
    NS_ASSERTION(aContentType.EqualsLiteral("application/vnd.mozilla.xul+xml"),
                 "xul-documents always has content-type application/vnd.mozilla.xul+xml");
    
    
}



nsresult
XULDocument::StartDocumentLoad(const char* aCommand, nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup,
                               nsISupports* aContainer,
                               nsIStreamListener **aDocListener,
                               bool aReset, nsIContentSink* aSink)
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_WARNING)) {

        nsCOMPtr<nsIURI> uri;
        nsresult rv = aChannel->GetOriginalURI(getter_AddRefs(uri));
        if (NS_SUCCEEDED(rv)) {
            nsAutoCString urlspec;
            rv = uri->GetSpec(urlspec);
            if (NS_SUCCEEDED(rv)) {
                PR_LOG(gXULLog, PR_LOG_WARNING,
                       ("xul: load document '%s'", urlspec.get()));
            }
        }
    }
#endif
    
    
    mStillWalking = true;
    mMayStartLayout = false;
    mDocumentLoadGroup = do_GetWeakReference(aLoadGroup);

    mChannel = aChannel;

    mHaveInputEncoding = true;

    
    nsresult rv =
        NS_GetFinalChannelURI(aChannel, getter_AddRefs(mDocumentURI));
    NS_ENSURE_SUCCESS(rv, rv);
    
    ResetStylesheetsToURI(mDocumentURI);

    RetrieveRelevantHeaders(aChannel);

    
    
    nsXULPrototypeDocument* proto = IsChromeURI(mDocumentURI) ?
            nsXULPrototypeCache::GetInstance()->GetPrototype(mDocumentURI) :
            nullptr;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (proto) {
        
        
        
        
        
        bool loaded;
        rv = proto->AwaitLoadDone(this, &loaded);
        if (NS_FAILED(rv)) return rv;

        mMasterPrototype = mCurrentPrototype = proto;

        
        SetPrincipal(proto->DocumentPrincipal());

        
        
        
        
        *aDocListener = new CachedChromeStreamListener(this, loaded);
        if (! *aDocListener)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    else {
        bool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();
        bool fillXULCache = (useXULCache && IsChromeURI(mDocumentURI));


        
        

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
XULDocument::EndLoad()
{
    
    if (!mCurrentPrototype)
        return;

    nsresult rv;

    
    

    nsCOMPtr<nsIURI> uri = mCurrentPrototype->GetURI();
    bool isChrome = IsChromeURI(uri);

    
    bool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();

    
    
    
    
    if (useXULCache && mIsWritingFastLoad && isChrome &&
        mMasterPrototype != mCurrentPrototype) {
        nsXULPrototypeCache::GetInstance()->WritePrototype(mCurrentPrototype);
    }

    if (IsOverlayAllowed(uri)) {
        nsCOMPtr<nsIXULOverlayProvider> reg =
            mozilla::services::GetXULOverlayProviderService();

        if (reg) {
            nsCOMPtr<nsISimpleEnumerator> overlays;
            rv = reg->GetStyleOverlays(uri, getter_AddRefs(overlays));
            if (NS_FAILED(rv)) return;

            bool moreSheets;
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

        if (isChrome && useXULCache) {
            
            
            
            rv = mCurrentPrototype->NotifyLoadDone();
            if (NS_FAILED(rv)) return;
        }
    }

    OnPrototypeLoadDone(true);
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_WARNING)) {
        nsAutoCString urlspec;
        rv = uri->GetSpec(urlspec);
        if (NS_SUCCEEDED(rv)) {
            PR_LOG(gXULLog, PR_LOG_WARNING,
                   ("xul: Finished loading document '%s'", urlspec.get()));
        }
    }
#endif
}

NS_IMETHODIMP
XULDocument::OnPrototypeLoadDone(bool aResumeWalk)
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


bool
XULDocument::OnDocumentParserError()
{
  
  if (mCurrentPrototype && mMasterPrototype != mCurrentPrototype) {
    nsCOMPtr<nsIURI> uri = mCurrentPrototype->GetURI();
    if (IsChromeURI(uri)) {
      nsCOMPtr<nsIObserverService> os =
        mozilla::services::GetObserverService();
      if (os)
        os->NotifyObservers(uri, "xul-overlay-parsererror",
                            EmptyString().get());
    }

    return false;
  }

  return true;
}

static void
ClearBroadcasterMapEntry(PLDHashTable* aTable, PLDHashEntryHdr* aEntry)
{
    BroadcasterMapEntry* entry =
        static_cast<BroadcasterMapEntry*>(aEntry);
    for (int32_t i = entry->mListeners.Count() - 1; i >= 0; --i) {
        delete (BroadcastListener*)entry->mListeners[i];
    }

    
    
    entry->mListeners.~nsSmallVoidArray();
}

static bool
CanBroadcast(int32_t aNameSpaceID, nsIAtom* aAttribute)
{
    
    
    if (aNameSpaceID == kNameSpaceID_None) {
        if ((aAttribute == nsGkAtoms::id) ||
            (aAttribute == nsGkAtoms::ref) ||
            (aAttribute == nsGkAtoms::persist) ||
            (aAttribute == nsGkAtoms::command) ||
            (aAttribute == nsGkAtoms::observes)) {
            return false;
        }
    }
    return true;
}

struct nsAttrNameInfo
{
  nsAttrNameInfo(int32_t aNamespaceID, nsIAtom* aName, nsIAtom* aPrefix) :
    mNamespaceID(aNamespaceID), mName(aName), mPrefix(aPrefix) {}
  nsAttrNameInfo(const nsAttrNameInfo& aOther) :
    mNamespaceID(aOther.mNamespaceID), mName(aOther.mName),
    mPrefix(aOther.mPrefix) {}
  int32_t           mNamespaceID;
  nsCOMPtr<nsIAtom> mName;
  nsCOMPtr<nsIAtom> mPrefix;
};

void
XULDocument::SynchronizeBroadcastListener(Element *aBroadcaster,
                                          Element *aListener,
                                          const nsAString &aAttr)
{
    if (!nsContentUtils::IsSafeToRunScript()) {
        nsDelayedBroadcastUpdate delayedUpdate(aBroadcaster, aListener,
                                               aAttr);
        mDelayedBroadcasters.AppendElement(delayedUpdate);
        MaybeBroadcast();
        return;
    }
    bool notify = mDocumentLoaded || mHandlingDelayedBroadcasters;

    if (aAttr.EqualsLiteral("*")) {
        uint32_t count = aBroadcaster->GetAttrCount();
        nsTArray<nsAttrNameInfo> attributes(count);
        for (uint32_t i = 0; i < count; ++i) {
            const nsAttrName* attrName = aBroadcaster->GetAttrNameAt(i);
            int32_t nameSpaceID = attrName->NamespaceID();
            nsIAtom* name = attrName->LocalName();

            
            if (! CanBroadcast(nameSpaceID, name))
                continue;

            attributes.AppendElement(nsAttrNameInfo(nameSpaceID, name,
                                                    attrName->GetPrefix()));
        }

        count = attributes.Length();
        while (count-- > 0) {
            int32_t nameSpaceID = attributes[count].mNamespaceID;
            nsIAtom* name = attributes[count].mName;
            nsAutoString value;
            if (aBroadcaster->GetAttr(nameSpaceID, name, value)) {
              aListener->SetAttr(nameSpaceID, name, attributes[count].mPrefix,
                                 value, notify);
            }

#if 0
            
            
            
            
            
            ExecuteOnBroadcastHandlerFor(aBroadcaster, aListener, name);
#endif
        }
    }
    else {
        
        nsCOMPtr<nsIAtom> name = do_GetAtom(aAttr);

        nsAutoString value;
        if (aBroadcaster->GetAttr(kNameSpaceID_None, name, value)) {
            aListener->SetAttr(kNameSpaceID_None, name, value, notify);
        } else {
            aListener->UnsetAttr(kNameSpaceID_None, name, notify);
        }

#if 0
        
        
        
        
        ExecuteOnBroadcastHandlerFor(aBroadcaster, aListener, name);
#endif
    }
}

NS_IMETHODIMP
XULDocument::AddBroadcastListenerFor(nsIDOMElement* aBroadcaster,
                                     nsIDOMElement* aListener,
                                     const nsAString& aAttr)
{
    ErrorResult rv;
    nsCOMPtr<Element> broadcaster = do_QueryInterface(aBroadcaster);
    nsCOMPtr<Element> listener = do_QueryInterface(aListener);
    NS_ENSURE_ARG(broadcaster && listener);
    AddBroadcastListenerFor(*broadcaster, *listener, aAttr, rv);
    return rv.ErrorCode();
}

void
XULDocument::AddBroadcastListenerFor(Element& aBroadcaster, Element& aListener,
                                     const nsAString& aAttr, ErrorResult& aRv)
{
    nsresult rv =
        nsContentUtils::CheckSameOrigin(this, &aBroadcaster);

    if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return;
    }

    rv = nsContentUtils::CheckSameOrigin(this, &aListener);

    if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return;
    }

    static const PLDHashTableOps gOps = {
        PL_DHashVoidPtrKeyStub,
        PL_DHashMatchEntryStub,
        PL_DHashMoveEntryStub,
        ClearBroadcasterMapEntry,
        nullptr
    };

    if (! mBroadcasterMap) {
        mBroadcasterMap = PL_NewDHashTable(&gOps, sizeof(BroadcasterMapEntry));

        if (! mBroadcasterMap) {
            aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
            return;
        }
    }

    BroadcasterMapEntry* entry =
        static_cast<BroadcasterMapEntry*>
                   (PL_DHashTableSearch(mBroadcasterMap, &aBroadcaster));

    if (!entry) {
        entry = static_cast<BroadcasterMapEntry*>
            (PL_DHashTableAdd(mBroadcasterMap, &aBroadcaster, fallible));

        if (! entry) {
            aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
            return;
        }

        entry->mBroadcaster = &aBroadcaster;

        
        
        new (&entry->mListeners) nsSmallVoidArray();
    }

    
    nsCOMPtr<nsIAtom> attr = do_GetAtom(aAttr);

    BroadcastListener* bl;
    for (int32_t i = entry->mListeners.Count() - 1; i >= 0; --i) {
        bl = static_cast<BroadcastListener*>(entry->mListeners[i]);

        nsCOMPtr<Element> blListener = do_QueryReferent(bl->mListener);

        if (blListener == &aListener && bl->mAttribute == attr)
            return;
    }

    bl = new BroadcastListener;

    bl->mListener  = do_GetWeakReference(&aListener);
    bl->mAttribute = attr;

    entry->mListeners.AppendElement(bl);

    SynchronizeBroadcastListener(&aBroadcaster, &aListener, aAttr);
}

NS_IMETHODIMP
XULDocument::RemoveBroadcastListenerFor(nsIDOMElement* aBroadcaster,
                                        nsIDOMElement* aListener,
                                        const nsAString& aAttr)
{
    nsCOMPtr<Element> broadcaster = do_QueryInterface(aBroadcaster);
    nsCOMPtr<Element> listener = do_QueryInterface(aListener);
    NS_ENSURE_ARG(broadcaster && listener);
    RemoveBroadcastListenerFor(*broadcaster, *listener, aAttr);
    return NS_OK;
}

void
XULDocument::RemoveBroadcastListenerFor(Element& aBroadcaster,
                                        Element& aListener,
                                        const nsAString& aAttr)
{
    
    
    if (! mBroadcasterMap)
        return;

    BroadcasterMapEntry* entry =
        static_cast<BroadcasterMapEntry*>
                   (PL_DHashTableSearch(mBroadcasterMap, &aBroadcaster));

    if (entry) {
        nsCOMPtr<nsIAtom> attr = do_GetAtom(aAttr);
        for (int32_t i = entry->mListeners.Count() - 1; i >= 0; --i) {
            BroadcastListener* bl =
                static_cast<BroadcastListener*>(entry->mListeners[i]);

            nsCOMPtr<Element> blListener = do_QueryReferent(bl->mListener);

            if (blListener == &aListener && bl->mAttribute == attr) {
                entry->mListeners.RemoveElementAt(i);
                delete bl;

                if (entry->mListeners.Count() == 0)
                    PL_DHashTableRemove(mBroadcasterMap, &aBroadcaster);

                break;
            }
        }
    }
}

nsresult
XULDocument::ExecuteOnBroadcastHandlerFor(Element* aBroadcaster,
                                          Element* aListener,
                                          nsIAtom* aAttr)
{
    
    
    

    for (nsIContent* child = aListener->GetFirstChild();
         child;
         child = child->GetNextSibling()) {

        
        
        
        
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

        
        
        WidgetEvent event(true, NS_XUL_BROADCAST);

        nsCOMPtr<nsIPresShell> shell = GetShell();
        if (shell) {
            nsRefPtr<nsPresContext> aPresContext = shell->GetPresContext();

            
            nsEventStatus status = nsEventStatus_eIgnore;
            EventDispatcher::Dispatch(child, aPresContext, &event, nullptr,
                                      &status);
        }
    }

    return NS_OK;
}

void
XULDocument::AttributeWillChange(nsIDocument* aDocument,
                                 Element* aElement, int32_t aNameSpaceID,
                                 nsIAtom* aAttribute, int32_t aModType)
{
    MOZ_ASSERT(aElement, "Null content!");
    NS_PRECONDITION(aAttribute, "Must have an attribute that's changing!");

    
    
    if (aAttribute == nsGkAtoms::ref) {
        
        nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);
        RemoveElementFromRefMap(aElement);
    }
}

void
XULDocument::AttributeChanged(nsIDocument* aDocument,
                              Element* aElement, int32_t aNameSpaceID,
                              nsIAtom* aAttribute, int32_t aModType)
{
    NS_ASSERTION(aDocument == this, "unexpected doc");

    
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);

    
    
    if (aAttribute == nsGkAtoms::ref) {
        AddElementToRefMap(aElement);
    }

    nsresult rv;

    
    if (mBroadcasterMap &&
        CanBroadcast(aNameSpaceID, aAttribute)) {
        BroadcasterMapEntry* entry =
            static_cast<BroadcasterMapEntry*>
                       (PL_DHashTableSearch(mBroadcasterMap, aElement));

        if (entry) {
            
            nsAutoString value;
            bool attrSet = aElement->GetAttr(kNameSpaceID_None, aAttribute, value);

            int32_t i;
            for (i = entry->mListeners.Count() - 1; i >= 0; --i) {
                BroadcastListener* bl =
                    static_cast<BroadcastListener*>(entry->mListeners[i]);

                if ((bl->mAttribute == aAttribute) ||
                    (bl->mAttribute == nsGkAtoms::_asterisk)) {
                    nsCOMPtr<Element> listenerEl
                        = do_QueryReferent(bl->mListener);
                    if (listenerEl) {
                        nsAutoString currentValue;
                        bool hasAttr = listenerEl->GetAttr(kNameSpaceID_None,
                                                           aAttribute,
                                                           currentValue);
                        
                        
                        
                        
                        bool needsAttrChange =
                            attrSet != hasAttr || !value.Equals(currentValue);
                        nsDelayedBroadcastUpdate delayedUpdate(aElement,
                                                               listenerEl,
                                                               aAttribute,
                                                               value,
                                                               attrSet,
                                                               needsAttrChange);

                        size_t index =
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

    
    bool listener, resolved;
    CheckBroadcasterHookup(aElement, &listener, &resolved);

    
    
    
    nsAutoString persist;
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::persist, persist);
    if (!persist.IsEmpty()) {
        
        if (persist.Find(nsDependentAtomString(aAttribute)) >= 0) {
            rv = Persist(aElement, kNameSpaceID_None, aAttribute);
            if (NS_FAILED(rv)) return;
        }
    }
}

void
XULDocument::ContentAppended(nsIDocument* aDocument,
                             nsIContent* aContainer,
                             nsIContent* aFirstNewContent,
                             int32_t aNewIndexInContainer)
{
    NS_ASSERTION(aDocument == this, "unexpected doc");
    
    
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);

    
    nsresult rv = NS_OK;
    for (nsIContent* cur = aFirstNewContent; cur && NS_SUCCEEDED(rv);
         cur = cur->GetNextSibling()) {
        rv = AddSubtreeToDocument(cur);
    }
}

void
XULDocument::ContentInserted(nsIDocument* aDocument,
                             nsIContent* aContainer,
                             nsIContent* aChild,
                             int32_t aIndexInContainer)
{
    NS_ASSERTION(aDocument == this, "unexpected doc");

    
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);

    AddSubtreeToDocument(aChild);
}

void
XULDocument::ContentRemoved(nsIDocument* aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            int32_t aIndexInContainer,
                            nsIContent* aPreviousSibling)
{
    NS_ASSERTION(aDocument == this, "unexpected doc");

    
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);

    RemoveSubtreeFromDocument(aChild);
}






void
XULDocument::GetElementsForID(const nsAString& aID,
                              nsCOMArray<nsIContent>& aElements)
{
    aElements.Clear();

    nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aID);
    if (entry) {
        entry->AppendAllIdContent(&aElements);
    }
    nsRefMapEntry *refEntry = mRefMap.GetEntry(aID);
    if (refEntry) {
        refEntry->AppendAll(&aElements);
    }
}

nsresult
XULDocument::AddForwardReference(nsForwardReference* aRef)
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
XULDocument::ResolveForwardReferences()
{
    if (mResolutionPhase == nsForwardReference::eDone)
        return NS_OK;

    NS_ASSERTION(mResolutionPhase == nsForwardReference::eStart,
                 "nested ResolveForwardReferences()");
        
    
    
    
    
    

    const nsForwardReference::Phase* pass = nsForwardReference::kPasses;
    while ((mResolutionPhase = *pass) != nsForwardReference::eDone) {
        uint32_t previous = 0;
        while (mForwardReferences.Length() &&
               mForwardReferences.Length() != previous) {
            previous = mForwardReferences.Length();

            for (uint32_t i = 0; i < mForwardReferences.Length(); ++i) {
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
XULDocument::GetElementsByAttribute(const nsAString& aAttribute,
                                    const nsAString& aValue,
                                    nsIDOMNodeList** aReturn)
{
    *aReturn = GetElementsByAttribute(aAttribute, aValue).take();
    return NS_OK;
}

already_AddRefed<nsINodeList>
XULDocument::GetElementsByAttribute(const nsAString& aAttribute,
                                    const nsAString& aValue)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));
    void* attrValue = new nsString(aValue);
    nsRefPtr<nsContentList> list = new nsContentList(this,
                                            MatchAttribute,
                                            nsContentUtils::DestroyMatchString,
                                            attrValue,
                                            true,
                                            attrAtom,
                                            kNameSpaceID_Unknown);
    
    return list.forget();
}

NS_IMETHODIMP
XULDocument::GetElementsByAttributeNS(const nsAString& aNamespaceURI,
                                      const nsAString& aAttribute,
                                      const nsAString& aValue,
                                      nsIDOMNodeList** aReturn)
{
    ErrorResult rv;
    *aReturn = GetElementsByAttributeNS(aNamespaceURI, aAttribute,
                                        aValue, rv).take();
    return rv.ErrorCode();
}

already_AddRefed<nsINodeList>
XULDocument::GetElementsByAttributeNS(const nsAString& aNamespaceURI,
                                      const nsAString& aAttribute,
                                      const nsAString& aValue,
                                      ErrorResult& aRv)
{
    nsCOMPtr<nsIAtom> attrAtom(do_GetAtom(aAttribute));
    void* attrValue = new nsString(aValue);

    int32_t nameSpaceId = kNameSpaceID_Wildcard;
    if (!aNamespaceURI.EqualsLiteral("*")) {
      nsresult rv =
        nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                              nameSpaceId);
      if (NS_FAILED(rv)) {
          aRv.Throw(rv);
          return nullptr;
      }
    }

    nsRefPtr<nsContentList> list = new nsContentList(this,
                                            MatchAttribute,
                                            nsContentUtils::DestroyMatchString,
                                            attrValue,
                                            true,
                                            attrAtom,
                                            nameSpaceId);
    return list.forget();
}

NS_IMETHODIMP
XULDocument::Persist(const nsAString& aID,
                     const nsAString& aAttr)
{
    
    
    if (mApplyingPersistedAttrs)
        return NS_OK;

    Element* element = nsDocument::GetElementById(aID);
    if (!element)
        return NS_OK;

    nsCOMPtr<nsIAtom> tag;
    int32_t nameSpaceID;

    nsRefPtr<mozilla::dom::NodeInfo> ni = element->GetExistingAttrNameFromQName(aAttr);
    nsresult rv;
    if (ni) {
        tag = ni->NameAtom();
        nameSpaceID = ni->NamespaceID();
    }
    else {
        
        const char16_t *colon;
        rv = nsContentUtils::CheckQName(PromiseFlatString(aAttr), true, &colon);

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

    return Persist(element, nameSpaceID, tag);
}

nsresult
XULDocument::Persist(nsIContent* aElement, int32_t aNameSpaceID,
                     nsIAtom* aAttribute)
{
    
    if (!nsContentUtils::IsSystemPrincipal(NodePrincipal()))
        return NS_ERROR_NOT_AVAILABLE;

    if (!mLocalStore) {
        mLocalStore = do_GetService("@mozilla.org/xul/xulstore;1");
        if (NS_WARN_IF(!mLocalStore)) {
            return NS_ERROR_NOT_INITIALIZED;
        }
    }

    nsAutoString id;

    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);
    nsAtomString attrstr(aAttribute);

    nsAutoString valuestr;
    aElement->GetAttr(kNameSpaceID_None, aAttribute, valuestr);

    nsAutoCString utf8uri;
    nsresult rv = mDocumentURI->GetSpec(utf8uri);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }
    NS_ConvertUTF8toUTF16 uri(utf8uri);

    bool hasAttr;
    rv = mLocalStore->HasValue(uri, id, attrstr, &hasAttr);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }

    if (hasAttr && valuestr.IsEmpty()) {
        return mLocalStore->RemoveValue(uri, id, attrstr);
    } else {
        return mLocalStore->SetValue(uri, id, attrstr, valuestr);
    }
}


nsresult
XULDocument::GetViewportSize(int32_t* aWidth,
                             int32_t* aHeight)
{
    *aWidth = *aHeight = 0;

    FlushPendingNotifications(Flush_Layout);

    nsIPresShell *shell = GetShell();
    NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

    nsIFrame* frame = shell->GetRootFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

    nsSize size = frame->GetSize();

    *aWidth = nsPresContext::AppUnitsToIntCSSPixels(size.width);
    *aHeight = nsPresContext::AppUnitsToIntCSSPixels(size.height);

    return NS_OK;
}

NS_IMETHODIMP
XULDocument::GetWidth(int32_t* aWidth)
{
    NS_ENSURE_ARG_POINTER(aWidth);

    int32_t height;
    return GetViewportSize(aWidth, &height);
}

int32_t
XULDocument::GetWidth(ErrorResult& aRv)
{
    int32_t width;
    aRv = GetWidth(&width);
    return width;
}

NS_IMETHODIMP
XULDocument::GetHeight(int32_t* aHeight)
{
    NS_ENSURE_ARG_POINTER(aHeight);

    int32_t width;
    return GetViewportSize(&width, aHeight);
}

int32_t
XULDocument::GetHeight(ErrorResult& aRv)
{
    int32_t height;
    aRv = GetHeight(&height);
    return height;
}

JSObject*
GetScopeObjectOfNode(nsIDOMNode* node)
{
    MOZ_ASSERT(node, "Must not be called with null.");

    
    
    
    
    
    
    
    
    nsCOMPtr<nsINode> inode = do_QueryInterface(node);
    MOZ_ASSERT(inode, "How can this happen?");

    nsIDocument* doc = inode->OwnerDoc();
    MOZ_ASSERT(inode, "This should never happen.");

    nsIGlobalObject* global = doc->GetScopeObject();
    return global ? global->GetGlobalJSObject() : nullptr;
}






NS_IMETHODIMP
XULDocument::GetPopupNode(nsIDOMNode** aNode)
{
    *aNode = nullptr;

    nsCOMPtr<nsIDOMNode> node;
    nsCOMPtr<nsPIWindowRoot> rootWin = GetWindowRoot();
    if (rootWin)
        node = rootWin->GetPopupNode(); 

    if (!node) {
        nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
        if (pm) {
            node = pm->GetLastTriggerPopupNode(this);
        }
    }

    if (node && nsContentUtils::CanCallerAccess(node)
        && GetScopeObjectOfNode(node)) {
        node.swap(*aNode);
    }

    return NS_OK;
}

already_AddRefed<nsINode>
XULDocument::GetPopupNode()
{
    nsCOMPtr<nsIDOMNode> node;
    DebugOnly<nsresult> rv = GetPopupNode(getter_AddRefs(node));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    nsCOMPtr<nsINode> retval(do_QueryInterface(node));
    return retval.forget();
}

NS_IMETHODIMP
XULDocument::SetPopupNode(nsIDOMNode* aNode)
{
    if (aNode) {
        
        nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
        NS_ENSURE_ARG(node);
    }

    nsCOMPtr<nsPIWindowRoot> rootWin = GetWindowRoot();
    if (rootWin)
        rootWin->SetPopupNode(aNode); 

    return NS_OK;
}

void
XULDocument::SetPopupNode(nsINode* aNode)
{
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aNode));
    DebugOnly<nsresult> rv = SetPopupNode(node);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
}



NS_IMETHODIMP
XULDocument::GetPopupRangeParent(nsIDOMNode** aRangeParent)
{
    NS_ENSURE_ARG_POINTER(aRangeParent);
    *aRangeParent = nullptr;

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (!pm)
        return NS_ERROR_FAILURE;

    int32_t offset;
    pm->GetMouseLocation(aRangeParent, &offset);

    if (*aRangeParent && !nsContentUtils::CanCallerAccess(*aRangeParent)) {
        NS_RELEASE(*aRangeParent);
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    return NS_OK;
}

already_AddRefed<nsINode>
XULDocument::GetPopupRangeParent(ErrorResult& aRv)
{
    nsCOMPtr<nsIDOMNode> node;
    aRv = GetPopupRangeParent(getter_AddRefs(node));
    nsCOMPtr<nsINode> retval(do_QueryInterface(node));
    return retval.forget();
}




NS_IMETHODIMP
XULDocument::GetPopupRangeOffset(int32_t* aRangeOffset)
{
    ErrorResult rv;
    *aRangeOffset = GetPopupRangeOffset(rv);
    return rv.ErrorCode();
}

int32_t
XULDocument::GetPopupRangeOffset(ErrorResult& aRv)
{
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (!pm) {
        aRv.Throw(NS_ERROR_FAILURE);
        return 0;
    }

    int32_t offset;
    nsCOMPtr<nsIDOMNode> parent;
    pm->GetMouseLocation(getter_AddRefs(parent), &offset);

    if (parent && !nsContentUtils::CanCallerAccess(parent)) {
        aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
        return 0;
    }
    return offset;
}

NS_IMETHODIMP
XULDocument::GetTooltipNode(nsIDOMNode** aNode)
{
    *aNode = nullptr;

    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
        nsCOMPtr<nsIDOMNode> node = pm->GetLastTriggerTooltipNode(this);
        if (node && nsContentUtils::CanCallerAccess(node))
            node.swap(*aNode);
    }

    return NS_OK;
}

already_AddRefed<nsINode>
XULDocument::GetTooltipNode()
{
    nsCOMPtr<nsIDOMNode> node;
    DebugOnly<nsresult> rv = GetTooltipNode(getter_AddRefs(node));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    nsCOMPtr<nsINode> retval(do_QueryInterface(node));
    return retval.forget();
}

NS_IMETHODIMP
XULDocument::SetTooltipNode(nsIDOMNode* aNode)
{
    
    return NS_OK;
}


NS_IMETHODIMP
XULDocument::GetCommandDispatcher(nsIDOMXULCommandDispatcher** aTracker)
{
    *aTracker = mCommandDispatcher;
    NS_IF_ADDREF(*aTracker);
    return NS_OK;
}

Element*
XULDocument::GetElementById(const nsAString& aId)
{
    if (!CheckGetElementByIdArg(aId))
        return nullptr;

    nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aId);
    if (entry) {
        Element* element = entry->GetIdElement();
        if (element)
            return element;
    }

    nsRefMapEntry* refEntry = mRefMap.GetEntry(aId);
    if (refEntry) {
        NS_ASSERTION(refEntry->GetFirstElement(),
                     "nsRefMapEntries should have nonempty content lists");
        return refEntry->GetFirstElement();
    }
    return nullptr;
}

nsresult
XULDocument::AddElementToDocumentPre(Element* aElement)
{
    
    
    nsresult rv;

    
    
    
    nsIAtom* id = aElement->GetID();
    if (id) {
        
        nsAutoScriptBlocker scriptBlocker;
        AddToIdTable(aElement, id);
    }
    rv = AddElementToRefMap(aElement);
    if (NS_FAILED(rv)) return rv;

    
    
    
    if (aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::commandupdater,
                              nsGkAtoms::_true, eCaseMatters)) {
        rv = nsXULContentUtils::SetCommandUpdater(this, aElement);
        if (NS_FAILED(rv)) return rv;
    }

    
    
    bool listener, resolved;
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
XULDocument::AddElementToDocumentPost(Element* aElement)
{
    
    if (aElement->NodeInfo()->Equals(nsGkAtoms::keyset, kNameSpaceID_XUL)) {
        
        nsXBLService::AttachGlobalKeyHandler(aElement);
    }

    
    bool needsHookup;
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
XULDocument::AddSubtreeToDocument(nsIContent* aContent)
{
    NS_ASSERTION(aContent->GetUncomposedDoc() == this, "Element not in doc!");
    
    if (!aContent->IsElement()) {
        return NS_OK;
    }

    Element* aElement = aContent->AsElement();

    
    nsresult rv = AddElementToDocumentPre(aElement);
    if (NS_FAILED(rv)) return rv;

    
    for (nsIContent* child = aElement->GetLastChild();
         child;
         child = child->GetPreviousSibling()) {

        rv = AddSubtreeToDocument(child);
        if (NS_FAILED(rv))
            return rv;
    }

    
    return AddElementToDocumentPost(aElement);
}

NS_IMETHODIMP
XULDocument::RemoveSubtreeFromDocument(nsIContent* aContent)
{
    
    if (!aContent->IsElement()) {
        return NS_OK;
    }

    Element* aElement = aContent->AsElement();

    
    
    nsresult rv;

    if (aElement->NodeInfo()->Equals(nsGkAtoms::keyset, kNameSpaceID_XUL)) {
        nsXBLService::DetachGlobalKeyHandler(aElement);
    }

    
    for (nsIContent* child = aElement->GetLastChild();
         child;
         child = child->GetPreviousSibling()) {

        rv = RemoveSubtreeFromDocument(child);
        if (NS_FAILED(rv))
            return rv;
    }

    
    
    
    RemoveElementFromRefMap(aElement);
    nsIAtom* id = aElement->GetID();
    if (id) {
        
        nsAutoScriptBlocker scriptBlocker;
        RemoveFromIdTable(aElement, id);
    }

    
    
    if (aElement->AttrValueIs(kNameSpaceID_None, nsGkAtoms::commandupdater,
                              nsGkAtoms::_true, eCaseMatters)) {
        nsCOMPtr<nsIDOMElement> domelement = do_QueryInterface(aElement);
        NS_ASSERTION(domelement != nullptr, "not a DOM element");
        if (! domelement)
            return NS_ERROR_UNEXPECTED;

        rv = mCommandDispatcher->RemoveCommandUpdater(domelement);
        if (NS_FAILED(rv)) return rv;
    }

    
    
    nsCOMPtr<Element> broadcaster, listener;
    nsAutoString attribute, broadcasterID;
    rv = FindBroadcaster(aElement, getter_AddRefs(listener),
                         broadcasterID, attribute, getter_AddRefs(broadcaster));
    if (rv == NS_FINDBROADCASTER_FOUND) {
        RemoveBroadcastListenerFor(*broadcaster, *listener, attribute);
    }

    return NS_OK;
}

NS_IMETHODIMP
XULDocument::SetTemplateBuilderFor(nsIContent* aContent,
                                   nsIXULTemplateBuilder* aBuilder)
{
    if (! mTemplateBuilderTable) {
        if (!aBuilder) {
            return NS_OK;
        }
        mTemplateBuilderTable = new BuilderTable;
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
XULDocument::GetTemplateBuilderFor(nsIContent* aContent,
                                   nsIXULTemplateBuilder** aResult)
{
    if (mTemplateBuilderTable) {
        mTemplateBuilderTable->Get(aContent, aResult);
    }
    else
        *aResult = nullptr;

    return NS_OK;
}

static void
GetRefMapAttribute(Element* aElement, nsAutoString* aValue)
{
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::ref, *aValue);
}

nsresult
XULDocument::AddElementToRefMap(Element* aElement)
{
    
    
    nsAutoString value;
    GetRefMapAttribute(aElement, &value);
    if (!value.IsEmpty()) {
        nsRefMapEntry *entry = mRefMap.PutEntry(value);
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;
        if (!entry->AddElement(aElement))
            return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

void
XULDocument::RemoveElementFromRefMap(Element* aElement)
{
    
    nsAutoString value;
    GetRefMapAttribute(aElement, &value);
    if (!value.IsEmpty()) {
        nsRefMapEntry *entry = mRefMap.GetEntry(value);
        if (!entry)
            return;
        if (entry->RemoveElement(aElement)) {
            mRefMap.RawRemoveEntry(entry);
        }
    }
}






nsresult
XULDocument::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
    
    *aResult = nullptr;
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}







nsresult
XULDocument::Init()
{
    nsresult rv = XMLDocument::Init();
    NS_ENSURE_SUCCESS(rv, rv);

    
    mCommandDispatcher = new nsXULCommandDispatcher(this);
    NS_ENSURE_TRUE(mCommandDispatcher, NS_ERROR_OUT_OF_MEMORY);

    if (gRefCnt++ == 0) {
        
        
        
        nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
        if (!cache) {
          NS_ERROR("Could not instantiate nsXULPrototypeCache");
          return NS_ERROR_FAILURE;
        }
    }

    Preferences::RegisterCallback(XULDocument::DirectionChanged,
                                  "intl.uidirection.", this);

#ifdef PR_LOGGING
    if (! gXULLog)
        gXULLog = PR_NewLogModule("XULDocument");
#endif

    return NS_OK;
}


nsresult
XULDocument::StartLayout(void)
{
    mMayStartLayout = true;
    nsCOMPtr<nsIPresShell> shell = GetShell();
    if (shell) {
        
        nsPresContext *cx = shell->GetPresContext();
        NS_ASSERTION(cx != nullptr, "no pres context");
        if (! cx)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIDocShell> docShell = cx->GetDocShell();
        NS_ASSERTION(docShell != nullptr, "container is not a docshell");
        if (! docShell)
            return NS_ERROR_UNEXPECTED;

        nsresult rv = NS_OK;
        nsRect r = cx->GetVisibleArea();
        rv = shell->Initialize(r.width, r.height);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}


bool
XULDocument::MatchAttribute(nsIContent* aContent,
                            int32_t aNamespaceID,
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

    

    uint32_t count = aContent->GetAttrCount();
    for (uint32_t i = 0; i < count; ++i) {
        const nsAttrName* name = aContent->GetAttrNameAt(i);
        bool nameMatch;
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

    return false;
}

nsresult
XULDocument::PrepareToLoad(nsISupports* aContainer,
                           const char* aCommand,
                           nsIChannel* aChannel,
                           nsILoadGroup* aLoadGroup,
                           nsIParser** aResult)
{
    
    nsCOMPtr<nsIPrincipal> principal;
    nsContentUtils::GetSecurityManager()->
        GetChannelResultPrincipal(aChannel, getter_AddRefs(principal));
    return PrepareToLoadPrototype(mDocumentURI, aCommand, principal, aResult);
}


nsresult
XULDocument::PrepareToLoadPrototype(nsIURI* aURI, const char* aCommand,
                                    nsIPrincipal* aDocumentPrincipal,
                                    nsIParser** aResult)
{
    nsresult rv;

    
    rv = NS_NewXULPrototypeDocument(getter_AddRefs(mCurrentPrototype));
    if (NS_FAILED(rv)) return rv;

    rv = mCurrentPrototype->InitPrincipal(aURI, aDocumentPrincipal);
    if (NS_FAILED(rv)) {
        mCurrentPrototype = nullptr;
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

    parser.forget(aResult);
    return NS_OK;
}


nsresult
XULDocument::ApplyPersistentAttributes()
{
    
    if (!nsContentUtils::IsSystemPrincipal(NodePrincipal()))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    if (!mLocalStore) {
        mLocalStore = do_GetService("@mozilla.org/xul/xulstore;1");
        if (NS_WARN_IF(!mLocalStore)) {
            return NS_ERROR_NOT_INITIALIZED;
        }
    }

    mApplyingPersistedAttrs = true;
    ApplyPersistentAttributesInternal();
    mApplyingPersistedAttrs = false;

    
    
    mRestrictPersistence = true;
    mPersistenceIds.Clear();

    return NS_OK;
}


nsresult
XULDocument::ApplyPersistentAttributesInternal()
{
    nsCOMArray<nsIContent> elements;

    nsAutoCString utf8uri;
    nsresult rv = mDocumentURI->GetSpec(utf8uri);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }
    NS_ConvertUTF8toUTF16 uri(utf8uri);

    
    nsCOMPtr<nsIStringEnumerator> ids;
    rv = mLocalStore->GetIDsEnumerator(uri, getter_AddRefs(ids));
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }

    while (1) {
        bool hasmore = false;
        ids->HasMore(&hasmore);
        if (!hasmore) {
            break;
        }

        nsAutoString id;
        ids->GetNext(id);

        if (mRestrictPersistence && !mPersistenceIds.Contains(id)) {
            continue;
        }

        
        GetElementsForID(id, elements);
        if (!elements.Count()) {
            continue;
        }

        rv = ApplyPersistentAttributesToElements(id, elements);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }
    }

    return NS_OK;
}


nsresult
XULDocument::ApplyPersistentAttributesToElements(const nsAString &aID,
                                                 nsCOMArray<nsIContent>& aElements)
{
    nsAutoCString utf8uri;
    nsresult rv = mDocumentURI->GetSpec(utf8uri);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }
    NS_ConvertUTF8toUTF16 uri(utf8uri);

    
    nsCOMPtr<nsIStringEnumerator> attrs;
    rv = mLocalStore->GetAttributeEnumerator(uri, aID, getter_AddRefs(attrs));
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }

    while (1) {
        bool hasmore = PR_FALSE;
        attrs->HasMore(&hasmore);
        if (!hasmore) {
            break;
        }

        nsAutoString attrstr;
        attrs->GetNext(attrstr);

        nsAutoString value;
        rv = mLocalStore->GetValue(uri, aID, attrstr, value);
        if (NS_WARN_IF(NS_FAILED(rv))) {
            return rv;
        }

        nsCOMPtr<nsIAtom> attr = do_GetAtom(attrstr);
        if (NS_WARN_IF(!attr)) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        uint32_t cnt = aElements.Count();

        for (int32_t i = int32_t(cnt) - 1; i >= 0; --i) {
            nsCOMPtr<nsIContent> element = aElements.SafeObjectAt(i);
            if (!element) {
                 continue;
            }

            rv = element->SetAttr(kNameSpaceID_None, attr, value, PR_TRUE);
        }
    }

    return NS_OK;
}

void
XULDocument::TraceProtos(JSTracer* aTrc, uint32_t aGCNumber)
{
    uint32_t i, count = mPrototypes.Length();
    for (i = 0; i < count; ++i) {
        mPrototypes[i]->TraceProtos(aTrc, aGCNumber);
    }
}






XULDocument::ContextStack::ContextStack()
    : mTop(nullptr), mDepth(0)
{
}

XULDocument::ContextStack::~ContextStack()
{
    while (mTop) {
        Entry* doomed = mTop;
        mTop = mTop->mNext;
        NS_IF_RELEASE(doomed->mElement);
        delete doomed;
    }
}

nsresult
XULDocument::ContextStack::Push(nsXULPrototypeElement* aPrototype,
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
XULDocument::ContextStack::Pop()
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
XULDocument::ContextStack::Peek(nsXULPrototypeElement** aPrototype,
                                nsIContent** aElement,
                                int32_t* aIndex)
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
XULDocument::ContextStack::SetTopIndex(int32_t aIndex)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    mTop->mIndex = aIndex;
    return NS_OK;
}







nsresult
XULDocument::PrepareToWalk()
{
    
    nsresult rv;

    
    
    mPrototypes.AppendElement(mCurrentPrototype);

    
    
    nsXULPrototypeElement* proto = mCurrentPrototype->GetRootElement();

    if (! proto) {
#ifdef PR_LOGGING
        if (PR_LOG_TEST(gXULLog, PR_LOG_ERROR)) {
            nsCOMPtr<nsIURI> url = mCurrentPrototype->GetURI();

            nsAutoCString urlspec;
            rv = url->GetSpec(urlspec);
            if (NS_FAILED(rv)) return rv;

            PR_LOG(gXULLog, PR_LOG_ERROR,
                   ("xul: error parsing '%s'", urlspec.get()));
        }
#endif

        return NS_OK;
    }

    uint32_t piInsertionPoint = 0;
    if (mState != eState_Master) {
        int32_t indexOfRoot = IndexOf(GetRootElement());
        NS_ASSERTION(indexOfRoot >= 0,
                     "No root content when preparing to walk overlay!");
        piInsertionPoint = indexOfRoot;
    }

    const nsTArray<nsRefPtr<nsXULPrototypePI> >& processingInstructions =
        mCurrentPrototype->GetProcessingInstructions();

    uint32_t total = processingInstructions.Length();
    for (uint32_t i = 0; i < total; ++i) {
        rv = CreateAndInsertPI(processingInstructions[i],
                               this, piInsertionPoint + i);
        if (NS_FAILED(rv)) return rv;
    }

    
    rv = AddChromeOverlays();
    if (NS_FAILED(rv)) return rv;

    
    
    nsRefPtr<Element> root;

    if (mState == eState_Master) {
        
        rv = CreateElementFromPrototype(proto, getter_AddRefs(root), true);
        if (NS_FAILED(rv)) return rv;

        rv = AppendChildTo(root, false);
        if (NS_FAILED(rv)) return rv;
        
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
XULDocument::CreateAndInsertPI(const nsXULPrototypePI* aProtoPI,
                               nsINode* aParent, uint32_t aIndex)
{
    NS_PRECONDITION(aProtoPI, "null ptr");
    NS_PRECONDITION(aParent, "null ptr");

    nsRefPtr<ProcessingInstruction> node =
        NS_NewXMLProcessingInstruction(mNodeInfoManager, aProtoPI->mTarget,
                                       aProtoPI->mData);

    nsresult rv;
    if (aProtoPI->mTarget.EqualsLiteral("xml-stylesheet")) {
        rv = InsertXMLStylesheetPI(aProtoPI, aParent, aIndex, node);
    } else if (aProtoPI->mTarget.EqualsLiteral("xul-overlay")) {
        rv = InsertXULOverlayPI(aProtoPI, aParent, aIndex, node);
    } else {
        
        rv = aParent->InsertChildAt(node, aIndex, false);
    }

    return rv;
}

nsresult
XULDocument::InsertXMLStylesheetPI(const nsXULPrototypePI* aProtoPI,
                                   nsINode* aParent,
                                   uint32_t aIndex,
                                   nsIContent* aPINode)
{
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(aPINode));
    NS_ASSERTION(ssle, "passed XML Stylesheet node does not "
                       "implement nsIStyleSheetLinkingElement!");

    nsresult rv;

    ssle->InitStyleLinkElement(false);
    
    
    ssle->SetEnableUpdates(false);
    ssle->OverrideBaseURI(mCurrentPrototype->GetURI());

    rv = aParent->InsertChildAt(aPINode, aIndex, false);
    if (NS_FAILED(rv)) return rv;

    ssle->SetEnableUpdates(true);

    
    
    bool willNotify;
    bool isAlternate;
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
XULDocument::InsertXULOverlayPI(const nsXULPrototypePI* aProtoPI,
                                nsINode* aParent,
                                uint32_t aIndex,
                                nsIContent* aPINode)
{
    nsresult rv;

    rv = aParent->InsertChildAt(aPINode, aIndex, false);
    if (NS_FAILED(rv)) return rv;

    
    if (!nsContentUtils::InProlog(aPINode)) {
        return NS_OK;
    }

    nsAutoString href;
    nsContentUtils::GetPseudoAttributeValue(aProtoPI->mData,
                                            nsGkAtoms::href,
                                            href);

    
    if (href.IsEmpty()) {
        return NS_OK;
    }

    
    nsCOMPtr<nsIURI> uri;

    rv = NS_NewURI(getter_AddRefs(uri), href, nullptr,
                   mCurrentPrototype->GetURI());
    if (NS_SUCCEEDED(rv)) {
        
        
        
        
        
        
        mUnloadedOverlays.InsertElementAt(0, uri);
        rv = NS_OK;
    } else if (rv == NS_ERROR_MALFORMED_URI) {
        
        
        rv = NS_OK;
    }

    return rv;
}

nsresult
XULDocument::AddChromeOverlays()
{
    nsresult rv;

    nsCOMPtr<nsIURI> docUri = mCurrentPrototype->GetURI();

    
    if (!IsOverlayAllowed(docUri)) return NS_OK;

    nsCOMPtr<nsIXULOverlayProvider> chromeReg =
        mozilla::services::GetXULOverlayProviderService();
    
    
    NS_ENSURE_TRUE(chromeReg, NS_OK);

    nsCOMPtr<nsISimpleEnumerator> overlays;
    rv = chromeReg->GetXULOverlays(docUri, getter_AddRefs(overlays));
    NS_ENSURE_SUCCESS(rv, rv);

    bool moreOverlays;
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

        
        mUnloadedOverlays.InsertElementAt(0, uri);
    }

    return rv;
}

NS_IMETHODIMP
XULDocument::LoadOverlay(const nsAString& aURL, nsIObserver* aObserver)
{
    nsresult rv;

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), aURL, nullptr);
    if (NS_FAILED(rv)) return rv;

    if (aObserver) {
        nsIObserver* obs = nullptr;
        if (!mOverlayLoadObservers) {
          mOverlayLoadObservers = new nsInterfaceHashtable<nsURIHashKey,nsIObserver>;
        }
        obs = mOverlayLoadObservers->GetWeak(uri);

        if (obs) {
            
            
            return NS_ERROR_FAILURE;
        }
        mOverlayLoadObservers->Put(uri, aObserver);
    }
    bool shouldReturn, failureFromContent;
    rv = LoadOverlayInternal(uri, true, &shouldReturn, &failureFromContent);
    if (NS_FAILED(rv) && mOverlayLoadObservers)
        mOverlayLoadObservers->Remove(uri); 
    return rv;
}

nsresult
XULDocument::LoadOverlayInternal(nsIURI* aURI, bool aIsDynamic,
                                 bool* aShouldReturn,
                                 bool* aFailureFromContent)
{
    nsresult rv;

    *aShouldReturn = false;
    *aFailureFromContent = false;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
        nsAutoCString urlspec;
        aURI->GetSpec(urlspec);
        nsAutoCString parentDoc;
        nsCOMPtr<nsIURI> uri;
        nsresult rv = mChannel->GetOriginalURI(getter_AddRefs(uri));
        if (NS_SUCCEEDED(rv))
            rv = uri->GetSpec(parentDoc);
        if (!(parentDoc.get()))
            parentDoc = "";

        PR_LOG(gXULLog, PR_LOG_DEBUG,
                ("xul: %s loading overlay %s", parentDoc.get(), urlspec.get()));
    }
#endif

    if (aIsDynamic)
        mResolutionPhase = nsForwardReference::eStart;

    
    
    

    bool documentIsChrome = IsChromeURI(mDocumentURI);
    if (!documentIsChrome) {
        
        rv = NodePrincipal()->CheckMayLoad(aURI, true, false);
        if (NS_FAILED(rv)) {
            *aFailureFromContent = true;
            return rv;
        }
    }

    
    
    
    
    bool overlayIsChrome = IsChromeURI(aURI);
    mCurrentPrototype = overlayIsChrome && documentIsChrome ?
        nsXULPrototypeCache::GetInstance()->GetPrototype(aURI) : nullptr;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    bool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();
    if (useXULCache && mCurrentPrototype) {
        bool loaded;
        rv = mCurrentPrototype->AwaitLoadDone(this, &loaded);
        if (NS_FAILED(rv)) return rv;

        if (! loaded) {
            
            
            
            
            *aShouldReturn = true;
            return NS_OK;
        }

        PR_LOG(gXULLog, PR_LOG_DEBUG, ("xul: overlay was cached"));

        
        
        
        return OnPrototypeLoadDone(aIsDynamic);
    }
    else {
        
        PR_LOG(gXULLog, PR_LOG_DEBUG, ("xul: overlay was not cached"));

        if (mIsGoingAway) {
            PR_LOG(gXULLog, PR_LOG_DEBUG, ("xul: ...and document already destroyed"));
            return NS_ERROR_NOT_AVAILABLE;
        }

        
        
        
        nsCOMPtr<nsIParser> parser;
        rv = PrepareToLoadPrototype(aURI, "view", nullptr, getter_AddRefs(parser));
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
        nsCOMPtr<nsIChannel> channel;
        
        
        
        
        rv = NS_NewChannel(getter_AddRefs(channel),
                           aURI,
                           NodePrincipal(),
                           nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                           nsIContentPolicy::TYPE_OTHER,
                           group);

        if (NS_SUCCEEDED(rv)) {
            rv = channel->AsyncOpen(listener, nullptr);
        }

        if (NS_FAILED(rv)) {
            
            mCurrentPrototype = nullptr;

            
            
            parser->Terminate();

            
            ReportMissingOverlay(aURI);

            
            *aFailureFromContent = true;
            return rv;
        }

        
        
        
        
        
        
        
        if (useXULCache && overlayIsChrome && documentIsChrome) {
            nsXULPrototypeCache::GetInstance()->PutPrototype(mCurrentPrototype);
        }

        
        
        
        
        if (!aIsDynamic)
            *aShouldReturn = true;
    }
    return NS_OK;
}

static PLDHashOperator
FirePendingMergeNotification(nsIURI* aKey, nsCOMPtr<nsIObserver>& aObserver, void* aClosure)
{
    aObserver->Observe(aKey, "xul-overlay-merged", EmptyString().get());

    typedef nsInterfaceHashtable<nsURIHashKey,nsIObserver> table;
    table* observers = static_cast<table*>(aClosure);
    if (observers) {
      observers->Remove(aKey);
    }

    return PL_DHASH_REMOVE;
}

nsresult
XULDocument::ResumeWalk()
{
    
    
    
    
    
    
    
    
    
    nsresult rv;
    nsCOMPtr<nsIURI> overlayURI =
        mCurrentPrototype ? mCurrentPrototype->GetURI() : nullptr;

    while (1) {
        

        while (mContextStack.Depth() > 0) {
            
            
            
            
            nsXULPrototypeElement* proto;
            nsCOMPtr<nsIContent> element;
            int32_t indx; 
                          
            rv = mContextStack.Peek(&proto, getter_AddRefs(element), &indx);
            if (NS_FAILED(rv)) return rv;

            if (indx >= (int32_t)proto->mChildren.Length()) {
                if (element) {
                    
                    
                    
                    
                    
                    if (mState == eState_Master) {
                        AddElementToDocumentPost(element->AsElement());

                        if (element->NodeInfo()->Equals(nsGkAtoms::style,
                                                        kNameSpaceID_XHTML) ||
                            element->NodeInfo()->Equals(nsGkAtoms::style,
                                                        kNameSpaceID_SVG)) {
                            
                            
                            nsCOMPtr<nsIStyleSheetLinkingElement> ssle =
                                do_QueryInterface(element);
                            NS_ASSERTION(ssle, "<html:style> doesn't implement "
                                               "nsIStyleSheetLinkingElement?");
                            bool willNotify;
                            bool isAlternate;
                            ssle->UpdateStyleSheet(nullptr, &willNotify,
                                                   &isAlternate);
                        }
                    }
                }
                
                
                mContextStack.Pop();
                continue;
            }

            
            
            nsXULPrototypeNode* childproto = proto->mChildren[indx];
            mContextStack.SetTopIndex(++indx);

            
            
            
            
            
            bool processingOverlayHookupNodes = (mState == eState_Overlay) && 
                                                  (mContextStack.Depth() == 1);

            NS_ASSERTION(element || processingOverlayHookupNodes,
                         "no element on context stack");

            switch (childproto->mType) {
            case nsXULPrototypeNode::eType_Element: {
                
                nsXULPrototypeElement* protoele =
                    static_cast<nsXULPrototypeElement*>(childproto);

                nsRefPtr<Element> child;

                if (!processingOverlayHookupNodes) {
                    rv = CreateElementFromPrototype(protoele,
                                                    getter_AddRefs(child),
                                                    false);
                    if (NS_FAILED(rv)) return rv;

                    
                    rv = element->AppendChildTo(child, false);
                    if (NS_FAILED(rv)) return rv;

                    
                    
                    if (mRestrictPersistence) {
                        nsIAtom* id = child->GetID();
                        if (id) {
                            mPersistenceIds.PutEntry(nsDependentAtomString(id));
                        }
                    }

                    
                    
                    
                    
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
                }
            }
            break;

            case nsXULPrototypeNode::eType_Script: {
                
                
                nsXULPrototypeScript* scriptproto =
                    static_cast<nsXULPrototypeScript*>(childproto);

                if (scriptproto->mSrcURI) {
                    
                    
                    
                    
                    bool blocked;
                    rv = LoadScript(scriptproto, &blocked);
                    

                    if (NS_SUCCEEDED(rv) && blocked)
                        return NS_OK;
                }
                else if (scriptproto->GetScriptObject()) {
                    
                    rv = ExecuteScript(scriptproto);
                    if (NS_FAILED(rv)) return rv;
                }
            }
            break;

            case nsXULPrototypeNode::eType_Text: {
                

                if (!processingOverlayHookupNodes) {
                    
                    

                    nsRefPtr<nsTextNode> text =
                        new nsTextNode(mNodeInfoManager);

                    nsXULPrototypeText* textproto =
                        static_cast<nsXULPrototypeText*>(childproto);
                    text->SetText(textproto->mValue, false);

                    rv = element->AppendChildTo(text, false);
                    NS_ENSURE_SUCCESS(rv, rv);
                }
            }
            break;

            case nsXULPrototypeNode::eType_PI: {
                nsXULPrototypePI* piProto =
                    static_cast<nsXULPrototypePI*>(childproto);

                
                

                if (piProto->mTarget.EqualsLiteral("xml-stylesheet") ||
                    piProto->mTarget.EqualsLiteral("xul-overlay")) {

                    const char16_t* params[] = { piProto->mTarget.get() };

                    nsContentUtils::ReportToConsole(
                                        nsIScriptError::warningFlag,
                                        NS_LITERAL_CSTRING("XUL Document"), nullptr,
                                        nsContentUtils::eXUL_PROPERTIES,
                                        "PINotInProlog",
                                        params, ArrayLength(params),
                                        overlayURI);
                }

                nsIContent* parent = processingOverlayHookupNodes ?
                    GetRootElement() : element.get();

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

        
        uint32_t count = mUnloadedOverlays.Length();
        if (! count)
            break;

        nsCOMPtr<nsIURI> uri = mUnloadedOverlays[count-1];
        mUnloadedOverlays.RemoveElementAt(count - 1);

        bool shouldReturn, failureFromContent;
        rv = LoadOverlayInternal(uri, false, &shouldReturn,
                                 &failureFromContent);
        if (failureFromContent)
            
            
            
            continue;
        if (NS_FAILED(rv))
            return rv;
        if (mOverlayLoadObservers) {
            nsIObserver *obs = mOverlayLoadObservers->GetWeak(overlayURI);
            if (obs) {
                
                
                
                
                if (!mOverlayLoadObservers->GetWeak(uri))
                    mOverlayLoadObservers->Put(uri, obs);
                mOverlayLoadObservers->Remove(overlayURI);
            }
        }
        if (shouldReturn)
            return NS_OK;
        overlayURI.swap(uri);
    }

    
    
    rv = ResolveForwardReferences();
    if (NS_FAILED(rv)) return rv;

    ApplyPersistentAttributes();

    mStillWalking = false;
    if (mPendingSheets == 0) {
        rv = DoneWalking();
    }
    return rv;
}

nsresult
XULDocument::DoneWalking()
{
    NS_PRECONDITION(mPendingSheets == 0, "there are sheets to be loaded");
    NS_PRECONDITION(!mStillWalking, "walk not done");

    
    

    uint32_t count = mOverlaySheets.Length();
    for (uint32_t i = 0; i < count; ++i) {
        AddStyleSheet(mOverlaySheets[i]);
    }
    mOverlaySheets.Clear();

    if (!mDocumentLoaded) {
        
        
        
        
        
        
        
        mDocumentLoaded = true;

        NotifyPossibleTitleChange(false);

        
        
        
        nsCOMPtr<nsIDocShellTreeItem> item = GetDocShell();
        if (item) {
            nsCOMPtr<nsIDocShellTreeOwner> owner;
            item->GetTreeOwner(getter_AddRefs(owner));
            nsCOMPtr<nsIXULWindow> xulWin = do_GetInterface(owner);
            if (xulWin) {
                nsCOMPtr<nsIDocShell> xulWinShell;
                xulWin->GetDocShell(getter_AddRefs(xulWinShell));
                if (SameCOMIdentity(xulWinShell, item)) {
                    
                    xulWin->ApplyChromeFlags();
                }
            }
        }

        StartLayout();

        if (mIsWritingFastLoad && IsChromeURI(mDocumentURI))
            nsXULPrototypeCache::GetInstance()->WritePrototype(mMasterPrototype);

        NS_ASSERTION(mDelayFrameLoaderInitialization,
                     "mDelayFrameLoaderInitialization should be true!");
        mDelayFrameLoaderInitialization = false;
        NS_WARN_IF_FALSE(mUpdateNestLevel == 0,
                         "Constructing XUL document in middle of an update?");
        if (mUpdateNestLevel == 0) {
            MaybeInitializeFinalizeFrameLoaders();
        }

        NS_DOCUMENT_NOTIFY_OBSERVERS(EndLoad, (this));

        
        
        DispatchContentLoadedEvents();

        mInitialLayoutComplete = true;

        
        
        if (mPendingOverlayLoadNotifications)
            mPendingOverlayLoadNotifications->Enumerate(
                FirePendingMergeNotification, mOverlayLoadObservers.get());
    }
    else {
        if (mOverlayLoadObservers) {
            nsCOMPtr<nsIURI> overlayURI = mCurrentPrototype->GetURI();
            nsCOMPtr<nsIObserver> obs;
            if (mInitialLayoutComplete) {
                
                mOverlayLoadObservers->Get(overlayURI, getter_AddRefs(obs));
                if (obs)
                    obs->Observe(overlayURI, "xul-overlay-merged", EmptyString().get());
                mOverlayLoadObservers->Remove(overlayURI);
            }
            else {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                

                if (!mPendingOverlayLoadNotifications) {
                    mPendingOverlayLoadNotifications =
                        new nsInterfaceHashtable<nsURIHashKey,nsIObserver>;
                }
                
                mPendingOverlayLoadNotifications->Get(overlayURI, getter_AddRefs(obs));
                if (!obs) {
                    mOverlayLoadObservers->Get(overlayURI, getter_AddRefs(obs));
                    NS_ASSERTION(obs, "null overlay load observer?");
                    mPendingOverlayLoadNotifications->Put(overlayURI, obs);
                }
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
XULDocument::StyleSheetLoaded(CSSStyleSheet* aSheet,
                              bool aWasAlternate,
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
XULDocument::MaybeBroadcast()
{
    
    if (mUpdateNestLevel == 0 &&
        (mDelayedAttrChangeBroadcasts.Length() ||
         mDelayedBroadcasters.Length())) {
        if (!nsContentUtils::IsSafeToRunScript()) {
            if (!mInDestructor) {
                nsContentUtils::AddScriptRunner(
                    NS_NewRunnableMethod(this, &XULDocument::MaybeBroadcast));
            }
            return;
        }
        if (!mHandlingDelayedAttrChange) {
            mHandlingDelayedAttrChange = true;
            for (uint32_t i = 0; i < mDelayedAttrChangeBroadcasts.Length(); ++i) {
                nsIAtom* attrName = mDelayedAttrChangeBroadcasts[i].mAttrName;
                if (mDelayedAttrChangeBroadcasts[i].mNeedsAttrChange) {
                    nsCOMPtr<nsIContent> listener =
                        do_QueryInterface(mDelayedAttrChangeBroadcasts[i].mListener);
                    nsString value = mDelayedAttrChangeBroadcasts[i].mAttr;
                    if (mDelayedAttrChangeBroadcasts[i].mSetAttr) {
                        listener->SetAttr(kNameSpaceID_None, attrName, value,
                                          true);
                    } else {
                        listener->UnsetAttr(kNameSpaceID_None, attrName,
                                            true);
                    }
                }
                ExecuteOnBroadcastHandlerFor(mDelayedAttrChangeBroadcasts[i].mBroadcaster,
                                             mDelayedAttrChangeBroadcasts[i].mListener,
                                             attrName);
            }
            mDelayedAttrChangeBroadcasts.Clear();
            mHandlingDelayedAttrChange = false;
        }

        uint32_t length = mDelayedBroadcasters.Length();
        if (length) {
            bool oldValue = mHandlingDelayedBroadcasters;
            mHandlingDelayedBroadcasters = true;
            nsTArray<nsDelayedBroadcastUpdate> delayedBroadcasters;
            mDelayedBroadcasters.SwapElements(delayedBroadcasters);
            for (uint32_t i = 0; i < length; ++i) {
                SynchronizeBroadcastListener(delayedBroadcasters[i].mBroadcaster,
                                             delayedBroadcasters[i].mListener,
                                             delayedBroadcasters[i].mAttr);
            }
            mHandlingDelayedBroadcasters = oldValue;
        }
    }
}

void
XULDocument::EndUpdate(nsUpdateType aUpdateType)
{
    XMLDocument::EndUpdate(aUpdateType);

    MaybeBroadcast();
}

void
XULDocument::ReportMissingOverlay(nsIURI* aURI)
{
    NS_PRECONDITION(aURI, "Must have a URI");
    
    nsAutoCString spec;
    aURI->GetSpec(spec);

    NS_ConvertUTF8toUTF16 utfSpec(spec);
    const char16_t* params[] = { utfSpec.get() };
    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                    NS_LITERAL_CSTRING("XUL Document"), this,
                                    nsContentUtils::eXUL_PROPERTIES,
                                    "MissingOverlay",
                                    params, ArrayLength(params));
}

nsresult
XULDocument::LoadScript(nsXULPrototypeScript* aScriptProto, bool* aBlock)
{
    
    nsresult rv;

    bool isChromeDoc = IsChromeURI(mDocumentURI);

    if (isChromeDoc && aScriptProto->GetScriptObject()) {
        rv = ExecuteScript(aScriptProto);

        
        *aBlock = false;
        return NS_OK;
    }

    
    
    
    bool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();

    if (isChromeDoc && useXULCache) {
        JSScript* newScriptObject =
            nsXULPrototypeCache::GetInstance()->GetScript(
                                   aScriptProto->mSrcURI);
        if (newScriptObject) {
            
            
            aScriptProto->Set(newScriptObject);
        }

        if (aScriptProto->GetScriptObject()) {
            rv = ExecuteScript(aScriptProto);

            
            *aBlock = false;
            return NS_OK;
        }
    }

    
    
    rv = nsScriptLoader::ShouldLoadScript(
                            this,
                            static_cast<nsIDocument*>(this),
                            aScriptProto->mSrcURI,
                            NS_LITERAL_STRING("application/x-javascript"));
    if (NS_FAILED(rv)) {
      *aBlock = false;
      return rv;
    }

    
    aScriptProto->UnlinkJSObjects();

    
    
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
        rv = NS_NewStreamLoader(getter_AddRefs(loader),
                                aScriptProto->mSrcURI,
                                this, 
                                this, 
                                nsILoadInfo::SEC_NORMAL,
                                nsIContentPolicy::TYPE_OTHER,
                                nullptr, 
                                group);

        if (NS_FAILED(rv)) {
            mCurrentScriptProto = nullptr;
            return rv;
        }

        aScriptProto->mSrcLoading = true;
    }

    
    *aBlock = true;
    return NS_OK;
}

NS_IMETHODIMP
XULDocument::OnStreamComplete(nsIStreamLoader* aLoader,
                              nsISupports* context,
                              nsresult aStatus,
                              uint32_t stringLen,
                              const uint8_t* string)
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
                nsAutoCString uriSpec;
                uri->GetSpec(uriSpec);
                printf("Failed to load %s\n", uriSpec.get());
            }
        }
    }
#endif

    
    
    
    
    nsresult rv = aStatus;

    NS_ASSERTION(mCurrentScriptProto && mCurrentScriptProto->mSrcLoading,
                 "script source not loading on unichar stream complete?");
    if (!mCurrentScriptProto) {
        
        return NS_OK;
    }

    if (NS_SUCCEEDED(aStatus)) {
        
        
        
        
        
        
        
        nsCOMPtr<nsIURI> uri = mCurrentScriptProto->mSrcURI;

        

        MOZ_ASSERT(!mOffThreadCompiling && (mOffThreadCompileStringLength == 0 &&
                                            !mOffThreadCompileStringBuf),
                   "XULDocument can't load multiple scripts at once");

        rv = nsScriptLoader::ConvertToUTF16(channel, string, stringLen,
                                            EmptyString(), this,
                                            mOffThreadCompileStringBuf,
                                            mOffThreadCompileStringLength);
        if (NS_SUCCEEDED(rv)) {
            
            
            
            
            JS::SourceBufferHolder srcBuf(mOffThreadCompileStringBuf,
                                          mOffThreadCompileStringLength,
                                          JS::SourceBufferHolder::GiveOwnership);
            mOffThreadCompileStringBuf = nullptr;
            mOffThreadCompileStringLength = 0;

            rv = mCurrentScriptProto->Compile(srcBuf, uri, 1, this, this);
            if (NS_SUCCEEDED(rv) && !mCurrentScriptProto->GetScriptObject()) {
                
                
                
                mOffThreadCompiling = true;
                
                
                mOffThreadCompileStringBuf = srcBuf.take();
                if (mOffThreadCompileStringBuf) {
                  mOffThreadCompileStringLength = srcBuf.length();
                }
                BlockOnload();
                return NS_OK;
            }
        }
    }

    return OnScriptCompileComplete(mCurrentScriptProto->GetScriptObject(), rv);
}

NS_IMETHODIMP
XULDocument::OnScriptCompileComplete(JSScript* aScript, nsresult aStatus)
{
    
    
    if (aScript && !mCurrentScriptProto->GetScriptObject())
        mCurrentScriptProto->Set(aScript);

    
    if (mOffThreadCompiling) {
        mOffThreadCompiling = false;
        UnblockOnload(false);
    }

    
    if (mOffThreadCompileStringBuf) {
      js_free(mOffThreadCompileStringBuf);
      mOffThreadCompileStringBuf = nullptr;
      mOffThreadCompileStringLength = 0;
    }

    
    
    
    nsXULPrototypeScript* scriptProto = mCurrentScriptProto;
    mCurrentScriptProto = nullptr;

    
    
    
    scriptProto->mSrcLoading = false;

    nsresult rv = aStatus;
    if (NS_SUCCEEDED(rv)) {
        rv = ExecuteScript(scriptProto);

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        bool useXULCache = nsXULPrototypeCache::GetInstance()->IsEnabled();
  
        if (useXULCache && IsChromeURI(mDocumentURI) && scriptProto->GetScriptObject()) {
            nsXULPrototypeCache::GetInstance()->PutScript(
                               scriptProto->mSrcURI,
                               scriptProto->GetScriptObject());
        }

        if (mIsWritingFastLoad && mCurrentPrototype != mMasterPrototype) {
            
            
            
            
            
            
            
            
            
            
            
            scriptProto->SerializeOutOfLine(nullptr, mCurrentPrototype);
        }
        
    }

    rv = ResumeWalk();

    
    
    XULDocument** docp = &scriptProto->mSrcLoadWaiters;

    
    
    XULDocument* doc;
    while ((doc = *docp) != nullptr) {
        NS_ASSERTION(doc->mCurrentScriptProto == scriptProto,
                     "waiting for wrong script to load?");
        doc->mCurrentScriptProto = nullptr;

        
        *docp = doc->mNextSrcLoadWaiter;
        doc->mNextSrcLoadWaiter = nullptr;

        
        if (NS_SUCCEEDED(aStatus) && scriptProto->GetScriptObject()) {
            doc->ExecuteScript(scriptProto);
        }
        doc->ResumeWalk();
        NS_RELEASE(doc);
    }

    return rv;
}

nsresult
XULDocument::ExecuteScript(nsXULPrototypeScript *aScript)
{
    NS_PRECONDITION(aScript != nullptr, "null ptr");
    NS_ENSURE_TRUE(aScript, NS_ERROR_NULL_POINTER);
    NS_ENSURE_TRUE(mScriptGlobalObject, NS_ERROR_NOT_INITIALIZED);

    nsresult rv;
    rv = mScriptGlobalObject->EnsureScriptEnvironment();
    NS_ENSURE_SUCCESS(rv, rv);

    JS::HandleScript scriptObject = aScript->GetScriptObject();
    NS_ENSURE_TRUE(scriptObject, NS_ERROR_UNEXPECTED);

    
    
    
    AutoEntryScript aes(mScriptGlobalObject);
    aes.TakeOwnershipOfErrorReporting();
    JSContext* cx = aes.cx();
    JS::Rooted<JSObject*> baseGlobal(cx, JS::CurrentGlobalOrNull(cx));
    NS_ENSURE_TRUE(nsContentUtils::GetSecurityManager()->ScriptAllowed(baseGlobal), NS_OK);

    JSAddonId* addonId = mCurrentPrototype ? MapURIToAddonID(mCurrentPrototype->GetURI()) : nullptr;
    JS::Rooted<JSObject*> global(cx, xpc::GetAddonScope(cx, baseGlobal, addonId));
    NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);

    JS::ExposeObjectToActiveJS(global);
    xpc_UnmarkGrayScript(scriptObject);
    JSAutoCompartment ac(cx, global);

    
    
    
    JS::CloneAndExecuteScript(cx, scriptObject);

    return NS_OK;
}


nsresult
XULDocument::CreateElementFromPrototype(nsXULPrototypeElement* aPrototype,
                                        Element** aResult,
                                        bool aIsRoot)
{
    
    NS_PRECONDITION(aPrototype != nullptr, "null ptr");
    if (! aPrototype)
        return NS_ERROR_NULL_POINTER;

    *aResult = nullptr;
    nsresult rv = NS_OK;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_NOTICE)) {
        PR_LOG(gXULLog, PR_LOG_NOTICE,
               ("xul: creating <%s> from prototype",
                NS_ConvertUTF16toUTF8(aPrototype->mNodeInfo->QualifiedName()).get()));
    }
#endif

    nsRefPtr<Element> result;

    if (aPrototype->mNodeInfo->NamespaceEquals(kNameSpaceID_XUL)) {
        
        
        rv = nsXULElement::Create(aPrototype, this, true, aIsRoot, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        
        
        
        
        nsRefPtr<mozilla::dom::NodeInfo> newNodeInfo;
        newNodeInfo = mNodeInfoManager->GetNodeInfo(aPrototype->mNodeInfo->NameAtom(),
                                                    aPrototype->mNodeInfo->GetPrefixAtom(),
                                                    aPrototype->mNodeInfo->NamespaceID(),
                                                    nsIDOMNode::ELEMENT_NODE);
        if (!newNodeInfo) return NS_ERROR_OUT_OF_MEMORY;
        nsRefPtr<mozilla::dom::NodeInfo> xtfNi = newNodeInfo;
        rv = NS_NewElement(getter_AddRefs(result), newNodeInfo.forget(),
                           NOT_FROM_PARSER);
        if (NS_FAILED(rv))
            return rv;

        rv = AddAttributes(aPrototype, result);
        if (NS_FAILED(rv)) return rv;
    }

    result.swap(*aResult);

    return NS_OK;
}

nsresult
XULDocument::CreateOverlayElement(nsXULPrototypeElement* aPrototype,
                                  Element** aResult)
{
    nsresult rv;

    nsRefPtr<Element> element;
    rv = CreateElementFromPrototype(aPrototype, getter_AddRefs(element), false);
    if (NS_FAILED(rv)) return rv;

    OverlayForwardReference* fwdref =
        new OverlayForwardReference(this, element);
    if (! fwdref)
        return NS_ERROR_OUT_OF_MEMORY;

    
    rv = AddForwardReference(fwdref);
    if (NS_FAILED(rv)) return rv;

    element.forget(aResult);
    return NS_OK;
}

nsresult
XULDocument::AddAttributes(nsXULPrototypeElement* aPrototype,
                           nsIContent* aElement)
{
    nsresult rv;

    for (uint32_t i = 0; i < aPrototype->mNumAttributes; ++i) {
        nsXULPrototypeAttribute* protoattr = &(aPrototype->mAttributes[i]);
        nsAutoString  valueStr;
        protoattr->mValue.ToString(valueStr);

        rv = aElement->SetAttr(protoattr->mName.NamespaceID(),
                               protoattr->mName.LocalName(),
                               protoattr->mName.GetPrefix(),
                               valueStr,
                               false);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
XULDocument::CheckTemplateBuilderHookup(nsIContent* aElement,
                                        bool* aNeedsHookup)
{
    
    
    
    
    
    
    
    nsCOMPtr<nsIDOMXULElement> xulElement = do_QueryInterface(aElement);
    if (xulElement) {
        nsCOMPtr<nsIRDFCompositeDataSource> ds;
        xulElement->GetDatabase(getter_AddRefs(ds));
        if (ds) {
            *aNeedsHookup = false;
            return NS_OK;
        }
    }

    
    
    *aNeedsHookup = aElement->HasAttr(kNameSpaceID_None,
                                      nsGkAtoms::datasources);
    return NS_OK;
}

 nsresult
XULDocument::CreateTemplateBuilder(nsIContent* aElement)
{
    
    bool isTreeBuilder = false;

    
    
    nsIDocument* document = aElement->GetUncomposedDoc();
    NS_ENSURE_TRUE(document, NS_OK);

    int32_t nameSpaceID;
    nsIAtom* baseTag = document->BindingManager()->
      ResolveTag(aElement, &nameSpaceID);

    if ((nameSpaceID == kNameSpaceID_XUL) && (baseTag == nsGkAtoms::tree)) {
        
        
        
        

        nsAutoString flags;
        aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::flags, flags);
        if (flags.Find(NS_LITERAL_STRING("dont-build-content")) >= 0) {
            isTreeBuilder = true;
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
            nsresult rv =
                document->CreateElem(nsDependentAtomString(nsGkAtoms::treechildren),
                                     nullptr, kNameSpaceID_XUL,
                                     getter_AddRefs(bodyContent));
            NS_ENSURE_SUCCESS(rv, rv);

            aElement->AppendChildTo(bodyContent, false);
        }
    }
    else {
        
        nsCOMPtr<nsIXULTemplateBuilder> builder
            = do_CreateInstance("@mozilla.org/xul/xul-template-builder;1");

        if (! builder)
            return NS_ERROR_FAILURE;

        builder->Init(aElement);
        builder->CreateContents(aElement, false);
    }

    return NS_OK;
}


nsresult
XULDocument::AddPrototypeSheets()
{
    nsresult rv;

    const nsCOMArray<nsIURI>& sheets = mCurrentPrototype->GetStyleSheetReferences();

    for (int32_t i = 0; i < sheets.Count(); i++) {
        nsCOMPtr<nsIURI> uri = sheets[i];

        nsRefPtr<CSSStyleSheet> incompleteSheet;
        rv = CSSLoader()->LoadSheet(uri,
                                    mCurrentPrototype->DocumentPrincipal(),
                                    EmptyCString(), this,
                                    getter_AddRefs(incompleteSheet));

        
        
        
        if (NS_SUCCEEDED(rv)) {
            ++mPendingSheets;
            if (!mOverlaySheets.AppendElement(incompleteSheet)) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
    }

    return NS_OK;
}







nsForwardReference::Result
XULDocument::OverlayForwardReference::Resolve()
{
    
    
    nsresult rv;
    nsCOMPtr<nsIContent> target;

    nsIPresShell *shell = mDocument->GetShell();
    bool notify = shell && shell->DidInitialize();

    nsAutoString id;
    mOverlay->GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);
    if (id.IsEmpty()) {
        
        
        Element* root = mDocument->GetRootElement();
        if (!root) {
            return eResolve_Error;
        }

        rv = mDocument->InsertElement(root, mOverlay, notify);
        if (NS_FAILED(rv)) return eResolve_Error;

        target = mOverlay;
    }
    else {
        
        
        target = mDocument->GetElementById(id);

        
        
        if (!target)
            return eResolve_Later;

        rv = Merge(target, mOverlay, notify);
        if (NS_FAILED(rv)) return eResolve_Error;
    }

    
    if (!notify && target->GetUncomposedDoc() == mDocument) {
        
        
        
        rv = mDocument->AddSubtreeToDocument(target);
        if (NS_FAILED(rv)) return eResolve_Error;
    }

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_NOTICE)) {
        nsAutoCString idC;
        idC.AssignWithConversion(id);
        PR_LOG(gXULLog, PR_LOG_NOTICE,
               ("xul: overlay resolved '%s'",
                idC.get()));
    }
#endif

    mResolved = true;
    return eResolve_Succeeded;
}



nsresult
XULDocument::OverlayForwardReference::Merge(nsIContent* aTargetNode,
                                            nsIContent* aOverlayNode,
                                            bool aNotify)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv;

    
    
    uint32_t i;
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

        int32_t nameSpaceID = name->NamespaceID();
        nsIAtom* attr = name->LocalName();
        nsIAtom* prefix = name->GetPrefix();

        nsAutoString value;
        aOverlayNode->GetAttr(nameSpaceID, attr, value);

        
        
        if (attr == nsGkAtoms::removeelement &&
            value.EqualsLiteral("true")) {

            nsCOMPtr<nsINode> parent = aTargetNode->GetParentNode();
            if (!parent) return NS_ERROR_FAILURE;
            rv = RemoveElement(parent, aTargetNode);
            if (NS_FAILED(rv)) return rv;

            return NS_OK;
        }

        rv = aTargetNode->SetAttr(nameSpaceID, attr, prefix, value, aNotify);
        if (!NS_FAILED(rv) && !aNotify)
            rv = mDocument->BroadcastAttributeChangeFromOverlay(aTargetNode,
                                                                nameSpaceID,
                                                                attr, prefix,
                                                                value);
        if (NS_FAILED(rv)) return rv;
    }


    
    
    
    
    
    

    uint32_t childCount = aOverlayNode->GetChildCount();

    
    
    nsCOMPtr<nsIContent> currContent;

    for (i = 0; i < childCount; ++i) {
        currContent = aOverlayNode->GetFirstChild();

        nsIAtom *idAtom = currContent->GetID();

        nsIContent *elementInDocument = nullptr;
        if (idAtom) {
            nsDependentAtomString id(idAtom);

            if (!id.IsEmpty()) {
                nsIDocument *doc = aTargetNode->GetUncomposedDoc();
                
                
                if (!doc) return NS_ERROR_FAILURE;

                elementInDocument = doc->GetElementById(id);
            }
        }

        
        
        
        
        
        if (elementInDocument) {
            
            
            
            

            nsIContent *elementParent = elementInDocument->GetParent();

            nsIAtom *parentID = elementParent->GetID();
            if (parentID &&
                aTargetNode->AttrValueIs(kNameSpaceID_None, nsGkAtoms::id,
                                         nsDependentAtomString(parentID),
                                         eCaseMatters)) {
                
                rv = Merge(elementInDocument, currContent, aNotify);
                if (NS_FAILED(rv)) return rv;
                aOverlayNode->RemoveChildAt(0, false);

                continue;
            }
        }

        aOverlayNode->RemoveChildAt(0, false);

        rv = InsertElement(aTargetNode, currContent, aNotify);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}



XULDocument::OverlayForwardReference::~OverlayForwardReference()
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_WARNING) && !mResolved) {
        nsAutoString id;
        mOverlay->GetAttr(kNameSpaceID_None, nsGkAtoms::id, id);

        nsAutoCString idC;
        idC.AssignWithConversion(id);

        nsIURI *protoURI = mDocument->mCurrentPrototype->GetURI();
        nsAutoCString urlspec;
        protoURI->GetSpec(urlspec);

        nsCOMPtr<nsIURI> docURI;
        nsAutoCString parentDoc;
        nsresult rv = mDocument->mChannel->GetOriginalURI(getter_AddRefs(docURI));
        if (NS_SUCCEEDED(rv))
            docURI->GetSpec(parentDoc);
        PR_LOG(gXULLog, PR_LOG_WARNING,
               ("xul: %s overlay failed to resolve '%s' in %s",
                urlspec.get(), idC.get(), parentDoc.get()));
    }
#endif
}







nsForwardReference::Result
XULDocument::BroadcasterHookup::Resolve()
{
    nsresult rv;

    bool listener;
    rv = mDocument->CheckBroadcasterHookup(mObservesElement, &listener, &mResolved);
    if (NS_FAILED(rv)) return eResolve_Error;

    return mResolved ? eResolve_Succeeded : eResolve_Later;
}


XULDocument::BroadcasterHookup::~BroadcasterHookup()
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_WARNING) && !mResolved) {
        

        nsAutoString broadcasterID;
        nsAutoString attribute;

        if (mObservesElement->IsXULElement(nsGkAtoms::observes)) {
            mObservesElement->GetAttr(kNameSpaceID_None, nsGkAtoms::element, broadcasterID);
            mObservesElement->GetAttr(kNameSpaceID_None, nsGkAtoms::attribute, attribute);
        }
        else {
            mObservesElement->GetAttr(kNameSpaceID_None, nsGkAtoms::observes, broadcasterID);
            attribute.Assign('*');
        }

        nsAutoCString attributeC,broadcasteridC;
        attributeC.AssignWithConversion(attribute);
        broadcasteridC.AssignWithConversion(broadcasterID);
        PR_LOG(gXULLog, PR_LOG_WARNING,
               ("xul: broadcaster hookup failed <%s attribute='%s'> to %s",
                nsAtomCString(mObservesElement->NodeInfo()->NameAtom()).get(),
                attributeC.get(),
                broadcasteridC.get()));
    }
#endif
}







nsForwardReference::Result
XULDocument::TemplateBuilderHookup::Resolve()
{
    bool needsHookup;
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
XULDocument::BroadcastAttributeChangeFromOverlay(nsIContent* aNode,
                                                 int32_t aNameSpaceID,
                                                 nsIAtom* aAttribute,
                                                 nsIAtom* aPrefix,
                                                 const nsAString& aValue)
{
    nsresult rv = NS_OK;

    if (!mBroadcasterMap || !CanBroadcast(aNameSpaceID, aAttribute))
        return rv;

    if (!aNode->IsElement())
        return rv;

    BroadcasterMapEntry* entry = static_cast<BroadcasterMapEntry*>
        (PL_DHashTableSearch(mBroadcasterMap, aNode->AsElement()));
    if (!entry)
        return rv;

    
    int32_t i;
    for (i = entry->mListeners.Count() - 1; i >= 0; --i) {
        BroadcastListener* bl = static_cast<BroadcastListener*>
            (entry->mListeners[i]);

        if ((bl->mAttribute != aAttribute) &&
            (bl->mAttribute != nsGkAtoms::_asterisk))
            continue;

        nsCOMPtr<nsIContent> l = do_QueryReferent(bl->mListener);
        if (l) {
            rv = l->SetAttr(aNameSpaceID, aAttribute,
                            aPrefix, aValue, false);
            if (NS_FAILED(rv)) return rv;
        }
    }
    return rv;
}

nsresult
XULDocument::FindBroadcaster(Element* aElement,
                             Element** aListener,
                             nsString& aBroadcasterID,
                             nsString& aAttribute,
                             Element** aBroadcaster)
{
    mozilla::dom::NodeInfo *ni = aElement->NodeInfo();
    *aListener = nullptr;
    *aBroadcaster = nullptr;

    if (ni->Equals(nsGkAtoms::observes, kNameSpaceID_XUL)) {
        
        
        
        
        
        nsIContent* parent = aElement->GetParent();
        if (!parent) {
             
            return NS_FINDBROADCASTER_NOT_FOUND;
        }

        
        
        if (parent->NodeInfo()->Equals(nsGkAtoms::overlay,
                                       kNameSpaceID_XUL)) {
            return NS_FINDBROADCASTER_AWAIT_OVERLAYS;
        }

        *aListener = parent->IsElement() ? parent->AsElement() : nullptr;
        NS_IF_ADDREF(*aListener);

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

        *aListener = aElement;
        NS_ADDREF(*aListener);

        aAttribute.Assign('*');
    }

    
    NS_ENSURE_TRUE(*aListener, NS_ERROR_UNEXPECTED);

    
    *aBroadcaster = GetElementById(aBroadcasterID);

    
    
    
    if (! *aBroadcaster) {
        return NS_FINDBROADCASTER_AWAIT_OVERLAYS;
    }

    NS_ADDREF(*aBroadcaster);

    return NS_FINDBROADCASTER_FOUND;
}

nsresult
XULDocument::CheckBroadcasterHookup(Element* aElement,
                                    bool* aNeedsHookup,
                                    bool* aDidResolve)
{
    
    
    
    nsresult rv;

    *aDidResolve = false;

    nsCOMPtr<Element> listener;
    nsAutoString broadcasterID;
    nsAutoString attribute;
    nsCOMPtr<Element> broadcaster;

    rv = FindBroadcaster(aElement, getter_AddRefs(listener),
                         broadcasterID, attribute, getter_AddRefs(broadcaster));
    switch (rv) {
        case NS_FINDBROADCASTER_NOT_FOUND:
            *aNeedsHookup = false;
            return NS_OK;
        case NS_FINDBROADCASTER_AWAIT_OVERLAYS:
            *aNeedsHookup = true;
            return NS_OK;
        case NS_FINDBROADCASTER_FOUND:
            break;
        default:
            return rv;
    }

    NS_ENSURE_ARG(broadcaster && listener);
    ErrorResult domRv;
    AddBroadcastListenerFor(*broadcaster, *listener, attribute, domRv);
    if (domRv.Failed()) {
        return domRv.ErrorCode();
    }

#ifdef PR_LOGGING
    
    if (PR_LOG_TEST(gXULLog, PR_LOG_NOTICE)) {
        nsCOMPtr<nsIContent> content =
            do_QueryInterface(listener);

        NS_ASSERTION(content != nullptr, "not an nsIContent");
        if (! content)
            return rv;

        nsAutoCString attributeC,broadcasteridC;
        attributeC.AssignWithConversion(attribute);
        broadcasteridC.AssignWithConversion(broadcasterID);
        PR_LOG(gXULLog, PR_LOG_NOTICE,
               ("xul: broadcaster hookup <%s attribute='%s'> to %s",
                nsAtomCString(content->NodeInfo()->NameAtom()).get(),
                attributeC.get(),
                broadcasteridC.get()));
    }
#endif

    *aNeedsHookup = false;
    *aDidResolve = true;
    return NS_OK;
}

nsresult
XULDocument::InsertElement(nsINode* aParent, nsIContent* aChild,
                           bool aNotify)
{
    
    

    nsAutoString posStr;
    bool wasInserted = false;

    
    aChild->GetAttr(kNameSpaceID_None, nsGkAtoms::insertafter, posStr);
    bool isInsertAfter = true;

    if (posStr.IsEmpty()) {
        aChild->GetAttr(kNameSpaceID_None, nsGkAtoms::insertbefore, posStr);
        isInsertAfter = false;
    }

    if (!posStr.IsEmpty()) {
        nsIDocument *document = aParent->OwnerDoc();

        nsIContent *content = nullptr;

        char* str = ToNewCString(posStr);
        char* rest;
        char* token = nsCRT::strtok(str, ", ", &rest);

        while (token) {
            content = document->GetElementById(NS_ConvertASCIItoUTF16(token));
            if (content)
                break;

            token = nsCRT::strtok(rest, ", ", &rest);
        }
        free(str);

        if (content) {
            int32_t pos = aParent->IndexOf(content);

            if (pos != -1) {
                pos = isInsertAfter ? pos + 1 : pos;
                nsresult rv = aParent->InsertChildAt(aChild, pos, aNotify);
                if (NS_FAILED(rv))
                    return rv;

                wasInserted = true;
            }
        }
    }

    if (!wasInserted) {

        aChild->GetAttr(kNameSpaceID_None, nsGkAtoms::position, posStr);
        if (!posStr.IsEmpty()) {
            nsresult rv;
            
            int32_t pos = posStr.ToInteger(&rv);
            
            
            
            
            if (NS_SUCCEEDED(rv) && pos > 0 &&
                uint32_t(pos - 1) <= aParent->GetChildCount()) {
                rv = aParent->InsertChildAt(aChild, pos - 1, aNotify);
                if (NS_SUCCEEDED(rv))
                    wasInserted = true;
                
                
                
                
            }
        }
    }

    if (!wasInserted) {
        return aParent->AppendChildTo(aChild, aNotify);
    }
    return NS_OK;
}

nsresult
XULDocument::RemoveElement(nsINode* aParent, nsINode* aChild)
{
    int32_t nodeOffset = aParent->IndexOf(aChild);

    aParent->RemoveChildAt(nodeOffset, true);
    return NS_OK;
}






XULDocument::CachedChromeStreamListener::CachedChromeStreamListener(XULDocument* aDocument, bool aProtoLoaded)
    : mDocument(aDocument),
      mProtoLoaded(aProtoLoaded)
{
    NS_ADDREF(mDocument);
}


XULDocument::CachedChromeStreamListener::~CachedChromeStreamListener()
{
    NS_RELEASE(mDocument);
}


NS_IMPL_ISUPPORTS(XULDocument::CachedChromeStreamListener,
                  nsIRequestObserver, nsIStreamListener)

NS_IMETHODIMP
XULDocument::CachedChromeStreamListener::OnStartRequest(nsIRequest *request,
                                                        nsISupports* acontext)
{
    return NS_ERROR_PARSED_DATA_CACHED;
}


NS_IMETHODIMP
XULDocument::CachedChromeStreamListener::OnStopRequest(nsIRequest *request,
                                                       nsISupports* aContext,
                                                       nsresult aStatus)
{
    if (! mProtoLoaded)
        return NS_OK;

    return mDocument->OnPrototypeLoadDone(true);
}


NS_IMETHODIMP
XULDocument::CachedChromeStreamListener::OnDataAvailable(nsIRequest *request,
                                                         nsISupports* aContext,
                                                         nsIInputStream* aInStr,
                                                         uint64_t aSourceOffset,
                                                         uint32_t aCount)
{
    NS_NOTREACHED("CachedChromeStream doesn't receive data");
    return NS_ERROR_UNEXPECTED;
}






XULDocument::ParserObserver::ParserObserver(XULDocument* aDocument,
                                            nsXULPrototypeDocument* aPrototype)
    : mDocument(aDocument), mPrototype(aPrototype)
{
}

XULDocument::ParserObserver::~ParserObserver()
{
}

NS_IMPL_ISUPPORTS(XULDocument::ParserObserver, nsIRequestObserver)

NS_IMETHODIMP
XULDocument::ParserObserver::OnStartRequest(nsIRequest *request,
                                            nsISupports* aContext)
{
    
    if (mPrototype) {
        nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
        nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
        if (channel && secMan) {
            nsCOMPtr<nsIPrincipal> principal;
            secMan->GetChannelResultPrincipal(channel, getter_AddRefs(principal));

            
            mPrototype->SetDocumentPrincipal(principal);
        }

        
        mPrototype = nullptr;
    }
        
    return NS_OK;
}

NS_IMETHODIMP
XULDocument::ParserObserver::OnStopRequest(nsIRequest *request,
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

    
    
    
    mDocument = nullptr;

    return rv;
}

already_AddRefed<nsPIWindowRoot>
XULDocument::GetWindowRoot()
{
  if (!mDocumentContainer) {
    return nullptr;
  }

    nsCOMPtr<nsPIDOMWindow> piWin = mDocumentContainer->GetWindow();
    return piWin ? piWin->GetTopWindowRoot() : nullptr;
}

bool
XULDocument::IsDocumentRightToLeft()
{
    
    
    Element* element = GetRootElement();
    if (element) {
        static nsIContent::AttrValuesArray strings[] =
            {&nsGkAtoms::ltr, &nsGkAtoms::rtl, nullptr};
        switch (element->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::localedir,
                                         strings, eCaseMatters)) {
            case 0: return false;
            case 1: return true;
            default: break; 
        }
    }

    
    
    nsCOMPtr<nsIXULChromeRegistry> reg =
        mozilla::services::GetXULChromeRegistryService();
    if (!reg)
        return false;

    nsAutoCString package;
    bool isChrome;
    if (NS_SUCCEEDED(mDocumentURI->SchemeIs("chrome", &isChrome)) &&
        isChrome) {
        mDocumentURI->GetHostPort(package);
    }
    else {
        
        
        bool isAbout, isResource;
        if (NS_SUCCEEDED(mDocumentURI->SchemeIs("about", &isAbout)) &&
            isAbout) {
            package.AssignLiteral("global");
        }
        else if (NS_SUCCEEDED(mDocumentURI->SchemeIs("resource", &isResource)) &&
            isResource) {
            package.AssignLiteral("global");
        }
        else {
            return false;
        }
    }

    bool isRTL = false;
    reg->IsLocaleRTL(package, &isRTL);
    return isRTL;
}

void
XULDocument::ResetDocumentDirection()
{
    DocumentStatesChanged(NS_DOCUMENT_STATE_RTL_LOCALE);
}

void
XULDocument::DirectionChanged(const char* aPrefName, void* aData)
{
  
  XULDocument* doc = (XULDocument *)aData;
  if (doc) {
      doc->ResetDocumentDirection();
  }
}

int
XULDocument::GetDocumentLWTheme()
{
    if (mDocLWTheme == Doc_Theme_Uninitialized) {
        mDocLWTheme = Doc_Theme_None; 

        Element* element = GetRootElement();
        nsAutoString hasLWTheme;
        if (element &&
            element->GetAttr(kNameSpaceID_None, nsGkAtoms::lwtheme, hasLWTheme) &&
            !(hasLWTheme.IsEmpty()) &&
            hasLWTheme.EqualsLiteral("true")) {
            mDocLWTheme = Doc_Theme_Neutral;
            nsAutoString lwTheme;
            element->GetAttr(kNameSpaceID_None, nsGkAtoms::lwthemetextcolor, lwTheme);
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
XULDocument::GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult)
{
    ErrorResult rv;
    nsCOMPtr<Element> el = do_QueryInterface(aElement);
    *aResult = GetBoxObjectFor(el, rv).take();
    return rv.ErrorCode();
}

JSObject*
XULDocument::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return XULDocumentBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
