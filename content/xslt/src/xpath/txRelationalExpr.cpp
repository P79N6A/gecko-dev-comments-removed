





































#include "txExpr.h"
#include "txDouble.h"
#include "txNodeSet.h"
#include "txIXPathContext.h"
#include "txXPathTreeWalker.h"




PRBool
RelationalExpr::compareResults(txIEvalContext* aContext, txAExprResult* aLeft,
                               txAExprResult* aRight)
{
    short ltype = aLeft->getResultType();
    short rtype = aRight->getResultType();
    nsresult rv = NS_OK;

    
    if (ltype == txAExprResult::NODESET) {
        if (rtype == txAExprResult::BOOLEAN) {
            BooleanResult leftBool(aLeft->booleanValue());
            return compareResults(aContext, &leftBool, aRight);
        }

        txNodeSet* nodeSet = NS_STATIC_CAST(txNodeSet*, aLeft);
        nsRefPtr<StringResult> strResult;
        rv = aContext->recycler()->getStringResult(getter_AddRefs(strResult));
        NS_ENSURE_SUCCESS(rv, rv);

        PRInt32 i;
        for (i = 0; i < nodeSet->size(); ++i) {
            strResult->mValue.Truncate();
            txXPathNodeUtils::appendNodeValue(nodeSet->get(i),
                                              strResult->mValue);
            if (compareResults(aContext, strResult, aRight)) {
                return PR_TRUE;
            }
        }

        return PR_FALSE;
    }

    
    if (rtype == txAExprResult::NODESET) {
        if (ltype == txAExprResult::BOOLEAN) {
            BooleanResult rightBool(aRight->booleanValue());
            return compareResults(aContext, aLeft, &rightBool);
        }

        txNodeSet* nodeSet = NS_STATIC_CAST(txNodeSet*, aRight);
        nsRefPtr<StringResult> strResult;
        rv = aContext->recycler()->getStringResult(getter_AddRefs(strResult));
        NS_ENSURE_SUCCESS(rv, rv);

        PRInt32 i;
        for (i = 0; i < nodeSet->size(); ++i) {
            strResult->mValue.Truncate();
            txXPathNodeUtils::appendNodeValue(nodeSet->get(i),
                                              strResult->mValue);
            if (compareResults(aContext, aLeft, strResult)) {
                return PR_TRUE;
            }
        }

        return PR_FALSE;
    }

    
    if (mOp == EQUAL || mOp == NOT_EQUAL) {
        PRBool result;
        const nsString *lString, *rString;

        
        if (ltype == txAExprResult::BOOLEAN ||
            rtype == txAExprResult::BOOLEAN) {
            result = aLeft->booleanValue() == aRight->booleanValue();
        }

        
        else if (ltype == txAExprResult::NUMBER ||
                 rtype == txAExprResult::NUMBER) {
            double lval = aLeft->numberValue();
            double rval = aRight->numberValue();
            result = TX_DOUBLE_COMPARE(lval, ==, rval);
        }

        
        
        else if ((lString = aLeft->stringValuePointer())) {
            if ((rString = aRight->stringValuePointer())) {
                result = lString->Equals(*rString);
            }
            else {
                nsAutoString rStr;
                aRight->stringValue(rStr);
                result = lString->Equals(rStr);
            }
        }
        else if ((rString = aRight->stringValuePointer())) {
            nsAutoString lStr;
            aLeft->stringValue(lStr);
            result = rString->Equals(lStr);
        }
        else {
            nsAutoString lStr, rStr;
            aLeft->stringValue(lStr);
            aRight->stringValue(rStr);
            result = lStr.Equals(rStr);
        }

        return mOp == EQUAL ? result : !result;
    }

    double leftDbl = aLeft->numberValue();
    double rightDbl = aRight->numberValue();
    switch (mOp) {
        case LESS_THAN:
        {
            return TX_DOUBLE_COMPARE(leftDbl, <, rightDbl);
        }
        case LESS_OR_EQUAL:
        {
            return TX_DOUBLE_COMPARE(leftDbl, <=, rightDbl);
        }
        case GREATER_THAN:
        {
            return TX_DOUBLE_COMPARE(leftDbl, >, rightDbl);
        }
        case GREATER_OR_EQUAL:
        {
            return TX_DOUBLE_COMPARE(leftDbl, >=, rightDbl);
        }
        default:
        {
            NS_NOTREACHED("We should have caught all cases");
        }
    }

    return PR_FALSE;
}

nsresult
RelationalExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    *aResult = nsnull;
    nsRefPtr<txAExprResult> lResult;
    nsresult rv = mLeftExpr->evaluate(aContext, getter_AddRefs(lResult));
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<txAExprResult> rResult;
    rv = mRightExpr->evaluate(aContext, getter_AddRefs(rResult));
    NS_ENSURE_SUCCESS(rv, rv);
    
    aContext->recycler()->
        getBoolResult(compareResults(aContext, lResult, rResult), aResult);

    return NS_OK;
}

TX_IMPL_EXPR_STUBS_2(RelationalExpr, BOOLEAN_RESULT, mLeftExpr, mRightExpr)

PRBool
RelationalExpr::isSensitiveTo(ContextSensitivity aContext)
{
    return mLeftExpr->isSensitiveTo(aContext) ||
           mRightExpr->isSensitiveTo(aContext);
}

#ifdef TX_TO_STRING
void
RelationalExpr::toString(nsAString& str)
{
    mLeftExpr->toString(str);

    switch (mOp) {
        case NOT_EQUAL:
            str.AppendLiteral("!=");
            break;
        case LESS_THAN:
            str.Append(PRUnichar('<'));
            break;
        case LESS_OR_EQUAL:
            str.AppendLiteral("<=");
            break;
        case GREATER_THAN :
            str.Append(PRUnichar('>'));
            break;
        case GREATER_OR_EQUAL:
            str.AppendLiteral(">=");
            break;
        default:
            str.Append(PRUnichar('='));
            break;
    }

    mRightExpr->toString(str);
}
#endif
