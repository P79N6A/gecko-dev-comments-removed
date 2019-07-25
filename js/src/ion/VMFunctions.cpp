







































#include "Ion.h"
#include "jsinterp.h"
#include "ion/Snapshots.h"
#include "ion/IonFrames.h"

using namespace js;
using namespace js::ion;

namespace js {
namespace ion {

bool InvokeFunction(JSContext *cx, JSFunction *fun, uint32 argc, Value *argv, Value *rval)
{
    Value fval = ObjectValue(*fun);

    
    Value thisv = argv[0];
    Value *argvWithoutThis = argv + 1;

    
    bool ok = Invoke(cx, thisv, fval, argc, argvWithoutThis, rval);
    return ok;
}

bool ReportOverRecursed(JSContext *cx)
{
    js_ReportOverRecursed(cx);

    
    return false;
}

} 
} 

