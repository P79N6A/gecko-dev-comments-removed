

























































#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsFixedSizeAllocator.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXMLDocument.h"
#include "nsIPrivateDOMImplementation.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsBindingManager.h"
#include "nsIDOMNodeList.h"
#include "nsINameSpaceManager.h"
#include "nsIObserverService.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFInferDataSource.h"
#include "nsIRDFContainerUtils.h"
#include "nsIXULDocument.h"
#include "nsIXULTemplateBuilder.h"
#include "nsIXULBuilderListener.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIMutableArray.h"
#include "nsIURL.h"
#include "nsIXPConnect.h"
#include "nsContentCID.h"
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
#include "nsIDOMClassInfo.h"
#include "nsPIDOMWindow.h"

#include "nsNetUtil.h"
#include "nsXULTemplateBuilder.h"
#include "nsXULTemplateQueryProcessorRDF.h"
#include "nsXULTemplateQueryProcessorXML.h"
#include "nsXULTemplateQueryProcessorStorage.h"



static NS_DEFINE_CID(kRDFContainerUtilsCID,      NS_RDFCONTAINERUTILS_CID);
static NS_DEFINE_CID(kRDFServiceCID,             NS_RDFSERVICE_CID);






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
    : mQueriesCompiled(PR_FALSE),
      mFlags(0),
      mTop(nsnull),
      mObservedDocument(nsnull)
{
}

static PLDHashOperator
DestroyMatchList(nsISupports* aKey, nsTemplateMatch* aMatch, void* aContext)
{
    nsFixedSizeAllocator* pool = static_cast<nsFixedSizeAllocator *>(aContext);

    
    while (aMatch) {
        nsTemplateMatch* next = aMatch->mNext;
        nsTemplateMatch::Destroy(*pool, aMatch, PR_TRUE);
        aMatch = next;
    }

    return PL_DHASH_NEXT;
}

nsXULTemplateBuilder::~nsXULTemplateBuilder(void)
{
    if (--gRefCnt == 0) {
        NS_IF_RELEASE(gRDFService);
        NS_IF_RELEASE(gRDFContainerUtils);
        NS_IF_RELEASE(gSystemPrincipal);
        NS_IF_RELEASE(gScriptSecurityManager);
        NS_IF_RELEASE(gObserverService);
    }

    Uninit(PR_TRUE);
}


nsresult
nsXULTemplateBuilder::InitGlobals()
{
    nsresult rv;

    if (gRefCnt++ == 0) {
        
        
        rv = CallGetService(kRDFServiceCID, &gRDFService);
        if (NS_FAILED(rv))
            return rv;

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

    if (!mMatchMap.IsInitialized() && !mMatchMap.Init())
        return NS_ERROR_OUT_OF_MEMORY;

    const size_t bucketsizes[] = { sizeof(nsTemplateMatch) };
    return mPool.Init("nsXULTemplateBuilder", bucketsizes, 1, 256);
}


void
nsXULTemplateBuilder::Uninit(PRBool aIsFinal)
{
    if (mObservedDocument && aIsFinal) {
        gObserverService->RemoveObserver(this, DOM_WINDOW_DESTROYED_TOPIC);
        mObservedDocument->RemoveObserver(this);
        mObservedDocument = nsnull;
    }

    if (mQueryProcessor)
        mQueryProcessor->Done();

    for (PRInt32 q = mQuerySets.Length() - 1; q >= 0; q--) {
        nsTemplateQuerySet* qs = mQuerySets[q];
        delete qs;
    }

    mQuerySets.Clear();

    mMatchMap.EnumerateRead(DestroyMatchList, &mPool);
    mMatchMap.Clear();

    mRootResult = nsnull;
    mRefVariable = nsnull;
    mMemberVariable = nsnull;

    mQueriesCompiled = PR_FALSE;
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
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDataSource)
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDB)
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCompDB)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULTemplateBuilder)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDataSource)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDB)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCompDB)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRoot)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRootResult)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mListeners)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mQueryProcessor)
    if (tmp->mMatchMap.IsInitialized())
        tmp->mMatchMap.EnumerateRead(TraverseMatchList, &cb);
    {
      PRUint32 i, count = tmp->mQuerySets.Length();
      for (i = 0; i < count; ++i) {
        nsTemplateQuerySet *set = tmp->mQuerySets[i];
        cb.NoteXPCOMChild(set->mQueryNode);
        cb.NoteXPCOMChild(set->mCompiledQuery);
        PRUint16 j, rulesCount = set->RuleCount();
        for (j = 0; j < rulesCount; ++j) {
          set->GetRuleAt(j)->Traverse(cb);
        }
      }
    }
    tmp->Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsXULTemplateBuilder,
                                          nsIXULTemplateBuilder)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsXULTemplateBuilder,
                                           nsIXULTemplateBuilder)

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
    *aResult = nsnull;
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

    
    
    

    PRInt32 count = mQuerySets.Length();
    for (PRInt32 q = 0; q < count; q++) {
        nsTemplateQuerySet* queryset = mQuerySets[q];

        PRInt16 rulecount = queryset->RuleCount();
        for (PRInt16 r = 0; r < rulecount; r++) {
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
    PRInt32 i;

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

    PRBool hasMore;
    nsCOMPtr<nsISupports> next;
    nsCOMPtr<nsIRDFRemoteDataSource> rds;

    while(NS_SUCCEEDED(dslist->HasMoreElements(&hasMore)) && hasMore) {
        dslist->GetNext(getter_AddRefs(next));
        if (next && (rds = do_QueryInterface(next))) {
            rds->Refresh(PR_FALSE);
        }
    }

    
    

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::Init(nsIContent* aElement)
{
    NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);
    mRoot = aElement;

    nsCOMPtr<nsIDocument> doc = mRoot->GetDocument();
    NS_ASSERTION(doc, "element has no document");
    if (! doc)
        return NS_ERROR_UNEXPECTED;

    PRBool shouldDelay;
    nsresult rv = LoadDataSources(doc, &shouldDelay);

    if (NS_SUCCEEDED(rv)) {
        
        doc->AddObserver(this);

        mObservedDocument = doc;
        gObserverService->AddObserver(this, DOM_WINDOW_DESTROYED_TOPIC,
                                      PR_FALSE);
    }

    return rv;
}

NS_IMETHODIMP
nsXULTemplateBuilder::CreateContents(nsIContent* aElement, PRBool aForceCreation)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::HasGeneratedContent(nsIRDFResource* aResource,
                                          nsIAtom* aTag,
                                          PRBool* aGenerated)
{
    *aGenerated = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateBuilder::AddResult(nsIXULTemplateResult* aResult,
                                nsIDOMNode* aQueryNode)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(aQueryNode);

    return UpdateResult(nsnull, aResult, aQueryNode);
}

NS_IMETHODIMP
nsXULTemplateBuilder::RemoveResult(nsIXULTemplateResult* aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);

    return UpdateResult(aResult, nsnull, nsnull);
}

