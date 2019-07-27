


















#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsBindingManager.h"
#include "nsIDOMNodeList.h"
#include "nsIObserverService.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFInferDataSource.h"
#include "nsIRDFContainerUtils.h"
#include "nsIXULDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIXULBuilderListener.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIMutableArray.h"
#include "nsIURL.h"
#include "nsIXPConnect.h"
#include "nsContentCID.h"
#include "nsNameSpaceManager.h"
#include "nsRDFCID.h"
#include "nsXULContentUtils.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsXPIDLString.h"
#include "nsWhitespaceTokenizer.h"
#include "nsGkAtoms.h"
#include "nsXULElement.h"
#include "jsapi.h"
#include "prlog.h"
#include "rdf.h"
#include "pldhash.h"
#include "plhash.h"
#include "nsDOMClassInfoID.h"
#include "nsPIDOMWindow.h"
#include "nsIConsoleService.h" 
#include "nsNetUtil.h"
#include "nsXULTemplateBuilder.h"
#include "nsXULTemplateQueryProcessorRDF.h"
#include "nsXULTemplateQueryProcessorXML.h"
#include "nsXULTemplateQueryProcessorStorage.h"
#include "nsContentUtils.h"
#include "ChildIterator.h"
#include "mozilla/dom/ScriptSettings.h"

using namespace mozilla::dom;
using namespace mozilla;






nsrefcnt                  nsXULTemplateBuilder::gRefCnt = 0;
nsIRDFService*            nsXULTemplateBuilder::gRDFService;
nsIRDFContainerUtils*     nsXULTemplateBuilder::gRDFContainerUtils;
nsIScriptSecurityManager* nsXULTemplateBuilder::gScriptSecurityManager;
nsIPrincipal*             nsXULTemplateBuilder::gSystemPrincipal;
nsIObserverService*       nsXULTemplateBuilder::gObserverService;

#ifdef PR_LOGGING
PRLogModuleInfo* gXULTemplateLog;
#endif

#define NS_QUERY_PROCESSOR_CONTRACTID_PREFIX "@mozilla.org/xul/xul-query-processor;1?name="






nsXULTemplateBuilder::nsXULTemplateBuilder(void)
    : mQueriesCompiled(false),
      mFlags(0),
      mTop(nullptr),
      mObservedDocument(nullptr)
{
    MOZ_COUNT_CTOR(nsXULTemplateBuilder);
}

static PLDHashOperator
DestroyMatchList(nsISupports* aKey, nsTemplateMatch*& aMatch, void* aContext)
{
    
    while (aMatch) {
        nsTemplateMatch* next = aMatch->mNext;
        nsTemplateMatch::Destroy(aMatch, true);
        aMatch = next;
    }

    return PL_DHASH_REMOVE;
}

nsXULTemplateBuilder::~nsXULTemplateBuilder(void)
{
    Uninit(true);

    if (--gRefCnt == 0) {
        NS_IF_RELEASE(gRDFService);
        NS_IF_RELEASE(gRDFContainerUtils);
        NS_IF_RELEASE(gSystemPrincipal);
        NS_IF_RELEASE(gScriptSecurityManager);
        NS_IF_RELEASE(gObserverService);
    }

    MOZ_COUNT_DTOR(nsXULTemplateBuilder);
}


nsresult
nsXULTemplateBuilder::InitGlobals()
{
    nsresult rv;

    if (gRefCnt++ == 0) {
        
        
        NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
        rv = CallGetService(kRDFServiceCID, &gRDFService);
        if (NS_FAILED(rv))
            return rv;

        NS_DEFINE_CID(kRDFContainerUtilsCID, NS_RDFCONTAINERUTILS_CID);
        rv = CallGetService(kRDFContainerUtilsCID, &gRDFContainerUtils);
        if (NS_FAILED(rv))
            return rv;

        rv = CallGetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID,
                            &gScriptSecurityManager);
        if (NS_FAILED(rv))
            return rv;

        rv = gScriptSecurityManager->GetSystemPrincipal(&gSystemPrincipal);
        if (NS_FAILED(rv))
            return rv;

        rv = CallGetService(NS_OBSERVERSERVICE_CONTRACTID, &gObserverService);
        if (NS_FAILED(rv))
            return rv;
    }

#ifdef PR_LOGGING
    if (! gXULTemplateLog)
        gXULTemplateLog = PR_NewLogModule("nsXULTemplateBuilder");
#endif

    return NS_OK;
}

void
nsXULTemplateBuilder::StartObserving(nsIDocument* aDocument)
{
    aDocument->AddObserver(this);
    mObservedDocument = aDocument;
    gObserverService->AddObserver(this, DOM_WINDOW_DESTROYED_TOPIC, false);
}

void
nsXULTemplateBuilder::StopObserving()
{
    MOZ_ASSERT(mObservedDocument);
    mObservedDocument->RemoveObserver(this);
    mObservedDocument = nullptr;
    gObserverService->RemoveObserver(this, DOM_WINDOW_DESTROYED_TOPIC);
}

void
nsXULTemplateBuilder::CleanUp(bool aIsFinal)
{
    for (int32_t q = mQuerySets.Length() - 1; q >= 0; q--) {
        nsTemplateQuerySet* qs = mQuerySets[q];
        delete qs;
    }

    mQuerySets.Clear();

    mMatchMap.Enumerate(DestroyMatchList, nullptr);

    
    
    if (aIsFinal)
        mQueryProcessor = nullptr;
}

void
nsXULTemplateBuilder::Uninit(bool aIsFinal)
{
    if (mObservedDocument && aIsFinal) {
        StopObserving();
    }

    if (mQueryProcessor)
        mQueryProcessor->Done();

    CleanUp(aIsFinal);

    mRootResult = nullptr;
    mRefVariable = nullptr;
    mMemberVariable = nullptr;

    mQueriesCompiled = false;
}

static PLDHashOperator
TraverseMatchList(nsISupports* aKey, nsTemplateMatch* aMatch, void* aContext)
{
    nsCycleCollectionTraversalCallback *cb =
        static_cast<nsCycleCollectionTraversalCallback*>(aContext);

    cb->NoteXPCOMChild(aKey);
    nsTemplateMatch* match = aMatch;
    while (match) {
        cb->NoteXPCOMChild(match->GetContainer());
        cb->NoteXPCOMChild(match->mResult);
        match = match->mNext;
    }

    return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULTemplateBuilder)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXULTemplateBuilder)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mDataSource)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mDB)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mCompDB)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mRoot)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mRootResult)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mListeners)
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mQueryProcessor)
    tmp->mMatchMap.Enumerate(DestroyMatchList, nullptr);
    for (uint32_t i = 0; i < tmp->mQuerySets.Length(); ++i) {
        nsTemplateQuerySet* qs = tmp->mQuerySets[i];
        delete qs;
    }
    tmp->mQuerySets.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULTemplateBuilder)
    if (tmp->mObservedDocument && !cb.WantAllTraces()) {
        
        return NS_SUCCESS_INTERRUPTED_TRAVERSE;
    }

    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDataSource)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDB)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCompDB)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRoot)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRootResult)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mListeners)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mQueryProcessor)
    tmp->mMatchMap.EnumerateRead(TraverseMatchList, &cb);
    {
      uint32_t i, count = tmp->mQuerySets.Length();
      for (i = 0; i < count; ++i) {
        nsTemplateQuerySet *set = tmp->mQuerySets[i];
        cb.NoteXPCOMChild(set->mQueryNode);
        cb.NoteXPCOMChild(set->mCompiledQuery);
        uint16_t j, rulesCount = set->RuleCount();
        for (j = 0; j < rulesCount; ++j) {
          set->GetRuleAt(j)->Traverse(cb);
        }
      }
    }
    tmp->Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULTemplateBuilder)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULTemplateBuilder)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXULTemplateBuilder)
  NS_INTERFACE_MAP_ENTRY(nsIXULTemplateBuilder)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXULTemplateBuilder)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XULTemplateBuilder)
NS_INTERFACE_MAP_END






