





































#include "txExpr.h"
#include "txNodeSet.h"
#include "txIXPathContext.h"
#include "txXPathTreeWalker.h"








nsresult
RootExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    txXPathTreeWalker walker(aContext->getContextNode());
    walker.moveToRoot();
    
    return aContext->recycler()->getNodeSet(walker.getCurrentPosition(),
                                            aResult);
}

TX_IMPL_EXPR_STUBS_0(RootExpr, NODESET_RESULT)

PRBool
RootExpr::isSensitiveTo(ContextSensitivity aContext)
{
    return !!(aContext & NODE_CONTEXT);
}

#ifdef TX_TO_STRING
void
RootExpr::toString(nsAString& dest)
{
    if (mSerialize)
        dest.Append(PRUnichar('/'));
}
#endif