NS_IMETHODIMP
nsXULTemplateBuilder::ReplaceResult(nsIXULTemplateResult* aOldResult,
                                    nsIXULTemplateResult* aNewResult,
                                    nsIDOMNode* aQueryNode)
{
    NS_ENSURE_ARG_POINTER(aOldResult);
    NS_ENSURE_ARG_POINTER(aNewResult);
    NS_ENSURE_ARG_POINTER(aQueryNode);

    

    nsresult rv = UpdateResult(aOldResult, nsnull, nsnull);
    if (NS_FAILED(rv))
        return rv;

    return UpdateResult(nsnull, aNewResult, aQueryNode);
}

nsresult
nsXULTemplateBuilder::UpdateResult(nsIXULTemplateResult* aOldResult,
                                   nsIXULTemplateResult* aNewResult,
                                   nsIDOMNode* aQueryNode)
{
    PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
           ("nsXULTemplateBuilder::UpdateResult %p %p %p",
           aOldResult, aNewResult, aQueryNode));

    
    
    
    
    

    nsAutoPtr<nsCOMArray<nsIContent> > insertionPoints;
    PRBool mayReplace = GetInsertionLocations(aOldResult ? aOldResult : aNewResult,
                                              getter_Transfers(insertionPoints));
    if (! mayReplace)
        return NS_OK;

    nsresult rv = NS_OK;

    nsCOMPtr<nsIRDFResource> oldId, newId;
    nsTemplateQuerySet* queryset = nsnull;

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

        PRInt32 count = mQuerySets.Length();
        for (PRInt32 q = 0; q < count; q++) {
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
        
        
        PRUint32 count = insertionPoints->Count();
        for (PRUint32 t = 0; t < count; t++) {
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
                                     oldId, newId, nsnull);
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
    PRInt16 ruleindex;
    nsTemplateRule* matchedrule = nsnull;

    
    
    PRBool oldMatchWasActive = PR_FALSE;

    
    
    
    
    
    
    nsTemplateMatch* acceptedmatch = nsnull;

    
    
    
    
    nsTemplateMatch* removedmatch = nsnull;

    
    
    
    
    
    
    
    
    
    
    nsTemplateMatch* replacedmatch = nsnull, * replacedmatchtodelete = nsnull;

    if (aOldResult) {
        nsTemplateMatch* firstmatch;
        if (mMatchMap.Get(aOldId, &firstmatch)) {
            nsTemplateMatch* oldmatch = firstmatch;
            nsTemplateMatch* prevmatch = nsnull;

            
            while (oldmatch && (oldmatch->mResult != aOldResult)) {
                prevmatch = oldmatch;
                oldmatch = oldmatch->mNext;
            }

            if (oldmatch) {
                nsTemplateMatch* findmatch = oldmatch->mNext;

                
                
                nsTemplateMatch* nextmatch = findmatch;

                if (oldmatch->IsActive()) {
                    
                    
                    oldMatchWasActive = PR_TRUE;

                    
                    
                    
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
                        if (!mMatchMap.Put(aOldId, oldmatch->mNext))
                            return NS_ERROR_OUT_OF_MEMORY;
                    }
                    else {
                        mMatchMap.Remove(aOldId);
                    }
                }

                if (prevmatch)
                    prevmatch->mNext = nextmatch;

                removedmatch = oldmatch;
            }
        }
    }

    if (aNewResult) {
        
        nsIAtom* tag = aQuerySet->GetTag();
        if (aInsertionPoint && tag && tag != aInsertionPoint->Tag())
            return NS_OK;

        PRInt32 findpriority = aQuerySet->Priority();

        nsTemplateMatch *newmatch =
            nsTemplateMatch::Create(mPool, findpriority,
                                    aNewResult, aInsertionPoint);
        if (!newmatch)
            return NS_ERROR_OUT_OF_MEMORY;

        nsTemplateMatch* firstmatch;
        if (mMatchMap.Get(aNewId, &firstmatch)) {
            PRBool hasEarlierActiveMatch = PR_FALSE;

            
            
            
            nsTemplateMatch* prevmatch = nsnull;
            nsTemplateMatch* oldmatch = firstmatch;
            while (oldmatch) {
                
                
                
                PRInt32 priority = oldmatch->QuerySetPriority();
                if (priority > findpriority) {
                    oldmatch = nsnull;
                    break;
                }

                
                if (oldmatch->GetContainer() == aInsertionPoint) {
                    if (priority == findpriority)
                        break;

                    
                    
                    if (oldmatch->IsActive())
                        hasEarlierActiveMatch = PR_TRUE;
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
                    nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                    return rv;
                }

                if (matchedrule) {
                    rv = newmatch->RuleMatched(aQuerySet,
                                               matchedrule, ruleindex,
                                               newmatch->mResult);
                    if (NS_FAILED(rv)) {
                        nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
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
                                nsTemplateMatch::Destroy(mPool, newmatch,
                                                         PR_FALSE);
                                return rv;
                            }

                            if (matchedrule) {
                                rv = newmatch->RuleMatched(aQuerySet,
                                                           matchedrule, ruleindex,
                                                           newmatch->mResult);
                                if (NS_FAILED(rv)) {
                                    nsTemplateMatch::Destroy(mPool, newmatch,
                                                             PR_FALSE);
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
                    if (!mMatchMap.Put(aNewId, newmatch)) {
                        
                        
                        
                        nsTemplateMatch::Destroy(mPool, newmatch, PR_TRUE);
                        return rv;
                    }
                }
            }

            
            if (prevmatch)
                prevmatch->mNext = newmatch;
        }
        else {
            
            
            rv = DetermineMatchedRule(aInsertionPoint, aNewResult,
                                      aQuerySet, &matchedrule, &ruleindex);
            if (NS_FAILED(rv)) {
                nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                return rv;
            }

            if (matchedrule) {
                rv = newmatch->RuleMatched(aQuerySet, matchedrule,
                                           ruleindex, aNewResult);
                if (NS_FAILED(rv)) {
                    nsTemplateMatch::Destroy(mPool, newmatch, PR_FALSE);
                    return rv;
                }

                acceptedmatch = newmatch;
            }

            if (!mMatchMap.Put(aNewId, newmatch)) {
                nsTemplateMatch::Destroy(mPool, newmatch, PR_TRUE);
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
    }

    
    

    
    if (replacedmatch)
        rv = ReplaceMatch(replacedmatch->mResult, nsnull, nsnull,
                          aInsertionPoint);
 
    
    if (replacedmatchtodelete)
        nsTemplateMatch::Destroy(mPool, replacedmatchtodelete, PR_TRUE);

    
    
    
    
    if (oldMatchWasActive || acceptedmatch)
        rv = ReplaceMatch(oldMatchWasActive ? aOldResult : nsnull,
                          acceptedmatch, matchedrule, aInsertionPoint);

    
    if (removedmatch)
        nsTemplateMatch::Destroy(mPool, removedmatch, PR_TRUE);

    return rv;
}

NS_IMETHODIMP
nsXULTemplateBuilder::ResultBindingChanged(nsIXULTemplateResult* aResult)
{
    
    
    
    NS_ENSURE_ARG_POINTER(aResult);

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

    *aResult = nsnull;

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
    *aResult = nsnull;
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
                              const PRUnichar* aData)
{
    
    
    if (!strcmp(aTopic, DOM_WINDOW_DESTROYED_TOPIC)) {
        nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aSubject);
        if (window) {
            nsCOMPtr<nsIDocument> doc =
                do_QueryInterface(window->GetExtantDocument());
            if (doc && doc == mObservedDocument)
                NodeWillBeDestroyed(doc);
        }
    }
    return NS_OK;
}





void
nsXULTemplateBuilder::AttributeChanged(nsIDocument* aDocument,
                                       nsIContent*  aContent,
                                       PRInt32      aNameSpaceID,
                                       nsIAtom*     aAttribute,
                                       PRInt32      aModType,
                                       PRUint32     aStateMask)
{
    if (aContent == mRoot && aNameSpaceID == kNameSpaceID_None) {
        
        
        
        if (aAttribute == nsGkAtoms::ref)
            nsContentUtils::AddScriptRunner(
                NS_NEW_RUNNABLE_METHOD(nsXULTemplateBuilder, this,
                                       RunnableRebuild));

        
        
        else if (aAttribute == nsGkAtoms::datasources) {
            Uninit(PR_FALSE);  
            
            PRBool shouldDelay;
            LoadDataSources(aDocument, &shouldDelay);
            if (!shouldDelay)
                nsContentUtils::AddScriptRunner(
                    NS_NEW_RUNNABLE_METHOD(nsXULTemplateBuilder, this,
                                           RunnableRebuild));
        }
    }
}

void
nsXULTemplateBuilder::ContentRemoved(nsIDocument* aDocument,
                                     nsIContent* aContainer,
                                     nsIContent* aChild,
                                     PRInt32 aIndexInContainer)
{
    if (mRoot && nsContentUtils::ContentIsDescendantOf(mRoot, aChild)) {
        nsRefPtr<nsXULTemplateBuilder> kungFuDeathGrip(this);

        if (mQueryProcessor)
            mQueryProcessor->Done();

        
        Uninit(PR_FALSE);

        aDocument->RemoveObserver(this);

        nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(aDocument);
        if (xuldoc)
            xuldoc->SetTemplateBuilderFor(mRoot, nsnull);

        
        
        nsXULElement *xulcontent = nsXULElement::FromContent(mRoot);
        if (xulcontent)
            xulcontent->ClearTemplateGenerated();

        mDB = nsnull;
        mCompDB = nsnull;
        mRoot = nsnull;
        mDataSource = nsnull;
    }
}

void
nsXULTemplateBuilder::NodeWillBeDestroyed(const nsINode* aNode)
{
    
    
    nsRefPtr<nsXULTemplateBuilder> kungFuDeathGrip(this);

    
    if (mQueryProcessor)
        mQueryProcessor->Done();

    mDataSource = nsnull;
    mDB = nsnull;
    mCompDB = nsnull;
    mRoot = nsnull;

    Uninit(PR_TRUE);
}









nsresult
nsXULTemplateBuilder::LoadDataSources(nsIDocument* aDocument,
                                      PRBool* aShouldDelayBuilding)
{
    NS_PRECONDITION(mRoot != nsnull, "not initialized");

    nsresult rv;
    PRBool isRDFQuery = PR_FALSE;
  
    
    mDB = nsnull;
    mCompDB = nsnull;
    mDataSource = nsnull;

    *aShouldDelayBuilding = PR_FALSE;

    nsAutoString datasources;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::datasources, datasources);

    nsAutoString querytype;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::querytype, querytype);

    
    
  
    
    if (querytype.IsEmpty())
        querytype.AssignLiteral("rdf");

    if (querytype.EqualsLiteral("rdf")) {
        isRDFQuery = PR_TRUE;
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
        nsCAutoString cid(NS_QUERY_PROCESSOR_CONTRACTID_PREFIX);
        AppendUTF16toUTF8(querytype, cid);
        mQueryProcessor = do_CreateInstance(cid.get(), &rv);
        
        NS_ENSURE_TRUE(mQueryProcessor, rv);
    }

    rv = LoadDataSourceUrls(aDocument, datasources,
                            isRDFQuery, aShouldDelayBuilding);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(aDocument);
    if (xuldoc)
        xuldoc->SetTemplateBuilderFor(mRoot, this);

    if (!mRoot->IsXUL()) {
        
        
        InitHTMLTemplateRoot();
    }
  
    return NS_OK;
}
  
