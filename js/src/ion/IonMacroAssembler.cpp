








































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
                branchPtr(Equal, obj, ImmGCPtr(object), &matched);
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
MacroAssembler::PushVolatileRegsInMask(RegisterSet set)
{
    size_t diff = 0;
    for (AnyRegisterIterator iter(set); iter.more(); iter++) {
        AnyRegister reg = *iter;
        if (!reg.volatile_())
            continue;
        if (reg.isFloat())
            diff += sizeof(double);
        else
            diff += STACK_SLOT_SIZE;
    }

    reserveStack(diff);

    diff = 0;
    for (AnyRegisterIterator iter(set); iter.more(); iter++) {
        AnyRegister reg = *iter;
        if (!reg.volatile_())
            continue;
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
MacroAssembler::PopVolatileRegsInMask(RegisterSet set)
{
    size_t diff = 0;
    for (AnyRegisterIterator iter(set); iter.more(); iter++) {
        AnyRegister reg = *iter;
        if (!reg.volatile_())
            continue;
        if (reg.isFloat()) {
            loadDouble(Address(StackPointer, diff), reg.fpu());
            diff += sizeof(double);
        } else {
            loadPtr(Address(StackPointer, diff), reg.gpr());
            diff += STACK_SLOT_SIZE;
        }
    }

    freeStack(diff);
}
