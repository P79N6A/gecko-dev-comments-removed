





#ifndef jit_C1Spewer_h
#define jit_C1Spewer_h

#ifdef DEBUG

#include "NamespaceImports.h"

#include "js/RootingAPI.h"
#include "vm/Printer.h"

namespace js {
namespace jit {

class BacktrackingAllocator;
class MBasicBlock;
class MIRGraph;
class LNode;

class C1Spewer
{
    MIRGraph* graph;
    GenericPrinter& out_;

  public:
    explicit C1Spewer(GenericPrinter& out)
      : graph(nullptr), out_(out)
    { }

    void beginFunction(MIRGraph* graph, JSScript* script);
    void spewPass(const char* pass);
    void spewRanges(const char* pass, BacktrackingAllocator* regalloc);
    void endFunction();

  private:
    void spewPass(GenericPrinter& out, MBasicBlock* block);
    void spewRanges(GenericPrinter& out, BacktrackingAllocator* regalloc, LNode* ins);
    void spewRanges(GenericPrinter& out, MBasicBlock* block, BacktrackingAllocator* regalloc);
};

} 
} 

#endif 

#endif 