nsresult
nsXULTemplateBuilder::LoadDataSourceUrls(nsIDocument* aDocument,
                                         const nsAString& aDataSources,
                                         PRBool aIsRDFQuery,
                                         PRBool* aShouldDelayBuilding)
{
    
    nsIPrincipal *docPrincipal = aDocument->NodePrincipal();

    NS_ASSERTION(docPrincipal == mRoot->NodePrincipal(),
                 "Principal mismatch?  Which one to use?");

    PRBool isTrusted = PR_FALSE;
    nsresult rv = IsSystemPrincipal(docPrincipal, &isTrusted);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    
    nsIURI *docurl = aDocument->GetDocumentURI();

    nsCOMPtr<nsIMutableArray> uriList = do_CreateInstance(NS_ARRAY_CONTRACTID);
    if (!uriList)
        return NS_ERROR_FAILURE;

    nsAutoString datasources(aDataSources);
    PRUint32 first = 0;
    while (1) {
        while (first < datasources.Length() && nsCRT::IsAsciiSpace(datasources.CharAt(first)))
            ++first;

        if (first >= datasources.Length())
            break;

        PRUint32 last = first;
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
                uriList->AppendElement(dsnode, PR_FALSE);
            continue;
        }

        
        
        NS_MakeAbsoluteURI(uriStr, uriStr, docurl);

        nsCOMPtr<nsIURI> uri;
        rv = NS_NewURI(getter_AddRefs(uri), uriStr);
        if (NS_FAILED(rv) || !uri)
            continue; 

        
        
        if (!isTrusted && NS_FAILED(docPrincipal->CheckMayLoad(uri, PR_TRUE)))
          continue;

        uriList->AppendElement(uri, PR_FALSE);
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

    nsCOMPtr<nsIDocument> doc = mRoot->GetDocument();
    NS_ASSERTION(doc, "no document");
    if (! doc)
        return NS_ERROR_UNEXPECTED;

    nsIScriptGlobalObject *global = doc->GetScriptGlobalObject();
    if (! global)
        return NS_ERROR_UNEXPECTED;

    JSObject *scope = global->GetGlobalJSObject();

    nsIScriptContext *context = global->GetContext();
    if (! context)
        return NS_ERROR_UNEXPECTED;

    JSContext* jscontext = reinterpret_cast<JSContext*>(context->GetNativeContext());
    NS_ASSERTION(context != nsnull, "no jscontext");
    if (! jscontext)
        return NS_ERROR_UNEXPECTED;

    JSAutoRequest ar(jscontext);

    nsIXPConnect *xpc = nsContentUtils::XPConnect();

    JSObject* jselement = nsnull;

    nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
    rv = xpc->WrapNative(jscontext, scope, mRoot, NS_GET_IID(nsIDOMElement),
                         getter_AddRefs(wrapper));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = wrapper->GetJSObject(&jselement);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mDB) {
        
        rv = xpc->WrapNative(jscontext, scope, mDB,
                             NS_GET_IID(nsIRDFCompositeDataSource),
                             getter_AddRefs(wrapper));
        NS_ENSURE_SUCCESS(rv, rv);

        JSObject* jsobj;
        rv = wrapper->GetJSObject(&jsobj);
        NS_ENSURE_SUCCESS(rv, rv);

        jsval jsdatabase = OBJECT_TO_JSVAL(jsobj);

        PRBool ok;
        ok = JS_SetProperty(jscontext, jselement, "database", &jsdatabase);
        NS_ASSERTION(ok, "unable to set database property");
        if (! ok)
            return NS_ERROR_FAILURE;
    }

    {
        
        nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
        rv = xpc->WrapNative(jscontext, jselement,
                             static_cast<nsIXULTemplateBuilder*>(this),
                             NS_GET_IID(nsIXULTemplateBuilder),
                             getter_AddRefs(wrapper));
        NS_ENSURE_SUCCESS(rv, rv);

        JSObject* jsobj;
        rv = wrapper->GetJSObject(&jsobj);
        NS_ENSURE_SUCCESS(rv, rv);

        jsval jsbuilder = OBJECT_TO_JSVAL(jsobj);

        PRBool ok;
        ok = JS_SetProperty(jscontext, jselement, "builder", &jsbuilder);
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
                                           PRInt16 *aRuleIndex)
{
    
    PRInt16 count = aQuerySet->RuleCount();
    for (PRInt16 r = 0; r < count; r++) {
        nsTemplateRule* rule = aQuerySet->GetRuleAt(r);
        
        
        nsIAtom* tag = rule->GetTag();
        if ((!aContainer || !tag || tag == aContainer->Tag()) &&
            rule->CheckMatch(aResult)) {
            *aMatchedRule = rule;
            *aRuleIndex = r;
            return NS_OK;
        }
    }

    *aRuleIndex = -1;
    *aMatchedRule = nsnull;
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
        
        
        PRBool isvar;
        if (*iter == PRUnichar('?') && (++iter != done_parsing)) {
            isvar = PR_TRUE;
        }
        else if ((*iter == PRUnichar('r') && (++iter != done_parsing)) &&
                 (*iter == PRUnichar('d') && (++iter != done_parsing)) &&
                 (*iter == PRUnichar('f') && (++iter != done_parsing)) &&
                 (*iter == PRUnichar(':') && (++iter != done_parsing))) {
            isvar = PR_TRUE;
        }
        else {
            isvar = PR_FALSE;
        }

        if (! isvar) {
            
            
            
            
            iter = backup;
            continue;
        }
        else if (backup != mark && aTextCallback) {
            
            
            (*aTextCallback)(this, Substring(mark, backup), aClosure);
        }

        if (*iter == PRUnichar('?')) {
            
            
            mark = iter;
            continue;
        }

        
        
        
        
        nsAString::const_iterator first(backup);

        PRUnichar c = 0;
        while (iter != done_parsing) {
            c = *iter;
            if ((c == PRUnichar(' ')) || (c == PRUnichar('^')))
                break;

            ++iter;
        }

        nsAString::const_iterator last(iter);

        
        
        
        if (c != PRUnichar('^'))
            --iter;

        (*aVariableCallback)(this, Substring(first, last), aClosure);
        mark = iter;
        ++mark;
    }

    if (backup != mark && aTextCallback) {
        
        (*aTextCallback)(this, Substring(mark, backup), aClosure);
    }
}


