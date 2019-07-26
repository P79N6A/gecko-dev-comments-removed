





#ifndef jit_x64_CodeGenerator_x64_h
#define jit_x64_CodeGenerator_x64_h

#include "jit/shared/CodeGenerator-x86-shared.h"

namespace js {
namespace jit {

class CodeGeneratorX64 : public CodeGeneratorX86Shared
{
    CodeGeneratorX64 *thisFromCtor() {
        return this;
    }

  protected:
    ValueOperand ToValue(LInstruction *ins, size_t pos);
    ValueOperand ToOutValue(LInstruction *ins);
    ValueOperand ToTempValue(LInstruction *ins, size_t pos);


    void loadUnboxedValue(Operand source, MIRType type, const LDefinition *dest);
    void storeUnboxedValue(const LAllocation *value, MIRType valueType,
                           Operand dest, MIRType slotType);

    void storeElementTyped(const LAllocation *value, MIRType valueType, MIRType elementType,
                           const Register &elements, const LAllocation *index);

  public:
    CodeGeneratorX64(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm);

  public:
    bool visitValue(LValue *value);
    bool visitOsrValue(LOsrValue *value);
    bool visitBox(LBox *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitLoadSlotV(LLoadSlotV *ins);
    bool visitLoadSlotT(LLoadSlotT *load);
    bool visitStoreSlotT(LStoreSlotT *store);
    bool visitLoadElementT(LLoadElementT *load);
    bool visitImplicitThis(LImplicitThis *lir);
    bool visitInterruptCheck(LInterruptCheck *lir);
    bool visitCompareB(LCompareB *lir);
    bool visitCompareBAndBranch(LCompareBAndBranch *lir);
    bool visitCompareV(LCompareV *lir);
    bool visitCompareVAndBranch(LCompareVAndBranch *lir);
    bool visitTruncateDToInt32(LTruncateDToInt32 *ins);
    bool visitLoadTypedArrayElementStatic(LLoadTypedArrayElementStatic *ins);
    bool visitStoreTypedArrayElementStatic(LStoreTypedArrayElementStatic *ins);
    bool visitAsmJSLoadHeap(LAsmJSLoadHeap *ins);
    bool visitAsmJSStoreHeap(LAsmJSStoreHeap *ins);
    bool visitAsmJSLoadGlobalVar(LAsmJSLoadGlobalVar *ins);
    bool visitAsmJSStoreGlobalVar(LAsmJSStoreGlobalVar *ins);
    bool visitAsmJSLoadFuncPtr(LAsmJSLoadFuncPtr *ins);
    bool visitAsmJSLoadFFIFunc(LAsmJSLoadFFIFunc *ins);
    bool visitAsmJSUInt32ToDouble(LAsmJSUInt32ToDouble *lir);

    void postAsmJSCall(LAsmJSCall *lir) {}
};

typedef CodeGeneratorX64 CodeGeneratorSpecific;

} 
} 

#endif 
