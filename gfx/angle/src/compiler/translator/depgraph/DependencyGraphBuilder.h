





#ifndef COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_BUILDER_H
#define COMPILER_DEPGRAPH_DEPENDENCY_GRAPH_BUILDER_H

#include "compiler/translator/depgraph/DependencyGraph.h"





class TDependencyGraphBuilder : public TIntermTraverser {
public:
    static void build(TIntermNode* node, TDependencyGraph* graph);

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitBinary(Visit visit, TIntermBinary*);
    virtual bool visitSelection(Visit visit, TIntermSelection*);
    virtual bool visitAggregate(Visit visit, TIntermAggregate*);
    virtual bool visitLoop(Visit visit, TIntermLoop*);

private:
    typedef std::stack<TGraphSymbol*> TSymbolStack;
    typedef std::set<TGraphParentNode*> TParentNodeSet;

    
    
    
    
    
    
    class TNodeSetStack {
    public:
        TNodeSetStack() {};
        ~TNodeSetStack() { clear(); }

        
        
        TParentNodeSet* getTopSet() const
        {
            ASSERT(!nodeSets.empty());
            TParentNodeSet* topSet = nodeSets.top();
            return !topSet->empty() ? topSet : NULL;
        }

        void pushSet() { nodeSets.push(new TParentNodeSet()); }
        void popSet()
        {
            ASSERT(!nodeSets.empty());
            delete nodeSets.top();
            nodeSets.pop();
        }

        
        
        
        void popSetIntoNext()
        {
            ASSERT(!nodeSets.empty());
            TParentNodeSet* oldTopSet = nodeSets.top();
            nodeSets.pop();

            if (!nodeSets.empty()) {
                TParentNodeSet* newTopSet = nodeSets.top();
                newTopSet->insert(oldTopSet->begin(), oldTopSet->end());
            }

            delete oldTopSet;
        }

        
        
        
        
        void insertIntoTopSet(TGraphParentNode* node)
        {
            if (nodeSets.empty())
                return;

            nodeSets.top()->insert(node);
        }

        void clear()
        {
            while (!nodeSets.empty())
                popSet();
        }

    private:
        typedef std::stack<TParentNodeSet*> TParentNodeSetStack;

        TParentNodeSetStack nodeSets;
    };

    
    
    
    
    class TNodeSetMaintainer {
    public:
        TNodeSetMaintainer(TDependencyGraphBuilder* factory)
            : sets(factory->mNodeSets) { sets.pushSet(); }
        ~TNodeSetMaintainer() { sets.popSet(); }
    protected:
        TNodeSetStack& sets;
    };

    
    
    
    
    
    class TNodeSetPropagatingMaintainer {
    public:
        TNodeSetPropagatingMaintainer(TDependencyGraphBuilder* factory)
            : sets(factory->mNodeSets) { sets.pushSet(); }
        ~TNodeSetPropagatingMaintainer() { sets.popSetIntoNext(); }
    protected:
        TNodeSetStack& sets;
    };

    
    
    
    
    
    
    
    
    
    
    class TLeftmostSymbolMaintainer {
    public:
        TLeftmostSymbolMaintainer(TDependencyGraphBuilder* factory, TGraphSymbol& subtree)
            : leftmostSymbols(factory->mLeftmostSymbols)
        {
            needsPlaceholderSymbol = leftmostSymbols.empty() || leftmostSymbols.top() != &subtree;
            if (needsPlaceholderSymbol)
                leftmostSymbols.push(&subtree);
        }

        ~TLeftmostSymbolMaintainer()
        {
            if (needsPlaceholderSymbol)
                leftmostSymbols.pop();
        }

    protected:
        TSymbolStack& leftmostSymbols;
        bool needsPlaceholderSymbol;
    };

    TDependencyGraphBuilder(TDependencyGraph* graph)
        : TIntermTraverser(true, false, false)
        , mLeftSubtree(NULL)
        , mRightSubtree(NULL)
        , mGraph(graph) {}
    void build(TIntermNode* intermNode) { intermNode->traverse(this); }

    void connectMultipleNodesToSingleNode(TParentNodeSet* nodes, TGraphNode* node) const;

    void visitAssignment(TIntermBinary*);
    void visitLogicalOp(TIntermBinary*);
    void visitBinaryChildren(TIntermBinary*);
    void visitFunctionDefinition(TIntermAggregate*);
    void visitFunctionCall(TIntermAggregate* intermFunctionCall);
    void visitAggregateChildren(TIntermAggregate*);

    TGraphSymbol mLeftSubtree;
    TGraphSymbol mRightSubtree;

    TDependencyGraph* mGraph;
    TNodeSetStack mNodeSets;
    TSymbolStack mLeftmostSymbols;
};

#endif  
