








































#ifndef jsion_codegen_h__
#define jsion_codegen_h__

#include "ion/IonLIR.h"

namespace js {
namespace ion {

class CodeGeneratorShared : public LInstructionVisitor
{
  protected:
    MIRGenerator *gen;
    LIRGraph &graph;
    LBlock *current;

    static inline int32 ToInt32(const LAllocation *a) {
        return a->toConstant()->toInt32();
    }

  private:
    virtual bool generatePrologue() = 0;
    bool generateBody();

  public:
    CodeGeneratorShared(MIRGenerator *gen, LIRGraph &graph);

    bool generate();

  public:
    
    virtual bool visitParameter(LParameter *param);
};

} 
} 

#endif 

