






































#include "txExpr.h"

nsresult
Expr::evaluateToBool(txIEvalContext* aContext, PRBool& aResult)
{
    nsRefPtr<txAExprResult> exprRes;
    nsresult rv = evaluate(aContext, getter_AddRefs(exprRes));
    NS_ENSURE_SUCCESS(rv, rv);

    aResult = exprRes->booleanValue();

    return NS_OK;
}

nsresult
Expr::evaluateToString(txIEvalContext* aContext, nsString& aResult)
{
    nsRefPtr<txAExprResult> exprRes;
    nsresult rv = evaluate(aContext, getter_AddRefs(exprRes));
    NS_ENSURE_SUCCESS(rv, rv);

    exprRes->stringValue(aResult);

    return NS_OK;
}
