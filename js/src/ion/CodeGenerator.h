








































#ifndef jsion_codegen_h__
#define jsion_codegen_h__

#if defined(JS_CPU_X86)
# include "x86/CodeGenerator-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/CodeGenerator-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/CodeGenerator-arm.h"
#else
#error "CPU Not Supported"
#endif

namespace js {
namespace ion {

class CodeGenerator : public CodeGeneratorSpecific
{
    bool generateArgumentsChecks();
    bool generateBody();

  public:
    CodeGenerator(MIRGenerator *gen, LIRGraph &graph);

  public:
    bool generate();

    virtual bool visitParameter(LParameter *lir);
    virtual bool visitStart(LStart *lir);
    virtual bool visitValueToInt32(LValueToInt32 *lir);
    virtual bool visitValueToDouble(LValueToDouble *lir);
    virtual bool visitInt32ToDouble(LInt32ToDouble *lir);
    virtual bool visitTestVAndBranch(LTestVAndBranch *lir);
    virtual bool visitTruncateDToInt32(LTruncateDToInt32 *lir);
    virtual bool visitPointer(LPointer *lir);
    virtual bool visitSlots(LSlots *lir);
    virtual bool visitTypeBarrier(LTypeBarrier *lir);
};

} 
} 

#endif 

