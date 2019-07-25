






#ifndef js_typedarray_ic_h___
#define js_typedarray_ic_h___

#include "jscntxt.h"
#include "jstypedarray.h"

#include "vm/NumericConversions.h"

#include "jsnuminlines.h"
#include "jstypedarrayinlines.h"

namespace js {
namespace mjit {

#ifdef JS_METHODJIT_TYPED_ARRAY

typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;
typedef JSC::MacroAssembler::Jump Jump;
typedef JSC::MacroAssembler::Imm32 Imm32;
typedef JSC::MacroAssembler::ImmDouble ImmDouble;

static inline bool
ConstantFoldForFloatArray(JSContext *cx, ValueRemat *vr)
{
    if (!vr->isTypeKnown())
        return true;

    
    
    if (vr->knownType() == JSVAL_TYPE_OBJECT ||
        vr->knownType() == JSVAL_TYPE_UNDEFINED) {
        *vr = ValueRemat::FromConstant(DoubleValue(js_NaN));
        return true;
    }
    if (vr->knownType() == JSVAL_TYPE_NULL) {
        *vr = ValueRemat::FromConstant(DoubleValue(0));
        return true;
    }

    if (!vr->isConstant())
        return true;

    if (vr->knownType() == JSVAL_TYPE_DOUBLE)
        return true;

    double d = 0;
    Value v = vr->value();
    if (v.isString()) {
        if (!StringToNumberType<double>(cx, v.toString(), &d))
            return false;
    } else if (v.isBoolean()) {
        d = v.toBoolean() ? 1 : 0;
    } else if (v.isInt32()) {
        d = v.toInt32();
    } else {
        JS_NOT_REACHED("unknown constant type");
    }
    *vr = ValueRemat::FromConstant(DoubleValue(d));
    return true;
}

static inline bool
ConstantFoldForIntArray(JSContext *cx, JSObject *tarray, ValueRemat *vr)
{
    if (!vr->isTypeKnown())
        return true;

    
    
    if (vr->knownType() == JSVAL_TYPE_OBJECT ||
        vr->knownType() == JSVAL_TYPE_UNDEFINED ||
        vr->knownType() == JSVAL_TYPE_NULL) {
        *vr = ValueRemat::FromConstant(Int32Value(0));
        return true;
    }

    if (!vr->isConstant())
        return true;

    
    Value v = vr->value();
    if (v.isString()) {
        double d;
        if (!StringToNumberType<double>(cx, v.toString(), &d))
            return false;
        v.setNumber(d);
    }

    int32_t i32 = 0;
    if (v.isDouble()) {
        i32 = (TypedArray::getType(tarray) == js::TypedArray::TYPE_UINT8_CLAMPED)
              ? ClampDoubleToUint8(v.toDouble())
              : ToInt32(v.toDouble());
    } else if (v.isInt32()) {
        i32 = v.toInt32();
        if (TypedArray::getType(tarray) == js::TypedArray::TYPE_UINT8_CLAMPED)
            i32 = ClampIntForUint8Array(i32);
    } else if (v.isBoolean()) {
        i32 = v.toBoolean() ? 1 : 0;
    } else {
        JS_NOT_REACHED("unknown constant type");
    }

    *vr = ValueRemat::FromConstant(Int32Value(i32));

    return true;
}






static void
GenConversionForIntArray(Assembler &masm, JSObject *tarray, const ValueRemat &vr,
                         uint32_t saveMask)
{
    if (vr.isConstant()) {
        
        JS_ASSERT(vr.knownType() == JSVAL_TYPE_INT32);
        return;
    }

    if (!vr.isTypeKnown() || vr.knownType() != JSVAL_TYPE_INT32) {
        
        MaybeJump checkInt32;
        if (!vr.isTypeKnown())
            checkInt32 = masm.testInt32(Assembler::Equal, vr.typeReg());

        
        StackMarker vp = masm.allocStack(sizeof(Value), sizeof(double));
        masm.storeValue(vr, masm.addressOfExtra(vp));

        
        PreserveRegisters saveForCall(masm);
        saveForCall.preserve(saveMask & Registers::TempRegs);

        masm.setupABICall(Registers::FastCall, 2);
        masm.storeArg(0, masm.vmFrameOffset(offsetof(VMFrame, cx)));
        masm.storeArgAddr(1, masm.addressOfExtra(vp));

        typedef int32_t (JS_FASTCALL *Int32CxVp)(JSContext *, Value *);
        Int32CxVp stub;
        if (TypedArray::getType(tarray) == js::TypedArray::TYPE_UINT8_CLAMPED)
            stub = stubs::ConvertToTypedInt<true>;
        else
            stub = stubs::ConvertToTypedInt<false>;
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, stub), false);
        if (vr.dataReg() != Registers::ReturnReg)
            masm.move(Registers::ReturnReg, vr.dataReg());

