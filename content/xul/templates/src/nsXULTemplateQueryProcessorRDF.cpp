










































#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIServiceManager.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsIXULDocument.h"
#include "nsUnicharUtils.h"
#include "nsAttrName.h"
#include "rdf.h"

#include "nsContentTestNode.h"
#include "nsRDFConInstanceTestNode.h"
#include "nsRDFConMemberTestNode.h"
#include "nsRDFPropertyTestNode.h"
#include "nsInstantiationNode.h"
#include "nsRDFTestNode.h"
#include "nsXULContentUtils.h"
#include "nsXULTemplateBuilder.h"
#include "nsXULTemplateResultRDF.h"
#include "nsXULTemplateResultSetRDF.h"
#include "nsXULTemplateQueryProcessorRDF.h"



static NS_DEFINE_CID(kRDFContainerUtilsCID,      NS_RDFCONTAINERUTILS_CID);
static NS_DEFINE_CID(kRDFServiceCID,             NS_RDFSERVICE_CID);

#define PARSE_TYPE_INTEGER  "Integer"

nsrefcnt                  nsXULTemplateQueryProcessorRDF::gRefCnt = 0;
nsIRDFService*            nsXULTemplateQueryProcessorRDF::gRDFService;
nsIRDFContainerUtils*     nsXULTemplateQueryProcessorRDF::gRDFContainerUtils;
nsIRDFResource*           nsXULTemplateQueryProcessorRDF::kNC_BookmarkSeparator;
nsIRDFResource*           nsXULTemplateQueryProcessorRDF::kRDF_type;

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULTemplateQueryProcessorRDF)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsXULTemplateQueryProcessorRDF)

PR_STATIC_CALLBACK(PLDHashOperator)
BindingDependenciesTraverser(nsISupports* key,
                             nsCOMArray<nsXULTemplateResultRDF>* array,
                             void* userArg)
{
    nsCycleCollectionTraversalCallback *cb = 
        NS_STATIC_CAST(nsCycleCollectionTraversalCallback*, userArg);

    PRInt32 i, count = array->Count();
    for (i = 0; i < count; ++i) {
        cb->NoteXPCOMChild(array->ObjectAt(i));
    }

    return PL_DHASH_NEXT;
}

PR_STATIC_CALLBACK(PLDHashOperator)
MemoryElementTraverser(const PRUint32& key,
                       nsCOMArray<nsXULTemplateResultRDF>* array,
                       void* userArg)
{
    nsCycleCollectionTraversalCallback *cb = 
        NS_STATIC_CAST(nsCycleCollectionTraversalCallback*, userArg);

    PRInt32 i, count = array->Count();
    for (i = 0; i < count; ++i) {
        cb->NoteXPCOMChild(array->ObjectAt(i));
    }

    return PL_DHASH_NEXT;
}

PR_STATIC_CALLBACK(PLDHashOperator)
RuleToBindingTraverser(nsISupports* key, RDFBindingSet* binding, void* userArg)
{
    nsCycleCollectionTraversalCallback *cb = 
        NS_STATIC_CAST(nsCycleCollectionTraversalCallback*, userArg);

    cb->NoteXPCOMChild(key);

    return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULTemplateQueryProcessorRDF)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDB)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastRef)
    if (tmp->mBindingDependencies.IsInitialized()) {
        tmp->mBindingDependencies.EnumerateRead(BindingDependenciesTraverser,
                                                &cb);
    }
    if (tmp->mMemoryElementToResultMap.IsInitialized()) {
        tmp->mMemoryElementToResultMap.EnumerateRead(MemoryElementTraverser,
                                                     &cb);
    }
    if (tmp->mRuleToBindingsMap.IsInitialized()) {
        tmp->mRuleToBindingsMap.EnumerateRead(RuleToBindingTraverser, &cb);
    }
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mQueries)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsXULTemplateQueryProcessorRDF,
                                          nsIXULTemplateQueryProcessor)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsXULTemplateQueryProcessorRDF,
                                           nsIXULTemplateQueryProcessor)
NS_INTERFACE_MAP_BEGIN(nsXULTemplateQueryProcessorRDF)
    NS_INTERFACE_MAP_ENTRY(nsIXULTemplateQueryProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIRDFObserver)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXULTemplateQueryProcessor)
    NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsXULTemplateQueryProcessorRDF)
NS_INTERFACE_MAP_END

nsXULTemplateQueryProcessorRDF::nsXULTemplateQueryProcessorRDF(void)
    : mDB(nsnull),
      mBuilder(nsnull),
      mQueryProcessorRDFInited(PR_FALSE),
      mGenerationStarted(PR_FALSE),
      mUpdateBatchNest(0),
      mSimpleRuleMemberTest(nsnull)
{
    gRefCnt++;
}

nsXULTemplateQueryProcessorRDF::~nsXULTemplateQueryProcessorRDF(void)
{
    if (--gRefCnt == 0) {
        NS_IF_RELEASE(gRDFService);
        NS_IF_RELEASE(gRDFContainerUtils);
        NS_IF_RELEASE(kNC_BookmarkSeparator);
        NS_IF_RELEASE(kRDF_type);
    }
}

nsresult
nsXULTemplateQueryProcessorRDF::InitGlobals()
{
    nsresult rv;

    
    
    if (!gRDFService) {
        rv = CallGetService(kRDFServiceCID, &gRDFService);
        if (NS_FAILED(rv))
            return rv;
    }

    if (!gRDFContainerUtils) {
        rv = CallGetService(kRDFContainerUtilsCID, &gRDFContainerUtils);
        if (NS_FAILED(rv))
            return rv;
    }
  
    if (!kNC_BookmarkSeparator) {
        gRDFService->GetResource(
          NS_LITERAL_CSTRING(NC_NAMESPACE_URI "BookmarkSeparator"),
                             &kNC_BookmarkSeparator);
    }

    if (!kRDF_type) {
        gRDFService->GetResource(
          NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "type"),
                             &kRDF_type);
    }

    return NS_OK;
}







NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::InitializeForBuilding(nsISupports* aDatasource,
                                                      nsIXULTemplateBuilder* aBuilder,
                                                      nsIDOMNode* aRootNode)
{
    if (!mQueryProcessorRDFInited) {
        nsresult rv = InitGlobals();
        if (NS_FAILED(rv))
            return rv;

        if (!mMemoryElementToResultMap.IsInitialized() &&
            !mMemoryElementToResultMap.Init())
            return NS_ERROR_OUT_OF_MEMORY;
        if (!mBindingDependencies.IsInitialized() &&
            !mBindingDependencies.Init())
            return NS_ERROR_OUT_OF_MEMORY;
        if (!mRuleToBindingsMap.IsInitialized() &&
            !mRuleToBindingsMap.Init())
            return NS_ERROR_OUT_OF_MEMORY;

        const size_t bucketsizes[] = {
            sizeof (nsRDFConMemberTestNode::Element),
            sizeof (nsRDFPropertyTestNode::Element)
        };

        rv = mPool.Init("nsXULTemplateQueryProcessorRDF", bucketsizes, 2, 256);
        if (NS_FAILED(rv))
            return rv;

        mQueryProcessorRDFInited = PR_TRUE;
    }

    
    if (mGenerationStarted)
        return NS_ERROR_UNEXPECTED;

    mDB = do_QueryInterface(aDatasource);
    mBuilder = aBuilder;

    ComputeContainmentProperties(aRootNode);

    
    if (mDB)
        mDB->AddObserver(this);

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::Done()
{
    if (!mQueryProcessorRDFInited)
        return NS_OK;

    if (mDB)
        mDB->RemoveObserver(this);

    mDB = nsnull;
    mBuilder = nsnull;
    mRefVariable = nsnull;
    mLastRef = nsnull;

    mGenerationStarted = PR_FALSE;
    mUpdateBatchNest = 0;

    mContainmentProperties.Clear();

    for (ReteNodeSet::Iterator node = mAllTests.First();
         node != mAllTests.Last(); ++node)
        delete *node;

    mAllTests.Clear();
    mRDFTests.Clear();
    mQueries.Clear();

    mSimpleRuleMemberTest = nsnull;

    mBindingDependencies.Clear();

    mMemoryElementToResultMap.Clear();

    mRuleToBindingsMap.Clear();

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::CompileQuery(nsIXULTemplateBuilder* aBuilder,
                                             nsIDOMNode* aQueryNode,
                                             nsIAtom* aRefVariable,
                                             nsIAtom* aMemberVariable,
                                             nsISupports** _retval)
{
    nsRefPtr<nsRDFQuery> query = new nsRDFQuery(this);
    if (!query)
        return NS_ERROR_OUT_OF_MEMORY;

    query->mRefVariable = aRefVariable;
    if (!mRefVariable)
      mRefVariable = aRefVariable;

    if (!aMemberVariable)
        query->mMemberVariable = do_GetAtom("?");
    else
        query->mMemberVariable = aMemberVariable;

    nsresult rv;
    TestNode *lastnode = nsnull;

    nsCOMPtr<nsIContent> content = do_QueryInterface(aQueryNode);

    if (content->NodeInfo()->Equals(nsGkAtoms::_template, kNameSpaceID_XUL)) {
        

        query->SetSimple();
        NS_ASSERTION(!mSimpleRuleMemberTest,
                     "CompileQuery called twice with the same template");
        if (!mSimpleRuleMemberTest)
            rv = CompileSimpleQuery(query, content, &lastnode);
        else
            rv = NS_ERROR_FAILURE;
    }
    else if (content->NodeInfo()->Equals(nsGkAtoms::rule, kNameSpaceID_XUL)) {
        
        query->SetSimple();
        rv = CompileSimpleQuery(query, content, &lastnode);
    }
    else {
        rv = CompileExtendedQuery(query, content, &lastnode);
    }

    if (NS_FAILED(rv))
        return rv;

    query->SetQueryNode(aQueryNode);

    nsInstantiationNode* instnode = new nsInstantiationNode(this, query);
    if (!instnode)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    
    rv = mAllTests.Add(instnode);
    if (NS_FAILED(rv)) {
        delete instnode;
        return rv;
    }

    rv = lastnode->AddChild(instnode);
    if (NS_FAILED(rv))
        return rv;

    rv = mQueries.AppendObject(query);
    if (NS_FAILED(rv))
        return rv;

    *_retval = query;
    NS_ADDREF(*_retval);

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::GenerateResults(nsISupports* aDatasource,
                                                nsIXULTemplateResult* aRef,
                                                nsISupports* aQuery,
                                                nsISimpleEnumerator** aResults)
{
    nsCOMPtr<nsITemplateRDFQuery> rdfquery = do_QueryInterface(aQuery);
    if (! rdfquery)
        return NS_ERROR_INVALID_ARG;

    mGenerationStarted = PR_TRUE;

    
    
    nsRDFQuery* query = NS_STATIC_CAST(nsRDFQuery *, aQuery);

    *aResults = nsnull;

    nsCOMPtr<nsISimpleEnumerator> results;

    if (aRef) {
        
        
        
        if (aRef == mLastRef) {
            query->UseCachedResults(getter_AddRefs(results));
        }
        else {
            
            PRInt32 count = mQueries.Count();
            for (PRInt32 r = 0; r < count; r++) {
                mQueries[r]->ClearCachedResults();
            }
        }

        if (! results) {
            if (! query->mRefVariable)
                query->mRefVariable = do_GetAtom("?uri");

            nsCOMPtr<nsIRDFResource> refResource;
            aRef->GetResource(getter_AddRefs(refResource));
            if (! refResource)
                return NS_ERROR_FAILURE;

            
            TestNode* root = query->GetRoot();

            if (query->IsSimple() && mSimpleRuleMemberTest) {
                
                root = mSimpleRuleMemberTest->GetParent();
                mLastRef = aRef;
            }

#ifdef PR_LOGGING
            if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                nsAutoString id;
                aRef->GetId(id);

                nsAutoString rvar;
                query->mRefVariable->ToString(rvar);
                nsAutoString mvar;
                query->mMemberVariable->ToString(mvar);

                PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
                       ("QueryProcessor::GenerateResults using ref %s and vars [ ref: %s  member: %s]",
                       NS_ConvertUTF16toUTF8(id).get(),
                       NS_ConvertUTF16toUTF8(rvar).get(),
                       NS_ConvertUTF16toUTF8(mvar).get()));
            }
#endif

            if (root) {
                
                
                Instantiation seed;
                seed.AddAssignment(query->mRefVariable, refResource);

                InstantiationSet* instantiations = new InstantiationSet();
                if (!instantiations)
                    return NS_ERROR_OUT_OF_MEMORY;
                instantiations->Append(seed);

                
                
                
                
                
                
                PRBool owned = PR_FALSE;
                nsresult rv = root->Propagate(*instantiations, PR_FALSE, owned);
                if (! owned)
                    delete instantiations;
                if (NS_FAILED(rv))
                    return rv;

                query->UseCachedResults(getter_AddRefs(results));
            }
        }
    }

    if (! results) {
        
        results = new nsXULTemplateResultSetRDF(this, query, nsnull);
        if (! results)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    results.swap(*aResults);

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::AddBinding(nsIDOMNode* aRuleNode,
                                           nsIAtom* aVar,
                                           nsIAtom* aRef,
                                           const nsAString& aExpr)
{
    
    

    
    
    if (mGenerationStarted)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIRDFResource> property;
    nsresult rv = gRDFService->GetUnicodeResource(aExpr, getter_AddRefs(property));
    if (NS_FAILED(rv))
        return rv;

    nsRefPtr<RDFBindingSet> bindings = mRuleToBindingsMap.GetWeak(aRuleNode);
    if (!bindings) {
        bindings = new RDFBindingSet();
        if (!bindings || !mRuleToBindingsMap.Put(aRuleNode, bindings))
            return NS_ERROR_OUT_OF_MEMORY;
    }

    return bindings->AddBinding(aVar, aRef, property);
}

NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::TranslateRef(nsISupports* aDatasource,
                                                           const nsAString& aRefString,
                                                           nsIXULTemplateResult** aRef)
{
    
    nsresult rv = InitGlobals();
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIRDFResource> uri;
    gRDFService->GetUnicodeResource(aRefString, getter_AddRefs(uri));

    nsXULTemplateResultRDF* refresult = new nsXULTemplateResultRDF(uri);
    if (! refresult)
        return NS_ERROR_OUT_OF_MEMORY;

    *aRef = refresult;
    NS_ADDREF(*aRef);

    return NS_OK;
}

NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::CompareResults(nsIXULTemplateResult* aLeft,
                                                             nsIXULTemplateResult* aRight,
                                                             nsIAtom* aVar,
                                                             PRInt32* aResult)
{
    NS_ENSURE_ARG_POINTER(aLeft);
    NS_ENSURE_ARG_POINTER(aRight);

    *aResult = 0;

    
    
    if (!aVar) {
        
        PRInt32 leftindex = GetContainerIndexOf(aLeft);
        PRInt32 rightindex = GetContainerIndexOf(aRight);
        *aResult = leftindex == rightindex ? 0 :
                   leftindex > rightindex ? 1 :
                   -1;
        return NS_OK;
    }

    nsAutoString sortkey;
    aVar->ToString(sortkey);

    nsCOMPtr<nsISupports> leftNode, rightNode;

    if (!sortkey.IsEmpty() && sortkey[0] != '?' &&
        !StringBeginsWith(sortkey, NS_LITERAL_STRING("rdf:")) &&
        mDB) {
        
        
        nsCOMPtr<nsIRDFResource> predicate;
        nsresult rv = gRDFService->GetUnicodeResource(sortkey, getter_AddRefs(predicate));
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        
        sortkey.AppendLiteral("?sort=true");

        nsCOMPtr<nsIRDFResource> sortPredicate;
        rv = gRDFService->GetUnicodeResource(sortkey, getter_AddRefs(sortPredicate));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = GetSortValue(aLeft, predicate, sortPredicate, getter_AddRefs(leftNode));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = GetSortValue(aRight, predicate, sortPredicate, getter_AddRefs(rightNode));
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        
        aLeft->GetBindingObjectFor(aVar, getter_AddRefs(leftNode));
    aRight->GetBindingObjectFor(aVar, getter_AddRefs(rightNode));
    }

    {
        
        nsCOMPtr<nsIRDFLiteral> l = do_QueryInterface(leftNode);
        if (l) {
            nsCOMPtr<nsIRDFLiteral> r = do_QueryInterface(rightNode);
            if (r) {
                const PRUnichar *lstr, *rstr;
                l->GetValueConst(&lstr);
                r->GetValueConst(&rstr);

                nsICollation* collation = nsXULContentUtils::GetCollation();
                if (collation) {
                    collation->CompareString(nsICollation::kCollationCaseInSensitive,
                                             nsDependentString(lstr),
                                             nsDependentString(rstr),
                                             aResult);
                }
                else
                    *aResult = ::Compare(nsDependentString(lstr),
                                         nsDependentString(rstr),
                                         nsCaseInsensitiveStringComparator());
            }
        }
    }

    {
        
        nsCOMPtr<nsIRDFDate> l = do_QueryInterface(leftNode);
        if (l) {
            nsCOMPtr<nsIRDFDate> r = do_QueryInterface(rightNode);
            if (r) {
                PRTime ldate, rdate;
                l->GetValue(&ldate);
                r->GetValue(&rdate);

                PRInt64 delta;
                LL_SUB(delta, ldate, rdate);

                if (LL_IS_ZERO(delta))
                    *aResult = 0;
                else if (LL_GE_ZERO(delta))
                    *aResult = 1;
                else
                    *aResult = -1;
            }
        }
    }

    {
        
        nsCOMPtr<nsIRDFInt> l = do_QueryInterface(leftNode);
        if (l) {
            nsCOMPtr<nsIRDFInt> r = do_QueryInterface(rightNode);
            if (r) {
                PRInt32 lval, rval;
                l->GetValue(&lval);
                r->GetValue(&rval);

                *aResult = lval - rval;
            }
        }
    }

    nsICollation* collation = nsXULContentUtils::GetCollation();
    if (collation) {
        
        
        nsCOMPtr<nsIRDFBlob> l = do_QueryInterface(leftNode);
        if (l) {
            nsCOMPtr<nsIRDFBlob> r = do_QueryInterface(rightNode);
            if (r) {
                const PRUint8 *lval, *rval;
                PRInt32 llen, rlen;
                l->GetValue(&lval);
                l->GetLength(&llen);
                r->GetValue(&rval);
                r->GetLength(&rlen);
                
                collation->CompareRawSortKey(lval, llen, rval, rlen, aResult);
            }
        }
    }

    
    return NS_OK;
}







NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::OnAssert(nsIRDFDataSource* aDataSource,
                                         nsIRDFResource* aSource,
                                         nsIRDFResource* aProperty,
                                         nsIRDFNode* aTarget)
{
    
    if (mUpdateBatchNest)
        return(NS_OK);

    if (! mBuilder)
        return NS_OK;

    LOG("onassert", aSource, aProperty, aTarget);

    Propagate(aSource, aProperty, aTarget);
    SynchronizeAll(aSource, aProperty, nsnull, aTarget);
    return NS_OK;
}



NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::OnUnassert(nsIRDFDataSource* aDataSource,
                                           nsIRDFResource* aSource,
                                           nsIRDFResource* aProperty,
                                           nsIRDFNode* aTarget)
{
    
    if (mUpdateBatchNest)
        return NS_OK;

    if (! mBuilder)
        return NS_OK;

    LOG("onunassert", aSource, aProperty, aTarget);

    Retract(aSource, aProperty, aTarget);
    SynchronizeAll(aSource, aProperty, aTarget, nsnull);
    return NS_OK;
}


NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::OnChange(nsIRDFDataSource* aDataSource,
                                         nsIRDFResource* aSource,
                                         nsIRDFResource* aProperty,
                                         nsIRDFNode* aOldTarget,
                                         nsIRDFNode* aNewTarget)
{
    
    if (mUpdateBatchNest)
        return NS_OK;

    if (! mBuilder)
        return NS_OK;

    LOG("onchange", aSource, aProperty, aNewTarget);

    if (aOldTarget) {
        
        Retract(aSource, aProperty, aOldTarget);
    }

    if (aNewTarget) {
        
        Propagate(aSource, aProperty, aNewTarget);
    }

    
    SynchronizeAll(aSource, aProperty, aOldTarget, aNewTarget);
    return NS_OK;
}


NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::OnMove(nsIRDFDataSource* aDataSource,
                                       nsIRDFResource* aOldSource,
                                       nsIRDFResource* aNewSource,
                                       nsIRDFResource* aProperty,
                                       nsIRDFNode* aTarget)
{
    
    if (mUpdateBatchNest)
        return NS_OK;

    NS_NOTYETIMPLEMENTED("write me");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::OnBeginUpdateBatch(nsIRDFDataSource* aDataSource)
{
    mUpdateBatchNest++;
    return NS_OK;
}


NS_IMETHODIMP
nsXULTemplateQueryProcessorRDF::OnEndUpdateBatch(nsIRDFDataSource* aDataSource)
{
    NS_ASSERTION(mUpdateBatchNest > 0, "badly nested update batch");
    if (--mUpdateBatchNest <= 0) {
        mUpdateBatchNest = 0;

        if (mBuilder)
            mBuilder->Rebuild();
    }

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::Propagate(nsIRDFResource* aSource,
                                          nsIRDFResource* aProperty,
                                          nsIRDFNode* aTarget)
{
    
    
    
    
    nsresult rv;

    ReteNodeSet livenodes;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        const char* sourceStr;
        aSource->GetValueConst(&sourceStr);
        const char* propertyStr;
        aProperty->GetValueConst(&propertyStr);
        nsAutoString targetStr;
        nsXULContentUtils::GetTextForNode(aTarget, targetStr);

        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("nsXULTemplateQueryProcessorRDF::Propagate: [%s] -> [%s] -> [%s]\n",
               sourceStr, propertyStr, NS_ConvertUTF16toUTF8(targetStr).get()));
    }
#endif

    {
        ReteNodeSet::Iterator last = mRDFTests.Last();
        for (ReteNodeSet::Iterator i = mRDFTests.First(); i != last; ++i) {
            nsRDFTestNode* rdftestnode = NS_STATIC_CAST(nsRDFTestNode*, *i);

            Instantiation seed;
            if (rdftestnode->CanPropagate(aSource, aProperty, aTarget, seed)) {
                rv = livenodes.Add(rdftestnode);
                if (NS_FAILED(rv))
                    return rv;
            }
        }
    }

    
    
    
    {
        ReteNodeSet::Iterator last = livenodes.Last();
        for (ReteNodeSet::Iterator i = livenodes.First(); i != last; ++i) {
            nsRDFTestNode* rdftestnode = NS_STATIC_CAST(nsRDFTestNode*, *i);

            
            
            
            
            
            
            
            
            
            
            
            
            

            
            Instantiation seed;
            rdftestnode->CanPropagate(aSource, aProperty, aTarget, seed);

            InstantiationSet* instantiations = new InstantiationSet();
            if (!instantiations)
                return NS_ERROR_OUT_OF_MEMORY;
            instantiations->Append(seed);

            rv = rdftestnode->Constrain(*instantiations);
            if (NS_FAILED(rv)) {
                delete instantiations;
                return rv;
            }

            PRBool owned = PR_FALSE;
            if (!instantiations->Empty())
                rv = rdftestnode->Propagate(*instantiations, PR_TRUE, owned);

            
            
            if (!owned)
                delete instantiations;
            if (NS_FAILED(rv))
                return rv;
        }
    }

    return NS_OK;
}


nsresult
nsXULTemplateQueryProcessorRDF::Retract(nsIRDFResource* aSource,
                                        nsIRDFResource* aProperty,
                                        nsIRDFNode* aTarget)
{

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        const char* sourceStr;
        aSource->GetValueConst(&sourceStr);
        const char* propertyStr;
        aProperty->GetValueConst(&propertyStr);
        nsAutoString targetStr;
        nsXULContentUtils::GetTextForNode(aTarget, targetStr);

        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("nsXULTemplateQueryProcessorRDF::Retract: [%s] -> [%s] -> [%s]\n",
               sourceStr, propertyStr, NS_ConvertUTF16toUTF8(targetStr).get()));
    }
#endif

    
    ReteNodeSet::ConstIterator lastnode = mRDFTests.Last();
    for (ReteNodeSet::ConstIterator node = mRDFTests.First(); node != lastnode; ++node) {
        const nsRDFTestNode* rdftestnode = NS_STATIC_CAST(const nsRDFTestNode*, *node);

        rdftestnode->Retract(aSource, aProperty, aTarget);

        
        
        
        
        
        
    }

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::SynchronizeAll(nsIRDFResource* aSource,
                                               nsIRDFResource* aProperty,
                                               nsIRDFNode* aOldTarget,
                                               nsIRDFNode* aNewTarget)
{
    

    
    
    nsCOMArray<nsXULTemplateResultRDF>* results;
    if (!mBindingDependencies.Get(aSource, &results))
        return NS_OK;

    PRUint32 length = results->Count();

    for (PRUint32 r = 0; r < length; r++) {
        nsXULTemplateResultRDF* result = (*results)[r];
        if (result) {
            
            
            if (result->SyncAssignments(aSource, aProperty, aNewTarget)) {
                nsITemplateRDFQuery* query = result->Query();
                if (query) {
                    nsCOMPtr<nsIDOMNode> querynode;
                    query->GetQueryNode(getter_AddRefs(querynode));

                    mBuilder->ResultBindingChanged(result);
                }
            }
        }
    }

    return NS_OK;
}

#ifdef PR_LOGGING
nsresult
nsXULTemplateQueryProcessorRDF::Log(const char* aOperation,
                                    nsIRDFResource* aSource,
                                    nsIRDFResource* aProperty,
                                    nsIRDFNode* aTarget)
{
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        nsresult rv;

        const char* sourceStr;
        rv = aSource->GetValueConst(&sourceStr);
        if (NS_FAILED(rv))
            return rv;

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("xultemplate[%p] %8s [%s]--", this, aOperation, sourceStr));

        const char* propertyStr;
        rv = aProperty->GetValueConst(&propertyStr);
        if (NS_FAILED(rv))
            return rv;

        nsAutoString targetStr;
        rv = nsXULContentUtils::GetTextForNode(aTarget, targetStr);
        if (NS_FAILED(rv))
            return rv;

        nsCAutoString targetstrC;
        targetstrC.AssignWithConversion(targetStr);
        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("                        --[%s]-->[%s]",
                propertyStr,
                targetstrC.get()));
    }
    return NS_OK;
}
#endif