NS_IMETHODIMP
nsXULTemplateBuilder::GetRoot(nsIDOMElement** aResult)
{
    if (mRoot) {
        return CallQueryInterface(mRoot, aResult);
    }
    *aResult = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::GetDatasource(nsISupports** aResult)
{
    if (mCompDB)
        NS_ADDREF(*aResult = mCompDB);
    else
        NS_IF_ADDREF(*aResult = mDataSource);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::SetDatasource(nsISupports* aResult)
{
    mDataSource = aResult;
    mCompDB = do_QueryInterface(mDataSource);

    return Rebuild();
}

NS_IMETHODIMP
nsXULTemplateBuilder::GetDatabase(nsIRDFCompositeDataSource** aResult)
{
    NS_IF_ADDREF(*aResult = mCompDB);
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::GetQueryProcessor(nsIXULTemplateQueryProcessor** aResult)
{
    NS_IF_ADDREF(*aResult = mQueryProcessor.get());
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::AddRuleFilter(nsIDOMNode* aRule, nsIXULTemplateRuleFilter* aFilter)
{
    if (!aRule || !aFilter)
        return NS_ERROR_NULL_POINTER;

    
    
    

    int32_t count = mQuerySets.Length();
    for (int32_t q = 0; q < count; q++) {
        nsTemplateQuerySet* queryset = mQuerySets[q];

        int16_t rulecount = queryset->RuleCount();
        for (int16_t r = 0; r < rulecount; r++) {
            nsTemplateRule* rule = queryset->GetRuleAt(r);

            nsCOMPtr<nsIDOMNode> rulenode;
            rule->GetRuleNode(getter_AddRefs(rulenode));
            if (aRule == rulenode) {
                rule->SetRuleFilter(aFilter);
                return NS_OK;
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::Rebuild()
{
    int32_t i;

    for (i = mListeners.Count() - 1; i >= 0; --i) {
        mListeners[i]->WillRebuild(this);
    }

    nsresult rv = RebuildAll();

    for (i = mListeners.Count() - 1; i >= 0; --i) {
        mListeners[i]->DidRebuild(this);
    }

    return rv;
}

NS_IMETHODIMP
nsXULTemplateBuilder::Refresh()
{
    nsresult rv;

    if (!mCompDB)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISimpleEnumerator> dslist;
    rv = mCompDB->GetDataSources(getter_AddRefs(dslist));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    nsCOMPtr<nsISupports> next;
    nsCOMPtr<nsIRDFRemoteDataSource> rds;

    while(NS_SUCCEEDED(dslist->HasMoreElements(&hasMore)) && hasMore) {
        dslist->GetNext(getter_AddRefs(next));
        if (next && (rds = do_QueryInterface(next))) {
            rds->Refresh(false);
        }
    }

    
    

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::Init(nsIContent* aElement)
{
    NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
    mRoot = aElement;

    nsCOMPtr<nsIDocument> doc = mRoot->GetComposedDoc();
    NS_ASSERTION(doc, "element has no document");
    if (! doc)
        return NS_ERROR_UNEXPECTED;

    bool shouldDelay;
    nsresult rv = LoadDataSources(doc, &shouldDelay);

    if (NS_SUCCEEDED(rv)) {
        StartObserving(doc);
    }

    return rv;
}

NS_IMETHODIMP
nsXULTemplateBuilder::CreateContents(nsIContent* aElement, bool aForceCreation)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::HasGeneratedContent(nsIRDFResource* aResource,
                                          nsIAtom* aTag,
                                          bool* aGenerated)
{
    *aGenerated = false;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::AddResult(nsIXULTemplateResult* aResult,
                                nsIDOMNode* aQueryNode)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aQueryNode);

    return UpdateResult(nullptr, aResult, aQueryNode);
}

NS_IMETHODIMP
nsXULTemplateBuilder::RemoveResult(nsIXULTemplateResult* aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);

    return UpdateResult(aResult, nullptr, nullptr);
}

NS_IMETHODIMP
nsXULTemplateBuilder::ReplaceResult(nsIXULTemplateResult* aOldResult,
                                    nsIXULTemplateResult* aNewResult,
                                    nsIDOMNode* aQueryNode)
{
    NS_ENSURE_ARG_POINTER(aOldResult);
    NS_ENSURE_ARG_POINTER(aNewResult);
    NS_ENSURE_ARG_POINTER(aQueryNode);

    

    nsresult rv = UpdateResult(aOldResult, nullptr, nullptr);
    if (NS_FAILED(rv))
        return rv;

    return UpdateResult(nullptr, aNewResult, aQueryNode);
}

nsresult
nsXULTemplateBuilder::UpdateResult(nsIXULTemplateResult* aOldResult,
                                   nsIXULTemplateResult* aNewResult,
                                   nsIDOMNode* aQueryNode)
{
    PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
           ("nsXULTemplateBuilder::UpdateResult %p %p %p",
           aOldResult, aNewResult, aQueryNode));

    if (!mRoot || !mQueriesCompiled)
      return NS_OK;

    
    
    
    
    

    nsAutoPtr<nsCOMArray<nsIContent> > insertionPoints;
    bool mayReplace = GetInsertionLocations(aOldResult ? aOldResult : aNewResult,
                                              getter_Transfers(insertionPoints));
    if (! mayReplace)
        return NS_OK;

    nsresult rv = NS_OK;

    nsCOMPtr<nsIRDFResource> oldId, newId;
    nsTemplateQuerySet* queryset = nullptr;

    if (aOldResult) {
        rv = GetResultResource(aOldResult, getter_AddRefs(oldId));
        if (NS_FAILED(rv))
            return rv;

        
        
        if (IsActivated(oldId))
            return NS_OK;
    }

    if (aNewResult) {
        rv = GetResultResource(aNewResult, getter_AddRefs(newId));
        if (NS_FAILED(rv))
            return rv;

        
        if (! newId)
            return NS_OK;

        
        
        if (IsActivated(newId))
            return NS_OK;

        
        nsCOMPtr<nsIContent> querycontent = do_QueryInterface(aQueryNode);

        int32_t count = mQuerySets.Length();
        for (int32_t q = 0; q < count; q++) {
            nsTemplateQuerySet* qs = mQuerySets[q];
            if (qs->mQueryNode == querycontent) {
                queryset = qs;
                break;
            }
        }

        if (! queryset)
            return NS_OK;
    }

    if (insertionPoints) {
        
        
        uint32_t count = insertionPoints->Count();
        for (uint32_t t = 0; t < count; t++) {
            nsCOMPtr<nsIContent> insertionPoint = insertionPoints->SafeObjectAt(t);
            if (insertionPoint) {
                rv = UpdateResultInContainer(aOldResult, aNewResult, queryset,
                                             oldId, newId, insertionPoint);
                if (NS_FAILED(rv))
                    return rv;
            }
        }
    }
    else {
        
        
        rv = UpdateResultInContainer(aOldResult, aNewResult, queryset,
                                     oldId, newId, nullptr);
    }

    return NS_OK;
}

nsresult
nsXULTemplateBuilder::UpdateResultInContainer(nsIXULTemplateResult* aOldResult,
                                              nsIXULTemplateResult* aNewResult,
                                              nsTemplateQuerySet* aQuerySet,
                                              nsIRDFResource* aOldId,
                                              nsIRDFResource* aNewId,
                                              nsIContent* aInsertionPoint)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv = NS_OK;
    int16_t ruleindex;
    nsTemplateRule* matchedrule = nullptr;

    
    
    bool oldMatchWasActive = false;

    
    
    
    
    
    
    nsTemplateMatch* acceptedmatch = nullptr;

    
    
    
    
    nsTemplateMatch* removedmatch = nullptr;

    
    
    
    
    
    
    
    
    
    
    nsTemplateMatch* replacedmatch = nullptr;
    nsTemplateMatch* replacedmatchtodelete = nullptr;

    if (aOldResult) {
        nsTemplateMatch* firstmatch;
        if (mMatchMap.Get(aOldId, &firstmatch)) {
            nsTemplateMatch* oldmatch = firstmatch;
            nsTemplateMatch* prevmatch = nullptr;

            
            while (oldmatch && (oldmatch->mResult != aOldResult)) {
                prevmatch = oldmatch;
                oldmatch = oldmatch->mNext;
            }

            if (oldmatch) {
                nsTemplateMatch* findmatch = oldmatch->mNext;

                
                
                nsTemplateMatch* nextmatch = findmatch;

                if (oldmatch->IsActive()) {
                    
                    
                    oldMatchWasActive = true;

                    
                    
                    
                    while (findmatch) {
                        
                        
                        if (findmatch->GetContainer() == aInsertionPoint) {
                            nsTemplateQuerySet* qs =
                                mQuerySets[findmatch->QuerySetPriority()];
                        
                            DetermineMatchedRule(aInsertionPoint, findmatch->mResult,
                                                 qs, &matchedrule, &ruleindex);

                            if (matchedrule) {
                                rv = findmatch->RuleMatched(qs,
                                                            matchedrule, ruleindex,
                                                            findmatch->mResult);
                                if (NS_FAILED(rv))
                                    return rv;

                                acceptedmatch = findmatch;
                                break;
                            }
                        }

                        findmatch = findmatch->mNext;
                    }
                }

                if (oldmatch == firstmatch) {
                    
                    if (oldmatch->mNext) {
                        mMatchMap.Put(aOldId, oldmatch->mNext);
                    }
                    else {
                        mMatchMap.Remove(aOldId);
                    }
                }

                if (prevmatch)
                    prevmatch->mNext = nextmatch;

                removedmatch = oldmatch;
                if (mFlags & eLoggingEnabled)
                    OutputMatchToLog(aOldId, removedmatch, false);
            }
        }
    }

    nsTemplateMatch *newmatch = nullptr;
    if (aNewResult) {
        
        nsIAtom* tag = aQuerySet->GetTag();
        if (aInsertionPoint && tag &&
            tag != aInsertionPoint->NodeInfo()->NameAtom())
            return NS_OK;

        int32_t findpriority = aQuerySet->Priority();

        newmatch = nsTemplateMatch::Create(findpriority,
                                           aNewResult, aInsertionPoint);
        if (!newmatch)
            return NS_ERROR_OUT_OF_MEMORY;

        nsTemplateMatch* firstmatch;
        if (mMatchMap.Get(aNewId, &firstmatch)) {
            bool hasEarlierActiveMatch = false;

            
            
            
            nsTemplateMatch* prevmatch = nullptr;
            nsTemplateMatch* oldmatch = firstmatch;
            while (oldmatch) {
                
                
                
                int32_t priority = oldmatch->QuerySetPriority();
                if (priority > findpriority) {
                    oldmatch = nullptr;
                    break;
                }

                
                if (oldmatch->GetContainer() == aInsertionPoint) {
                    if (priority == findpriority)
                        break;

                    
                    
                    if (oldmatch->IsActive())
                        hasEarlierActiveMatch = true;
                }

                prevmatch = oldmatch;
                oldmatch = oldmatch->mNext;
            }

            
            
            

            if (oldmatch)
                newmatch->mNext = oldmatch->mNext;
            else if (prevmatch)
                newmatch->mNext = prevmatch->mNext;
            else
                newmatch->mNext = firstmatch;

            
            
            
            
            
            
            
            
            
            if (! hasEarlierActiveMatch) {
                
                
                if (oldmatch) {
                    if (oldmatch->IsActive())
                        replacedmatch = oldmatch;
                    replacedmatchtodelete = oldmatch;
                }

                
                rv = DetermineMatchedRule(aInsertionPoint, newmatch->mResult,
                                          aQuerySet, &matchedrule, &ruleindex);
                if (NS_FAILED(rv)) {
                    nsTemplateMatch::Destroy(newmatch, false);
                    return rv;
                }

                if (matchedrule) {
                    rv = newmatch->RuleMatched(aQuerySet,
                                               matchedrule, ruleindex,
                                               newmatch->mResult);
                    if (NS_FAILED(rv)) {
                        nsTemplateMatch::Destroy(newmatch, false);
                        return rv;
                    }

                    
                    
                    
                    
                    
                    
                    acceptedmatch = newmatch;

                    
                    
                    nsTemplateMatch* clearmatch = newmatch->mNext;
                    while (clearmatch) {
                        if (clearmatch->GetContainer() == aInsertionPoint &&
                            clearmatch->IsActive()) {
                            clearmatch->SetInactive();
                            
                            
                            
                            NS_ASSERTION(!replacedmatch,
                                         "replaced match already set");
                            replacedmatch = clearmatch;
                            break;
                        }
                        clearmatch = clearmatch->mNext;
                    }
                }
                else if (oldmatch && oldmatch->IsActive()) {
                    
                    
                    
                    newmatch = newmatch->mNext;
                    while (newmatch) {
                        if (newmatch->GetContainer() == aInsertionPoint) {
                            rv = DetermineMatchedRule(aInsertionPoint, newmatch->mResult,
                                                      aQuerySet, &matchedrule, &ruleindex);
                            if (NS_FAILED(rv)) {
                                nsTemplateMatch::Destroy(newmatch, false);
                                return rv;
                            }

                            if (matchedrule) {
                                rv = newmatch->RuleMatched(aQuerySet,
                                                           matchedrule, ruleindex,
                                                           newmatch->mResult);
                                if (NS_FAILED(rv)) {
                                    nsTemplateMatch::Destroy(newmatch, false);
                                    return rv;
                                }

                                acceptedmatch = newmatch;
                                break;
                            }
                        }

                        newmatch = newmatch->mNext;
                    }
                }

                
                if (! prevmatch) {
                    mMatchMap.Put(aNewId, newmatch);
                }
            }

            
            if (prevmatch)
                prevmatch->mNext = newmatch;
        }
        else {
            
            
            rv = DetermineMatchedRule(aInsertionPoint, aNewResult,
                                      aQuerySet, &matchedrule, &ruleindex);
            if (NS_FAILED(rv)) {
                nsTemplateMatch::Destroy(newmatch, false);
                return rv;
            }

            if (matchedrule) {
                rv = newmatch->RuleMatched(aQuerySet, matchedrule,
                                           ruleindex, aNewResult);
                if (NS_FAILED(rv)) {
                    nsTemplateMatch::Destroy(newmatch, false);
                    return rv;
                }

                acceptedmatch = newmatch;
            }

            mMatchMap.Put(aNewId, newmatch);
        }
    }

    
    

    
    if (replacedmatch) {
        rv = ReplaceMatch(replacedmatch->mResult, nullptr, nullptr,
                          aInsertionPoint);

        if (mFlags & eLoggingEnabled)
            OutputMatchToLog(aNewId, replacedmatch, false);
    }

    
    if (replacedmatchtodelete)
        nsTemplateMatch::Destroy(replacedmatchtodelete, true);

    
    
    
    
    if (oldMatchWasActive || acceptedmatch)
        rv = ReplaceMatch(oldMatchWasActive ? aOldResult : nullptr,
                          acceptedmatch, matchedrule, aInsertionPoint);

    
    if (removedmatch)
        nsTemplateMatch::Destroy(removedmatch, true);

    if (mFlags & eLoggingEnabled && newmatch)
        OutputMatchToLog(aNewId, newmatch, true);

    return rv;
}

NS_IMETHODIMP
nsXULTemplateBuilder::ResultBindingChanged(nsIXULTemplateResult* aResult)
{
    
    
    
    NS_ENSURE_ARG_POINTER(aResult);

    if (!mRoot || !mQueriesCompiled)
      return NS_OK;

    return SynchronizeResult(aResult);
}

NS_IMETHODIMP
nsXULTemplateBuilder::GetRootResult(nsIXULTemplateResult** aResult)
{
  *aResult = mRootResult;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::GetResultForId(const nsAString& aId,
                                     nsIXULTemplateResult** aResult)
{
    if (aId.IsEmpty())
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIRDFResource> resource;
    gRDFService->GetUnicodeResource(aId, getter_AddRefs(resource));

    *aResult = nullptr;

    nsTemplateMatch* match;
    if (mMatchMap.Get(resource, &match)) {
        
        while (match) {
            if (match->IsActive()) {
                *aResult = match->mResult;
                NS_IF_ADDREF(*aResult);
                break;
            }
            match = match->mNext;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::GetResultForContent(nsIDOMElement* aContent,
                                          nsIXULTemplateResult** aResult)
{
    *aResult = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::AddListener(nsIXULBuilderListener* aListener)
{
    NS_ENSURE_ARG(aListener);

    if (!mListeners.AppendObject(aListener))
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::RemoveListener(nsIXULBuilderListener* aListener)
{
    NS_ENSURE_ARG(aListener);

    mListeners.RemoveObject(aListener);

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const char16_t* aData)
{
    
    
    if (!strcmp(aTopic, DOM_WINDOW_DESTROYED_TOPIC)) {
        nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aSubject);
        if (window) {
            nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
            if (doc && doc == mObservedDocument)
                NodeWillBeDestroyed(doc);
        }
    }
    return NS_OK;
}





void
nsXULTemplateBuilder::AttributeChanged(nsIDocument* aDocument,
                                       Element*     aElement,
                                       int32_t      aNameSpaceID,
                                       nsIAtom*     aAttribute,
                                       int32_t      aModType)
{
    if (aElement == mRoot && aNameSpaceID == kNameSpaceID_None) {
        
        
        
        if (aAttribute == nsGkAtoms::ref)
            nsContentUtils::AddScriptRunner(
                NS_NewRunnableMethod(this, &nsXULTemplateBuilder::RunnableRebuild));

        
        
        else if (aAttribute == nsGkAtoms::datasources) {
            nsContentUtils::AddScriptRunner(
                NS_NewRunnableMethod(this, &nsXULTemplateBuilder::RunnableLoadAndRebuild));
        }
    }
}

void
nsXULTemplateBuilder::ContentRemoved(nsIDocument* aDocument,
                                     nsIContent* aContainer,
                                     nsIContent* aChild,
                                     int32_t aIndexInContainer,
                                     nsIContent* aPreviousSibling)
{
    if (mRoot && nsContentUtils::ContentIsDescendantOf(mRoot, aChild)) {
        nsRefPtr<nsXULTemplateBuilder> kungFuDeathGrip(this);

        if (mQueryProcessor)
            mQueryProcessor->Done();

        
        nsContentUtils::AddScriptRunner(
            NS_NewRunnableMethod(this, &nsXULTemplateBuilder::UninitFalse));

        MOZ_ASSERT(aDocument == mObservedDocument);
        StopObserving();

        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(aDocument);
        if (xuldoc)
            xuldoc->SetTemplateBuilderFor(mRoot, nullptr);

        
        
        nsXULElement *xulcontent = nsXULElement::FromContent(mRoot);
        if (xulcontent)
            xulcontent->ClearTemplateGenerated();

        CleanUp(true);

        mDB = nullptr;
        mCompDB = nullptr;
        mDataSource = nullptr;
    }
}

void
nsXULTemplateBuilder::NodeWillBeDestroyed(const nsINode* aNode)
{
    
    
    nsRefPtr<nsXULTemplateBuilder> kungFuDeathGrip(this);

    
    if (mQueryProcessor)
        mQueryProcessor->Done();

    mDataSource = nullptr;
    mDB = nullptr;
    mCompDB = nullptr;

    nsContentUtils::AddScriptRunner(
        NS_NewRunnableMethod(this, &nsXULTemplateBuilder::UninitTrue));
}









nsresult
nsXULTemplateBuilder::LoadDataSources(nsIDocument* aDocument,
                                      bool* aShouldDelayBuilding)
{
    NS_PRECONDITION(mRoot != nullptr, "not initialized");

    nsresult rv;
    bool isRDFQuery = false;
  
    
    mDB = nullptr;
    mCompDB = nullptr;
    mDataSource = nullptr;

    *aShouldDelayBuilding = false;

    nsAutoString datasources;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::datasources, datasources);

    nsAutoString querytype;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::querytype, querytype);

    
    
  
    
    if (querytype.IsEmpty())
        querytype.AssignLiteral("rdf");

    if (querytype.EqualsLiteral("rdf")) {
        isRDFQuery = true;
        mQueryProcessor = new nsXULTemplateQueryProcessorRDF();
        NS_ENSURE_TRUE(mQueryProcessor, NS_ERROR_OUT_OF_MEMORY);
    }
    else if (querytype.EqualsLiteral("xml")) {
        mQueryProcessor = new nsXULTemplateQueryProcessorXML();
        NS_ENSURE_TRUE(mQueryProcessor, NS_ERROR_OUT_OF_MEMORY);
    }
    else if (querytype.EqualsLiteral("storage")) {
        mQueryProcessor = new nsXULTemplateQueryProcessorStorage();
        NS_ENSURE_TRUE(mQueryProcessor, NS_ERROR_OUT_OF_MEMORY);
    }
    else {
        nsAutoCString cid(NS_QUERY_PROCESSOR_CONTRACTID_PREFIX);
        AppendUTF16toUTF8(querytype, cid);
        mQueryProcessor = do_CreateInstance(cid.get(), &rv);

        if (!mQueryProcessor) {
            nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_INVALID_QUERYPROCESSOR);
            return rv;
        }
    }

    rv = LoadDataSourceUrls(aDocument, datasources,
                            isRDFQuery, aShouldDelayBuilding);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(aDocument);
    if (xuldoc)
        xuldoc->SetTemplateBuilderFor(mRoot, this);

    if (!mRoot->IsXULElement()) {
        
        
        InitHTMLTemplateRoot();
    }
  
    return NS_OK;
}
  
nsresult
nsXULTemplateBuilder::LoadDataSourceUrls(nsIDocument* aDocument,
                                         const nsAString& aDataSources,
                                         bool aIsRDFQuery,
                                         bool* aShouldDelayBuilding)
{
    
    nsIPrincipal *docPrincipal = aDocument->NodePrincipal();

    NS_ASSERTION(docPrincipal == mRoot->NodePrincipal(),
                 "Principal mismatch?  Which one to use?");

    bool isTrusted = false;
    nsresult rv = IsSystemPrincipal(docPrincipal, &isTrusted);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    
    nsIURI *docurl = aDocument->GetDocumentURI();

    nsCOMPtr<nsIMutableArray> uriList = do_CreateInstance(NS_ARRAY_CONTRACTID);
    if (!uriList)
        return NS_ERROR_FAILURE;

    nsAutoString datasources(aDataSources);
    uint32_t first = 0;
    while (1) {
        while (first < datasources.Length() && nsCRT::IsAsciiSpace(datasources.CharAt(first)))
            ++first;

        if (first >= datasources.Length())
            break;

        uint32_t last = first;
        while (last < datasources.Length() && !nsCRT::IsAsciiSpace(datasources.CharAt(last)))
            ++last;

        nsAutoString uriStr;
        datasources.Mid(uriStr, first, last - first);
        first = last + 1;

        
        if (uriStr.EqualsLiteral("rdf:null"))
            continue;

        if (uriStr.CharAt(0) == '#') {
            
            nsCOMPtr<nsIDOMDocument> domdoc = do_QueryInterface(aDocument);
            nsCOMPtr<nsIDOMElement> dsnode;

            domdoc->GetElementById(Substring(uriStr, 1),
                                   getter_AddRefs(dsnode));

            if (dsnode)
                uriList->AppendElement(dsnode, false);
            continue;
        }

        
        
        NS_MakeAbsoluteURI(uriStr, uriStr, docurl);

        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), uriStr);
        if (NS_FAILED(rv) || !uri)
            continue; 

        
        
        if (!isTrusted && NS_FAILED(docPrincipal->CheckMayLoad(uri, true, false)))
          continue;

        uriList->AppendElement(uri, false);
    }

    nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(mRoot);
    rv = mQueryProcessor->GetDatasource(uriList,
                                        rootNode,
                                        isTrusted,
                                        this,
                                        aShouldDelayBuilding,
                                        getter_AddRefs(mDataSource));
    NS_ENSURE_SUCCESS(rv, rv);

    if (aIsRDFQuery && mDataSource) {  
        
        nsCOMPtr<nsIRDFInferDataSource> inferDB = do_QueryInterface(mDataSource);
        if (inferDB) {
            nsCOMPtr<nsIRDFDataSource> ds;
            inferDB->GetBaseDataSource(getter_AddRefs(ds));
            if (ds)
                mCompDB = do_QueryInterface(ds);
        }

        if (!mCompDB)
            mCompDB = do_QueryInterface(mDataSource);

        mDB = do_QueryInterface(mDataSource);
    }

    if (!mDB && isTrusted) {
        gRDFService->GetDataSource("rdf:local-store", getter_AddRefs(mDB));
    }

    return NS_OK;
}

nsresult
nsXULTemplateBuilder::InitHTMLTemplateRoot()
{
    
    
    nsresult rv;

    nsCOMPtr<nsIDocument> doc = mRoot->GetComposedDoc();
    NS_ASSERTION(doc, "no document");
    if (! doc)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIScriptGlobalObject> global =
      do_QueryInterface(doc->GetWindow());
    if (! global)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIGlobalObject> innerWin =
        do_QueryInterface(doc->GetInnerWindow());

    
    
    AutoEntryScript entryScript(innerWin, "nsXULTemplateBuilder creation", true);
    JSContext* jscontext = entryScript.cx();

    JS::Rooted<JS::Value> v(jscontext);
    rv = nsContentUtils::WrapNative(jscontext, mRoot, mRoot, &v);
    NS_ENSURE_SUCCESS(rv, rv);

    JS::Rooted<JSObject*> jselement(jscontext, v.toObjectOrNull());

    if (mDB) {
        
        JS::Rooted<JS::Value> jsdatabase(jscontext);
        rv = nsContentUtils::WrapNative(jscontext, mDB,
                                        &NS_GET_IID(nsIRDFCompositeDataSource),
                                        &jsdatabase);
        NS_ENSURE_SUCCESS(rv, rv);

        bool ok = JS_SetProperty(jscontext, jselement, "database", jsdatabase);
        NS_ASSERTION(ok, "unable to set database property");
        if (! ok)
            return NS_ERROR_FAILURE;
    }

    {
        
        JS::Rooted<JS::Value> jsbuilder(jscontext);
        rv = nsContentUtils::WrapNative(jscontext,
                                        static_cast<nsIXULTemplateBuilder*>(this),
                                        &NS_GET_IID(nsIXULTemplateBuilder),
                                        &jsbuilder);
        NS_ENSURE_SUCCESS(rv, rv);

        bool ok = JS_SetProperty(jscontext, jselement, "builder", jsbuilder);
        if (! ok)
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

nsresult
nsXULTemplateBuilder::DetermineMatchedRule(nsIContent *aContainer,
                                           nsIXULTemplateResult* aResult,
                                           nsTemplateQuerySet* aQuerySet,
                                           nsTemplateRule** aMatchedRule,
                                           int16_t *aRuleIndex)
{
    
    int16_t count = aQuerySet->RuleCount();
    for (int16_t r = 0; r < count; r++) {
        nsTemplateRule* rule = aQuerySet->GetRuleAt(r);
        
        
        nsIAtom* tag = rule->GetTag();
        if ((!aContainer || !tag ||
             tag == aContainer->NodeInfo()->NameAtom()) &&
            rule->CheckMatch(aResult)) {
            *aMatchedRule = rule;
            *aRuleIndex = r;
            return NS_OK;
        }
    }

    *aRuleIndex = -1;
    *aMatchedRule = nullptr;
    return NS_OK;
}

void
nsXULTemplateBuilder::ParseAttribute(const nsAString& aAttributeValue,
                                     void (*aVariableCallback)(nsXULTemplateBuilder*, const nsAString&, void*),
                                     void (*aTextCallback)(nsXULTemplateBuilder*, const nsAString&, void*),
                                     void* aClosure)
{
    nsAString::const_iterator done_parsing;
    aAttributeValue.EndReading(done_parsing);

    nsAString::const_iterator iter;
    aAttributeValue.BeginReading(iter);

    nsAString::const_iterator mark(iter), backup(iter);

    for (; iter != done_parsing; backup = ++iter) {
        
        
        bool isvar;
        if (*iter == char16_t('?') && (++iter != done_parsing)) {
            isvar = true;
        }
        else if ((*iter == char16_t('r') && (++iter != done_parsing)) &&
                 (*iter == char16_t('d') && (++iter != done_parsing)) &&
                 (*iter == char16_t('f') && (++iter != done_parsing)) &&
                 (*iter == char16_t(':') && (++iter != done_parsing))) {
            isvar = true;
        }
        else {
            isvar = false;
        }

        if (! isvar) {
            
            
            
            
            iter = backup;
            continue;
        }
        else if (backup != mark && aTextCallback) {
            
            
            (*aTextCallback)(this, Substring(mark, backup), aClosure);
        }

        if (*iter == char16_t('?')) {
            
            
            mark = iter;
            continue;
        }

        
        
        
        
        nsAString::const_iterator first(backup);

        char16_t c = 0;
        while (iter != done_parsing) {
            c = *iter;
            if ((c == char16_t(' ')) || (c == char16_t('^')))
                break;

            ++iter;
        }

        nsAString::const_iterator last(iter);

        
        
        
        if (c != char16_t('^'))
            --iter;

        (*aVariableCallback)(this, Substring(first, last), aClosure);
        mark = iter;
        ++mark;
    }

    if (backup != mark && aTextCallback) {
        
        (*aTextCallback)(this, Substring(mark, backup), aClosure);
    }
}


struct MOZ_STACK_CLASS SubstituteTextClosure {
    SubstituteTextClosure(nsIXULTemplateResult* aResult, nsAString& aString)
        : result(aResult), str(aString) {}

    
    
    
    nsCOMPtr<nsIXULTemplateResult> result;
    nsAString& str;
};

nsresult
nsXULTemplateBuilder::SubstituteText(nsIXULTemplateResult* aResult,
                                     const nsAString& aAttributeValue,
                                     nsAString& aString)
{
    
    if (aAttributeValue.EqualsLiteral("...")) {
        aResult->GetId(aString);
        return NS_OK;
    }

    
    aString.SetCapacity(aAttributeValue.Length());

    SubstituteTextClosure closure(aResult, aString);
    ParseAttribute(aAttributeValue,
                   SubstituteTextReplaceVariable,
                   SubstituteTextAppendText,
                   &closure);

    return NS_OK;
}


void
nsXULTemplateBuilder::SubstituteTextAppendText(nsXULTemplateBuilder* aThis,
                                               const nsAString& aText,
                                               void* aClosure)
{
    
    SubstituteTextClosure* c = static_cast<SubstituteTextClosure*>(aClosure);
    c->str.Append(aText);
}

void
nsXULTemplateBuilder::SubstituteTextReplaceVariable(nsXULTemplateBuilder* aThis,
                                                    const nsAString& aVariable,
                                                    void* aClosure)
{
    
    
    SubstituteTextClosure* c = static_cast<SubstituteTextClosure*>(aClosure);

    nsAutoString replacementText;

    
    if (aVariable.EqualsLiteral("rdf:*")){
        c->result->GetId(replacementText);
    }
    else {
        
        nsCOMPtr<nsIAtom> var = do_GetAtom(aVariable);
        c->result->GetBindingFor(var, replacementText);
    }

    c->str += replacementText;
}

bool
nsXULTemplateBuilder::IsTemplateElement(nsIContent* aContent)
{
    return aContent->NodeInfo()->Equals(nsGkAtoms::_template,
                                        kNameSpaceID_XUL);
}

nsresult
nsXULTemplateBuilder::GetTemplateRoot(nsIContent** aResult)
{
    NS_PRECONDITION(mRoot != nullptr, "not initialized");
    if (! mRoot)
        return NS_ERROR_NOT_INITIALIZED;

    
    
    
    
    
    
    
    
    nsAutoString templateID;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::_template, templateID);

    if (! templateID.IsEmpty()) {
        nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(mRoot->GetComposedDoc());
        if (! domDoc)
            return NS_OK;

        nsCOMPtr<nsIDOMElement> domElement;
        domDoc->GetElementById(templateID, getter_AddRefs(domElement));

        if (domElement) {
            nsCOMPtr<nsIContent> content = do_QueryInterface(domElement);
            NS_ENSURE_STATE(content &&
                            !nsContentUtils::ContentIsDescendantOf(mRoot,
                                                                   content));
            content.forget(aResult);
            return NS_OK;
        }
    }

    
    
    for (nsIContent* child = mRoot->GetFirstChild();
         child;
         child = child->GetNextSibling()) {

        if (IsTemplateElement(child)) {
            NS_ADDREF(*aResult = child);
            return NS_OK;
        }
    }

    
    
    
    
    
    FlattenedChildIterator iter(mRoot);
    for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
        if (IsTemplateElement(child)) {
            NS_ADDREF(*aResult = child);
            return NS_OK;
        }
    }

    *aResult = nullptr;
    return NS_OK;
}

nsresult
nsXULTemplateBuilder::CompileQueries()
{
    nsCOMPtr<nsIContent> tmpl;
    GetTemplateRoot(getter_AddRefs(tmpl));
    if (! tmpl)
        return NS_OK;

    if (! mRoot)
        return NS_ERROR_NOT_INITIALIZED;

    
    mFlags = 0;

    nsAutoString flags;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::flags, flags);

    
    
    
    
    
    nsWhitespaceTokenizer tokenizer(flags);
    while (tokenizer.hasMoreTokens()) {
      const nsDependentSubstring& token(tokenizer.nextToken());
      if (token.EqualsLiteral("dont-test-empty"))
        mFlags |= eDontTestEmpty;
      else if (token.EqualsLiteral("dont-recurse"))
        mFlags |= eDontRecurse;
      else if (token.EqualsLiteral("logging"))
        mFlags |= eLoggingEnabled;
    }

#ifdef PR_LOGGING
    
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG))
        mFlags |= eLoggingEnabled;
#endif

    nsCOMPtr<nsIDOMNode> rootnode = do_QueryInterface(mRoot);
    nsresult rv =
        mQueryProcessor->InitializeForBuilding(mDataSource, this, rootnode);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    
    
    

    
    

    nsAutoString containervar;
    tmpl->GetAttr(kNameSpaceID_None, nsGkAtoms::container, containervar);

    if (containervar.IsEmpty())
        mRefVariable = do_GetAtom("?uri");
    else
        mRefVariable = do_GetAtom(containervar);

    nsAutoString membervar;
    tmpl->GetAttr(kNameSpaceID_None, nsGkAtoms::member, membervar);

    if (membervar.IsEmpty())
        mMemberVariable = nullptr;
    else
        mMemberVariable = do_GetAtom(membervar);

    nsTemplateQuerySet* queryset = new nsTemplateQuerySet(0);
    if (!queryset)
        return NS_ERROR_OUT_OF_MEMORY;

    if (!mQuerySets.AppendElement(queryset)) {
        delete queryset;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    bool canUseTemplate = false;
    int32_t priority = 0;
    rv = CompileTemplate(tmpl, queryset, false, &priority, &canUseTemplate);

    if (NS_FAILED(rv) || !canUseTemplate) {
        for (int32_t q = mQuerySets.Length() - 1; q >= 0; q--) {
            nsTemplateQuerySet* qs = mQuerySets[q];
            delete qs;
        }
        mQuerySets.Clear();
    }

    mQueriesCompiled = true;

    return NS_OK;
}

nsresult
nsXULTemplateBuilder::CompileTemplate(nsIContent* aTemplate,
                                      nsTemplateQuerySet* aQuerySet,
                                      bool aIsQuerySet,
                                      int32_t* aPriority,
                                      bool* aCanUseTemplate)
{
    NS_ASSERTION(aQuerySet, "No queryset supplied");

    nsresult rv = NS_OK;

    bool isQuerySetMode = false;
    bool hasQuerySet = false, hasRule = false, hasQuery = false;

    for (nsIContent* rulenode = aTemplate->GetFirstChild();
         rulenode;
         rulenode = rulenode->GetNextSibling()) {

        mozilla::dom::NodeInfo *ni = rulenode->NodeInfo();

        
        if (*aPriority == INT16_MAX)
            return NS_ERROR_FAILURE;

        
        
        if (!aIsQuerySet && ni->Equals(nsGkAtoms::queryset, kNameSpaceID_XUL)) {
            if (hasRule || hasQuery) {
              nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_INVALID_QUERYSET);
              continue;
            }

            isQuerySetMode = true;

            
            
            if (hasQuerySet) {
                aQuerySet = new nsTemplateQuerySet(++*aPriority);
                if (!aQuerySet)
                    return NS_ERROR_OUT_OF_MEMORY;

                
                
                if (!mQuerySets.AppendElement(aQuerySet)) {
                    delete aQuerySet;
                    return NS_ERROR_OUT_OF_MEMORY;
                }
            }

            hasQuerySet = true;

            rv = CompileTemplate(rulenode, aQuerySet, true, aPriority, aCanUseTemplate);
            if (NS_FAILED(rv))
                return rv;
        }

        
        if (isQuerySetMode)
            continue;

        if (ni->Equals(nsGkAtoms::rule, kNameSpaceID_XUL)) {
            nsCOMPtr<nsIContent> action;
            nsXULContentUtils::FindChildByTag(rulenode,
                                              kNameSpaceID_XUL,
                                              nsGkAtoms::action,
                                              getter_AddRefs(action));

            if (action){
                nsCOMPtr<nsIAtom> memberVariable = mMemberVariable;
                if (!memberVariable) {
                    memberVariable = DetermineMemberVariable(action);
                    if (!memberVariable) {
                        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_NO_MEMBERVAR);
                        continue;
                    }
                }

                if (hasQuery) {
                    nsCOMPtr<nsIAtom> tag;
                    DetermineRDFQueryRef(aQuerySet->mQueryNode,
                                         getter_AddRefs(tag));
                    if (tag)
                        aQuerySet->SetTag(tag);

                    if (! aQuerySet->mCompiledQuery) {
                        nsCOMPtr<nsIDOMNode> query(do_QueryInterface(aQuerySet->mQueryNode));

                        rv = mQueryProcessor->CompileQuery(this, query,
                                                           mRefVariable, memberVariable,
                                                           getter_AddRefs(aQuerySet->mCompiledQuery));
                        if (NS_FAILED(rv))
                            return rv;
                    }

                    if (aQuerySet->mCompiledQuery) {
                        rv = CompileExtendedQuery(rulenode, action, memberVariable,
                                                  aQuerySet);
                        if (NS_FAILED(rv))
                            return rv;

                        *aCanUseTemplate = true;
                    }
                }
                else {
                    
                    
                    

                    nsCOMPtr<nsIContent> conditions;
                    nsXULContentUtils::FindChildByTag(rulenode,
                                                      kNameSpaceID_XUL,
                                                      nsGkAtoms::conditions,
                                                      getter_AddRefs(conditions));

                    if (conditions) {
                        
                        if (hasQuerySet) {
                            aQuerySet = new nsTemplateQuerySet(++*aPriority);
                            if (! aQuerySet)
                                return NS_ERROR_OUT_OF_MEMORY;

                            if (!mQuerySets.AppendElement(aQuerySet)) {
                                delete aQuerySet;
                                return NS_ERROR_OUT_OF_MEMORY;
                            }
                        }

                        nsCOMPtr<nsIAtom> tag;
                        DetermineRDFQueryRef(conditions, getter_AddRefs(tag));
                        if (tag)
                            aQuerySet->SetTag(tag);

                        hasQuerySet = true;

                        nsCOMPtr<nsIDOMNode> conditionsnode(do_QueryInterface(conditions));

                        aQuerySet->mQueryNode = conditions;
                        rv = mQueryProcessor->CompileQuery(this, conditionsnode,
                                                           mRefVariable,
                                                           memberVariable,
                                                           getter_AddRefs(aQuerySet->mCompiledQuery));
                        if (NS_FAILED(rv))
                            return rv;

                        if (aQuerySet->mCompiledQuery) {
                            rv = CompileExtendedQuery(rulenode, action, memberVariable,
                                                      aQuerySet);
                            if (NS_FAILED(rv))
                                return rv;

                            *aCanUseTemplate = true;
                        }
                    }
                }
            }
            else {
                if (hasQuery)
                    continue;

                
                if (hasQuerySet) {
                    aQuerySet = new nsTemplateQuerySet(++*aPriority);
                    if (! aQuerySet)
                        return NS_ERROR_OUT_OF_MEMORY;

                    if (!mQuerySets.AppendElement(aQuerySet)) {
                        delete aQuerySet;
                        return NS_ERROR_OUT_OF_MEMORY;
                    }
                }

                hasQuerySet = true;

                rv = CompileSimpleQuery(rulenode, aQuerySet, aCanUseTemplate);
                if (NS_FAILED(rv))
                    return rv;
            }

            hasRule = true;
        }
        else if (ni->Equals(nsGkAtoms::query, kNameSpaceID_XUL)) {
            if (hasQuery)
              continue;

            aQuerySet->mQueryNode = rulenode;
            hasQuery = true;
        }
        else if (ni->Equals(nsGkAtoms::action, kNameSpaceID_XUL)) {
            
            if (! hasQuery)
                continue;

            nsCOMPtr<nsIAtom> tag;
            DetermineRDFQueryRef(aQuerySet->mQueryNode, getter_AddRefs(tag));
            if (tag)
                aQuerySet->SetTag(tag);

            nsCOMPtr<nsIAtom> memberVariable = mMemberVariable;
            if (!memberVariable) {
                memberVariable = DetermineMemberVariable(rulenode);
                if (!memberVariable) {
                    nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_NO_MEMBERVAR);
                    continue;
                }
            }

            nsCOMPtr<nsIDOMNode> query(do_QueryInterface(aQuerySet->mQueryNode));

            rv = mQueryProcessor->CompileQuery(this, query,
                                               mRefVariable, memberVariable,
                                               getter_AddRefs(aQuerySet->mCompiledQuery));

            if (aQuerySet->mCompiledQuery) {
                nsTemplateRule* rule = aQuerySet->NewRule(aTemplate, rulenode, aQuerySet);
                if (! rule)
                    return NS_ERROR_OUT_OF_MEMORY;

                rule->SetVars(mRefVariable, memberVariable);

                *aCanUseTemplate = true;

                return NS_OK;
            }
        }
    }

    if (! hasRule && ! hasQuery && ! hasQuerySet) {
        
        
        rv = CompileSimpleQuery(aTemplate, aQuerySet, aCanUseTemplate);
     }

    return rv;
}

