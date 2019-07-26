





#ifndef jsion_ion_range_analysis_h__
#define jsion_ion_range_analysis_h__

namespace js {
namespace ion {

class MIRGraph;

class RangeAnalysis
{
    MIRGraph &graph;

  public:
    RangeAnalysis(MIRGraph &graph);
    bool analyzeEarly();
    bool analyzeLate();
    static bool AllUsesTruncate(MInstruction *m);
};


} 
} 

#endif 

