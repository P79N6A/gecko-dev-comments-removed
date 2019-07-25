





































#include "txExpr.h"
#include <math.h>
#include "txIXPathContext.h"

nsresult
txNumberExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;

    nsRefPtr<txAExprResult> exprRes;
    nsresult rv = mRightExpr->evaluate(aContext, getter_AddRefs(exprRes));
    NS_ENSURE_SUCCESS(rv, rv);

    double rightDbl = exprRes->numberValue();

    rv = mLeftExpr->evaluate(aContext, getter_AddRefs(exprRes));
    NS_ENSURE_SUCCESS(rv, rv);

    double leftDbl = exprRes->numberValue();
    double result = 0;

    switch (mOp) {
        case ADD:
            result = leftDbl + rightDbl;
            break;

        case SUBTRACT:
            result = leftDbl - rightDbl;
            break;

        case DIVIDE:
            if (rightDbl == 0) {
#if defined(XP_WIN)
                
                if (Double::isNaN(rightDbl))
                    result = Double::NaN;
                else
#endif
                if (leftDbl == 0 || Double::isNaN(leftDbl))
                    result = Double::NaN;
                else if (Double::isNeg(leftDbl) ^ Double::isNeg(rightDbl))
                    result = Double::NEGATIVE_INFINITY;
                else
                    result = Double::POSITIVE_INFINITY;
            }
            else
                result = leftDbl / rightDbl;
            break;

        case MODULUS:
            if (rightDbl == 0) {
                result = Double::NaN;
            }
            else {
#if defined(XP_WIN)
                
                if (!Double::isInfinite(leftDbl) && Double::isInfinite(rightDbl))
                    result = leftDbl;
                else
#endif
                result = fmod(leftDbl, rightDbl);
            }
            break;

        case MULTIPLY:
            result = leftDbl * rightDbl;
            break;
    }

    return aContext->recycler()->getNumberResult(result, aResult);
} 

TX_IMPL_EXPR_STUBS_2(txNumberExpr, NUMBER_RESULT, mLeftExpr, mRightExpr)

PRBool
txNumberExpr::isSensitiveTo(ContextSensitivity aContext)
{
    return mLeftExpr->isSensitiveTo(aContext) ||
           mRightExpr->isSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
void
txNumberExpr::toString(nsAString& str)
{
    mLeftExpr->toString(str);

    switch (mOp) {
        case ADD:
            str.AppendLiteral(" + ");
            break;
        case SUBTRACT:
            str.AppendLiteral(" - ");
            break;
        case DIVIDE:
            str.AppendLiteral(" div ");
            break;
        case MODULUS:
            str.AppendLiteral(" mod ");
            break;
        case MULTIPLY:
            str.AppendLiteral(" * ");
            break;
    }

    mRightExpr->toString(str);

}
#endif
