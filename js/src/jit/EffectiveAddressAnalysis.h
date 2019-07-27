





#ifndef jit_EffectiveAddressAnalysis_h
#define jit_EffectiveAddressAnalysis_h

namespace js {
namespace jit {

class MIRGraph;

class EffectiveAddressAnalysis
{
    MIRGenerator* mir_;
    MIRGraph& graph_;

    template<typename MAsmJSHeapAccessType>
    bool tryAddDisplacement(MAsmJSHeapAccessType *ins, int32_t o);

    template<typename MAsmJSHeapAccessType>
    void analyzeAsmHeapAccess(MAsmJSHeapAccessType* ins);

  public:
    EffectiveAddressAnalysis(MIRGenerator *mir, MIRGraph& graph)
      : mir_(mir), graph_(graph)
    {}

    bool analyze();
};

} 
} 

#endif 
