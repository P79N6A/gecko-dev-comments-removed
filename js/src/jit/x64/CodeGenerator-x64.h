





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

    void storeUnboxedValue(const LAllocation *value, MIRType valueType,
                           Operand dest, MIRType slotType);

  public:
    CodeGeneratorX64(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm);

  public:
    bool visitValue(LValue *value);
    bool visitBox(LBox *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitCompareB(LCompareB *lir);
    bool visitCompareBAndBranch(LCompareBAndBranch *lir);
    bool visitCompareV(LCompareV *lir);
    bool visitCompareVAndBranch(LCompareVAndBranch *lir);
    bool visitTruncateDToInt32(LTruncateDToInt32 *ins);
    bool visitTruncateFToInt32(LTruncateFToInt32 *ins);
    bool visitLoadTypedArrayElementStatic(LLoadTypedArrayElementStatic *ins);
    bool visitStoreTypedArrayElementStatic(LStoreTypedArrayElementStatic *ins);
    bool visitAsmJSCall(LAsmJSCall *ins);
    bool visitAsmJSLoadHeap(LAsmJSLoadHeap *ins);
    bool visitAsmJSStoreHeap(LAsmJSStoreHeap *ins);
    bool visitAsmJSLoadGlobalVar(LAsmJSLoadGlobalVar *ins);
    bool visitAsmJSStoreGlobalVar(LAsmJSStoreGlobalVar *ins);
    bool visitAsmJSLoadFuncPtr(LAsmJSLoadFuncPtr *ins);
    bool visitAsmJSLoadFFIFunc(LAsmJSLoadFFIFunc *ins);
    bool visitAsmJSUInt32ToDouble(LAsmJSUInt32ToDouble *lir);
    bool visitAsmJSUInt32ToFloat32(LAsmJSUInt32ToFloat32 *lir);
};

typedef CodeGeneratorX64 CodeGeneratorSpecific;

} 
} 

#endif 
