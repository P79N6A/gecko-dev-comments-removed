




#include "txNodeSorter.h"
#include "txExecutionState.h"
#include "txXPathResultComparator.h"
#include "nsGkAtoms.h"
#include "txNodeSetContext.h"
#include "txExpr.h"
#include "txStringUtils.h"
#include "prmem.h"
#include "nsQuickSort.h"





txNodeSorter::txNodeSorter() : mNKeys(0)
{
}

txNodeSorter::~txNodeSorter()
{
    txListIterator iter(&mSortKeys);
    while (iter.hasNext()) {
        SortKey* key = (SortKey*)iter.next();
        delete key->mComparator;
        delete key;
    }
}

nsresult
txNodeSorter::addSortElement(Expr* aSelectExpr, Expr* aLangExpr,
                             Expr* aDataTypeExpr, Expr* aOrderExpr,
                             Expr* aCaseOrderExpr, txIEvalContext* aContext)
{
    nsAutoPtr<SortKey> key(new SortKey);
    NS_ENSURE_TRUE(key, NS_ERROR_OUT_OF_MEMORY);
    nsresult rv = NS_OK;

    
    key->mExpr = aSelectExpr;

    
    bool ascending = true;
    if (aOrderExpr) {
        nsAutoString attrValue;
        rv = aOrderExpr->evaluateToString(aContext, attrValue);
        NS_ENSURE_SUCCESS(rv, rv);

        if (TX_StringEqualsAtom(attrValue, nsGkAtoms::descending)) {
            ascending = false;
        }
        else if (!TX_StringEqualsAtom(attrValue, nsGkAtoms::ascending)) {
            
            return NS_ERROR_XSLT_BAD_VALUE;
        }
    }


    
    nsAutoString dataType;
    if (aDataTypeExpr) {
        rv = aDataTypeExpr->evaluateToString(aContext, dataType);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!aDataTypeExpr || TX_StringEqualsAtom(dataType, nsGkAtoms::text)) {
        
        
        
        nsAutoString lang;
        if (aLangExpr) {
            rv = aLangExpr->evaluateToString(aContext, lang);
            NS_ENSURE_SUCCESS(rv, rv);
        }

        
        bool upperFirst = false;
        if (aCaseOrderExpr) {
            nsAutoString attrValue;

            rv = aCaseOrderExpr->evaluateToString(aContext, attrValue);
            NS_ENSURE_SUCCESS(rv, rv);

            if (TX_StringEqualsAtom(attrValue, nsGkAtoms::upperFirst)) {
                upperFirst = true;
            }
            else if (!TX_StringEqualsAtom(attrValue,
                                          nsGkAtoms::lowerFirst)) {
                
                return NS_ERROR_XSLT_BAD_VALUE;
            }
        }

        key->mComparator = new txResultStringComparator(ascending,
                                                        upperFirst,
                                                        lang);
        NS_ENSURE_TRUE(key->mComparator, NS_ERROR_OUT_OF_MEMORY);
    }
    else if (TX_StringEqualsAtom(dataType, nsGkAtoms::number)) {
        
        key->mComparator = new txResultNumberComparator(ascending);
        NS_ENSURE_TRUE(key->mComparator, NS_ERROR_OUT_OF_MEMORY);
    }
    else {
        
        return NS_ERROR_XSLT_BAD_VALUE;
    }

    
    rv = mSortKeys.add(key);
    NS_ENSURE_SUCCESS(rv, rv);

    key.forget();
    mNKeys++;

    return NS_OK;
}

