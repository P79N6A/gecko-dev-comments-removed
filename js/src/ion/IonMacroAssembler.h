








































#ifndef jsion_macro_assembler_h__
#define jsion_macro_assembler_h__

#ifdef JS_CPU_X86
# include "ion/x86/MacroAssembler-x86.h"
#elif JS_CPU_X64
# include "ion/x64/MacroAssembler-x64.h"
#endif
#include "MoveResolver.h"

namespace js {
namespace ion {

class MacroAssembler : public MacroAssemblerSpecific
{
  private:
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
};

} 
} 

#endif 

