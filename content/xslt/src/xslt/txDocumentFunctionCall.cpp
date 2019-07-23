










































#include "txAtoms.h"
#include "txIXPathContext.h"
#include "txXSLTFunctions.h"
#include "txExecutionState.h"
#include "txURIUtils.h"




DocumentFunctionCall::DocumentFunctionCall(const nsAString& aBaseURI)
    : mBaseURI(aBaseURI)
{
}

static void
retrieveNode(txExecutionState* aExecutionState, const nsAString& aUri,
             const nsAString& aBaseUri, txNodeSet* aNodeSet)
{
    nsAutoString absUrl;
    URIUtils::resolveHref(aUri, aBaseUri, absUrl);

    PRInt32 hash = absUrl.RFindChar(PRUnichar('#'));
    PRUint32 urlEnd, fragStart, fragEnd;
    if (hash == kNotFound) {
        urlEnd = absUrl.Length();
        fragStart = 0;
        fragEnd = 0;
    }
    else {
        urlEnd = hash;
        fragStart = hash + 1;
        fragEnd = absUrl.Length();
    }

    nsDependentSubstring docUrl(absUrl, 0, urlEnd);
    nsDependentSubstring frag(absUrl, fragStart, fragEnd);

    const txXPathNode* loadNode = aExecutionState->retrieveDocument(docUrl);
    if (loadNode) {
        if (frag.IsEmpty()) {
            aNodeSet->add(*loadNode);
        }
        else {
            txXPathTreeWalker walker(*loadNode);
            if (walker.moveToElementById(frag)) {
                aNodeSet->add(walker.getCurrentPosition());
            }
        }
    }
}








nsresult
DocumentFunctionCall::evaluate(txIEvalContext* aContext,
                               txAExprResult** aResult)
{
    *aResult = nsnull;
    txExecutionState* es =
        static_cast<txExecutionState*>(aContext->getPrivateContext());

    nsRefPtr<txNodeSet> nodeSet;
    nsresult rv = aContext->recycler()->getNodeSet(getter_AddRefs(nodeSet));
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!requireParams(1, 2, aContext)) {
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;
    }

    nsRefPtr<txAExprResult> exprResult1;
    rv = mParams[0]->evaluate(aContext, getter_AddRefs(exprResult1));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString baseURI;
    MBool baseURISet = MB_FALSE;

    if (mParams.Length() == 2) {
        
        
        nsRefPtr<txNodeSet> nodeSet2;
        rv = evaluateToNodeSet(mParams[1],
                               aContext, getter_AddRefs(nodeSet2));
        NS_ENSURE_SUCCESS(rv, rv);

        
        
        
        baseURISet = MB_TRUE;

        if (!nodeSet2->isEmpty()) {
            txXPathNodeUtils::getBaseURI(nodeSet2->get(0), baseURI);
        }
    }

    if (exprResult1->getResultType() == txAExprResult::NODESET) {
        
        txNodeSet* nodeSet1 = static_cast<txNodeSet*>
                                         (static_cast<txAExprResult*>
                                                     (exprResult1));
        PRInt32 i;
        for (i = 0; i < nodeSet1->size(); ++i) {
            const txXPathNode& node = nodeSet1->get(i);
            nsAutoString uriStr;
            txXPathNodeUtils::appendNodeValue(node, uriStr);
            if (!baseURISet) {
                
                
                txXPathNodeUtils::getBaseURI(node, baseURI);
            }
            retrieveNode(es, uriStr, baseURI, nodeSet);
        }
        
        NS_ADDREF(*aResult = nodeSet);
        
        return NS_OK;
    }

    
    nsAutoString uriStr;
    exprResult1->stringValue(uriStr);
    const nsAString* base = baseURISet ? &baseURI : &mBaseURI;
    retrieveNode(es, uriStr, *base, nodeSet);

    NS_ADDREF(*aResult = nodeSet);

    return NS_OK;
}

Expr::ResultType
DocumentFunctionCall::getReturnType()
{
    return NODESET_RESULT;
}

PRBool
DocumentFunctionCall::isSensitiveTo(ContextSensitivity aContext)
{
    return (aContext & PRIVATE_CONTEXT) || argsSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
nsresult
DocumentFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    *aAtom = txXSLTAtoms::document;
    NS_ADDREF(*aAtom);
    return NS_OK;
}
#endif
