




#include "nsCOMPtr.h"
#include "nsICollation.h"
#include "nsIDOMNode.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFInferDataSource.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIServiceManager.h"
#include "nsNameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsIDOMDocument.h"
#include "nsAttrName.h"
#include "rdf.h"
#include "nsArrayUtils.h"
#include "nsIURI.h"

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
#include "nsXULSortService.h"
#include "nsIDocument.h"



#define PARSE_TYPE_INTEGER  "Integer"

nsrefcnt                  nsXULTemplateQueryProcessorRDF::gRefCnt = 0;
nsIRDFService*            nsXULTemplateQueryProcessorRDF::gRDFService;
nsIRDFContainerUtils*     nsXULTemplateQueryProcessorRDF::gRDFContainerUtils;
nsIRDFResource*           nsXULTemplateQueryProcessorRDF::kNC_BookmarkSeparator;
nsIRDFResource*           nsXULTemplateQueryProcessorRDF::kRDF_type;

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXULTemplateQueryProcessorRDF)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXULTemplateQueryProcessorRDF)
    tmp->Done();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

static PLDHashOperator
BindingDependenciesTraverser(nsISupports* key,
                             nsXULTemplateQueryProcessorRDF::ResultArray* array,
                             void* userArg)
{
    nsCycleCollectionTraversalCallback *cb = 
        static_cast<nsCycleCollectionTraversalCallback*>(userArg);

    int32_t i, count = array->Length();
    for (i = 0; i < count; ++i) {
        cb->NoteXPCOMChild(array->ElementAt(i));
    }

    return PL_DHASH_NEXT;
}

static PLDHashOperator
MemoryElementTraverser(const uint32_t& key,
                       nsCOMArray<nsXULTemplateResultRDF>* array,
                       void* userArg)
{
    nsCycleCollectionTraversalCallback *cb = 
        static_cast<nsCycleCollectionTraversalCallback*>(userArg);

    int32_t i, count = array->Count();
    for (i = 0; i < count; ++i) {
        cb->NoteXPCOMChild(array->ObjectAt(i));
    }

    return PL_DHASH_NEXT;
}

static PLDHashOperator
RuleToBindingTraverser(nsISupports* key, RDFBindingSet* binding, void* userArg)
{
    nsCycleCollectionTraversalCallback *cb = 
        static_cast<nsCycleCollectionTraversalCallback*>(userArg);

    cb->NoteXPCOMChild(key);

    return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXULTemplateQueryProcessorRDF)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDB)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mLastRef)
    tmp->mBindingDependencies.EnumerateRead(BindingDependenciesTraverser,
                                            &cb);
    tmp->mMemoryElementToResultMap.EnumerateRead(MemoryElementTraverser,
                                                 &cb);
    tmp->mRuleToBindingsMap.EnumerateRead(RuleToBindingTraverser, &cb);
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mQueries)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXULTemplateQueryProcessorRDF)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXULTemplateQueryProcessorRDF)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXULTemplateQueryProcessorRDF)
    NS_INTERFACE_MAP_ENTRY(nsIXULTemplateQueryProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIRDFObserver)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXULTemplateQueryProcessor)
NS_INTERFACE_MAP_END

