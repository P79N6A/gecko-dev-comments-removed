









































#include "txExprResult.h"





BooleanResult::BooleanResult(PRBool boolean)
    : txAExprResult(nsnull)
{
    this->value = boolean;
} 





short BooleanResult::getResultType() {
    return txAExprResult::BOOLEAN;
} 

void
BooleanResult::stringValue(nsString& aResult)
{
    if (value) {
        aResult.AppendLiteral("true");
    }
    else {
        aResult.AppendLiteral("false");
    }
}

const nsString*
BooleanResult::stringValuePointer()
{
    
    
    
    return nsnull;
}

MBool BooleanResult::booleanValue() {
   return this->value;
} 

double BooleanResult::numberValue() {
    return ( value ) ? 1.0 : 0.0;
} 
