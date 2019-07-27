





#ifndef jsapi_tests_jitTestGVN_h
#define jsapi_tests_jitTestGVN_h

#include "jit/IonAnalysis.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/ValueNumbering.h"

namespace js {
namespace jit {

struct MinimalFunc
{
    LifoAlloc lifo;
    TempAllocator alloc;
    JitCompileOptions options;
    CompileInfo info;
    MIRGraph graph;
    MIRGenerator mir;
    uint32_t numParams;

    MinimalFunc()
      : lifo(4096),
        alloc(&lifo),
        options(),
        info(0, SequentialExecution),
        graph(&alloc),
        mir(static_cast<CompileCompartment *>(nullptr), options, &alloc, &graph,
            &info, static_cast<const OptimizationInfo *>(nullptr)),
        numParams(0)
    { }

    MBasicBlock *createEntryBlock()
    {
        MBasicBlock *block = MBasicBlock::NewAsmJS(graph, info, nullptr, MBasicBlock::NORMAL);
        graph.addBlock(block);
        return block;
    }

    MBasicBlock *createOsrEntryBlock()
    {
        MBasicBlock *block = MBasicBlock::NewAsmJS(graph, info, nullptr, MBasicBlock::NORMAL);
        graph.addBlock(block);
        graph.setOsrBlock(block);
        return block;
    }

    MBasicBlock *createBlock(MBasicBlock *pred)
    {
        MBasicBlock *block = MBasicBlock::NewAsmJS(graph, info, pred, MBasicBlock::NORMAL);
        graph.addBlock(block);
        return block;
    }

    MParameter *createParameter()
    {
        MParameter *p = MParameter::New(alloc, numParams++, nullptr);
        return p;
    }

    bool runGVN()
    {
        if (!SplitCriticalEdges(graph))
            return false;
        if (!RenumberBlocks(graph))
            return false;
        if (!BuildDominatorTree(graph))
            return false;
        if (!BuildPhiReverseMapping(graph))
            return false;
        ValueNumberer gvn(&mir, graph);
        if (!gvn.init())
            return false;
        if (!gvn.run(ValueNumberer::DontUpdateAliasAnalysis))
            return false;
        return true;
    }
};

} 
} 

#endif