nsXULTemplateQueryProcessorRDF::nsXULTemplateQueryProcessorRDF(void)
    : mDB(nullptr),
      mBuilder(nullptr),
      mQueryProcessorRDFInited(false),
      mGenerationStarted(false),
      mUpdateBatchNest(0),
      mSimpleRuleMemberTest(nullptr)
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
        NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
        rv = CallGetService(kRDFServiceCID, &gRDFService);
        if (NS_FAILED(rv))
            return rv;
    }

    if (!gRDFContainerUtils) {
        NS_DEFINE_CID(kRDFContainerUtilsCID, NS_RDFCONTAINERUTILS_CID);
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
nsXULTemplateQueryProcessorRDF::GetDatasource(nsIArray* aDataSources,
                                              nsIDOMNode* aRootNode,
                                              bool aIsTrusted,
                                              nsIXULTemplateBuilder* aBuilder,
                                              bool* aShouldDelayBuilding,
                                              nsISupports** aResult)
{
    nsCOMPtr<nsIRDFCompositeDataSource> compDB;
    nsCOMPtr<nsIContent> root = do_QueryInterface(aRootNode);
    nsresult rv;

    *aResult = nullptr;
    *aShouldDelayBuilding = false;

    NS_ENSURE_TRUE(root, NS_ERROR_UNEXPECTED);

    
    rv = InitGlobals();
    NS_ENSURE_SUCCESS(rv, rv);

    
    compDB = do_CreateInstance(NS_RDF_DATASOURCE_CONTRACTID_PREFIX 
                               "composite-datasource");
    if (!compDB) {
        NS_ERROR("unable to construct new composite data source");
        return NS_ERROR_UNEXPECTED;
    }

    
    if (root->AttrValueIs(kNameSpaceID_None,
                          nsGkAtoms::coalesceduplicatearcs,
                          nsGkAtoms::_false, eCaseMatters))
        compDB->SetCoalesceDuplicateArcs(false);

    if (root->AttrValueIs(kNameSpaceID_None,
                          nsGkAtoms::allownegativeassertions,
                          nsGkAtoms::_false, eCaseMatters))
        compDB->SetAllowNegativeAssertions(false);

    if (aIsTrusted) {
        
        
        
        
        nsCOMPtr<nsIRDFDataSource> localstore;
        rv = gRDFService->GetDataSource("rdf:local-store",
                                        getter_AddRefs(localstore));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = compDB->AddDataSource(localstore);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add local store to db");
        NS_ENSURE_SUCCESS(rv, rv);
    }

    uint32_t length, index;
    rv = aDataSources->GetLength(&length);
    NS_ENSURE_SUCCESS(rv,rv);

    for (index = 0; index < length; index++) {

        nsCOMPtr<nsIURI> uri = do_QueryElementAt(aDataSources, index);
        if (!uri) 
            continue;

        nsCOMPtr<nsIRDFDataSource> ds;
        nsAutoCString uristrC;
        uri->GetSpec(uristrC);

        rv = gRDFService->GetDataSource(uristrC.get(), getter_AddRefs(ds));

        if (NS_FAILED(rv)) {
            
            
            
  #ifdef DEBUG
            nsAutoCString msg;
            msg.AppendLiteral("unable to load datasource '");
            msg.Append(uristrC);
            msg.Append('\'');
            NS_WARNING(msg.get());
  #endif
            continue;
        }

        compDB->AddDataSource(ds);
    }


    
    nsAutoString infer;
    nsCOMPtr<nsIRDFDataSource> db;
    root->GetAttr(kNameSpaceID_None, nsGkAtoms::infer, infer);
    if (!infer.IsEmpty()) {
        nsCString inferCID(NS_RDF_INFER_DATASOURCE_CONTRACTID_PREFIX);
        AppendUTF16toUTF8(infer, inferCID);
        nsCOMPtr<nsIRDFInferDataSource> inferDB =
            do_CreateInstance(inferCID.get());

        if (inferDB) {
            inferDB->SetBaseDataSource(compDB);
            db = do_QueryInterface(inferDB);
        }
        else {
            NS_WARNING("failed to construct inference engine specified on template");
        }
    }

    if (!db)
        db = compDB;

    return CallQueryInterface(db, aResult);
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

        mQueryProcessorRDFInited = true;
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

    mDB = nullptr;
    mBuilder = nullptr;
    mRefVariable = nullptr;
    mLastRef = nullptr;

    mGenerationStarted = false;
    mUpdateBatchNest = 0;

    mContainmentProperties.Clear();

    for (ReteNodeSet::Iterator node = mAllTests.First();
         node != mAllTests.Last(); ++node)
        delete *node;

    mAllTests.Clear();
    mRDFTests.Clear();
    mQueries.Clear();

    mSimpleRuleMemberTest = nullptr;

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
    TestNode *lastnode = nullptr;

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

    mQueries.AppendElement(query);

    query.forget(_retval);

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

    mGenerationStarted = true;

    
    
    nsRDFQuery* query = static_cast<nsRDFQuery *>(aQuery);

    *aResults = nullptr;

    nsCOMPtr<nsISimpleEnumerator> results;

    if (aRef) {
        
        
        
        if (aRef == mLastRef) {
            query->UseCachedResults(getter_AddRefs(results));
        }
        else {
            
            int32_t count = mQueries.Length();
            for (int32_t r = 0; r < count; r++) {
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

                
                
                
                
                
                
                bool owned = false;
                nsresult rv = root->Propagate(*instantiations, false, owned);
                if (! owned)
                    delete instantiations;
                if (NS_FAILED(rv))
                    return rv;

                query->UseCachedResults(getter_AddRefs(results));
            }
        }
    }

    if (! results) {
        
        results = new nsXULTemplateResultSetRDF(this, query, nullptr);
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
        mRuleToBindingsMap.Put(aRuleNode, bindings);
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
                                               uint32_t aSortHints,
                                               int32_t* aResult)
{
    NS_ENSURE_ARG_POINTER(aLeft);
    NS_ENSURE_ARG_POINTER(aRight);

    *aResult = 0;

    
    
    if (!aVar) {
        
        int32_t leftindex = GetContainerIndexOf(aLeft);
        int32_t rightindex = GetContainerIndexOf(aRight);
        *aResult = leftindex == rightindex ? 0 :
                   leftindex > rightindex ? 1 :
                   -1;
        return NS_OK;
    }

    nsDependentAtomString sortkey(aVar);

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
                const char16_t *lstr, *rstr;
                l->GetValueConst(&lstr);
                r->GetValueConst(&rstr);

                *aResult = XULSortServiceImpl::CompareValues(
                               nsDependentString(lstr),
                               nsDependentString(rstr), aSortHints);
            }

            return NS_OK;
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

                int64_t delta = ldate - rdate;
                if (delta == 0)
                    *aResult = 0;
                else if (delta >= 0)
                    *aResult = 1;
                else
                    *aResult = -1;
            }

            return NS_OK;
        }
    }

    {
        
        nsCOMPtr<nsIRDFInt> l = do_QueryInterface(leftNode);
        if (l) {
            nsCOMPtr<nsIRDFInt> r = do_QueryInterface(rightNode);
            if (r) {
                int32_t lval, rval;
                l->GetValue(&lval);
                r->GetValue(&rval);

                *aResult = lval - rval;
            }

            return NS_OK;
        }
    }

    nsICollation* collation = nsXULContentUtils::GetCollation();
    if (collation) {
        
        
        nsCOMPtr<nsIRDFBlob> l = do_QueryInterface(leftNode);
        if (l) {
            nsCOMPtr<nsIRDFBlob> r = do_QueryInterface(rightNode);
            if (r) {
                const uint8_t *lval, *rval;
                int32_t llen, rlen;
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
    SynchronizeAll(aSource, aProperty, nullptr, aTarget);
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
    SynchronizeAll(aSource, aProperty, aTarget, nullptr);
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
            nsRDFTestNode* rdftestnode = static_cast<nsRDFTestNode*>(*i);

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
            nsRDFTestNode* rdftestnode = static_cast<nsRDFTestNode*>(*i);

            
            
            
            
            
            
            
            
            
            
            
            
            

            
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

            bool owned = false;
            if (!instantiations->Empty())
                rv = rdftestnode->Propagate(*instantiations, true, owned);

            
            
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
        const nsRDFTestNode* rdftestnode = static_cast<const nsRDFTestNode*>(*node);

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
    

    
    
    ResultArray* results;
    if (!mBindingDependencies.Get(aSource, &results) || !mBuilder)
        return NS_OK;

    uint32_t length = results->Length();

    for (uint32_t r = 0; r < length; r++) {
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

        nsAutoCString targetstrC;
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
                                               bool* aIsContainer)
{
    NS_ENSURE_ARG_POINTER(aIsContainer);
    NS_ENSURE_STATE(mDB);

    
    
    
    bool isContainer = false;

    for (nsResourceSet::ConstIterator property = mContainmentProperties.First();
         property != mContainmentProperties.Last();
         property++) {
        bool hasArc = false;
        mDB->HasArcOut(aResource, *property, &hasArc);

        if (hasArc) {
            
            isContainer = true;
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
                                           bool* aIsEmpty)
{
    NS_ENSURE_STATE(mDB);
    *aIsEmpty = true;

    for (nsResourceSet::ConstIterator property = mContainmentProperties.First();
         property != mContainmentProperties.Last();
         property++) {

        nsCOMPtr<nsIRDFNode> dummy;
        mDB->GetTarget(aResource, *property, true, getter_AddRefs(dummy));

        if (dummy) {
            *aIsEmpty = false;
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
                                                 bool* aIsSeparator)
{
    NS_ENSURE_STATE(mDB);
    return mDB->HasAssertion(aResource, kRDF_type, kNC_BookmarkSeparator,
                             true, aIsSeparator);
}



nsresult
nsXULTemplateQueryProcessorRDF::ComputeContainmentProperties(nsIDOMNode* aRootNode)
{
    
    
    
    nsresult rv;

    mContainmentProperties.Clear();

    nsCOMPtr<nsIContent> content = do_QueryInterface(aRootNode);

    nsAutoString containment;
    content->GetAttr(kNameSpaceID_None, nsGkAtoms::containment, containment);

    uint32_t len = containment.Length();
    uint32_t offset = 0;
    while (offset < len) {
        while (offset < len && nsCRT::IsAsciiSpace(containment[offset]))
            ++offset;

        if (offset >= len)
            break;

        uint32_t end = offset;
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

    for (nsIContent* condition = aConditions->GetFirstChild();
         condition;
         condition = condition->GetNextSibling()) {

        
        if (condition->IsXULElement(nsGkAtoms::content)) {
            if (condition != aConditions->GetFirstChild()) {
                nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_CONTENT_NOT_FIRST);
                continue;
            }

            
            
            nsAutoString tagstr;
            condition->GetAttr(kNameSpaceID_None, nsGkAtoms::tag, tagstr);

            nsCOMPtr<nsIAtom> tag;
            if (! tagstr.IsEmpty()) {
                tag = do_GetAtom(tagstr);
            }

            nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(condition->GetComposedDoc());
            if (! doc)
                return NS_ERROR_FAILURE;

            idnode->SetTag(tag, doc);
            continue;
        }

        TestNode* testnode = nullptr;
        nsresult rv = CompileQueryChild(condition->NodeInfo()->NameAtom(),
                                        aQuery, condition, prevnode, &testnode);
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

        nsAutoCString tagstrC;
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
    *aResult = nullptr;

    if (aParseType.EqualsLiteral(PARSE_TYPE_INTEGER)) {
        nsCOMPtr<nsIRDFInt> intLiteral;
        nsresult errorCode;
        int32_t intValue = aValue.ToInteger(&errorCode);
        if (NS_FAILED(errorCode))
            return NS_ERROR_FAILURE;
        rv = gRDFService->GetIntLiteral(intValue, getter_AddRefs(intLiteral));
        if (NS_FAILED(rv)) 
            return rv;
        intLiteral.forget(aResult);
    }
    else {
        nsCOMPtr<nsIRDFLiteral> literal;
        rv = gRDFService->GetLiteral(aValue.get(), getter_AddRefs(literal));
        if (NS_FAILED(rv)) 
            return rv;
        literal.forget(aResult);
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
    if (subject.IsEmpty()) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_TRIPLE_BAD_SUBJECT);
        return NS_OK;
    }
    if (subject[0] == char16_t('?'))
        svar = do_GetAtom(subject);
    else
        gRDFService->GetUnicodeResource(subject, getter_AddRefs(sres));

    
    nsAutoString predicate;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::predicate, predicate);

    nsCOMPtr<nsIRDFResource> pres;
    if (predicate.IsEmpty() || predicate[0] == char16_t('?')) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_TRIPLE_BAD_PREDICATE);
        return NS_OK;
    }
    gRDFService->GetUnicodeResource(predicate, getter_AddRefs(pres));

    
    nsAutoString object;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::object, object);

    nsCOMPtr<nsIAtom> ovar;
    nsCOMPtr<nsIRDFNode> onode;
    if (object.IsEmpty()) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_TRIPLE_BAD_OBJECT);
        return NS_OK;
    }

    if (object[0] == char16_t('?')) {
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

    nsRDFPropertyTestNode* testnode = nullptr;

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
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_TRIPLE_NO_VAR);
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

    if (!container.IsEmpty() && container[0] != char16_t('?')) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_MEMBER_NOCONTAINERVAR);
        return NS_OK;
    }

    nsCOMPtr<nsIAtom> containervar = do_GetAtom(container);

    
    nsAutoString child;
    aCondition->GetAttr(kNameSpaceID_None, nsGkAtoms::child, child);

    if (!child.IsEmpty() && child[0] != char16_t('?')) {
        nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_MEMBER_NOCHILDVAR);
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

    bool hasContainerTest = false;

    TestNode* prevnode = mSimpleRuleMemberTest;

    
    const nsAttrName* name;
    for (uint32_t i = 0; (name = aQueryElement->GetAttrNameAt(i)); ++i) {
        

        
        if (name->Equals(nsGkAtoms::property, kNameSpaceID_RDF) ||
            name->Equals(nsGkAtoms::instanceOf, kNameSpaceID_RDF) ||
            name->Equals(nsGkAtoms::id, kNameSpaceID_None) ||
            name->Equals(nsGkAtoms::parsetype, kNameSpaceID_None)) {
            continue;
        }

        int32_t attrNameSpaceID = name->NamespaceID();
        if (attrNameSpaceID == kNameSpaceID_XMLNS)
          continue;
        nsIAtom* attr = name->LocalName();

        nsAutoString value;
        aQueryElement->GetAttr(attrNameSpaceID, attr, value);

        TestNode* testnode = nullptr;

        if (name->Equals(nsGkAtoms::iscontainer, kNameSpaceID_None) ||
            name->Equals(nsGkAtoms::isempty, kNameSpaceID_None)) {
            
            
            
            if (hasContainerTest)
                continue;

            nsRDFConInstanceTestNode::Test iscontainer =
                nsRDFConInstanceTestNode::eDontCare;

            static nsIContent::AttrValuesArray strings[] =
              {&nsGkAtoms::_true, &nsGkAtoms::_false, nullptr};
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

void
nsXULTemplateQueryProcessorRDF::AddBindingDependency(nsXULTemplateResultRDF* aResult,
                                                     nsIRDFResource* aResource)
{
    ResultArray* arr;
    if (!mBindingDependencies.Get(aResource, &arr)) {
        arr = new ResultArray();

        mBindingDependencies.Put(aResource, arr);
    }

    int32_t index = arr->IndexOf(aResult);
    if (index == -1)
        arr->AppendElement(aResult);
}

void
nsXULTemplateQueryProcessorRDF::RemoveBindingDependency(nsXULTemplateResultRDF* aResult,
                                                        nsIRDFResource* aResource)
{
    ResultArray* arr;
    if (mBindingDependencies.Get(aResource, &arr)) {
        int32_t index = arr->IndexOf(aResult);
        if (index >= 0)
            arr->RemoveElementAt(index);
    }
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

            mMemoryElementToResultMap.Put(hash, arr);
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
            int32_t index = arr->IndexOf(aResult);
            if (index >= 0)
                arr->RemoveObjectAt(index);

            uint32_t length = arr->Count();
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
        uint32_t length = arr->Count();

        for (int32_t r = length - 1; r >= 0; r--) {
            nsXULTemplateResultRDF* result = (*arr)[r];
            if (result) {
                
                
                
                
                if (result->HasMemoryElement(aMemoryElement)) {
                    nsITemplateRDFQuery* query = result->Query();
                    if (query) {
                        nsCOMPtr<nsIDOMNode> querynode;
                        query->GetQueryNode(getter_AddRefs(querynode));

                        mBuilder->RemoveResult(result);
                    }

                    
                    if (!mMemoryElementToResultMap.Get(hash, nullptr))
                        return;

                    
                    
                    uint32_t newlength = arr->Count();
                    if (r > (int32_t)newlength)
                        r = newlength;
                }
            }
        }

        
        if (!arr->Count())
            mMemoryElementToResultMap.Remove(hash);
    }
}

int32_t
nsXULTemplateQueryProcessorRDF::GetContainerIndexOf(nsIXULTemplateResult* aResult)
{
    
    nsCOMPtr<nsISupports> ref;
    nsresult rv = aResult->GetBindingObjectFor(mRefVariable,
                                               getter_AddRefs(ref));
    if (NS_FAILED(rv) || !mDB)
        return -1;

    nsCOMPtr<nsIRDFResource> container = do_QueryInterface(ref);
    if (container) {
        
        
        bool isSequence = false;
        gRDFContainerUtils->IsSeq(mDB, container, &isSequence);
        if (isSequence) {
            nsCOMPtr<nsIRDFResource> resource;
            aResult->GetResource(getter_AddRefs(resource));
            if (resource) {
                int32_t index;
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
    if (source && mDB) {
        
        
        rv = mDB->GetTarget(source, aSortPredicate, true,
                            getter_AddRefs(value));
        if (NS_FAILED(rv))
            return rv;

        if (!value) {
            rv = mDB->GetTarget(source, aPredicate, true,
                                getter_AddRefs(value));
            if (NS_FAILED(rv))
                return rv;
        }
    }

    *aResultNode = value;
    NS_IF_ADDREF(*aResultNode);
    return NS_OK;
}