nsresult
nsXULTemplateBuilder::CompileExtendedQuery(nsIContent* aRuleElement,
                                           nsIContent* aActionElement,
                                           nsIAtom* aMemberVariable,
                                           nsTemplateQuerySet* aQuerySet)
{
    
    
    nsresult rv;

    nsTemplateRule* rule = aQuerySet->NewRule(aRuleElement, aActionElement, aQuerySet);
    if (! rule)
         return NS_ERROR_OUT_OF_MEMORY;

    nsCOMPtr<nsIContent> conditions;
    nsXULContentUtils::FindChildByTag(aRuleElement,
                                      kNameSpaceID_XUL,
                                      nsGkAtoms::conditions,
                                      getter_AddRefs(conditions));

    
    if (!conditions)
        conditions = aRuleElement;
  
    rv = CompileConditions(rule, conditions);
    
    if (NS_FAILED(rv)) {
        aQuerySet->RemoveRule(rule);
        return rv;
    }

    rule->SetVars(mRefVariable, aMemberVariable);

    
    nsCOMPtr<nsIContent> bindings;
    nsXULContentUtils::FindChildByTag(aRuleElement,
                                      kNameSpaceID_XUL,
                                      nsGkAtoms::bindings,
                                      getter_AddRefs(bindings));

    
    if (!bindings)
        bindings = aRuleElement;

    rv = CompileBindings(rule, bindings);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

already_AddRefed<nsIAtom>
nsXULTemplateBuilder::DetermineMemberVariable(nsIContent* aElement)
{
    
    
    for (nsIContent* child = aElement->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
        nsAutoString uri;
        child->GetAttr(kNameSpaceID_None, nsGkAtoms::uri, uri);
        if (!uri.IsEmpty() && uri[0] == char16_t('?')) {
            return NS_NewAtom(uri);
        }

        nsCOMPtr<nsIAtom> result = DetermineMemberVariable(child);
        if (result) {
            return result.forget();
        }
    }

    return nullptr;
}

void
nsXULTemplateBuilder::DetermineRDFQueryRef(nsIContent* aQueryElement, nsIAtom** aTag)
{
    
    nsCOMPtr<nsIContent> content;
    nsXULContentUtils::FindChildByTag(aQueryElement,
                                      kNameSpaceID_XUL,
                                      nsGkAtoms::content,
                                      getter_AddRefs(content));

    if (! content) {
        
        nsXULContentUtils::FindChildByTag(aQueryElement,
                                          kNameSpaceID_XUL,
                                          nsGkAtoms::treeitem,
                                          getter_AddRefs(content));
    }

    if (content) {
        nsAutoString uri;
        content->GetAttr(kNameSpaceID_None, nsGkAtoms::uri, uri);

        if (!uri.IsEmpty())
            mRefVariable = do_GetAtom(uri);

        nsAutoString tag;
        content->GetAttr(kNameSpaceID_None, nsGkAtoms::tag, tag);

        if (!tag.IsEmpty())
            *aTag = NS_NewAtom(tag).take();
    }
}

nsresult
nsXULTemplateBuilder::CompileSimpleQuery(nsIContent* aRuleElement,
                                         nsTemplateQuerySet* aQuerySet,
                                         bool* aCanUseTemplate)
{
    
    
    nsCOMPtr<nsIDOMNode> query(do_QueryInterface(aRuleElement));

    nsCOMPtr<nsIAtom> memberVariable;
    if (mMemberVariable)
        memberVariable = mMemberVariable;
    else
        memberVariable = do_GetAtom("rdf:*");

    
    
    aQuerySet->mQueryNode = aRuleElement;
    nsresult rv = mQueryProcessor->CompileQuery(this, query,
                                                mRefVariable, memberVariable,
                                                getter_AddRefs(aQuerySet->mCompiledQuery));
    if (NS_FAILED(rv))
        return rv;

    if (! aQuerySet->mCompiledQuery) {
        *aCanUseTemplate = false;
        return NS_OK;
    }

    nsTemplateRule* rule = aQuerySet->NewRule(aRuleElement, aRuleElement, aQuerySet);
    if (! rule)
        return NS_ERROR_OUT_OF_MEMORY;

    rule->SetVars(mRefVariable, memberVariable);

    nsAutoString tag;
    aRuleElement->GetAttr(kNameSpaceID_None, nsGkAtoms::parent, tag);

    if (!tag.IsEmpty()) {
        nsCOMPtr<nsIAtom> tagatom = do_GetAtom(tag);
        aQuerySet->SetTag(tagatom);
    }

    *aCanUseTemplate = true;

    return AddSimpleRuleBindings(rule, aRuleElement);
}

nsresult
nsXULTemplateBuilder::CompileConditions(nsTemplateRule* aRule,
                                        nsIContent* aCondition)
{
    nsAutoString tag;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::parent, tag);

    if (!tag.IsEmpty()) {
        nsCOMPtr<nsIAtom> tagatom = do_GetAtom(tag);
        aRule->SetTag(tagatom);
    }

    nsTemplateCondition* currentCondition = nullptr;

    for (nsIContent* node = aCondition->GetFirstChild();
         node;
         node = node->GetNextSibling()) {

        if (node->NodeInfo()->Equals(nsGkAtoms::where, kNameSpaceID_XUL)) {
            nsresult rv = CompileWhereCondition(aRule, node, &currentCondition);
            if (NS_FAILED(rv))
                return rv;
        }
    }

    return NS_OK;
}

