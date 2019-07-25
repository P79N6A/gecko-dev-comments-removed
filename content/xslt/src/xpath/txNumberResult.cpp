










































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
    txDouble::toString(value, aResult);
}

const nsString*
NumberResult::stringValuePointer()
{
    return nsnull;
}

bool NumberResult::booleanValue() {
  
  
  
  return (bool)(value != 0.0 && !txDouble::isNaN(value));
  
} 

double NumberResult::numberValue() {
    return this->value;
} 

