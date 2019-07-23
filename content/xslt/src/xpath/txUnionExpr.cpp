





































#include "txExpr.h"
#include "txIXPathContext.h"
#include "txNodeSet.h"

  
 


    
  









nsresult
UnionExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;
    nsRefPtr<txNodeSet> nodes;
    nsresult rv = aContext->recycler()->getNodeSet(getter_AddRefs(nodes));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 i, len = mExpressions.Length();
    for (i = 0; i < len; ++i) {
        nsRefPtr<txAExprResult> exprResult;
        rv = mExpressions[i]->evaluate(aContext, getter_AddRefs(exprResult));
        NS_ENSURE_SUCCESS(rv, rv);

        if (exprResult->getResultType() != txAExprResult::NODESET) {
            
            return NS_ERROR_XSLT_NODESET_EXPECTED;
        }

        nsRefPtr<txNodeSet> resultSet, ownedSet;
        resultSet = NS_STATIC_CAST(txNodeSet*,
                                   NS_STATIC_CAST(txAExprResult*, exprResult));
        exprResult = nsnull;
        rv = aContext->recycler()->
            getNonSharedNodeSet(resultSet, getter_AddRefs(ownedSet));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = nodes->addAndTransfer(ownedSet);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    *aResult = nodes;
    NS_ADDREF(*aResult);

    return NS_OK;
} 

Expr::ExprType
UnionExpr::getType()
{
  return UNION_EXPR;
}

TX_IMPL_EXPR_STUBS_LIST(UnionExpr, NODESET_RESULT, mExpressions)

PRBool
UnionExpr::isSensitiveTo(ContextSensitivity aContext)
{
    PRUint32 i, len = mExpressions.Length();
    for (i = 0; i < len; ++i) {
        if (mExpressions[i]->isSensitiveTo(aContext)) {
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}

#ifdef TX_TO_STRING
void
UnionExpr::toString(nsAString& dest)
{
    PRUint32 i;
    for (i = 0; i < mExpressions.Length(); ++i) {
        if (i > 0)
            dest.AppendLiteral(" | ");
        mExpressions[i]->toString(dest);
    }
}
#endif