struct NS_STACK_CLASS SubstituteTextClosure {
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

PRBool
nsXULTemplateBuilder::IsTemplateElement(nsIContent* aContent)
{
    return aContent->NodeInfo()->Equals(nsGkAtoms::_template,
                                        kNameSpaceID_XUL);
}

nsresult
nsXULTemplateBuilder::GetTemplateRoot(nsIContent** aResult)
{
    NS_PRECONDITION(mRoot != nsnull, "not initialized");
    if (! mRoot)
        return NS_ERROR_NOT_INITIALIZED;

    
    
    
    
    
    
    
    
    nsAutoString templateID;
    mRoot->GetAttr(kNameSpaceID_None, nsGkAtoms::_template, templateID);

    if (! templateID.IsEmpty()) {
        nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(mRoot->GetDocument());
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

#if 1 
    {
        
        
        PRUint32 count = mRoot->GetChildCount();

        for (PRUint32 i = 0; i < count; ++i) {
            nsIContent *child = mRoot->GetChildAt(i);

            if (IsTemplateElement(child)) {
                NS_ADDREF(*aResult = child);
                return NS_OK;
            }
        }
    }
#endif

    
    
    nsCOMPtr<nsIDocument> doc = mRoot->GetDocument();
    if (! doc)
        return NS_OK;

    nsCOMPtr<nsIDOMNodeList> kids;
    doc->BindingManager()->GetXBLChildNodesFor(mRoot, getter_AddRefs(kids));

    if (kids) {
        PRUint32 length;
        kids->GetLength(&length);

        for (PRUint32 i = 0; i < length; ++i) {
            nsCOMPtr<nsIDOMNode> node;
            kids->Item(i, getter_AddRefs(node));
            if (! node)
                continue;

            nsCOMPtr<nsIContent> child = do_QueryInterface(node);

            if (IsTemplateElement(child)) {
                NS_ADDREF(*aResult = child.get());
                return NS_OK;
            }
        }
    }

    *aResult = nsnull;
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
    }

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
        mMemberVariable = nsnull;
    else
        mMemberVariable = do_GetAtom(membervar);

    nsTemplateQuerySet* queryset = new nsTemplateQuerySet(0);
    if (!queryset)
        return NS_ERROR_OUT_OF_MEMORY;

    if (!mQuerySets.AppendElement(queryset)) {
        delete queryset;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    PRBool canUseTemplate = PR_FALSE;
    PRInt32 priority = 0;
    rv = CompileTemplate(tmpl, queryset, PR_FALSE, &priority, &canUseTemplate);

    if (NS_FAILED(rv) || !canUseTemplate) {
        for (PRInt32 q = mQuerySets.Length() - 1; q >= 0; q--) {
            nsTemplateQuerySet* qs = mQuerySets[q];
            delete qs;
        }
        mQuerySets.Clear();
    }

    mQueriesCompiled = PR_TRUE;

    return NS_OK;
}

nsresult
nsXULTemplateBuilder::CompileTemplate(nsIContent* aTemplate,
                                      nsTemplateQuerySet* aQuerySet,
                                      PRBool aIsQuerySet,
                                      PRInt32* aPriority,
                                      PRBool* aCanUseTemplate)
{
    NS_ASSERTION(aQuerySet, "No queryset supplied");

    

    nsresult rv = NS_OK;

    PRBool isQuerySetMode = PR_FALSE;
    PRBool hasQuerySet = PR_FALSE, hasRule = PR_FALSE, hasQuery = PR_FALSE;

    PRUint32 count = aTemplate->GetChildCount();

    for (PRUint32 i = 0; i < count; i++) {
        nsIContent *rulenode = aTemplate->GetChildAt(i);
        nsINodeInfo *ni = rulenode->NodeInfo();

        
        if (*aPriority == PR_INT16_MAX)
            return NS_ERROR_FAILURE;

        
        
        if (!aIsQuerySet && ni->Equals(nsGkAtoms::queryset, kNameSpaceID_XUL)) {
            if (hasRule || hasQuery)
              continue;

            isQuerySetMode = PR_TRUE;

            
            
            if (hasQuerySet) {
                aQuerySet = new nsTemplateQuerySet(++*aPriority);
                if (!aQuerySet)
                    return NS_ERROR_OUT_OF_MEMORY;

                
                
                if (!mQuerySets.AppendElement(aQuerySet)) {
                    delete aQuerySet;
                    return NS_ERROR_OUT_OF_MEMORY;
                }
            }

            hasQuerySet = PR_TRUE;

            rv = CompileTemplate(rulenode, aQuerySet, PR_TRUE, aPriority, aCanUseTemplate);
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
                nsCOMPtr<nsIAtom> memberVariable;
                DetermineMemberVariable(action, getter_AddRefs(memberVariable));
                if (! memberVariable) continue;

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

                        *aCanUseTemplate = PR_TRUE;
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

                        hasQuerySet = PR_TRUE;

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

                            *aCanUseTemplate = PR_TRUE;
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

                hasQuerySet = PR_TRUE;

                rv = CompileSimpleQuery(rulenode, aQuerySet, aCanUseTemplate);
                if (NS_FAILED(rv))
                    return rv;
            }

            hasRule = PR_TRUE;
        }
        else if (ni->Equals(nsGkAtoms::query, kNameSpaceID_XUL)) {
            if (hasQuery)
              continue;

            aQuerySet->mQueryNode = rulenode;
            hasQuery = PR_TRUE;
        }
        else if (ni->Equals(nsGkAtoms::action, kNameSpaceID_XUL)) {
            
            if (! hasQuery)
                continue;

            nsCOMPtr<nsIAtom> tag;
            DetermineRDFQueryRef(aQuerySet->mQueryNode, getter_AddRefs(tag));
            if (tag)
                aQuerySet->SetTag(tag);

            nsCOMPtr<nsIAtom> memberVariable;
            DetermineMemberVariable(rulenode, getter_AddRefs(memberVariable));
            if (! memberVariable) continue;

            nsCOMPtr<nsIDOMNode> query(do_QueryInterface(aQuerySet->mQueryNode));

            rv = mQueryProcessor->CompileQuery(this, query,
                                               mRefVariable, memberVariable,
                                               getter_AddRefs(aQuerySet->mCompiledQuery));

            if (aQuerySet->mCompiledQuery) {
                nsTemplateRule* rule = aQuerySet->NewRule(aTemplate, rulenode, aQuerySet);
                if (! rule)
                    return NS_ERROR_OUT_OF_MEMORY;

                rule->SetVars(mRefVariable, memberVariable);

                *aCanUseTemplate = PR_TRUE;

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

nsresult
nsXULTemplateBuilder::DetermineMemberVariable(nsIContent* aActionElement,
                                              nsIAtom** aMemberVariable)
{
    
    
    

    if (mMemberVariable) {
        *aMemberVariable = mMemberVariable;
        NS_IF_ADDREF(*aMemberVariable);
    }
    else {
        *aMemberVariable = nsnull;

        nsCOMArray<nsIContent> unvisited;

        if (!unvisited.AppendObject(aActionElement))
            return NS_ERROR_OUT_OF_MEMORY;

        while (unvisited.Count()) {
            nsIContent* next = unvisited[0];
            unvisited.RemoveObjectAt(0);

            nsAutoString uri;
            next->GetAttr(kNameSpaceID_None, nsGkAtoms::uri, uri);

            if (!uri.IsEmpty() && uri[0] == PRUnichar('?')) {
                
                *aMemberVariable = NS_NewAtom(uri);
                break;
            }

            
            
            PRUint32 count = next->GetChildCount();

            for (PRUint32 i = 0; i < count; ++i) {
                nsIContent *child = next->GetChildAt(i);

                if (!unvisited.AppendObject(child))
                    return NS_ERROR_OUT_OF_MEMORY;
            }
        }
    }

    return NS_OK;
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
            *aTag = NS_NewAtom(tag);
    }
}

nsresult
nsXULTemplateBuilder::CompileSimpleQuery(nsIContent* aRuleElement,
                                         nsTemplateQuerySet* aQuerySet,
                                         PRBool* aCanUseTemplate)
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
        *aCanUseTemplate = PR_FALSE;
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

    *aCanUseTemplate = PR_TRUE;

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

    PRUint32 count = aCondition->GetChildCount();

    nsTemplateCondition* currentCondition = nsnull;

    for (PRUint32 i = 0; i < count; i++) {
        nsIContent *node = aCondition->GetChildAt(i);

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
    if (subject.IsEmpty())
        return NS_OK;

    nsCOMPtr<nsIAtom> svar;
    if (subject[0] == PRUnichar('?'))
        svar = do_GetAtom(subject);

    nsAutoString relstring;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, relstring);
    if (relstring.IsEmpty())
        return NS_OK;

    
    nsAutoString value;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::value, value);
    if (value.IsEmpty())
        return NS_OK;

    
    PRBool shouldMultiple =
      aCondition->AttrValueIs(kNameSpaceID_None, nsGkAtoms::multiple,
                              nsGkAtoms::_true, eCaseMatters);

    nsCOMPtr<nsIAtom> vvar;
    if (!shouldMultiple && (value[0] == PRUnichar('?'))) {
        vvar = do_GetAtom(value);
    }

    
    PRBool shouldIgnoreCase =
      aCondition->AttrValueIs(kNameSpaceID_None, nsGkAtoms::ignorecase,
                              nsGkAtoms::_true, eCaseMatters);

    
    PRBool shouldNegate =
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
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] on <where> test, expected at least one variable", this));
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

    PRUint32 count = aBindings->GetChildCount();

    for (PRUint32 i = 0; i < count; ++i) {
        nsIContent *binding = aBindings->GetChildAt(i);

        if (binding->NodeInfo()->Equals(nsGkAtoms::binding,
                                        kNameSpaceID_XUL)) {
            rv = CompileBinding(aRule, binding);
        }
        else {
#ifdef PR_LOGGING
            nsAutoString tagstr;
            binding->NodeInfo()->GetQualifiedName(tagstr);

            nsCAutoString tagstrC;
            tagstrC.AssignWithConversion(tagstr);
            PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
                   ("xultemplate[%p] unrecognized binding <%s>",
                    this, tagstrC.get()));
#endif

            continue;
        }

        if (NS_FAILED(rv))
            return rv;
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
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] <binding> requires `subject'", this));

        return NS_OK;
    }

    nsCOMPtr<nsIAtom> svar;
    if (subject[0] == PRUnichar('?')) {
        svar = do_GetAtom(subject);
    }
    else {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] <binding> requires `subject' to be a variable", this));

