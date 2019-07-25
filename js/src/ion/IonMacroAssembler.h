








































#ifndef jsion_macro_assembler_h__
#define jsion_macro_assembler_h__

#if defined(JS_CPU_X86)
# include "ion/x86/MacroAssembler-x86.h"
#elif defined(JS_CPU_X64)
# include "ion/x64/MacroAssembler-x64.h"
#elif defined(JS_CPU_ARM)
# include "ion/arm/MacroAssembler-arm.h"
#endif
#include "MoveResolver.h"

namespace js {
namespace ion {

class MacroAssembler : public MacroAssemblerSpecific
{
    typedef MoveResolver::MoveOperand MoveOperand;
    typedef MoveResolver::Move Move;

    MacroAssembler *thisFromCtor() {
        return this;
    }

  public:
    class AutoRooter : public AutoGCRooter
    {
        MacroAssembler *masm_;

      public:
        AutoRooter(JSContext *cx, MacroAssembler *masm)
          : AutoGCRooter(cx, IONMASM),
            masm_(masm)
        {
        }

        MacroAssembler *masm() const {
            return masm_;
        }
    };

    AutoRooter autoRooter_;
    MoveResolver moveResolver_;

    
    
    uint32 stackAdjust_;
    bool dynamicAlignment_;
    bool inCall_;

    bool enoughMemory_;

  public:
    MacroAssembler()
      : autoRooter_(GetIonContext()->cx, thisFromCtor()),
        stackAdjust_(0),
        inCall_(false),
        enoughMemory_(true)
    {
    }

    MacroAssembler(JSContext *cx)
      : autoRooter_(cx, thisFromCtor()),
        stackAdjust_(0),
        inCall_(false),
        enoughMemory_(true)
    {
    }

    MoveResolver &moveResolver() {
        return moveResolver_;
    }

    size_t instructionsSize() const {
        return size();
    }

    bool oom() const {
        return MacroAssemblerSpecific::oom() || !enoughMemory_;
    }

    
    
    
    
    
    
    
    void setupAlignedABICall(uint32 args);

    
    
    void setupUnalignedABICall(uint32 args, const Register &scratch);

    
    
    
    
    
    void setABIArg(uint32 arg, const Register &reg);

    
    void callWithABI(void *fun);

    
    
    void guardTypeSet(const Address &address, types::TypeSet *types, Register scratch,
                      Label *mismatched);
};

} 
} 

#endif 

