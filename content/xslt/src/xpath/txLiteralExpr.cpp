





































#include "txExpr.h"

nsresult
txLiteralExpr::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    NS_ENSURE_TRUE(mValue, NS_ERROR_OUT_OF_MEMORY);

    *aResult = mValue;
    NS_ADDREF(*aResult);

    return NS_OK;
}

static Expr::ResultType resultTypes[] =
{
    Expr::NODESET_RESULT, 
    Expr::BOOLEAN_RESULT, 
    Expr::NUMBER_RESULT,  
    Expr::STRING_RESULT,  
    Expr::RTF_RESULT      
};

Expr::ResultType
txLiteralExpr::getReturnType()
{
    return resultTypes[mValue->getResultType()];
}

Expr*
txLiteralExpr::getSubExprAt(PRUint32 aPos)
{
    return nsnull;
}
void
txLiteralExpr::setSubExprAt(PRUint32 aPos, Expr* aExpr)
{
    NS_NOTREACHED("setting bad subexpression index");
}

PRBool
txLiteralExpr::isSensitiveTo(ContextSensitivity aContext)
{
    return PR_FALSE;
}

#ifdef TX_TO_STRING
void
txLiteralExpr::toString(nsAString& aStr)
{
    switch (mValue->getResultType()) {
        case txAExprResult::NODESET:
        {
            aStr.AppendLiteral(" { Nodeset literal } ");
            return;
        }
        case txAExprResult::BOOLEAN:
        {
            if (mValue->booleanValue()) {
              aStr.AppendLiteral("true()");
            }
            else {
              aStr.AppendLiteral("false()");
            }
            return;
        }
        case txAExprResult::NUMBER:
        {
            Double::toString(mValue->numberValue(), aStr);
            return;
        }
        case txAExprResult::STRING:
        {
            StringResult* strRes =
                static_cast<StringResult*>(static_cast<txAExprResult*>
                                       (mValue));
            PRUnichar ch = '\'';
            if (strRes->mValue.FindChar(ch) != kNotFound) {
                ch = '\"';
            }
            aStr.Append(ch);
            aStr.Append(strRes->mValue);
            aStr.Append(ch);
            return;
        }
        case txAExprResult::RESULT_TREE_FRAGMENT:
        {
            aStr.AppendLiteral(" { RTF literal } ");
            return;
        }
    }
}
#endif
