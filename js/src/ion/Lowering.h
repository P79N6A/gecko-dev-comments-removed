








































#ifndef jsion_ion_lowering_h__
#define jsion_ion_lowering_h__




#include "IonAllocPolicy.h"
#include "IonLIR.h"
#include "MOpcodes.h"

#if defined(JS_CPU_X86)
# include "x86/Lowering-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Lowering-x64.h"
#else
# error "CPU!"
#endif

namespace js {
namespace ion {

class LIRGenerator : public LIRGeneratorSpecific
{
  public:
    LIRGenerator(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorSpecific(gen, graph, lirGraph)
    { }

    bool generate();

  private:
    bool lowerBitOp(JSOp op, MInstruction *ins);
    bool precreatePhi(LBlock *block, MPhi *phi);
    bool definePhis();

  public:
    bool visitInstruction(MInstruction *ins);
    bool visitBlock(MBasicBlock *block);

    
    
    bool visitParameter(MParameter *param);
    bool visitTableSwitch(MTableSwitch *tableswitch);
    bool visitGoto(MGoto *ins);
    bool visitTest(MTest *test);
    bool visitBitAnd(MBitAnd *ins);
    bool visitBitOr(MBitOr *ins);
    bool visitBitXor(MBitXor *ins);
    bool visitAdd(MAdd *ins);
    bool visitStart(MStart *start);
    bool visitToDouble(MToDouble *convert);
    bool visitToInt32(MToInt32 *convert);
    bool visitTruncateToInt32(MTruncateToInt32 *truncate);
    bool visitCopy(MCopy *ins);
};

} 
} 

#endif 

