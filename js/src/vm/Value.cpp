





#include "js/Value.h"

const jsval JSVAL_NULL  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_NULL,      0));
const jsval JSVAL_ZERO  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_INT32,     0));
const jsval JSVAL_ONE   = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_INT32,     1));
const jsval JSVAL_FALSE = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_BOOLEAN,   false));
const jsval JSVAL_TRUE  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_BOOLEAN,   true));
const jsval JSVAL_VOID  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_UNDEFINED, 0));

namespace JS {

const HandleValue NullHandleValue = HandleValue::fromMarkedLocation(&JSVAL_NULL);
const HandleValue UndefinedHandleValue = HandleValue::fromMarkedLocation(&JSVAL_VOID);

} 
