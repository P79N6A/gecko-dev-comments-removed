






#ifndef jsion_ion_edge_case_analysis_h__
#define jsion_ion_edge_case_analysis_h__

namespace js {
namespace ion {

class MIRGraph;

class EdgeCaseAnalysis
{
    MIRGraph &graph;

  public:
    EdgeCaseAnalysis(MIRGraph &graph);
    bool analyzeEarly();
    bool analyzeLate();
    static bool AllUsesTruncate(MInstruction *m);
};


} 
} 

#endif 

