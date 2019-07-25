








































#ifndef jsion_macro_assembler_h__
#define jsion_macro_assembler_h__

#if defined(JS_CPU_X86)
# include "ion/x86/MacroAssembler-x86.h"
#elif defined(JS_CPU_X64)
# include "ion/x64/MacroAssembler-x64.h"
#elif defined(JS_CPU_ARM)
# include "ion/arm/MacroAssembler-arm.h"
#endif

namespace js {
namespace ion {

class MacroAssembler : public MacroAssemblerSpecific
{
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

  public:
    MacroAssembler()
      : autoRooter_(GetIonContext()->cx, thisFromCtor())
    {
    }

    MacroAssembler(JSContext *cx)
      : autoRooter_(cx, thisFromCtor())
    {
    }

    MoveResolver &moveResolver() {
        return moveResolver_;
    }

    size_t instructionsSize() const {
        return size();
    }

    
    
    template <typename T>
    void guardTypeSet(const T &address, types::TypeSet *types, Register scratch,
                      Label *mismatched);
};

} 
} 

#endif 

