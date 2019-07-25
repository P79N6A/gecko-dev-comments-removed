








































#ifndef jsion_coderef_h__
#define jsion_coderef_h__

#include "jscell.h"

namespace JSC {
    class ExecutablePool;
}

namespace js {
namespace ion {

class IonCode : public gc::Cell
{
    uint8 *code_;
    uint32 size_;
    JSC::ExecutablePool *pool_;
    uint32 padding_;

    IonCode()
      : code_(NULL),
        pool_(NULL)
    { }
    IonCode(uint8 *code, uint32 size, JSC::ExecutablePool *pool)
      : code_(code),
        size_(size),
        pool_(pool)
    { }

  public:
    uint8 *code() const {
        return code_;
    }
    uint32 size() const {
        return size_;
    }
    void finalize(JSContext *cx);

    
    
    
    static IonCode *New(JSContext *cx, uint8 *code, uint32 size, JSC::ExecutablePool *pool);
};

#define ION_DISABLED_SCRIPT ((IonScript *)0x1)


struct IonScript
{
    IonCode *method;

  private:
    void trace(JSTracer *trc, JSScript *script);

  public:
    static void Trace(JSTracer *trc, JSScript *script);
    static void Destroy(JSContext *cx, JSScript *script);
};

}
}

#endif 

