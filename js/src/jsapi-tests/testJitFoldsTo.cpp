






#include "jit/IonAnalysis.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/ValueNumbering.h"

#include "jsapi-tests/tests.h"

using namespace js;
using namespace js::jit;

namespace {

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
        ValueNumberer gvn(&mir, graph);
        if (!gvn.run(ValueNumberer::DontUpdateAliasAnalysis))
            return false;
        return true;
    }
};

} 

BEGIN_TEST(testJitFoldsTo_DivReciprocal)
{
    MinimalFunc func;
    MBasicBlock *block = func.createEntryBlock();

    
    MParameter *p = func.createParameter();
    block->add(p);
    MConstant *c = MConstant::New(func.alloc, DoubleValue(4.0));
    block->add(c);
    MDiv *div = MDiv::New(func.alloc, p, c, MIRType_Double);
    block->add(div);
    MReturn *ret = MReturn::New(func.alloc, div);
    block->end(ret);

    if (!func.runGVN())
        return false;

    
    MDefinition *op = ret->getOperand(0);
    CHECK(op->isMul());
    CHECK(op->getOperand(0) == p);
    CHECK(op->getOperand(1)->isConstant());
    CHECK(op->getOperand(1)->toConstant()->value().toNumber() == 0.25);
    return true;
}
END_TEST(testJitFoldsTo_DivReciprocal)

BEGIN_TEST(testJitFoldsTo_NoDivReciprocal)
{
    MinimalFunc func;
    MBasicBlock *block = func.createEntryBlock();

    
    MParameter *p = func.createParameter();
    block->add(p);
    MConstant *c = MConstant::New(func.alloc, DoubleValue(5.0));
    block->add(c);
    MDiv *div = MDiv::New(func.alloc, p, c, MIRType_Double);
    block->add(div);
    MReturn *ret = MReturn::New(func.alloc, div);
    block->end(ret);

    if (!func.runGVN())
        return false;

    
    MDefinition *op = ret->getOperand(0);
    CHECK(op->isDiv());
    CHECK(op->getOperand(0) == p);
    CHECK(op->getOperand(1) == c);
    return true;
}
END_TEST(testJitFoldsTo_NoDivReciprocal)
