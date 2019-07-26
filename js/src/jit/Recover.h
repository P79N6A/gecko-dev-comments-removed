





#ifndef jit_Recover_h
#define jit_Recover_h

#include "jit/Snapshots.h"

namespace js {
namespace jit {

class RResumePoint
{
  private:
    uint32_t pcOffset_;           
    uint32_t numOperands_;        

    RResumePoint(CompactBufferReader &reader);

  public:
    static void readRecoverData(CompactBufferReader &reader, RInstructionStorage *raw);

    uint32_t pcOffset() const {
        return pcOffset_;
    }
    uint32_t numOperands() const {
        return numOperands_;
    }
};

}
}

#endif 
