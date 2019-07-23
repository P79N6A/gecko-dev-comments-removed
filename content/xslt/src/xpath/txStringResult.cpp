









































#include "txExprResult.h"




StringResult::StringResult(txResultRecycler* aRecycler)
    : txAExprResult(aRecycler)
{
}





StringResult::StringResult(const nsAString& aValue, txResultRecycler* aRecycler)
    : txAExprResult(aRecycler), mValue(aValue)
{
}





short StringResult::getResultType() {
    return txAExprResult::STRING;
} 

void
StringResult::stringValue(nsString& aResult)
{
    aResult.Append(mValue);
}

const nsString*
StringResult::stringValuePointer()
{
    return &mValue;
}

MBool StringResult::booleanValue() {
   return !mValue.IsEmpty();
} 

double StringResult::numberValue() {
    return Double::toDouble(mValue);
} 

