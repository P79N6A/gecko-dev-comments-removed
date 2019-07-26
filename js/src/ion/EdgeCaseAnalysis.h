






#ifndef jsion_ion_edge_case_analysis_h__
#define jsion_ion_edge_case_analysis_h__

namespace js {
namespace ion {

class MIRGraph;

class EdgeCaseAnalysis
{
    MIRGenerator *mir;
    MIRGraph &graph;

  public:
    EdgeCaseAnalysis(MIRGenerator *mir, MIRGraph &graph);
    bool analyzeEarly();
    bool analyzeLate();
    static bool AllUsesTruncate(MInstruction *m);
};


} 
} 

#endif 

