





#ifndef COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_OUTPUT_H
#define COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_OUTPUT_H

#include "compiler/depgraph/DependencyGraph.h"
#include "compiler/InfoSink.h"

class TDependencyGraphOutput : public TDependencyGraphTraverser {
public:
    TDependencyGraphOutput(TInfoSinkBase& sink) : mSink(sink) {}
    virtual void visitSymbol(TGraphSymbol* symbol);
    virtual void visitArgument(TGraphArgument* parameter);
    virtual void visitFunctionCall(TGraphFunctionCall* functionCall);
    virtual void visitSelection(TGraphSelection* selection);
    virtual void visitLoop(TGraphLoop* loop);
    virtual void visitLogicalOp(TGraphLogicalOp* logicalOp);

    void outputAllSpanningTrees(TDependencyGraph& graph);
private:
    void outputIndentation();

    TInfoSinkBase& mSink;
};

#endif  
