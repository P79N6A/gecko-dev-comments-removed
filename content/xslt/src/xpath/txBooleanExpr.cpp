











































#include "txExpr.h"
#include "txIXPathContext.h"








nsresult
BooleanExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;

    PRBool lval;
    nsresult rv = leftExpr->evaluateToBool(aContext, lval);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    if (op == OR && lval) {
        aContext->recycler()->getBoolResult(PR_TRUE, aResult);
        
        return NS_OK;
    }
    if (op == AND && !lval) {
        aContext->recycler()->getBoolResult(PR_FALSE, aResult);

        return NS_OK;
    }

    PRBool rval;
    rv = rightExpr->evaluateToBool(aContext, rval);
    NS_ENSURE_SUCCESS(rv, rv);

    
    aContext->recycler()->getBoolResult(rval, aResult);

    return NS_OK;
} 

TX_IMPL_EXPR_STUBS_2(BooleanExpr, BOOLEAN_RESULT, leftExpr, rightExpr)

PRBool
BooleanExpr::isSensitiveTo(ContextSensitivity aContext)
{
    return leftExpr->isSensitiveTo(aContext) ||
           rightExpr->isSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
void
BooleanExpr::toString(nsAString& str)
{
    if ( leftExpr ) leftExpr->toString(str);
    else str.AppendLiteral("null");

    switch ( op ) {
        case OR:
            str.AppendLiteral(" or ");
            break;
        default:
            str.AppendLiteral(" and ");
            break;
    }
    if ( rightExpr ) rightExpr->toString(str);
    else str.AppendLiteral("null");

}
#endif
