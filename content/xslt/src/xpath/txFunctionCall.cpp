





































#include "txExpr.h"
#include "nsIAtom.h"
#include "txIXPathContext.h"
#include "txNodeSet.h"





  
 





double FunctionCall::evaluateToNumber(Expr* aExpr, txIEvalContext* aContext)
{
    NS_ASSERTION(aExpr, "missing expression");
    nsRefPtr<txAExprResult> exprResult;
    nsresult rv = aExpr->evaluate(aContext, getter_AddRefs(exprResult));
    if (NS_FAILED(rv))
        return Double::NaN;

    return exprResult->numberValue();
}





nsresult
FunctionCall::evaluateToNodeSet(Expr* aExpr, txIEvalContext* aContext,
                                txNodeSet** aResult)
{
    NS_ASSERTION(aExpr, "Missing expression to evaluate");
    *aResult = nsnull;

    nsRefPtr<txAExprResult> exprRes;
    nsresult rv = aExpr->evaluate(aContext, getter_AddRefs(exprRes));
    NS_ENSURE_SUCCESS(rv, rv);

    if (exprRes->getResultType() != txAExprResult::NODESET) {
        aContext->receiveError(NS_LITERAL_STRING("NodeSet expected as argument"), NS_ERROR_XSLT_NODESET_EXPECTED);
        return NS_ERROR_XSLT_NODESET_EXPECTED;
    }

    *aResult =
        static_cast<txNodeSet*>(static_cast<txAExprResult*>(exprRes));
    NS_ADDREF(*aResult);

    return NS_OK;
}

PRBool FunctionCall::requireParams(PRInt32 aParamCountMin,
                                   PRInt32 aParamCountMax,
                                   txIEvalContext* aContext)
{
    PRInt32 argc = mParams.Length();
    if (argc < aParamCountMin ||
        (aParamCountMax > -1 && argc > aParamCountMax)) {
        nsAutoString err(NS_LITERAL_STRING("invalid number of parameters for function"));
#ifdef TX_TO_STRING
        err.AppendLiteral(": ");
        toString(err);
#endif
        aContext->receiveError(err, NS_ERROR_XPATH_INVALID_ARG);

        return PR_FALSE;
    }

    return PR_TRUE;
}

Expr*
FunctionCall::getSubExprAt(PRUint32 aPos)
{
    return mParams.SafeElementAt(aPos);
}

void
FunctionCall::setSubExprAt(PRUint32 aPos, Expr* aExpr)
{
    NS_ASSERTION(aPos < mParams.Length(),
                 "setting bad subexpression index");
    mParams[aPos] = aExpr;
}

PRBool
FunctionCall::argsSensitiveTo(ContextSensitivity aContext)
{
    PRUint32 i, len = mParams.Length();
    for (i = 0; i < len; ++i) {
        if (mParams[i]->isSensitiveTo(aContext)) {
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}

#ifdef TX_TO_STRING
void
FunctionCall::toString(nsAString& aDest)
{
    nsCOMPtr<nsIAtom> functionNameAtom;
    nsAutoString functionName;
    if (NS_FAILED(getNameAtom(getter_AddRefs(functionNameAtom))) ||
        NS_FAILED(functionNameAtom->ToString(functionName))) {
        NS_ERROR("Can't get function name.");
        return;
    }

    aDest.Append(functionName);
    aDest.Append(PRUnichar('('));
    for (PRUint32 i = 0; i < mParams.Length(); ++i) {
        if (i != 0) {
            aDest.Append(PRUnichar(','));
        }
        mParams[i]->toString(aDest);
    }
    aDest.Append(PRUnichar(')'));
}
#endif
