





#ifndef jit_C1Spewer_h
#define jit_C1Spewer_h

#ifdef DEBUG

#include "NamespaceImports.h"

#include "js/RootingAPI.h"

namespace js {
namespace jit {

class BacktrackingAllocator;
class MBasicBlock;
class MIRGraph;
class LNode;

class C1Spewer
{
    MIRGraph* graph;
    FILE* spewout_;

  public:
    C1Spewer()
      : graph(nullptr), spewout_(nullptr)
    { }

    bool init(const char* path);
    void beginFunction(MIRGraph* graph, HandleScript script);
    void spewPass(const char* pass);
    void spewIntervals(const char* pass, BacktrackingAllocator* regalloc);
    void endFunction();
    void finish();

  private:
    void spewPass(FILE* fp, MBasicBlock* block);
    void spewIntervals(FILE* fp, BacktrackingAllocator* regalloc, LNode* ins, size_t& nextId);
    void spewIntervals(FILE* fp, MBasicBlock* block, BacktrackingAllocator* regalloc, size_t& nextId);
};

} 
} 

#endif 

#endif 
