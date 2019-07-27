





#include "js/Value.h"

static const JS::Value JSVAL_NULL  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_NULL,      0));
static const JS::Value JSVAL_FALSE = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_BOOLEAN,   false));
static const JS::Value JSVAL_TRUE  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_BOOLEAN,   true));
static const JS::Value JSVAL_VOID  = IMPL_TO_JSVAL(BUILD_JSVAL(JSVAL_TAG_UNDEFINED, 0));

namespace JS {

const HandleValue NullHandleValue = HandleValue::fromMarkedLocation(&JSVAL_NULL);
const HandleValue UndefinedHandleValue = HandleValue::fromMarkedLocation(&JSVAL_VOID);
const HandleValue TrueHandleValue = HandleValue::fromMarkedLocation(&JSVAL_TRUE);
const HandleValue FalseHandleValue = HandleValue::fromMarkedLocation(&JSVAL_FALSE);

} 