        saveForCall.restore();
        masm.freeStack(vp);

        if (checkInt32.isSet())
            checkInt32.get().linkTo(masm.label(), &masm);
    }

    
    if (TypedArray::getType(tarray) == js::TypedArray::TYPE_UINT8_CLAMPED)
        masm.clampInt32ToUint8(vr.dataReg());
}







static void
GenConversionForFloatArray(Assembler &masm, JSObject *tarray, const ValueRemat &vr,
                           FPRegisterID destReg, uint32_t saveMask)
{
    if (vr.isConstant()) {
        
        JS_ASSERT(vr.knownType() == JSVAL_TYPE_DOUBLE);
        return;
    }

    
    MaybeJump isDouble;
    if (!vr.isTypeKnown())
        isDouble = masm.testDouble(Assembler::Equal, vr.typeReg());

    
    MaybeJump skip1, skip2;
    if (!vr.isTypeKnown() || vr.knownType() == JSVAL_TYPE_INT32) {
        MaybeJump isNotInt32;
        if (!vr.isTypeKnown())
            isNotInt32 = masm.testInt32(Assembler::NotEqual, vr.typeReg());
        masm.convertInt32ToDouble(vr.dataReg(), destReg);
        if (isNotInt32.isSet()) {
            skip1 = masm.jump();
            isNotInt32.get().linkTo(masm.label(), &masm);
        }
    }

    
    if (!vr.isTypeKnown() ||
        (vr.knownType() != JSVAL_TYPE_INT32 &&
         vr.knownType() != JSVAL_TYPE_DOUBLE)) {
        
        StackMarker vp = masm.allocStack(sizeof(Value), sizeof(double));
        masm.storeValue(vr, masm.addressOfExtra(vp));

        
        PreserveRegisters saveForCall(masm);
        saveForCall.preserve(saveMask & Registers::TempRegs);
        masm.setupABICall(Registers::FastCall, 2);
        masm.storeArg(0, masm.vmFrameOffset(offsetof(VMFrame, cx)));
        masm.storeArgAddr(1, masm.addressOfExtra(vp));
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, stubs::ConvertToTypedFloat), false);
        saveForCall.restore();

        
        masm.loadDouble(masm.addressOfExtra(vp), destReg);
        masm.freeStack(vp);
        skip2 = masm.jump();
    }

    if (isDouble.isSet())
        isDouble.get().linkTo(masm.label(), &masm);

    
    
    
    if (!vr.isTypeKnown() || vr.knownType() == JSVAL_TYPE_DOUBLE)
        masm.fastLoadDouble(vr.dataReg(), vr.typeReg(), destReg);

    
    if (skip1.isSet())
        skip1.get().linkTo(masm.label(), &masm);
    if (skip2.isSet())
        skip2.get().linkTo(masm.label(), &masm);

    if (TypedArray::getType(tarray) == js::TypedArray::TYPE_FLOAT32)
        masm.convertDoubleToFloat(destReg, destReg);
}