nsresult
nsXULTemplateBuilder::CompileWhereCondition(nsTemplateRule* aRule,
                                            nsIContent* aCondition,
                                            nsTemplateCondition** aCurrentCondition)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    nsAutoString subject;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::subject, subject);
    if (subject.IsEmpty()) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_WHERE_NO_SUBJECT);
        return NS_OK;
    }

    nsCOMPtr<nsIAtom> svar;
    if (subject[0] == char16_t('?'))
        svar = do_GetAtom(subject);

    nsAutoString relstring;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, relstring);
    if (relstring.IsEmpty()) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_WHERE_NO_RELATION);
        return NS_OK;
    }

    
    nsAutoString value;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::value, value);
    if (value.IsEmpty()) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_WHERE_NO_VALUE);
        return NS_OK;
    }

    
    bool shouldMultiple =
      aCondition->AttrValueIs(kNameSpaceID_None, nsGkAtoms::multiple,
                              nsGkAtoms::_true, eCaseMatters);

    nsCOMPtr<nsIAtom> vvar;
    if (!shouldMultiple && (value[0] == char16_t('?'))) {
        vvar = do_GetAtom(value);
    }

    
    bool shouldIgnoreCase =
      aCondition->AttrValueIs(kNameSpaceID_None, nsGkAtoms::ignorecase,
                              nsGkAtoms::_true, eCaseMatters);

    
    bool shouldNegate =
      aCondition->AttrValueIs(kNameSpaceID_None, nsGkAtoms::negate,
                              nsGkAtoms::_true, eCaseMatters);

    nsTemplateCondition* condition;

    if (svar && vvar) {
        condition = new nsTemplateCondition(svar, relstring, vvar,
                                            shouldIgnoreCase, shouldNegate);
    }
    else if (svar && !value.IsEmpty()) {
        condition = new nsTemplateCondition(svar, relstring, value,
                                            shouldIgnoreCase, shouldNegate, shouldMultiple);
    }
    else if (vvar) {
        condition = new nsTemplateCondition(subject, relstring, vvar,
                                            shouldIgnoreCase, shouldNegate);
    }
    else {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_WHERE_NO_VAR);
        return NS_OK;
    }

    if (! condition)
        return NS_ERROR_OUT_OF_MEMORY;

    if (*aCurrentCondition) {
        (*aCurrentCondition)->SetNext(condition);
    }
    else {
        aRule->SetCondition(condition);
    }

    *aCurrentCondition = condition;

    return NS_OK;
}

