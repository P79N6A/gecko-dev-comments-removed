








































#ifndef jsion_macro_assembler_h__
#define jsion_macro_assembler_h__

#if defined(JS_CPU_X86)
# include "ion/x86/MacroAssembler-x86.h"
#elif defined(JS_CPU_X64)
# include "ion/x64/MacroAssembler-x64.h"
#elif defined(JS_CPU_ARM)
# include "ion/arm/MacroAssembler-arm.h"
#endif
#include "ion/IonCompartment.h"
#include "ion/TypeOracle.h"

#include "jsscope.h"
#include "jstypedarray.h"
#include "jscompartment.h"

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
        { }

        MacroAssembler *masm() const {
            return masm_;
        }
    };

    AutoRooter autoRooter_;
    bool enoughMemory_;

  public:
    MacroAssembler()
      : autoRooter_(GetIonContext()->cx, thisFromCtor()),
        enoughMemory_(true)
    { }

    MacroAssembler(JSContext *cx)
      : autoRooter_(cx, thisFromCtor()),
        enoughMemory_(true)
    { }

    MoveResolver &moveResolver() {
        return moveResolver_;
    }

    size_t instructionsSize() const {
        return size();
    }

    bool oom() const {
        return !enoughMemory_ || MacroAssemblerSpecific::oom();
    }

    
    
    template <typename T>
    void guardTypeSet(const T &address, types::TypeSet *types, Register scratch,
                      Label *mismatched);

    void loadObjShape(Register objReg, Register dest) {
        loadPtr(Address(objReg, JSObject::offsetOfShape()), dest);
    }
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
    void branchTestObjClass(Condition cond, Register obj, Register scratch, js::Class *clasp,
                            Label *label) {
        loadBaseShape(obj, scratch);
        branchPtr(cond, Address(scratch, BaseShape::offsetOfClass()), ImmWord(clasp), label);
    }
    void branchTestObjShape(Condition cond, Register obj, const Shape *shape, Label *label) {
        branchPtr(Assembler::NotEqual, Address(obj, JSObject::offsetOfShape()),
                  ImmGCPtr(shape), label);
    }

    void loadObjPrivate(Register obj, uint32_t nfixed, Register dest) {
        loadPtr(Address(obj, JSObject::getPrivateDataOffset(nfixed)), dest);
    }

    void loadObjProto(Register obj, Register dest) {
        loadPtr(Address(obj, JSObject::offsetOfType()), dest);
        loadPtr(Address(dest, offsetof(types::TypeObject, proto)), dest);
    }

    void loadJSContext(const Register &dest) {
        movePtr(ImmWord(GetIonContext()->cx->runtime), dest);
        loadPtr(Address(dest, offsetof(JSRuntime, ionJSContext)), dest);
    }
    void loadIonActivation(const Register &dest) {
        movePtr(ImmWord(GetIonContext()->cx->runtime), dest);
        loadPtr(Address(dest, offsetof(JSRuntime, ionActivation)), dest);
    }

    template<typename T>
    void loadTypedOrValue(const T &src, TypedOrValueRegister dest) {
        if (dest.hasValue())
            loadValue(src, dest.valueReg());
        else
            loadUnboxedValue(src, dest.typedReg());
    }

    template<typename T>
    void loadElementTypedOrValue(const T &src, TypedOrValueRegister dest, bool holeCheck,
                                 Label *hole) {
        if (dest.hasValue()) {
            loadValue(src, dest.valueReg());
            if (holeCheck)
                branchTestMagic(Assembler::Equal, dest.valueReg(), hole);
        } else {
            if (holeCheck)
                branchTestMagic(Assembler::Equal, src, hole);
            loadUnboxedValue(src, dest.typedReg());
        }
    }

    template <typename T>
    void storeTypedOrValue(TypedOrValueRegister src, const T &dest) {
        if (src.hasValue())
            storeValue(src.valueReg(), dest);
        else if (src.type() == MIRType_Double)
            storeDouble(src.typedReg().fpu(), dest);
        else
            storeValue(ValueTypeFromMIRType(src.type()), src.typedReg().gpr(), dest);
    }

    template <typename T>
    void storeConstantOrRegister(ConstantOrRegister src, const T &dest) {
        if (src.constant())
            storeValue(src.value(), dest);
        else
            storeTypedOrValue(src.reg(), dest);
    }

    void storeCallResult(Register reg) {
        if (reg != ReturnReg)
            mov(ReturnReg, reg);
    }

    void storeCallResultValue(AnyRegister dest) {
#if defined(JS_NUNBOX32)
        unboxValue(ValueOperand(JSReturnReg_Type, JSReturnReg_Data), dest);
#elif defined(JS_PUNBOX64)
        unboxValue(ValueOperand(JSReturnReg), dest);
#else
#error "Bad architecture"
#endif
    }

    void storeCallResultValue(ValueOperand dest) {
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

    void storeCallResultValue(TypedOrValueRegister dest) {
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

    void branchTestValueTruthy(const ValueOperand &value, Label *ifTrue, FloatRegister fr);

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

    template<typename T>
    void branchKey(Condition cond, const T &length, const Int32Key &key, Label *label) {
        if (key.isRegister())
            branch32(cond, length, key.reg(), label);
        else
            branch32(cond, length, Imm32(key.constant()), label);
    }

    template <typename T>
    void emitPreBarrier(const T &address, MIRType type) {
        JS_ASSERT(type == MIRType_Value ||
                  type == MIRType_String ||
                  type == MIRType_Object);

        Label done;
        if (type == MIRType_Value)
            branchTestGCThing(Assembler::NotEqual, address, &done);

        Push(PreBarrierReg);
        computeEffectiveAddress(address, PreBarrierReg);

        JSContext *cx = GetIonContext()->cx;
        IonCode *preBarrier = cx->compartment->ionCompartment()->preBarrier(cx);
        if (!preBarrier) {
            enoughMemory_ = false;
            return;
        }

        call(preBarrier);
        Pop(PreBarrierReg);

        if (type == MIRType_Value)
            bind(&done);
    }

    template<typename T>
    void loadFromTypedArray(int arrayType, const T &src, AnyRegister dest, Register temp, Label *fail);

    template<typename T>
    void loadFromTypedArray(int arrayType, const T &src, const ValueOperand &dest, bool allowDouble,
                            Label *fail);

    template<typename S, typename T>
    void storeToTypedIntArray(int arrayType, const S &value, const T &dest) {
        switch (arrayType) {
          case TypedArray::TYPE_INT8:
          case TypedArray::TYPE_UINT8:
          case TypedArray::TYPE_UINT8_CLAMPED:
            store8(value, dest);
            break;
          case TypedArray::TYPE_INT16:
          case TypedArray::TYPE_UINT16:
            store16(value, dest);
            break;
          case TypedArray::TYPE_INT32:
          case TypedArray::TYPE_UINT32:
            store32(value, dest);
            break;
          default:
            JS_NOT_REACHED("Invalid typed array type");
            break;
        }
    }

    template<typename S, typename T>
    void storeToTypedFloatArray(int arrayType, const S &value, const T &dest) {
        switch (arrayType) {
          case TypedArray::TYPE_FLOAT32:
            convertDoubleToFloat(value, ScratchFloatReg);
            storeFloat(ScratchFloatReg, dest);
            break;
          case TypedArray::TYPE_FLOAT64:
            storeDouble(value, dest);
            break;
          default:
            JS_NOT_REACHED("Invalid typed array type");
            break;
        }
    }

    
    
    void clampDoubleToUint8(FloatRegister input, Register output);

    void getNewObject(JSContext *cx, const Register &result, HandleObject templateObject, Label *fail);

    
    
    
  private:
    CodeOffsetLabel exitCodePatch_;

  public:
    void enterExitFrame(const VMFunction *f = NULL) {
        linkExitFrame();
        
        exitCodePatch_ = PushWithPatch(ImmWord(-1));
        
        Push(ImmWord(f));
    }
    void enterFakeExitFrame() {
        linkExitFrame();
        Push(ImmWord(uintptr_t(NULL)));
        Push(ImmWord(uintptr_t(NULL)));
    }

    void leaveExitFrame() {
        freeStack(IonExitFooterFrame::Size());
    }

    void link(IonCode *code) {

        
        
        
        if (exitCodePatch_.offset() != 0) {
            patchDataWithValueCheck(CodeLocationLabel(code, exitCodePatch_),
                                    ImmWord(uintptr_t(code)),
                                    ImmWord(uintptr_t(-1)));
        }

    }

    
    
    void performOsr();

    
    void maybeRemoveOsrFrame(Register scratch);
};

} 
} 

#endif 