template <typename T>
static bool
StoreToTypedArray(JSContext *cx, Assembler &masm, JSObject *tarray, T address,
                  const ValueRemat &vrIn, uint32_t saveMask)
{
    ValueRemat vr = vrIn;

    uint32_t type = TypedArray::getType(tarray);
    switch (type) {
      case js::TypedArray::TYPE_INT8:
      case js::TypedArray::TYPE_UINT8:
      case js::TypedArray::TYPE_UINT8_CLAMPED:
      case js::TypedArray::TYPE_INT16:
      case js::TypedArray::TYPE_UINT16:
      case js::TypedArray::TYPE_INT32:
      case js::TypedArray::TYPE_UINT32:
      {
        if (!ConstantFoldForIntArray(cx, tarray, &vr))
            return false;

        PreserveRegisters saveRHS(masm);
        PreserveRegisters saveLHS(masm);

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        bool singleByte = (type == js::TypedArray::TYPE_INT8 ||
                           type == js::TypedArray::TYPE_UINT8 ||
                           type == js::TypedArray::TYPE_UINT8_CLAMPED);
        bool mayNeedConversion = (!vr.isTypeKnown() || vr.knownType() != JSVAL_TYPE_INT32);
        bool mayNeedClamping = !vr.isConstant() && (type == js::TypedArray::TYPE_UINT8_CLAMPED);
        bool needsSingleByteReg = singleByte &&
                                  !vr.isConstant() &&
                                  !(Registers::SingleByteRegs & Registers::maskReg(vr.dataReg()));
        bool rhsIsMutable = !vr.isConstant() && !(saveMask & Registers::maskReg(vr.dataReg()));

        if (((mayNeedConversion || mayNeedClamping) && !rhsIsMutable) || needsSingleByteReg) {
            
            
            
            
            
            uint32_t allowMask = Registers::AvailRegs;
            if (singleByte)
                allowMask &= Registers::SingleByteRegs;

            
            uint32_t pinned = Assembler::maskAddress(address);
            if (!vr.isTypeKnown())
                pinned |= Registers::maskReg(vr.typeReg());

            Registers avail = allowMask & ~(pinned | saveMask);

            RegisterID newReg;
            if (!avail.empty()) {
                newReg = avail.takeAnyReg().reg();
            } else {
                
                avail = allowMask & ~pinned;

                if (!avail.empty()) {
                    newReg = avail.takeAnyReg().reg();
                    saveRHS.preserve(Registers::maskReg(newReg));
                } else {
                    
                    
                    

                    
                    
                    uint32_t vrRegs = Registers::mask2Regs(vr.dataReg(), vr.typeReg());
                    uint32_t lhsMask = vrRegs & Assembler::maskAddress(address);

                    
                    
                    uint32_t rhsMask = vrRegs & ~lhsMask;

                    
                    saveRHS.preserve(rhsMask);
                    saveLHS.preserve(lhsMask);

                    
                    saveMask &= ~lhsMask;

                    
                    masm.swap(vr.typeReg(), vr.dataReg());
                    vr = ValueRemat::FromRegisters(vr.dataReg(), vr.typeReg());
                    newReg = vr.dataReg();
                }

                
                
                saveMask &= ~Registers::maskReg(newReg);
            }

            if (vr.dataReg() != newReg)
                masm.move(vr.dataReg(), newReg);

            
            if (vr.isTypeKnown())
                vr = ValueRemat::FromKnownType(vr.knownType(), newReg);
            else
                vr = ValueRemat::FromRegisters(vr.typeReg(), newReg);
        }

        GenConversionForIntArray(masm, tarray, vr, saveMask);

        
        
        saveLHS.restore();

        if (vr.isConstant())
            masm.storeToTypedIntArray(type, Imm32(vr.value().toInt32()), address);
        else
            masm.storeToTypedIntArray(type, vr.dataReg(), address);

        
        
        saveRHS.restore();
        break;
      }

      case js::TypedArray::TYPE_FLOAT32:
      case js::TypedArray::TYPE_FLOAT64: {
        



        Registers regs(Registers::TempFPRegs);
        FPRegisterID temp = regs.takeAnyReg().fpreg();

        if (!ConstantFoldForFloatArray(cx, &vr))
            return false;
        GenConversionForFloatArray(masm, tarray, vr, temp, saveMask);
        if (vr.isConstant())
            masm.storeToTypedFloatArray(type, ImmDouble(vr.value().toDouble()), address);
        else
            masm.storeToTypedFloatArray(type, temp, address);
        break;
      }
    }

    return true;
}

#endif 

} 
} 

#endif 