nsresult
nsXULTemplateBuilder::CompileBindings(nsTemplateRule* aRule, nsIContent* aBindings)
{
    
    nsresult rv;

    for (nsIContent* binding = aBindings->GetFirstChild();
         binding;
         binding = binding->GetNextSibling()) {

        if (binding->NodeInfo()->Equals(nsGkAtoms::binding,
                                        kNameSpaceID_XUL)) {
            rv = CompileBinding(aRule, binding);
            if (NS_FAILED(rv))
                return rv;
        }
    }

    aRule->AddBindingsToQueryProcessor(mQueryProcessor);

    return NS_OK;
}


nsresult
nsXULTemplateBuilder::CompileBinding(nsTemplateRule* aRule,
                                     nsIContent* aBinding)
{
    
    
    
    
    
    
    
    

    
    nsAutoString subject;
    aBinding->GetAttr(kNameSpaceID_None, nsGkAtoms::subject, subject);
    if (subject.IsEmpty()) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_BINDING_BAD_SUBJECT);
        return NS_OK;
    }

    nsCOMPtr<nsIAtom> svar;
    if (subject[0] == char16_t('?')) {
        svar = do_GetAtom(subject);
    }
    else {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_BINDING_BAD_SUBJECT);
        return NS_OK;
    }

    
    nsAutoString predicate;
    aBinding->GetAttr(kNameSpaceID_None, nsGkAtoms::predicate, predicate);
    if (predicate.IsEmpty()) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_BINDING_BAD_PREDICATE);
        return NS_OK;
    }

    
    nsAutoString object;
    aBinding->GetAttr(kNameSpaceID_None, nsGkAtoms::object, object);

    if (object.IsEmpty()) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_BINDING_BAD_OBJECT);
        return NS_OK;
    }

    nsCOMPtr<nsIAtom> ovar;
    if (object[0] == char16_t('?')) {
        ovar = do_GetAtom(object);
    }
    else {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_BINDING_BAD_OBJECT);
        return NS_OK;
    }

    return aRule->AddBinding(svar, predicate, ovar);
}

