








































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

    void loadJSContext(JSRuntime *runtime, const Register &dest) {
        movePtr(ImmWord(runtime), dest);
        loadPtr(Address(dest, offsetof(JSRuntime, ionJSContext)), dest);
    }

    void loadTypedOrValue(Address address, TypedOrValueRegister dest)
    {
        if (dest.hasValue())
            loadValue(address, dest.valueReg());
        else
            loadUnboxedValue(address, dest.typedReg());
    }

    void storeTypedOrValue(TypedOrValueRegister src, Address address)
    {
        if (src.hasValue())
            storeValue(src.valueReg(), address);
        else if (src.type() == MIRType_Double)
            storeDouble(src.typedReg().fpu(), address);
        else
            storeValue(ValueTypeFromMIRType(src.type()), src.typedReg().gpr(), address);
    }

    void storeConstantOrRegister(ConstantOrRegister src, Address address)
    {
        if (src.constant())
            storeValue(src.value(), address);
        else
            storeTypedOrValue(src.reg(), address);
    }

    void storeCallResult(Register reg)
    {
        if (reg != ReturnReg)
            mov(ReturnReg, reg);
    }

    void storeCallResultValue(AnyRegister dest)
    {
#if defined(JS_NUNBOX32)
        unboxValue(ValueOperand(JSReturnReg_Type, JSReturnReg_Data), dest);
#elif defined(JS_PUNBOX64)
        unboxValue(ValueOperand(JSReturnReg), dest);
#else
#error "Bad architecture"
#endif
    }

    void storeCallResultValue(ValueOperand dest)
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

    void storeCallResultValue(TypedOrValueRegister dest)
    {
        if (dest.hasValue())
            storeCallResultValue(dest.valueReg());
        else
            storeCallResultValue(dest.typedReg());
    }


    void PushRegsInMask(RegisterSet set);
    void PushRegsInMask(GeneralRegisterSet set) {
        PushRegsInMask(RegisterSet(set, FloatRegisterSet()));
    }
    void PopRegsInMask(RegisterSet set);
    void PopRegsInMask(GeneralRegisterSet set) {
        PopRegsInMask(RegisterSet(set, FloatRegisterSet()));
    }

    using MacroAssemblerSpecific::Push;

    void Push(TypedOrValueRegister v) {
        if (v.hasValue())
            Push(v.valueReg());
        else if (v.type() == MIRType_Double)
            Push(v.typedReg().fpu());
        else
            Push(ValueTypeFromMIRType(v.type()), v.typedReg().gpr());
    }

    void Push(ConstantOrRegister v) {
        if (v.constant())
            Push(v.value());
        else
            Push(v.reg());
    }

    void Push(const ValueOperand &val) {
        pushValue(val);
        framePushed_ += sizeof(Value);
    }

    void Push(const Value &val) {
        pushValue(val);
        framePushed_ += sizeof(Value);
    }

    void Push(JSValueType type, Register reg) {
        pushValue(type, reg);
        framePushed_ += sizeof(Value);
    }

    void adjustStack(int amount) {
        if (amount > 0)
            freeStack(amount);
        else if (amount < 0)
            reserveStack(-amount);
    }

    void bumpKey(Int32Key *key, int diff) {
        if (key->isRegister())
            add32(Imm32(diff), key->reg());
        else
            key->bumpConstant(diff);
    }

    void storeKey(const Int32Key &key, const Address &dest) {
        if (key.isRegister())
            store32(key.reg(), dest);
        else
            store32(Imm32(key.constant()), dest);
    }

    void branchKey(Condition cond, const Address &dest, const Int32Key &key, Label *label) {
        if (key.isRegister())
            branch32(cond, dest, key.reg(), label);
        else
            branch32(cond, dest, Imm32(key.constant()), label);
    }
};

} 
} 

#endif 

