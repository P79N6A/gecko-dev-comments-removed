





































#include "txExpr.h"
#include "txExprResult.h"
#include "txSingleNodeContext.h"

PRBool
txUnionNodeTest::matches(const txXPathNode& aNode,
                         txIMatchContext* aContext)
{
    PRUint32 i, len = mNodeTests.Length();
    for (i = 0; i < len; ++i) {
        if (mNodeTests[i]->matches(aNode, aContext)) {
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}

double
txUnionNodeTest::getDefaultPriority()
{
    NS_ERROR("Don't call getDefaultPriority on txUnionPattern");
    return Double::NaN;
}

PRBool
txUnionNodeTest::isSensitiveTo(Expr::ContextSensitivity aContext)
{
    PRUint32 i, len = mNodeTests.Length();
    for (i = 0; i < len; ++i) {
        if (mNodeTests[i]->isSensitiveTo(aContext)) {
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}

#ifdef TX_TO_STRING
void
txUnionNodeTest::toString(nsAString& aDest)
{
    aDest.AppendLiteral("(");
    for (PRUint32 i = 0; i < mNodeTests.Length(); ++i) {
        if (i != 0) {
            aDest.AppendLiteral(" | ");
        }
        mNodeTests[i]->toString(aDest);
    }
    aDest.AppendLiteral(")");
}
#endif
