





#ifndef jit_x86_CodeGenerator_x86_h
#define jit_x86_CodeGenerator_x86_h

#include "jit/shared/CodeGenerator-x86-shared.h"
#include "jit/x86/Assembler-x86.h"

namespace js {
namespace jit {

class OutOfLineTruncate;
class OutOfLineTruncateFloat32;

class CodeGeneratorX86 : public CodeGeneratorX86Shared
{
  private:
    CodeGeneratorX86 *thisFromCtor() {
        return this;
    }

  protected:
    ValueOperand ToValue(LInstruction *ins, size_t pos);
    ValueOperand ToOutValue(LInstruction *ins);
    ValueOperand ToTempValue(LInstruction *ins, size_t pos);

    template<typename T>
    bool loadAndNoteViewTypeElement(Scalar::Type vt, const T &srcAddr,
                             const LDefinition *out);
    template<typename T>
    void loadViewTypeElement(Scalar::Type vt, const T &srcAddr,
                                       const LDefinition *out);
    template<typename T>
    void storeAndNoteViewTypeElement(Scalar::Type vt, const LAllocation *value,
                                     const T &dstAddr);
    template<typename T>
    void storeViewTypeElement(Scalar::Type vt, const LAllocation *value,
                              const T &dstAddr);
  public:
    CodeGeneratorX86(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masm);

  public:
    bool visitBox(LBox *box);
    bool visitBoxFloatingPoint(LBoxFloatingPoint *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitValue(LValue *value);
    bool visitCompareB(LCompareB *lir);
    bool visitCompareBAndBranch(LCompareBAndBranch *lir);
    bool visitCompareV(LCompareV *lir);
    bool visitCompareVAndBranch(LCompareVAndBranch *lir);
    bool visitAsmJSUInt32ToDouble(LAsmJSUInt32ToDouble *lir);
    bool visitAsmJSUInt32ToFloat32(LAsmJSUInt32ToFloat32 *lir);
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

    bool visitOutOfLineTruncate(OutOfLineTruncate *ool);
    bool visitOutOfLineTruncateFloat32(OutOfLineTruncateFloat32 *ool);
};

typedef CodeGeneratorX86 CodeGeneratorSpecific;

} 
} 

#endif 
