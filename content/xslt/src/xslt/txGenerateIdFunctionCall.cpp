




































#include "nsGkAtoms.h"
#include "txIXPathContext.h"
#include "txNodeSet.h"
#include "txXPathTreeWalker.h"
#include "txXSLTFunctions.h"
#include "txExecutionState.h"








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

    txExecutionState* es = 
        static_cast<txExecutionState*>(aContext->getPrivateContext());
    if (!es) {
        NS_ERROR(
            "called xslt extension function \"current\" with wrong context");
        return NS_ERROR_UNEXPECTED;
    }

    nsresult rv = NS_OK;
    if (mParams.IsEmpty()) {
        StringResult* strRes;
        rv = aContext->recycler()->getStringResult(&strRes);
        NS_ENSURE_SUCCESS(rv, rv);

        txXPathNodeUtils::getXSLTId(aContext->getContextNode(),
                                    es->getSourceDocument(),
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

    txXPathNodeUtils::getXSLTId(nodes->get(0), es->getSourceDocument(),
                                strRes->mValue);

    *aResult = strRes;
 
    return NS_OK;
}

Expr::ResultType
GenerateIdFunctionCall::getReturnType()
{
    return STRING_RESULT;
}

bool
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
    *aAtom = nsGkAtoms::generateId;
    NS_ADDREF(*aAtom);
    return NS_OK;
}
#endif