nsresult
nsXULTemplateBuilder::AddSimpleRuleBindings(nsTemplateRule* aRule,
                                            nsIContent* aElement)
{
    
    

    nsAutoTArray<nsIContent*, 8> elements;

    if (elements.AppendElement(aElement) == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;

    while (elements.Length()) {
        
        uint32_t i = elements.Length() - 1;
        nsIContent* element = elements[i];
        elements.RemoveElementAt(i);

        
        
        uint32_t count = element->GetAttrCount();

        for (i = 0; i < count; ++i) {
            const nsAttrName* name = element->GetAttrNameAt(i);

            if (!name->Equals(nsGkAtoms::id, kNameSpaceID_None) &&
                !name->Equals(nsGkAtoms::uri, kNameSpaceID_None)) {
                nsAutoString value;
                element->GetAttr(name->NamespaceID(), name->LocalName(), value);

                
                
                ParseAttribute(value, AddBindingsFor, nullptr, aRule);
            }
        }

        
        for (nsIContent* child = element->GetLastChild();
             child;
             child = child->GetPreviousSibling()) {

            if (!elements.AppendElement(child))
                return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    aRule->AddBindingsToQueryProcessor(mQueryProcessor);

    return NS_OK;
}

void
nsXULTemplateBuilder::AddBindingsFor(nsXULTemplateBuilder* aThis,
                                     const nsAString& aVariable,
                                     void* aClosure)
{
    
    
    if (!StringBeginsWith(aVariable, NS_LITERAL_STRING("rdf:")))
        return;

    nsTemplateRule* rule = static_cast<nsTemplateRule*>(aClosure);

    nsCOMPtr<nsIAtom> var = do_GetAtom(aVariable);

    
    
    nsAutoString property;
    property.Assign(Substring(aVariable, uint32_t(4), aVariable.Length() - 4));

    if (! rule->HasBinding(rule->GetMemberVariable(), property, var))
        
        
        rule->AddBinding(rule->GetMemberVariable(), property, var);
}


nsresult
nsXULTemplateBuilder::IsSystemPrincipal(nsIPrincipal *principal, bool *result)
{
  if (!gSystemPrincipal)
    return NS_ERROR_UNEXPECTED;

  *result = (principal == gSystemPrincipal);
  return NS_OK;
}

bool
nsXULTemplateBuilder::IsActivated(nsIRDFResource *aResource)
{
    for (ActivationEntry *entry = mTop;
         entry != nullptr;
         entry = entry->mPrevious) {
        if (entry->mResource == aResource)
            return true;
    }
    return false;
}

nsresult
nsXULTemplateBuilder::GetResultResource(nsIXULTemplateResult* aResult,
                                        nsIRDFResource** aResource)
{
    
    
    
    nsresult rv = aResult->GetResource(aResource);
    if (NS_FAILED(rv))
        return rv;

    if (! *aResource) {
        nsAutoString id;
        rv = aResult->GetId(id);
        if (NS_FAILED(rv))
            return rv;

        return gRDFService->GetUnicodeResource(id, aResource);
    }

    return rv;
}


void
nsXULTemplateBuilder::OutputMatchToLog(nsIRDFResource* aId,
                                       nsTemplateMatch* aMatch,
                                       bool aIsNew)
{
    int32_t priority = aMatch->QuerySetPriority() + 1;
    int32_t activePriority = -1;

    nsAutoString msg;

    nsAutoString templateid;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::id, templateid);
    msg.AppendLiteral("In template");
    if (!templateid.IsEmpty()) {
        msg.AppendLiteral(" with id ");
        msg.Append(templateid);
    }

    nsAutoString refstring;
    aMatch->mResult->GetBindingFor(mRefVariable, refstring);
    if (!refstring.IsEmpty()) {
        msg.AppendLiteral(" using ref ");
        msg.Append(refstring);
    }

    msg.AppendLiteral("\n    ");

    nsTemplateMatch* match = nullptr;
    if (mMatchMap.Get(aId, &match)){
        while (match) {
            if (match == aMatch)
                break;
            if (match->IsActive() &&
                match->GetContainer() == aMatch->GetContainer()) {
                activePriority = match->QuerySetPriority() + 1;
                break;
            }
            match = match->mNext;
        }
    }

    if (aMatch->IsActive()) {
        if (aIsNew) {
            msg.AppendLiteral("New active result for query ");
            msg.AppendInt(priority);
            msg.AppendLiteral(" matching rule ");
            msg.AppendInt(aMatch->RuleIndex() + 1);
        }
        else {
            msg.AppendLiteral("Removed active result for query ");
            msg.AppendInt(priority);
            if (activePriority > 0) {
                msg.AppendLiteral(" (new active query is ");
                msg.AppendInt(activePriority);
                msg.Append(')');
            }
            else {
                msg.AppendLiteral(" (no new active query)");
            }
        }
    }
    else {
        if (aIsNew) {
            msg.AppendLiteral("New inactive result for query ");
            msg.AppendInt(priority);
            if (activePriority > 0) {
                msg.AppendLiteral(" (overridden by query ");
                msg.AppendInt(activePriority);
                msg.Append(')');
            }
            else {
                msg.AppendLiteral(" (didn't match a rule)");
            }
        }
        else {
            msg.AppendLiteral("Removed inactive result for query ");
            msg.AppendInt(priority);
            if (activePriority > 0) {
                msg.AppendLiteral(" (active query is ");
                msg.AppendInt(activePriority);
                msg.Append(')');
            }
            else {
                msg.AppendLiteral(" (no active query)");
            }
        }
    }

    nsAutoString idstring;
    nsXULContentUtils::GetTextForNode(aId, idstring);
    msg.AppendLiteral(": ");
    msg.Append(idstring);

    nsCOMPtr<nsIConsoleService> cs = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (cs)
      cs->LogStringMessage(msg.get());
}
