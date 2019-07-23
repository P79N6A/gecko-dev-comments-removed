










































#include "txExprResult.h"









NumberResult::NumberResult(double aValue, txResultRecycler* aRecycler)
    : txAExprResult(aRecycler), value(aValue)
{
} 





short NumberResult::getResultType() {
    return txAExprResult::NUMBER;
} 

void
NumberResult::stringValue(nsString& aResult)
{
    Double::toString(value, aResult);
}

const nsString*
NumberResult::stringValuePointer()
{
    return nsnull;
}

MBool NumberResult::booleanValue() {
  
  
  
  return (MBool)(value != 0.0 && !Double::isNaN(value));
  
} 

double NumberResult::numberValue() {
    return this->value;
} 