nsresult
nsXULTemplateQueryProcessorRDF::CheckContainer(nsIRDFResource* aResource,
                                               PRBool* aIsContainer)
{
    NS_ENSURE_ARG_POINTER(aIsContainer);

    
    
    
    PRBool isContainer = PR_FALSE;

    for (nsResourceSet::ConstIterator property = mContainmentProperties.First();
         property != mContainmentProperties.Last();
         property++) {
        PRBool hasArc = PR_FALSE;
        mDB->HasArcOut(aResource, *property, &hasArc);

        if (hasArc) {
            
            isContainer = PR_TRUE;
            break;
        }
    }

    
    
    if (! isContainer) {
        gRDFContainerUtils->IsContainer(mDB, aResource, &isContainer);
    }

    *aIsContainer = isContainer;

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::CheckEmpty(nsIRDFResource* aResource,
                                           PRBool* aIsEmpty)
{
    *aIsEmpty = PR_TRUE;

    for (nsResourceSet::ConstIterator property = mContainmentProperties.First();
         property != mContainmentProperties.Last();
         property++) {

        nsCOMPtr<nsIRDFNode> dummy;
        mDB->GetTarget(aResource, *property, PR_TRUE, getter_AddRefs(dummy));

        if (dummy) {
            *aIsEmpty = PR_FALSE;
            break;
        }
    }

    if (*aIsEmpty){
        return nsXULTemplateQueryProcessorRDF::gRDFContainerUtils->
                   IsEmpty(mDB, aResource, aIsEmpty);
    }

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::CheckIsSeparator(nsIRDFResource* aResource,
                                                 PRBool* aIsSeparator)
{
    return mDB->HasAssertion(aResource, kRDF_type, kNC_BookmarkSeparator,
                             PR_TRUE, aIsSeparator);
}



nsresult
nsXULTemplateQueryProcessorRDF::ComputeContainmentProperties(nsIDOMNode* aRootNode)
{
    
    
    
    nsresult rv;

    mContainmentProperties.Clear();

    nsCOMPtr<nsIContent> content = do_QueryInterface(aRootNode);

    nsAutoString containment;
    content->GetAttr(kNameSpaceID_None, nsGkAtoms::containment, containment);

    PRUint32 len = containment.Length();
    PRUint32 offset = 0;
    while (offset < len) {
        while (offset < len && nsCRT::IsAsciiSpace(containment[offset]))
            ++offset;

        if (offset >= len)
            break;

        PRUint32 end = offset;
        while (end < len && !nsCRT::IsAsciiSpace(containment[end]))
            ++end;

        nsAutoString propertyStr;
        containment.Mid(propertyStr, offset, end - offset);

        nsCOMPtr<nsIRDFResource> property;
        rv = gRDFService->GetUnicodeResource(propertyStr, getter_AddRefs(property));
        if (NS_FAILED(rv))
            return rv;

        rv = mContainmentProperties.Add(property);
        if (NS_FAILED(rv))
            return rv;

        offset = end;
    }

#define TREE_PROPERTY_HACK 1
#if defined(TREE_PROPERTY_HACK)
    if (! len) {
        
        mContainmentProperties.Add(nsXULContentUtils::NC_child);
        mContainmentProperties.Add(nsXULContentUtils::NC_Folder);
    }
#endif

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::CompileExtendedQuery(nsRDFQuery* aQuery,
                                                     nsIContent* aConditions,
                                                     TestNode** aLastNode)
{
    

    nsContentTestNode* idnode =
        new nsContentTestNode(this, aQuery->mRefVariable);
    if (! idnode)
        return NS_ERROR_OUT_OF_MEMORY;

    aQuery->SetRoot(idnode);
    nsresult rv = mAllTests.Add(idnode);
    if (NS_FAILED(rv)) {
        delete idnode;
        return rv;
    }

    TestNode* prevnode = idnode;

    PRUint32 count = aConditions->GetChildCount();

    for (PRUint32 i = 0; i < count; ++i) {
        nsIContent *condition = aConditions->GetChildAt(i);

        
        if (condition->Tag() == nsGkAtoms::content && !i) {
            
            
            nsAutoString tagstr;
            condition->GetAttr(kNameSpaceID_None, nsGkAtoms::tag, tagstr);

            nsCOMPtr<nsIAtom> tag;
            if (! tagstr.IsEmpty()) {
                tag = do_GetAtom(tagstr);
            }

            nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(condition->GetDocument());
            if (! doc)
                return NS_ERROR_FAILURE;

            idnode->SetTag(tag, doc);
            continue;
        }

        TestNode* testnode = nsnull;
        nsresult rv = CompileQueryChild(condition->Tag(), aQuery, condition,
                                        prevnode, &testnode);
        if (NS_FAILED(rv))
            return rv;

        if (testnode) {
            rv = prevnode->AddChild(testnode);
            if (NS_FAILED(rv))
                return rv;

            prevnode = testnode;
        }
    }

    *aLastNode = prevnode;

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::CompileQueryChild(nsIAtom* aTag,
                                                  nsRDFQuery* aQuery,
                                                  nsIContent* aCondition,
                                                  TestNode* aParentNode,
                                                  TestNode** aResult)
{
    nsresult rv;

    if (aTag == nsGkAtoms::triple) {
        rv = CompileTripleCondition(aQuery, aCondition, aParentNode, aResult);
    }
    else if (aTag == nsGkAtoms::member) {
        rv = CompileMemberCondition(aQuery, aCondition, aParentNode, aResult);
    }
    else {
#ifdef PR_LOGGING
        nsAutoString tagstr;
        aTag->ToString(tagstr);

        nsCAutoString tagstrC;
        tagstrC.AssignWithConversion(tagstr);
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] unrecognized condition test <%s>",
                this, tagstrC.get()));
#endif

        rv = NS_OK;
    }

    return rv;
}

nsresult
nsXULTemplateQueryProcessorRDF::ParseLiteral(const nsString& aParseType, 
                                             const nsString& aValue,
                                             nsIRDFNode** aResult)
{
    nsresult rv = NS_OK;
    *aResult = nsnull;

    if (aParseType.EqualsLiteral(PARSE_TYPE_INTEGER)) {
        nsCOMPtr<nsIRDFInt> intLiteral;
        PRInt32 errorCode;
        PRInt32 intValue = aValue.ToInteger(&errorCode);
        if (NS_FAILED(errorCode))
            return NS_ERROR_FAILURE;
        rv = gRDFService->GetIntLiteral(intValue, getter_AddRefs(intLiteral));
        if (NS_FAILED(rv)) 
            return rv;
        rv = CallQueryInterface(intLiteral, aResult);
    }
    else {
        nsCOMPtr<nsIRDFLiteral> literal;
        rv = gRDFService->GetLiteral(aValue.get(), getter_AddRefs(literal));
        if (NS_FAILED(rv)) 
            return rv;
        rv = CallQueryInterface(literal, aResult);
    }
    return rv;
}

nsresult
nsXULTemplateQueryProcessorRDF::CompileTripleCondition(nsRDFQuery* aQuery,
                                                       nsIContent* aCondition,
                                                       TestNode* aParentNode,
                                                       TestNode** aResult)
{
    
    
    
    
    
    
    
    

    
    nsAutoString subject;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::subject, subject);

    nsCOMPtr<nsIAtom> svar;
    nsCOMPtr<nsIRDFResource> sres;
    if (!subject.IsEmpty() && subject[0] == PRUnichar('?'))
        svar = do_GetAtom(subject);
    else
        gRDFService->GetUnicodeResource(subject, getter_AddRefs(sres));

    
    nsAutoString predicate;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::predicate, predicate);

    nsCOMPtr<nsIRDFResource> pres;
    if (!predicate.IsEmpty() && predicate[0] == PRUnichar('?')) {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] cannot handle variables in <triple> 'predicate'", this));

        return NS_OK;
    }
    else {
        gRDFService->GetUnicodeResource(predicate, getter_AddRefs(pres));
    }

    
    nsAutoString object;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::object, object);

    nsCOMPtr<nsIAtom> ovar;
    nsCOMPtr<nsIRDFNode> onode;
    if (!object.IsEmpty() && object[0] == PRUnichar('?')) {
        ovar = do_GetAtom(object);
    }
    else if (object.FindChar(':') != -1) { 
        
        nsCOMPtr<nsIRDFResource> resource;
        gRDFService->GetUnicodeResource(object, getter_AddRefs(resource));
        onode = do_QueryInterface(resource);
    }
    else {
        nsAutoString parseType;
        aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::parsetype, parseType);
        nsresult rv = ParseLiteral(parseType, object, getter_AddRefs(onode));
        if (NS_FAILED(rv))
            return rv;
    }

    nsRDFPropertyTestNode* testnode = nsnull;

    if (svar && ovar) {
        testnode = new nsRDFPropertyTestNode(aParentNode, this, svar, pres, ovar);
    }
    else if (svar) {
        testnode = new nsRDFPropertyTestNode(aParentNode, this, svar, pres, onode);
    }
    else if (ovar) {
        testnode = new nsRDFPropertyTestNode(aParentNode, this, sres, pres, ovar);
    }
    else {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] tautology in <triple> test", this));

        return NS_OK;
    }

    if (! testnode)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    nsresult rv = mAllTests.Add(testnode);
    if (NS_FAILED(rv)) {
        delete testnode;
        return rv;
    }

    rv = mRDFTests.Add(testnode);
    if (NS_FAILED(rv))
        return rv;

    *aResult = testnode;
    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::CompileMemberCondition(nsRDFQuery* aQuery,
                                                       nsIContent* aCondition,
                                                       TestNode* aParentNode,
                                                       TestNode** aResult)
{
    
    
    
    

    
    nsAutoString container;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::container, container);

    if (!container.IsEmpty() && container[0] != PRUnichar('?')) {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] on <member> test, expected 'container' attribute to name a variable", this));

        return NS_OK;
    }

    nsCOMPtr<nsIAtom> containervar = do_GetAtom(container);

    
    nsAutoString child;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::child, child);

    if (!child.IsEmpty() && child[0] != PRUnichar('?')) {
        PR_LOG(gXULTemplateLog, PR_LOG_ALWAYS,
               ("xultemplate[%p] on <member> test, expected 'child' attribute to name a variable", this));

        return NS_OK;
    }

    nsCOMPtr<nsIAtom> childvar = do_GetAtom(child);

    TestNode* testnode =
        new nsRDFConMemberTestNode(aParentNode,
                                   this,
                                   containervar,
                                   childvar);

    if (! testnode)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    nsresult rv = mAllTests.Add(testnode);
    if (NS_FAILED(rv)) {
        delete testnode;
        return rv;
    }

    rv = mRDFTests.Add(testnode);
    if (NS_FAILED(rv))
        return rv;

    *aResult = testnode;
    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::AddDefaultSimpleRules(nsRDFQuery* aQuery,
                                                      TestNode** aChildNode)
{
    
    nsContentTestNode* idnode =
        new nsContentTestNode(this,
                              aQuery->mRefVariable);
    if (! idnode)
        return NS_ERROR_OUT_OF_MEMORY;

    
    nsRDFConMemberTestNode* membernode =
        new nsRDFConMemberTestNode(idnode,
                                   this,
                                   aQuery->mRefVariable,
                                   aQuery->mMemberVariable);

    if (! membernode) {
        delete idnode;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    nsresult rv = mAllTests.Add(idnode);
    if (NS_FAILED(rv)) {
        delete idnode;
        delete membernode;
        return rv;
    }

    rv = mAllTests.Add(membernode);
    if (NS_FAILED(rv)) {
        delete membernode;
        return rv;
    }

    rv = mRDFTests.Add(membernode);
    if (NS_FAILED(rv))
        return rv;

    rv = idnode->AddChild(membernode);
    if (NS_FAILED(rv))
        return rv;

    mSimpleRuleMemberTest = membernode;
    *aChildNode = membernode;

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::CompileSimpleQuery(nsRDFQuery* aQuery,
                                                   nsIContent* aQueryElement,
                                                   TestNode** aLastNode)
{
    
    nsresult rv;

    TestNode* parentNode;

    if (! mSimpleRuleMemberTest) {
        rv = AddDefaultSimpleRules(aQuery, &parentNode);
        if (NS_FAILED(rv))
            return rv;
    }

    PRBool hasContainerTest = PR_FALSE;

    TestNode* prevnode = mSimpleRuleMemberTest;

    
    const nsAttrName* name;
    for (PRUint32 i = 0; (name = aQueryElement->GetAttrNameAt(i)); ++i) {
        

        
        if (name->Equals(nsGkAtoms::property, kNameSpaceID_RDF) ||
            name->Equals(nsGkAtoms::instanceOf, kNameSpaceID_RDF) ||
            name->Equals(nsGkAtoms::id, kNameSpaceID_None) ||
            name->Equals(nsGkAtoms::parsetype, kNameSpaceID_None)) {
            continue;
        }

        PRInt32 attrNameSpaceID = name->NamespaceID();
        if (attrNameSpaceID == kNameSpaceID_XMLNS)
          continue;
        nsIAtom* attr = name->LocalName();

        nsAutoString value;
        aQueryElement->GetAttr(attrNameSpaceID, attr, value);

        TestNode* testnode = nsnull;

        if (name->Equals(nsGkAtoms::iscontainer, kNameSpaceID_None) ||
            name->Equals(nsGkAtoms::isempty, kNameSpaceID_None)) {
            
            
            
            if (hasContainerTest)
                continue;

            nsRDFConInstanceTestNode::Test iscontainer =
                nsRDFConInstanceTestNode::eDontCare;

            static nsIContent::AttrValuesArray strings[] =
              {&nsGkAtoms::_true, &nsGkAtoms::_false, nsnull};
            switch (aQueryElement->FindAttrValueIn(kNameSpaceID_None,
                                                   nsGkAtoms::iscontainer,
                                                   strings, eCaseMatters)) {
                case 0: iscontainer = nsRDFConInstanceTestNode::eTrue; break;
                case 1: iscontainer = nsRDFConInstanceTestNode::eFalse; break;
            }

            nsRDFConInstanceTestNode::Test isempty =
                nsRDFConInstanceTestNode::eDontCare;

            switch (aQueryElement->FindAttrValueIn(kNameSpaceID_None,
                                                   nsGkAtoms::isempty,
                                                   strings, eCaseMatters)) {
                case 0: isempty = nsRDFConInstanceTestNode::eTrue; break;
                case 1: isempty = nsRDFConInstanceTestNode::eFalse; break;
            }

            testnode = new nsRDFConInstanceTestNode(prevnode,
                                                    this,
                                                    aQuery->mMemberVariable,
                                                    iscontainer,
                                                    isempty);

            if (! testnode)
                return NS_ERROR_OUT_OF_MEMORY;

            rv = mAllTests.Add(testnode);
            if (NS_FAILED(rv)) {
                delete testnode;
                return rv;
            }

            rv = mRDFTests.Add(testnode);
            if (NS_FAILED(rv))
                return rv;
        }
        else if (attrNameSpaceID != kNameSpaceID_None || attr != nsGkAtoms::parent) {
            
            nsCOMPtr<nsIRDFResource> property;
            rv = nsXULContentUtils::GetResource(attrNameSpaceID, attr, getter_AddRefs(property));
            if (NS_FAILED(rv))
                return rv;

            
            nsCOMPtr<nsIRDFNode> target;
            if (value.FindChar(':') != -1) { 
                nsCOMPtr<nsIRDFResource> resource;
                rv = gRDFService->GetUnicodeResource(value, getter_AddRefs(resource));
                if (NS_FAILED(rv))
                    return rv;

                target = do_QueryInterface(resource);
            }
            else {                
              nsAutoString parseType;
              aQueryElement->GetAttr(kNameSpaceID_None, nsGkAtoms::parsetype, parseType);
              rv = ParseLiteral(parseType, value, getter_AddRefs(target));
              if (NS_FAILED(rv))
                  return rv;
            }

            testnode = new nsRDFPropertyTestNode(prevnode, this,
                                                 aQuery->mMemberVariable, property, target);
            if (! testnode)
                return NS_ERROR_OUT_OF_MEMORY;

            rv = mAllTests.Add(testnode);
            if (NS_FAILED(rv)) {
                delete testnode;
                return rv;
            }

            rv = mRDFTests.Add(testnode);
            if (NS_FAILED(rv))
                return rv;
        }

        if (testnode) {
            if (prevnode) {
                rv = prevnode->AddChild(testnode);
                if (NS_FAILED(rv))
                    return rv;
            }                
            else {
                aQuery->SetRoot(testnode);
            }

            prevnode = testnode;
        }
    }

    *aLastNode = prevnode;

    return NS_OK;
}

RDFBindingSet*
nsXULTemplateQueryProcessorRDF::GetBindingsForRule(nsIDOMNode* aRuleNode)
{
    return mRuleToBindingsMap.GetWeak(aRuleNode);
}

nsresult
nsXULTemplateQueryProcessorRDF::AddBindingDependency(nsXULTemplateResultRDF* aResult,
                                                     nsIRDFResource* aResource)
{
    nsCOMArray<nsXULTemplateResultRDF>* arr;
    if (!mBindingDependencies.Get(aResource, &arr)) {
        arr = new nsCOMArray<nsXULTemplateResultRDF>();
        if (!arr)
            return NS_ERROR_OUT_OF_MEMORY;

        if (!mBindingDependencies.Put(aResource, arr)) {
            delete arr;
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    PRInt32 index = arr->IndexOf(aResult);
    if (index == -1)
        return arr->AppendObject(aResult);

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::RemoveBindingDependency(nsXULTemplateResultRDF* aResult,
                                                        nsIRDFResource* aResource)
{
    nsCOMArray<nsXULTemplateResultRDF>* arr;
    if (mBindingDependencies.Get(aResource, &arr)) {
        PRInt32 index = arr->IndexOf(aResult);
        if (index >= 0)
            return arr->RemoveObjectAt(index);
    }

    return NS_OK;
}


nsresult
nsXULTemplateQueryProcessorRDF::AddMemoryElements(const Instantiation& aInst,
                                                  nsXULTemplateResultRDF* aResult)
{
    
    MemoryElementSet::ConstIterator last = aInst.mSupport.Last();
    for (MemoryElementSet::ConstIterator element = aInst.mSupport.First();
                                         element != last; ++element) {

        PLHashNumber hash = (element.operator->())->Hash();

        nsCOMArray<nsXULTemplateResultRDF>* arr;
        if (!mMemoryElementToResultMap.Get(hash, &arr)) {
            arr = new nsCOMArray<nsXULTemplateResultRDF>();
            if (!arr)
                return NS_ERROR_OUT_OF_MEMORY;

            if (!mMemoryElementToResultMap.Put(hash, arr)) {
                delete arr;
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }

        
        arr->AppendObject(aResult);
    }

    return NS_OK;
}

nsresult
nsXULTemplateQueryProcessorRDF::RemoveMemoryElements(const Instantiation& aInst,
                                                     nsXULTemplateResultRDF* aResult)
{
    
    MemoryElementSet::ConstIterator last = aInst.mSupport.Last();
    for (MemoryElementSet::ConstIterator element = aInst.mSupport.First();
                                         element != last; ++element) {

        PLHashNumber hash = (element.operator->())->Hash();

        nsCOMArray<nsXULTemplateResultRDF>* arr;
        if (mMemoryElementToResultMap.Get(hash, &arr)) {
            PRInt32 index = arr->IndexOf(aResult);
            if (index >= 0)
                arr->RemoveObjectAt(index);

            PRUint32 length = arr->Count();
            if (! length)
                mMemoryElementToResultMap.Remove(hash);
        }
    }

    return NS_OK;
}

void
nsXULTemplateQueryProcessorRDF::RetractElement(const MemoryElement& aMemoryElement)
{
    if (! mBuilder)
        return;

    
    
    
    PLHashNumber hash = aMemoryElement.Hash();

    nsCOMArray<nsXULTemplateResultRDF>* arr;
    if (mMemoryElementToResultMap.Get(hash, &arr)) {
        PRUint32 length = arr->Count();

        for (PRInt32 r = length - 1; r >= 0; r--) {
            nsXULTemplateResultRDF* result = (*arr)[r];
            if (result) {
                
                
                
                
                if (result->HasMemoryElement(aMemoryElement)) {
                    nsITemplateRDFQuery* query = result->Query();
                    if (query) {
                        nsCOMPtr<nsIDOMNode> querynode;
                        query->GetQueryNode(getter_AddRefs(querynode));

                        mBuilder->RemoveResult(result);
                    }

                    
                    if (!mMemoryElementToResultMap.Get(hash, nsnull))
                        return;

                    
                    
                    PRUint32 newlength = arr->Count();
                    if (r > (PRInt32)newlength)
                        r = newlength;
                }
            }
        }

        
        if (!arr->Count())
            mMemoryElementToResultMap.Remove(hash);
    }
}

PRInt32
nsXULTemplateQueryProcessorRDF::GetContainerIndexOf(nsIXULTemplateResult* aResult)
{
    
    nsCOMPtr<nsISupports> ref;
    nsresult rv = aResult->GetBindingObjectFor(mRefVariable,
                                               getter_AddRefs(ref));
    if (NS_FAILED(rv))
        return -1;

    nsCOMPtr<nsIRDFResource> container = do_QueryInterface(ref);
    if (container) {
        
        
        PRBool isSequence = PR_FALSE;
        gRDFContainerUtils->IsSeq(mDB, container, &isSequence);
        if (isSequence) {
            nsCOMPtr<nsIRDFResource> resource;
            aResult->GetResource(getter_AddRefs(resource));
            if (resource) {
                PRInt32 index;
                gRDFContainerUtils->IndexOf(mDB, container, resource, &index);
                return index;
            }
        }
    }

    
    
    return -1;
}

nsresult
nsXULTemplateQueryProcessorRDF::GetSortValue(nsIXULTemplateResult* aResult,
                                             nsIRDFResource* aPredicate,
                                             nsIRDFResource* aSortPredicate,
                                             nsISupports** aResultNode)
{
    nsCOMPtr<nsIRDFResource> source;
    nsresult rv = aResult->GetResource(getter_AddRefs(source));
    if (NS_FAILED(rv))
        return rv;
    
    nsCOMPtr<nsIRDFNode> value;
    if (source) {
        
        
        rv = mDB->GetTarget(source, aSortPredicate, PR_TRUE,
                            getter_AddRefs(value));
        if (NS_FAILED(rv))
            return rv;

        if (!value) {
            rv = mDB->GetTarget(source, aPredicate, PR_TRUE,
                                getter_AddRefs(value));
            if (NS_FAILED(rv))
                return rv;
        }
    }

    *aResultNode = value;
    NS_IF_ADDREF(*aResultNode);
    return NS_OK;
}
