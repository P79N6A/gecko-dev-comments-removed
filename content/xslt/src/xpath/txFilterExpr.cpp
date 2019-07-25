






































#include "txExpr.h"
#include "txNodeSet.h"
#include "txIXPathContext.h"



  
 










nsresult
FilterExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;

    nsRefPtr<txAExprResult> exprRes;
    nsresult rv = expr->evaluate(aContext, getter_AddRefs(exprRes));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(exprRes->getResultType() == txAExprResult::NODESET,
                   NS_ERROR_XSLT_NODESET_EXPECTED);

    nsRefPtr<txNodeSet> nodes =
        static_cast<txNodeSet*>(static_cast<txAExprResult*>(exprRes));
    
    exprRes = nsnull;

    nsRefPtr<txNodeSet> nonShared;
    rv = aContext->recycler()->getNonSharedNodeSet(nodes,
                                                   getter_AddRefs(nonShared));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = evaluatePredicates(nonShared, aContext);
    NS_ENSURE_SUCCESS(rv, rv);

    *aResult = nonShared;
    NS_ADDREF(*aResult);

    return NS_OK;
} 

TX_IMPL_EXPR_STUBS_BASE(FilterExpr, NODESET_RESULT)

Expr*
FilterExpr::getSubExprAt(PRUint32 aPos)
{
    if (aPos == 0) {
      return expr;
    }
    return PredicateList::getSubExprAt(aPos - 1);
}

void
FilterExpr::setSubExprAt(PRUint32 aPos, Expr* aExpr)
{
    if (aPos == 0) {
      expr.forget();
      expr = aExpr;
    }
    else {
      PredicateList::setSubExprAt(aPos - 1, aExpr);
    }
}

PRBool
FilterExpr::isSensitiveTo(ContextSensitivity aContext)
{
    return expr->isSensitiveTo(aContext) ||
           PredicateList::isSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
void
FilterExpr::toString(nsAString& str)
{
    if ( expr ) expr->toString(str);
    else str.AppendLiteral("null");
    PredicateList::toString(str);
}
#endif

