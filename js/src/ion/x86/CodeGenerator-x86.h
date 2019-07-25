








































#ifndef jsion_codegen_x86_h__
#define jsion_codegen_x86_h__

#include "Assembler-x86.h"
#include "ion/shared/CodeGenerator-x86-shared.h"

namespace js {
namespace ion {

class CodeGeneratorX86 : public CodeGeneratorX86Shared
{
    class DeferredDouble : public TempObject
    {
        AbsoluteLabel label_;
        uint32 index_;

      public:
        DeferredDouble(uint32 index) : index_(index)
        { }

        AbsoluteLabel *label() {
            return &label_;
        }
        uint32 index() const {
            return index_;
        }
    };

  private:
    js::Vector<DeferredDouble *, 0, SystemAllocPolicy> deferredDoubles_;

    CodeGeneratorX86 *thisFromCtor() {
        return this;
    }

  protected:
    ValueOperand ToValue(LInstruction *ins, size_t pos);

  protected:
    void linkAbsoluteLabels();

  public:
    CodeGeneratorX86(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool visitBox(LBox *box);
    bool visitBoxDouble(LBoxDouble *box);
    bool visitUnbox(LUnbox *unbox);
    bool visitUnboxDouble(LUnboxDouble *ins);
    bool visitValue(LValue *value);
    bool visitReturn(LReturn *ret);
    bool visitDouble(LDouble *ins);
    bool visitCompareD(LCompareD *comp);
    bool visitCompareDAndBranch(LCompareDAndBranch *comp);
};

typedef CodeGeneratorX86 CodeGeneratorSpecific;

} 
} 

#endif 

