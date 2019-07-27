





#ifndef COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_H
#define COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_H

#include "compiler/translator/IntermNode.h"

#include <set>
#include <stack>

class TGraphNode;
class TGraphParentNode;
class TGraphArgument;
class TGraphFunctionCall;
class TGraphSymbol;
class TGraphSelection;
class TGraphLoop;
class TGraphLogicalOp;
class TDependencyGraphTraverser;
class TDependencyGraphOutput;

typedef std::set<TGraphNode*> TGraphNodeSet;
typedef std::vector<TGraphNode*> TGraphNodeVector;
typedef std::vector<TGraphSymbol*> TGraphSymbolVector;
typedef std::vector<TGraphFunctionCall*> TFunctionCallVector;




class TGraphNode {
public:
    TGraphNode(TIntermNode* node) : intermNode(node) {}
    virtual ~TGraphNode() {}
    virtual void traverse(TDependencyGraphTraverser* graphTraverser);
protected:
    TIntermNode* intermNode;
};




class TGraphParentNode : public TGraphNode {
public:
    TGraphParentNode(TIntermNode* node) : TGraphNode(node) {}
    virtual ~TGraphParentNode() {}
    void addDependentNode(TGraphNode* node) { if (node != this) mDependentNodes.insert(node); }
    virtual void traverse(TDependencyGraphTraverser* graphTraverser);
private:
    TGraphNodeSet mDependentNodes;
};




class TGraphArgument : public TGraphParentNode {
public:
    TGraphArgument(TIntermAggregate* intermFunctionCall, int argumentNumber)
        : TGraphParentNode(intermFunctionCall)
        , mArgumentNumber(argumentNumber) {}
    virtual ~TGraphArgument() {}
    const TIntermAggregate* getIntermFunctionCall() const { return intermNode->getAsAggregate(); }
    int getArgumentNumber() const { return mArgumentNumber; }
    virtual void traverse(TDependencyGraphTraverser* graphTraverser);
private:
    int mArgumentNumber;
};




class TGraphFunctionCall : public TGraphParentNode {
public:
    TGraphFunctionCall(TIntermAggregate* intermFunctionCall)
        : TGraphParentNode(intermFunctionCall) {}
    virtual ~TGraphFunctionCall() {}
    const TIntermAggregate* getIntermFunctionCall() const { return intermNode->getAsAggregate(); }
    virtual void traverse(TDependencyGraphTraverser* graphTraverser);
};




class TGraphSymbol : public TGraphParentNode {
public:
    TGraphSymbol(TIntermSymbol* intermSymbol) : TGraphParentNode(intermSymbol) {}
    virtual ~TGraphSymbol() {}
    const TIntermSymbol* getIntermSymbol() const { return intermNode->getAsSymbolNode(); }
    virtual void traverse(TDependencyGraphTraverser* graphTraverser);
};




class TGraphSelection : public TGraphNode {
public:
    TGraphSelection(TIntermSelection* intermSelection) : TGraphNode(intermSelection) {}
    virtual ~TGraphSelection() {}
    const TIntermSelection* getIntermSelection() const { return intermNode->getAsSelectionNode(); }
    virtual void traverse(TDependencyGraphTraverser* graphTraverser);
};




class TGraphLoop : public TGraphNode {
public:
    TGraphLoop(TIntermLoop* intermLoop) : TGraphNode(intermLoop) {}
    virtual ~TGraphLoop() {}
    const TIntermLoop* getIntermLoop() const { return intermNode->getAsLoopNode(); }
    virtual void traverse(TDependencyGraphTraverser* graphTraverser);
};




class TGraphLogicalOp : public TGraphNode {
public:
    TGraphLogicalOp(TIntermBinary* intermLogicalOp) : TGraphNode(intermLogicalOp) {}
    virtual ~TGraphLogicalOp() {}
    const TIntermBinary* getIntermLogicalOp() const { return intermNode->getAsBinaryNode(); }
    const char* getOpString() const;
    virtual void traverse(TDependencyGraphTraverser* graphTraverser);
};











class TDependencyGraph {
public:
    TDependencyGraph(TIntermNode* intermNode);
    ~TDependencyGraph();
    TGraphNodeVector::const_iterator begin() const { return mAllNodes.begin(); }
    TGraphNodeVector::const_iterator end() const { return mAllNodes.end(); }

    TGraphSymbolVector::const_iterator beginSamplerSymbols() const
    {
        return mSamplerSymbols.begin();
    }

    TGraphSymbolVector::const_iterator endSamplerSymbols() const
    {
        return mSamplerSymbols.end();
    }

    TFunctionCallVector::const_iterator beginUserDefinedFunctionCalls() const
    {
        return mUserDefinedFunctionCalls.begin();
    }

    TFunctionCallVector::const_iterator endUserDefinedFunctionCalls() const
    {
        return mUserDefinedFunctionCalls.end();
    }

    TGraphArgument* createArgument(TIntermAggregate* intermFunctionCall, int argumentNumber);
    TGraphFunctionCall* createFunctionCall(TIntermAggregate* intermFunctionCall);
    TGraphSymbol* getOrCreateSymbol(TIntermSymbol* intermSymbol);
    TGraphSelection* createSelection(TIntermSelection* intermSelection);
    TGraphLoop* createLoop(TIntermLoop* intermLoop);
    TGraphLogicalOp* createLogicalOp(TIntermBinary* intermLogicalOp);
private:
    typedef TMap<int, TGraphSymbol*> TSymbolIdMap;
    typedef std::pair<int, TGraphSymbol*> TSymbolIdPair;

    TGraphNodeVector mAllNodes;
    TGraphSymbolVector mSamplerSymbols;
    TFunctionCallVector mUserDefinedFunctionCalls;
    TSymbolIdMap mSymbolIdMap;
};








class TDependencyGraphTraverser {
public:
    TDependencyGraphTraverser() : mDepth(0) {}

    virtual void visitSymbol(TGraphSymbol* symbol) {};
    virtual void visitArgument(TGraphArgument* selection) {};
    virtual void visitFunctionCall(TGraphFunctionCall* functionCall) {};
    virtual void visitSelection(TGraphSelection* selection) {};
    virtual void visitLoop(TGraphLoop* loop) {};
    virtual void visitLogicalOp(TGraphLogicalOp* logicalOp) {};

    int getDepth() const { return mDepth; }
    void incrementDepth() { ++mDepth; }
    void decrementDepth() { --mDepth; }

    void clearVisited() { mVisited.clear(); }
    void markVisited(TGraphNode* node) { mVisited.insert(node); }
    bool isVisited(TGraphNode* node) const { return mVisited.find(node) != mVisited.end(); }
private:
    int mDepth;
    TGraphNodeSet mVisited;
};

#endif