        return NS_OK;
    }

    
    nsAutoString predicate;
    aBinding->GetAttr(kNameSpaceID_None, nsGkAtoms::predicate, predicate);
    if (predicate.IsEmpty()) {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] <binding> requires `predicate'", this));

        return NS_OK;
    }

    
    nsAutoString object;
    aBinding->GetAttr(kNameSpaceID_None, nsGkAtoms::object, object);

    if (object.IsEmpty()) {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] <binding> requires `object'", this));

        return NS_OK;
    }

    nsCOMPtr<nsIAtom> ovar;
    if (object[0] == PRUnichar('?')) {
        ovar = do_GetAtom(object);
    }
    else {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] <binding> requires `object' to be a variable", this));

        return NS_OK;
    }

    return aRule->AddBinding(svar, predicate, ovar);
}

nsresult
nsXULTemplateBuilder::AddSimpleRuleBindings(nsTemplateRule* aRule,
                                            nsIContent* aElement)
{
    
    

    nsAutoTArray<nsIContent*, 8> elements;

    if (elements.AppendElement(aElement) == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    while (elements.Length()) {
        
        PRUint32 i = elements.Length() - 1;
        nsIContent* element = elements[i];
        elements.RemoveElementAt(i);

        
        
        PRUint32 count = element->GetAttrCount();

        for (i = 0; i < count; ++i) {
            const nsAttrName* name = element->GetAttrNameAt(i);

            if (!name->Equals(nsGkAtoms::id, kNameSpaceID_None) &&
                !name->Equals(nsGkAtoms::uri, kNameSpaceID_None)) {
                nsAutoString value;
                element->GetAttr(name->NamespaceID(), name->LocalName(), value);

                
                
                ParseAttribute(value, AddBindingsFor, nsnull, aRule);
            }
        }

        
        count = element->GetChildCount();

        while (count-- > 0) {
            if (elements.AppendElement(element->GetChildAt(count)) == nsnull)
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
    property.Assign(Substring(aVariable, PRUint32(4), aVariable.Length() - 4));

    if (! rule->HasBinding(rule->GetMemberVariable(), property, var))
        
        
        rule->AddBinding(rule->GetMemberVariable(), property, var);
}


nsresult
nsXULTemplateBuilder::IsSystemPrincipal(nsIPrincipal *principal, PRBool *result)
{
  if (!gSystemPrincipal)
    return NS_ERROR_UNEXPECTED;

  *result = (principal == gSystemPrincipal);
  return NS_OK;
}

PRBool
nsXULTemplateBuilder::IsActivated(nsIRDFResource *aResource)
{
    for (ActivationEntry *entry = mTop;
         entry != nsnull;
         entry = entry->mPrevious) {
        if (entry->mResource == aResource)
            return PR_TRUE;
    }
    return PR_FALSE;
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
