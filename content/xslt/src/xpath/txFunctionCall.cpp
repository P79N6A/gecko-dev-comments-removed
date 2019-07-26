




#include "txExpr.h"
#include "nsIAtom.h"
#include "txIXPathContext.h"
#include "txNodeSet.h"





  
 






nsresult
FunctionCall::evaluateToNumber(Expr* aExpr, txIEvalContext* aContext,
                               double* aResult)
{
    NS_ASSERTION(aExpr, "missing expression");
    nsRefPtr<txAExprResult> exprResult;
    nsresult rv = aExpr->evaluate(aContext, getter_AddRefs(exprResult));
    NS_ENSURE_SUCCESS(rv, rv);

    *aResult = exprResult->numberValue();

    return NS_OK;
}





nsresult
FunctionCall::evaluateToNodeSet(Expr* aExpr, txIEvalContext* aContext,
                                txNodeSet** aResult)
{
    NS_ASSERTION(aExpr, "Missing expression to evaluate");
    *aResult = nullptr;

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

bool FunctionCall::requireParams(int32_t aParamCountMin,
                                   int32_t aParamCountMax,
                                   txIEvalContext* aContext)
{
    int32_t argc = mParams.Length();
    if (argc < aParamCountMin ||
        (aParamCountMax > -1 && argc > aParamCountMax)) {
        nsAutoString err(NS_LITERAL_STRING("invalid number of parameters for function"));
#ifdef TX_TO_STRING
        err.AppendLiteral(": ");
        toString(err);
#endif
        aContext->receiveError(err, NS_ERROR_XPATH_INVALID_ARG);

        return false;
    }

    return true;
}

Expr*
FunctionCall::getSubExprAt(uint32_t aPos)
{
    return mParams.SafeElementAt(aPos);
}

void
FunctionCall::setSubExprAt(uint32_t aPos, Expr* aExpr)
{
    NS_ASSERTION(aPos < mParams.Length(),
                 "setting bad subexpression index");
    mParams[aPos] = aExpr;
}

bool
FunctionCall::argsSensitiveTo(ContextSensitivity aContext)
{
    uint32_t i, len = mParams.Length();
    for (i = 0; i < len; ++i) {
        if (mParams[i]->isSensitiveTo(aContext)) {
            return true;
        }
    }

    return false;
}

#ifdef TX_TO_STRING
void
FunctionCall::toString(nsAString& aDest)
{
    nsCOMPtr<nsIAtom> functionNameAtom;
    if (NS_FAILED(getNameAtom(getter_AddRefs(functionNameAtom)))) {
        NS_ERROR("Can't get function name.");
        return;
    }



    aDest.Append(nsDependentAtomString(functionNameAtom) +
                 NS_LITERAL_STRING("("));
    for (uint32_t i = 0; i < mParams.Length(); ++i) {
        if (i != 0) {
            aDest.Append(char16_t(','));
        }
        mParams[i]->toString(aDest);
    }
    aDest.Append(char16_t(')'));
}
#endif
