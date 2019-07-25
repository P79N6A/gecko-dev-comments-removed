





































#include "txExpr.h"
#include "txExprResult.h"
#include "txSingleNodeContext.h"

txPredicatedNodeTest::txPredicatedNodeTest(txNodeTest* aNodeTest,
                                           Expr* aPredicate)
    : mNodeTest(aNodeTest),
      mPredicate(aPredicate)
{
    NS_ASSERTION(!mPredicate->isSensitiveTo(Expr::NODESET_CONTEXT),
                 "predicate must not be context-nodeset-sensitive");
}

PRBool
txPredicatedNodeTest::matches(const txXPathNode& aNode,
                              txIMatchContext* aContext)
{
    if (!mNodeTest->matches(aNode, aContext)) {
        return PR_FALSE;
    }

    txSingleNodeContext context(aNode, aContext);
    nsRefPtr<txAExprResult> res;
    nsresult rv = mPredicate->evaluate(&context, getter_AddRefs(res));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    return res->booleanValue();
}

double
txPredicatedNodeTest::getDefaultPriority()
{
    return 0.5;
}

PRBool
txPredicatedNodeTest::isSensitiveTo(Expr::ContextSensitivity aContext)
{
    return mNodeTest->isSensitiveTo(aContext) ||
           mPredicate->isSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
void
txPredicatedNodeTest::toString(nsAString& aDest)
{
    mNodeTest->toString(aDest);
    aDest.Append(PRUnichar('['));
    mPredicate->toString(aDest);
    aDest.Append(PRUnichar(']'));
}
#endif
