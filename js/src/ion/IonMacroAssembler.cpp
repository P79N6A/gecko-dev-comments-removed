








































#include "jsinfer.h"
#include "jsinferinlines.h"
#include "IonMacroAssembler.h"

using namespace js;
using namespace js::ion;

template <typename T> void
MacroAssembler::guardTypeSet(const T &address, types::TypeSet *types,
                             Register scratch, Label *mismatched)
{
    JS_ASSERT(!types->unknown());

    Label matched;
    Register tag = extractTag(address, scratch);

    if (types->hasType(types::Type::DoubleType())) {
        
        JS_ASSERT(types->hasType(types::Type::Int32Type()));
        branchTestNumber(Equal, tag, &matched);
    } else if (types->hasType(types::Type::Int32Type())) {
        branchTestInt32(Equal, tag, &matched);
    }

    if (types->hasType(types::Type::UndefinedType()))
        branchTestUndefined(Equal, tag, &matched);
    if (types->hasType(types::Type::BooleanType()))
        branchTestBoolean(Equal, tag, &matched);
    if (types->hasType(types::Type::StringType()))
        branchTestString(Equal, tag, &matched);
    if (types->hasType(types::Type::NullType()))
        branchTestNull(Equal, tag, &matched);

    if (types->hasType(types::Type::AnyObjectType())) {
        branchTestObject(Equal, tag, &matched);
    } else if (types->getObjectCount()) {
        branchTestObject(NotEqual, tag, mismatched);
        Register obj = extractObject(address, scratch);

        unsigned count = types->getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            if (JSObject *object = types->getSingleObject(i))
                branchPtr(Equal, obj, ImmGCPtr(object), &matched);
        }

        loadPtr(Address(obj, JSObject::offsetOfType()), scratch);

        for (unsigned i = 0; i < count; i++) {
            if (types::TypeObject *object = types->getTypeObject(i))
                branchPtr(Equal, scratch, ImmGCPtr(object), &matched);
        }
    }

    jump(mismatched);
    bind(&matched);
}

template void MacroAssembler::guardTypeSet(const Address &address, types::TypeSet *types,
                                           Register scratch, Label *mismatched);
template void MacroAssembler::guardTypeSet(const ValueOperand &value, types::TypeSet *types,
                                           Register scratch, Label *mismatched);

void
MacroAssembler::PushRegsInMask(RegisterSet set)
{
    size_t diff = 0;
    for (AnyRegisterIterator iter(set); iter.more(); iter++) {
        AnyRegister reg = *iter;
        if (reg.isFloat())
            diff += sizeof(double);
        else
            diff += STACK_SLOT_SIZE;
    }
    
    
    size_t new_diff = (diff + 7) & ~7;
    reserveStack(new_diff);

    diff = new_diff - diff;
    for (AnyRegisterIterator iter(set); iter.more(); iter++) {
        AnyRegister reg = *iter;
        if (reg.isFloat()) {
            storeDouble(reg.fpu(), Address(StackPointer, diff));
            diff += sizeof(double);
        } else {
            storePtr(reg.gpr(), Address(StackPointer, diff));
            diff += STACK_SLOT_SIZE;
        }
    }
}

void
MacroAssembler::PopRegsInMask(RegisterSet set)
{
    size_t diff = 0;
    
    for (AnyRegisterIterator iter(set); iter.more(); iter++) {
        AnyRegister reg = *iter;
        if (reg.isFloat())
            diff += sizeof(double);
        else
            diff += STACK_SLOT_SIZE;
    }
    size_t new_diff = (diff + 7) & ~7;

    diff = new_diff - diff;
    for (AnyRegisterIterator iter(set); iter.more(); iter++) {
        AnyRegister reg = *iter;
        if (reg.isFloat()) {
            loadDouble(Address(StackPointer, diff), reg.fpu());
            diff += sizeof(double);
        } else {
            loadPtr(Address(StackPointer, diff), reg.gpr());
            diff += STACK_SLOT_SIZE;
        }
    }
    freeStack(new_diff);
}

void
MacroAssembler::branchTestValueTruthy(const ValueOperand &value, Label *ifTrue, FloatRegister fr)
{
    Register tag = splitTagForTest(value);
    Label ifFalse;
    Assembler::Condition cond;

    
    
    
    
    branchTestUndefined(Assembler::Equal, tag, &ifFalse);

    branchTestNull(Assembler::Equal, tag, &ifFalse);
    branchTestObject(Assembler::Equal, tag, ifTrue);

    Label notBoolean;
    branchTestBoolean(Assembler::NotEqual, tag, &notBoolean);
    branchTestBooleanTruthy(false, value, &ifFalse);
    jump(ifTrue);
    bind(&notBoolean);

    Label notInt32;
    branchTestInt32(Assembler::NotEqual, tag, &notInt32);
    cond = testInt32Truthy(false, value);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&notInt32);

    
    Label notString;
    branchTestString(Assembler::NotEqual, tag, &notString);
    cond = testStringTruthy(false, value);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&notString);

    
    unboxDouble(value, fr);
    cond = testDoubleTruthy(false, fr);
    j(cond, &ifFalse);
    jump(ifTrue);
    bind(&ifFalse);
}