nsresult
txNodeSorter::sortNodeSet(txNodeSet* aNodes, txExecutionState* aEs,
                          txNodeSet** aResult)
{
    if (mNKeys == 0 || aNodes->isEmpty()) {
        NS_ADDREF(*aResult = aNodes);

        return NS_OK;
    }

    *aResult = nullptr;

    nsRefPtr<txNodeSet> sortedNodes;
    nsresult rv = aEs->recycler()->getNodeSet(getter_AddRefs(sortedNodes));
    NS_ENSURE_SUCCESS(rv, rv);

    txNodeSetContext* evalContext = new txNodeSetContext(aNodes, aEs);
    NS_ENSURE_TRUE(evalContext, NS_ERROR_OUT_OF_MEMORY);

    rv = aEs->pushEvalContext(evalContext);
    NS_ENSURE_SUCCESS(rv, rv);

    
    uint32_t len = static_cast<uint32_t>(aNodes->size());

    
    uint32_t itemSize = sizeof(uint32_t) + mNKeys * sizeof(txObject*);
    if (mNKeys > (UINT32_MAX - sizeof(uint32_t)) / sizeof(txObject*) ||
        len >= UINT32_MAX / itemSize) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    void* mem = PR_Malloc(len * itemSize);
    NS_ENSURE_TRUE(mem, NS_ERROR_OUT_OF_MEMORY);

    uint32_t* indexes = static_cast<uint32_t*>(mem);
    txObject** sortValues = reinterpret_cast<txObject**>(indexes + len);

    uint32_t i;
    for (i = 0; i < len; ++i) {
        indexes[i] = i;
    }
    memset(sortValues, 0, len * mNKeys * sizeof(txObject*));

    
    SortData sortData;
    sortData.mNodeSorter = this;
    sortData.mContext = evalContext;
    sortData.mSortValues = sortValues;
    sortData.mRv = NS_OK;
    NS_QuickSort(indexes, len, sizeof(uint32_t), compareNodes, &sortData);

    
    
    uint32_t numSortValues = len * mNKeys;
    for (i = 0; i < numSortValues; ++i) {
        delete sortValues[i];
    }

    if (NS_FAILED(sortData.mRv)) {
        PR_Free(mem);
        
        return sortData.mRv;
    }

    
    for (i = 0; i < len; ++i) {
        rv = sortedNodes->append(aNodes->get(indexes[i]));
        if (NS_FAILED(rv)) {
            PR_Free(mem);
            
            return rv;
        }
    }

    PR_Free(mem);
    delete aEs->popEvalContext();

    NS_ADDREF(*aResult = sortedNodes);

    return NS_OK;
}


int
txNodeSorter::compareNodes(const void* aIndexA, const void* aIndexB,
                           void* aSortData)
{
    SortData* sortData = static_cast<SortData*>(aSortData);
    NS_ENSURE_SUCCESS(sortData->mRv, -1);

    txListIterator iter(&sortData->mNodeSorter->mSortKeys);
    uint32_t indexA = *static_cast<const uint32_t*>(aIndexA);
    uint32_t indexB = *static_cast<const uint32_t*>(aIndexB);
    txObject** sortValuesA = sortData->mSortValues +
                             indexA * sortData->mNodeSorter->mNKeys;
    txObject** sortValuesB = sortData->mSortValues +
                             indexB * sortData->mNodeSorter->mNKeys;

    unsigned int i;
    
    for (i = 0; i < sortData->mNodeSorter->mNKeys; ++i) {
        SortKey* key = (SortKey*)iter.next();
        
        if (!sortValuesA[i] &&
            !calcSortValue(sortValuesA[i], key, sortData, indexA)) {
            return -1;
        }
        if (!sortValuesB[i] &&
            !calcSortValue(sortValuesB[i], key, sortData, indexB)) {
            return -1;
        }

        
        int compRes = key->mComparator->compareValues(sortValuesA[i],
                                                      sortValuesB[i]);
        if (compRes != 0)
            return compRes;
    }
    

    return indexA - indexB;
}


bool
txNodeSorter::calcSortValue(txObject*& aSortValue, SortKey* aKey,
                            SortData* aSortData, uint32_t aNodeIndex)
{
    aSortData->mContext->setPosition(aNodeIndex + 1); 

    nsresult rv = aKey->mComparator->createSortableValue(aKey->mExpr,
                                                         aSortData->mContext,
                                                         aSortValue);
    if (NS_FAILED(rv)) {
        aSortData->mRv = rv;
        return false;
    }

    return true;
}
