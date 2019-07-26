









#include "mozilla/FloatingPoint.h"

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
    return nullptr;
}

bool NumberResult::booleanValue() {
  
  
  
  return (bool)(value != 0.0 && !mozilla::IsNaN(value));
  
} 

double NumberResult::numberValue() {
    return this->value;
} 

