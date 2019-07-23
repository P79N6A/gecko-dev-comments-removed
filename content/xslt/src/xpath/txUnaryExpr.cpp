





































#include "txExpr.h"
#include "txIXPathContext.h"








nsresult
UnaryExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;

    nsRefPtr<txAExprResult> exprRes;
    nsresult rv = expr->evaluate(aContext, getter_AddRefs(exprRes));
    NS_ENSURE_SUCCESS(rv, rv);

    double value = exprRes->numberValue();
#ifdef HPUX
    




    return aContext->recycler()->getNumberResult(-1 * value, aResult);
#else
    return aContext->recycler()->getNumberResult(-value, aResult);
#endif
}

TX_IMPL_EXPR_STUBS_1(UnaryExpr, NODESET_RESULT, expr)

PRBool
UnaryExpr::isSensitiveTo(ContextSensitivity aContext)
{
    return expr->isSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
void
UnaryExpr::toString(nsAString& str)
{
    if (!expr)
        return;
    str.Append(PRUnichar('-'));
    expr->toString(str);
}
#endif
