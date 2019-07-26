






#ifndef jsion_baselinecompiler_shared_h__
#define jsion_baselinecompiler_shared_h__

#include "jscntxt.h"
#include "ion/BaselineFrameInfo.h"
#include "ion/BaselineIC.h"
#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

class BaselineCompilerShared
{
  protected:
    JSContext *cx;
    JSScript *script;
    jsbytecode *pc;
    MacroAssembler masm;

    FrameInfo frame;
    js::Vector<CacheData, 16, SystemAllocPolicy> caches_;

    BaselineCompilerShared(JSContext *cx, JSScript *script);

    bool allocateCache(const BaseCache &cache) {
        JS_ASSERT(cache.data.pc == pc);
        return caches_.append(cache.data);
    }
};

} 
} 

#endif 
