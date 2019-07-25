








































#ifndef jsion_macro_assembler_h__
#define jsion_macro_assembler_h__

#if defined(JS_CPU_X86)
# include "ion/x86/MacroAssembler-x86.h"
#elif defined(JS_CPU_X64)
# include "ion/x64/MacroAssembler-x64.h"
#elif defined(JS_CPU_ARM)
# include "ion/arm/MacroAssembler-arm.h"
#endif

#include "jsscope.h"

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

    void loadBaseShape(Register objReg, Register dest) {
        loadPtr(Address(objReg, JSObject::offsetOfShape()), dest);
        loadPtr(Address(dest, Shape::offsetOfBase()), dest);
    }
    void loadBaseShapeClass(Register baseShapeReg, Register dest) {
        loadPtr(Address(baseShapeReg, BaseShape::offsetOfClass()), dest);
    }
    void loadObjClass(Register objReg, Register dest) {
        loadBaseShape(objReg, dest);
        loadBaseShapeClass(dest, dest);
    }

    void storeCallResult(AnyRegister dest)
    {
#if defined(JS_NUNBOX32)
        unboxValue(ValueOperand(JSReturnReg_Type, JSReturnReg_Data), dest);
#elif defined(JS_PUNBOX64)
        unboxValue(ValueOperand(JSReturnReg), dest);
#else
#error "Bad architecture"
#endif
    }

    void storeCallResult(ValueOperand dest)
    {
#if defined(JS_NUNBOX32)
        
        
        
        
        
        if (dest.typeReg() == JSReturnReg_Data) {
            if (dest.payloadReg() == JSReturnReg_Type) {
                
                mov(JSReturnReg_Type, ReturnReg);
                mov(JSReturnReg_Data, JSReturnReg_Type);
                mov(ReturnReg, JSReturnReg_Data);
            } else {
                mov(JSReturnReg_Data, dest.payloadReg());
                mov(JSReturnReg_Type, dest.typeReg());
            }
        } else {
            mov(JSReturnReg_Type, dest.typeReg());
            mov(JSReturnReg_Data, dest.payloadReg());
        }
#elif defined(JS_PUNBOX64)
        if (dest.valueReg() != JSReturnReg)
            movq(JSReturnReg, dest.valueReg());
#else
#error "Bad architecture"
#endif
    }

    void storeCallResult(TypedOrValueRegister dest)
    {
        if (dest.hasValue())
            storeCallResult(dest.valueReg());
        else
            storeCallResult(dest.typedReg());
    }

    CodeOffsetLabel labelForPatch() {
        return CodeOffsetLabel(size());
    }

    void PushRegsInMask(RegisterSet set);
    void PopRegsInMask(RegisterSet set);
};

} 
} 

#endif 

