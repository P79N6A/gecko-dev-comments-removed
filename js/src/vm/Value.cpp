





#include "js/Value.h"

static const jsval JSVAL_NULL  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_NULL,      0));
static const jsval JSVAL_FALSE = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_BOOLEAN,   false));
static const jsval JSVAL_TRUE  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_BOOLEAN,   true));
static const jsval JSVAL_VOID  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_UNDEFINED, 0));

namespace JS {

const HandleValue NullHandleValue = HandleValue::fromMarkedLocation(&JSVAL_NULL);
const HandleValue UndefinedHandleValue = HandleValue::fromMarkedLocation(&JSVAL_VOID);
const HandleValue TrueHandleValue = HandleValue::fromMarkedLocation(&JSVAL_TRUE);
const HandleValue FalseHandleValue = HandleValue::fromMarkedLocation(&JSVAL_FALSE);

} 
