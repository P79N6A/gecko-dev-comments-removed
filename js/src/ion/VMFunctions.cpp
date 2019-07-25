







































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

bool AddValues(JSContext *cx, const Value &lhs, const Value &rhs, Value *res)
{
    FrameRecovery fr = FrameRecovery::FromFrameIterator(
        IonFrameIterator(JS_THREAD_DATA(cx)->ionTop));
    SnapshotIterator si(fr);
    jsbytecode *pc = fr.script()->code + si.pcOffset();

    return js::AddValues(cx, fr.script(), pc, lhs, rhs, res);
}

} 
} 

