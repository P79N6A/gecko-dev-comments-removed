





































#include "txError.h"
#include "txExpr.h"
#include "nsString.h"
#include "txIXPathContext.h"

nsresult
txErrorExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;

    nsAutoString err(NS_LITERAL_STRING("Invalid expression evaluated"));
#ifdef TX_TO_STRING
    err.AppendLiteral(": ");
    toString(err);
#endif
    aContext->receiveError(err,
                           NS_ERROR_XPATH_INVALID_EXPRESSION_EVALUATED);

    return NS_ERROR_XPATH_INVALID_EXPRESSION_EVALUATED;
}

TX_IMPL_EXPR_STUBS_0(txErrorExpr, ANY_RESULT)

PRBool
txErrorExpr::isSensitiveTo(ContextSensitivity aContext)
{
    
    
    return PR_TRUE;
}

#ifdef TX_TO_STRING
void
txErrorExpr::toString(nsAString& aStr)
{
    aStr.Append(mStr);
}
#endif
