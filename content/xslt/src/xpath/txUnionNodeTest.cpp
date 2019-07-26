





#include "mozilla/FloatingPoint.h"

#include "txExpr.h"
#include "txExprResult.h"
#include "txSingleNodeContext.h"

bool
txUnionNodeTest::matches(const txXPathNode& aNode,
                         txIMatchContext* aContext)
{
    uint32_t i, len = mNodeTests.Length();
    for (i = 0; i < len; ++i) {
        if (mNodeTests[i]->matches(aNode, aContext)) {
            return true;
        }
    }

    return false;
}

double
txUnionNodeTest::getDefaultPriority()
{
    NS_ERROR("Don't call getDefaultPriority on txUnionPattern");
    return MOZ_DOUBLE_NaN();
}

bool
txUnionNodeTest::isSensitiveTo(Expr::ContextSensitivity aContext)
{
    uint32_t i, len = mNodeTests.Length();
    for (i = 0; i < len; ++i) {
        if (mNodeTests[i]->isSensitiveTo(aContext)) {
            return true;
        }
    }

    return false;
}

#ifdef TX_TO_STRING
void
txUnionNodeTest::toString(nsAString& aDest)
{
    aDest.AppendLiteral("(");
    for (uint32_t i = 0; i < mNodeTests.Length(); ++i) {
        if (i != 0) {
            aDest.AppendLiteral(" | ");
        }
        mNodeTests[i]->toString(aDest);
    }
    aDest.AppendLiteral(")");
}
#endif
