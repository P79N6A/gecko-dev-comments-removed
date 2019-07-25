








































#ifndef jsion_mirgen_h__
#define jsion_mirgen_h__



#include <stdarg.h>

#include "jscntxt.h"
#include "jscompartment.h"
#include "IonAllocPolicy.h"
#include "IonCompartment.h"
#include "CompileInfo.h"

namespace js {
namespace ion {

class MBasicBlock;
class MIRGraph;
class MStart;

class MIRGenerator
{
  public:
    MIRGenerator(JSContext *cx, TempAllocator &temp, MIRGraph &graph, CompileInfo &info);

    TempAllocator &temp() {
        return temp_;
    }
    MIRGraph &graph() {
        return graph_;
    }
    bool ensureBallast() {
        return temp().ensureBallast();
    }
    IonCompartment *ionCompartment() const {
        return cx->compartment->ionCompartment();
    }
    CompileInfo &info() {
        return info_;
    }

    template <typename T>
    T * allocate(size_t count = 1) {
        return reinterpret_cast<T *>(temp().allocate(sizeof(T) * count));
    }

    
    
    bool abort(const char *message, ...);
    bool abortFmt(const char *message, va_list ap);

    bool errored() const {
        return error_;
    }

  public:
    JSContext *cx;

  protected:
    CompileInfo &info_;
    TempAllocator &temp_;
    JSFunction *fun_;
    uint32 nslots_;
    MIRGraph &graph_;
    bool error_;
};

} 
} 

#endif 

