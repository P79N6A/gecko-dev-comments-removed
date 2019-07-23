





































#include "txExecutionState.h"
#include "txAtoms.h"
#include "txSingleNodeContext.h"
#include "txXSLTFunctions.h"
#include "nsReadableUtils.h"
#include "txKey.h"
#include "txXSLTPatterns.h"
#include "txNamespaceMap.h"









txKeyFunctionCall::txKeyFunctionCall(txNamespaceMap* aMappings)
    : mMappings(aMappings)
{
}









nsresult
txKeyFunctionCall::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    if (!aContext || !requireParams(2, 2, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

    txExecutionState* es =
        static_cast<txExecutionState*>(aContext->getPrivateContext());

    nsAutoString keyQName;
    nsresult rv = mParams[0]->evaluateToString(aContext, keyQName);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName keyName;
    rv = keyName.init(keyQName, mMappings, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<txAExprResult> exprResult;
    rv = mParams[1]->evaluate(aContext, getter_AddRefs(exprResult));
    NS_ENSURE_SUCCESS(rv, rv);

    txXPathTreeWalker walker(aContext->getContextNode());
    walker.moveToRoot();

    nsRefPtr<txNodeSet> res;
    txNodeSet* nodeSet;
    if (exprResult->getResultType() == txAExprResult::NODESET &&
        (nodeSet = static_cast<txNodeSet*>
                              (static_cast<txAExprResult*>
                                          (exprResult)))->size() > 1) {
        rv = aContext->recycler()->getNodeSet(getter_AddRefs(res));
        NS_ENSURE_SUCCESS(rv, rv);

        PRInt32 i;
        for (i = 0; i < nodeSet->size(); ++i) {
            nsAutoString val;
            txXPathNodeUtils::appendNodeValue(nodeSet->get(i), val);

            nsRefPtr<txNodeSet> nodes;
            rv = es->getKeyNodes(keyName, walker.getCurrentPosition(), val,
                                 i == 0, getter_AddRefs(nodes));
            NS_ENSURE_SUCCESS(rv, rv);

            res->add(*nodes);
        }
    }
    else {
        nsAutoString val;
        exprResult->stringValue(val);
        rv = es->getKeyNodes(keyName, walker.getCurrentPosition(), val,
                             PR_TRUE, getter_AddRefs(res));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    *aResult = res;
    NS_ADDREF(*aResult);

    return NS_OK;
}

Expr::ResultType
txKeyFunctionCall::getReturnType()
{
    return NODESET_RESULT;
}

PRBool
txKeyFunctionCall::isSensitiveTo(ContextSensitivity aContext)
{
    return (aContext & NODE_CONTEXT) || argsSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
nsresult
txKeyFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    *aAtom = txXSLTAtoms::key;
    NS_ADDREF(*aAtom);
    return NS_OK;
}
#endif





DHASH_WRAPPER(txKeyValueHash, txKeyValueHashEntry, txKeyValueHashKey&)
DHASH_WRAPPER(txIndexedKeyHash, txIndexedKeyHashEntry, txIndexedKeyHashKey&)

PRBool
txKeyValueHashEntry::MatchEntry(const void* aKey) const
{
    const txKeyValueHashKey* key =
        static_cast<const txKeyValueHashKey*>(aKey);

    return mKey.mKeyName == key->mKeyName &&
           mKey.mRootIdentifier == key->mRootIdentifier &&
           mKey.mKeyValue.Equals(key->mKeyValue);
}

PLDHashNumber
txKeyValueHashEntry::HashKey(const void* aKey)
{
    const txKeyValueHashKey* key =
        static_cast<const txKeyValueHashKey*>(aKey);

    return key->mKeyName.mNamespaceID ^
           NS_PTR_TO_INT32(key->mKeyName.mLocalName.get()) ^
           key->mRootIdentifier ^
           HashString(key->mKeyValue);
}

PRBool
txIndexedKeyHashEntry::MatchEntry(const void* aKey) const
{
    const txIndexedKeyHashKey* key =
        static_cast<const txIndexedKeyHashKey*>(aKey);

    return mKey.mKeyName == key->mKeyName &&
           mKey.mRootIdentifier == key->mRootIdentifier;
}

PLDHashNumber
txIndexedKeyHashEntry::HashKey(const void* aKey)
{
    const txIndexedKeyHashKey* key =
        static_cast<const txIndexedKeyHashKey*>(aKey);

    return key->mKeyName.mNamespaceID ^
           NS_PTR_TO_INT32(key->mKeyName.mLocalName.get()) ^
           key->mRootIdentifier;
}





nsresult
txKeyHash::getKeyNodes(const txExpandedName& aKeyName,
                       const txXPathNode& aRoot,
                       const nsAString& aKeyValue,
                       PRBool aIndexIfNotFound,
                       txExecutionState& aEs,
                       txNodeSet** aResult)
{
    NS_ENSURE_TRUE(mKeyValues.mHashTable.ops && mIndexedKeys.mHashTable.ops,
                   NS_ERROR_OUT_OF_MEMORY);

    *aResult = nsnull;

    PRInt32 identifier = txXPathNodeUtils::getUniqueIdentifier(aRoot);

    txKeyValueHashKey valueKey(aKeyName, identifier, aKeyValue);
    txKeyValueHashEntry* valueEntry = mKeyValues.GetEntry(valueKey);
    if (valueEntry) {
        *aResult = valueEntry->mNodeSet;
        NS_ADDREF(*aResult);

        return NS_OK;
    }

    
    
    

    if (!aIndexIfNotFound) {
        
        
        *aResult = mEmptyNodeSet;
        NS_ADDREF(*aResult);

        return NS_OK;
    }

    txIndexedKeyHashKey indexKey(aKeyName, identifier);
    txIndexedKeyHashEntry* indexEntry = mIndexedKeys.AddEntry(indexKey);
    NS_ENSURE_TRUE(indexEntry, NS_ERROR_OUT_OF_MEMORY);

    if (indexEntry->mIndexed) {
        
        
        *aResult = mEmptyNodeSet;
        NS_ADDREF(*aResult);

        return NS_OK;
    }

    
    txXSLKey* xslKey = mKeys.get(aKeyName);
    if (!xslKey) {
        
        return NS_ERROR_INVALID_ARG;
    }

    nsresult rv = xslKey->indexSubtreeRoot(aRoot, mKeyValues, aEs);
    NS_ENSURE_SUCCESS(rv, rv);
    
    indexEntry->mIndexed = PR_TRUE;

    
    valueEntry = mKeyValues.GetEntry(valueKey);
    if (valueEntry) {
        *aResult = valueEntry->mNodeSet;
        NS_ADDREF(*aResult);
    }
    else {
        *aResult = mEmptyNodeSet;
        NS_ADDREF(*aResult);
    }

    return NS_OK;
}

nsresult
txKeyHash::init()
{
    nsresult rv = mKeyValues.Init(8);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mIndexedKeys.Init(1);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mEmptyNodeSet = new txNodeSet(nsnull);
    NS_ENSURE_TRUE(mEmptyNodeSet, NS_ERROR_OUT_OF_MEMORY);
    
    return NS_OK;
}








PRBool txXSLKey::addKey(nsAutoPtr<txPattern> aMatch, nsAutoPtr<Expr> aUse)
{
    if (!aMatch || !aUse)
        return PR_FALSE;

    Key* key = mKeys.AppendElement();
    if (!key)
        return PR_FALSE;

    key->matchPattern = aMatch;
    key->useExpr = aUse;

    return PR_TRUE;
}







nsresult txXSLKey::indexSubtreeRoot(const txXPathNode& aRoot,
                                    txKeyValueHash& aKeyValueHash,
                                    txExecutionState& aEs)
{
    txKeyValueHashKey key(mName,
                          txXPathNodeUtils::getUniqueIdentifier(aRoot),
                          EmptyString());
    return indexTree(aRoot, key, aKeyValueHash, aEs);
}









nsresult txXSLKey::indexTree(const txXPathNode& aNode,
                             txKeyValueHashKey& aKey,
                             txKeyValueHash& aKeyValueHash,
                             txExecutionState& aEs)
{
    nsresult rv = testNode(aNode, aKey, aKeyValueHash, aEs);
    NS_ENSURE_SUCCESS(rv, rv);

    
    txXPathTreeWalker walker(aNode);
    if (walker.moveToFirstAttribute()) {
        do {
            rv = testNode(walker.getCurrentPosition(), aKey, aKeyValueHash,
                          aEs);
            NS_ENSURE_SUCCESS(rv, rv);
        } while (walker.moveToNextAttribute());
        walker.moveToParent();
    }

    
    if (walker.moveToFirstChild()) {
        do {
            rv = indexTree(walker.getCurrentPosition(), aKey, aKeyValueHash,
                           aEs);
            NS_ENSURE_SUCCESS(rv, rv);
        } while (walker.moveToNextSibling());
    }

    return NS_OK;
}









nsresult txXSLKey::testNode(const txXPathNode& aNode,
                            txKeyValueHashKey& aKey,
                            txKeyValueHash& aKeyValueHash,
                            txExecutionState& aEs)
{
    nsAutoString val;
    PRUint32 currKey, numKeys = mKeys.Length();
    for (currKey = 0; currKey < numKeys; ++currKey) {
        if (mKeys[currKey].matchPattern->matches(aNode, &aEs)) {
            txSingleNodeContext *evalContext =
                new txSingleNodeContext(aNode, &aEs);
            NS_ENSURE_TRUE(evalContext, NS_ERROR_OUT_OF_MEMORY);

            nsresult rv = aEs.pushEvalContext(evalContext);
            NS_ENSURE_SUCCESS(rv, rv);

            nsRefPtr<txAExprResult> exprResult;
            rv = mKeys[currKey].useExpr->evaluate(evalContext,
                                                  getter_AddRefs(exprResult));

            delete aEs.popEvalContext();
            NS_ENSURE_SUCCESS(rv, rv);

            if (exprResult->getResultType() == txAExprResult::NODESET) {
                txNodeSet* res = static_cast<txNodeSet*>
                                            (static_cast<txAExprResult*>
                                                        (exprResult));
                PRInt32 i;
                for (i = 0; i < res->size(); ++i) {
                    val.Truncate();
                    txXPathNodeUtils::appendNodeValue(res->get(i), val);

                    aKey.mKeyValue.Assign(val);
                    txKeyValueHashEntry* entry = aKeyValueHash.AddEntry(aKey);
                    NS_ENSURE_TRUE(entry && entry->mNodeSet,
                                   NS_ERROR_OUT_OF_MEMORY);

                    if (entry->mNodeSet->isEmpty() ||
                        entry->mNodeSet->get(entry->mNodeSet->size() - 1) !=
                        aNode) {
                        entry->mNodeSet->append(aNode);
                    }
                }
            }
            else {
                exprResult->stringValue(val);

                aKey.mKeyValue.Assign(val);
                txKeyValueHashEntry* entry = aKeyValueHash.AddEntry(aKey);
                NS_ENSURE_TRUE(entry && entry->mNodeSet,
                               NS_ERROR_OUT_OF_MEMORY);

                if (entry->mNodeSet->isEmpty() ||
                    entry->mNodeSet->get(entry->mNodeSet->size() - 1) !=
                    aNode) {
                    entry->mNodeSet->append(aNode);
                }
            }
        }
    }
    
    return NS_OK;
}
