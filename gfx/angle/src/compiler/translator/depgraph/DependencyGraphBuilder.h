





#ifndef COMPILER_TRANSLATOR_DEPGRAPH_DEPENDENCY_GRAPH_BUILDER_H
#define COMPILER_TRANSLATOR_DEPGRAPH_DEPENDENCY_GRAPH_BUILDER_H

#include "compiler/translator/depgraph/DependencyGraph.h"





class TDependencyGraphBuilder : public TIntermTraverser
{
  public:
    static void build(TIntermNode *node, TDependencyGraph *graph);

    virtual void visitSymbol(TIntermSymbol *);
    virtual bool visitBinary(Visit visit, TIntermBinary *);
    virtual bool visitSelection(Visit visit, TIntermSelection *);
    virtual bool visitAggregate(Visit visit, TIntermAggregate *);
    virtual bool visitLoop(Visit visit, TIntermLoop *);

  private:
    typedef std::stack<TGraphSymbol *> TSymbolStack;
    typedef std::set<TGraphParentNode *> TParentNodeSet;

    
    
    
    
    
    
    
    class TNodeSetStack
    {
      public:
        TNodeSetStack() {};
        ~TNodeSetStack() { clear(); }

        
        
        TParentNodeSet *getTopSet() const
        {
            ASSERT(!mNodeSets.empty());
            TParentNodeSet *topSet = mNodeSets.top();
            return !topSet->empty() ? topSet : NULL;
        }

        void pushSet() { mNodeSets.push(new TParentNodeSet()); }
        void popSet()
        {
            ASSERT(!mNodeSets.empty());
            delete mNodeSets.top();
            mNodeSets.pop();
        }

        
        
        
        void popSetIntoNext()
        {
            ASSERT(!mNodeSets.empty());
            TParentNodeSet *oldTopSet = mNodeSets.top();
            mNodeSets.pop();

            if (!mNodeSets.empty())
            {
                TParentNodeSet *newTopSet = mNodeSets.top();
                newTopSet->insert(oldTopSet->begin(), oldTopSet->end());
            }

            delete oldTopSet;
        }

        
        
        
        
        void insertIntoTopSet(TGraphParentNode *node)
        {
            if (mNodeSets.empty())
                return;

            mNodeSets.top()->insert(node);
        }

        void clear()
        {
            while (!mNodeSets.empty())
                popSet();
        }

      private:
        typedef std::stack<TParentNodeSet *> TParentNodeSetStack;

        TParentNodeSetStack mNodeSets;
    };

    
    
    
    
    class TNodeSetMaintainer
    {
      public:
        TNodeSetMaintainer(TDependencyGraphBuilder *factory)
            : mSets(factory->mNodeSets)
        {
            mSets.pushSet();
        }
        ~TNodeSetMaintainer() { mSets.popSet(); }
      protected:
        TNodeSetStack &mSets;
    };

    
    
    
    
    
    class TNodeSetPropagatingMaintainer
    {
      public:
        TNodeSetPropagatingMaintainer(TDependencyGraphBuilder *factory)
            : mSets(factory->mNodeSets)
        {
            mSets.pushSet();
        }
        ~TNodeSetPropagatingMaintainer() { mSets.popSetIntoNext(); }
      protected:
        TNodeSetStack &mSets;
    };

    
    
    
    
    
    
    
    
    
    
    
    
    class TLeftmostSymbolMaintainer
    {
      public:
        TLeftmostSymbolMaintainer(
            TDependencyGraphBuilder *factory, TGraphSymbol &subtree)
            : mLeftmostSymbols(factory->mLeftmostSymbols)
        {
            mNeedsPlaceholderSymbol =
                mLeftmostSymbols.empty() || mLeftmostSymbols.top() != &subtree;
            if (mNeedsPlaceholderSymbol)
                mLeftmostSymbols.push(&subtree);
        }

        ~TLeftmostSymbolMaintainer()
        {
            if (mNeedsPlaceholderSymbol)
                mLeftmostSymbols.pop();
        }

      protected:
        TSymbolStack& mLeftmostSymbols;
        bool mNeedsPlaceholderSymbol;
    };

    TDependencyGraphBuilder(TDependencyGraph *graph)
        : TIntermTraverser(true, false, false),
          mLeftSubtree(NULL),
          mRightSubtree(NULL),
          mGraph(graph) {}
    void build(TIntermNode *intermNode) { intermNode->traverse(this); }

    void connectMultipleNodesToSingleNode(
        TParentNodeSet *nodes, TGraphNode *node) const;

    void visitAssignment(TIntermBinary *);
    void visitLogicalOp(TIntermBinary *);
    void visitBinaryChildren(TIntermBinary *);
    void visitFunctionDefinition(TIntermAggregate *);
    void visitFunctionCall(TIntermAggregate *intermFunctionCall);
    void visitAggregateChildren(TIntermAggregate *);

    TGraphSymbol mLeftSubtree;
    TGraphSymbol mRightSubtree;

    TDependencyGraph *mGraph;
    TNodeSetStack mNodeSets;
    TSymbolStack mLeftmostSymbols;
};

#endif  
