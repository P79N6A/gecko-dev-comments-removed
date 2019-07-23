




































#include "txAtoms.h"
#include "txIXPathContext.h"
#include "txNodeSet.h"
#include "txXPathTreeWalker.h"
#include "txXSLTFunctions.h"








GenerateIdFunctionCall::GenerateIdFunctionCall()
{
}









nsresult
GenerateIdFunctionCall::evaluate(txIEvalContext* aContext,
                                 txAExprResult** aResult)
{
    *aResult = nsnull;
    if (!requireParams(0, 1, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

    nsresult rv = NS_OK;
    if (mParams.IsEmpty()) {
        StringResult* strRes;
        rv = aContext->recycler()->getStringResult(&strRes);
        NS_ENSURE_SUCCESS(rv, rv);

        txXPathNodeUtils::getXSLTId(aContext->getContextNode(),
                                    strRes->mValue);

        *aResult = strRes;
 
        return NS_OK;
    }

    nsRefPtr<txNodeSet> nodes;
    rv = evaluateToNodeSet(mParams[0], aContext,
                           getter_AddRefs(nodes));
    NS_ENSURE_SUCCESS(rv, rv);

    if (nodes->isEmpty()) {
        aContext->recycler()->getEmptyStringResult(aResult);

        return NS_OK;
    }
    
    StringResult* strRes;
    rv = aContext->recycler()->getStringResult(&strRes);
    NS_ENSURE_SUCCESS(rv, rv);

    txXPathNodeUtils::getXSLTId(nodes->get(0), strRes->mValue);

    *aResult = strRes;
 
    return NS_OK;
}

Expr::ResultType
GenerateIdFunctionCall::getReturnType()
{
    return STRING_RESULT;
}

PRBool
GenerateIdFunctionCall::isSensitiveTo(ContextSensitivity aContext)
{
    if (mParams.IsEmpty()) {
        return !!(aContext & NODE_CONTEXT);
    }

    return argsSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
nsresult
GenerateIdFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    *aAtom = txXSLTAtoms::generateId;
    NS_ADDREF(*aAtom);
    return NS_OK;
}
#endif
