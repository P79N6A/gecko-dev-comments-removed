








































#ifndef jsion_mirgen_h__
#define jsion_mirgen_h__




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
    MIRGenerator(JSContext *cx, TempAllocator &temp, JSScript *script, JSFunction *fun,
                 MIRGraph &graph);

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
    const CompileInfo &info() const {
        return info_;
    }

    template <typename T>
    T * allocate(size_t count = 1)
    {
        return reinterpret_cast<T *>(temp().allocate(sizeof(T) * count));
    }

    
    
    bool abort(const char *message, ...);

    bool errored() const {
        return error_;
    }

  public:
    JSContext *cx;

  protected:
    CompileInfo info_;
    TempAllocator &temp_;
    JSFunction *fun_;
    uint32 nslots_;
    MIRGraph &graph_;
    bool error_;
};

} 
} 

#endif 

