








#ifndef COMPILER_UNFOLD_SHORT_CIRCUIT_AST_H_
#define COMPILER_UNFOLD_SHORT_CIRCUIT_AST_H_

#include "common/angleutils.h"
#include "compiler/translator/IntermNode.h"





class UnfoldShortCircuitAST : public TIntermTraverser
{
  public:
    UnfoldShortCircuitAST() { }

    virtual bool visitBinary(Visit visit, TIntermBinary *);

    void updateTree();

  private:
    struct NodeUpdateEntry
    {
        NodeUpdateEntry(TIntermNode *_parent,
                        TIntermNode *_original,
                        TIntermNode *_replacement)
            : parent(_parent),
              original(_original),
              replacement(_replacement) {}

        TIntermNode *parent;
        TIntermNode *original;
        TIntermNode *replacement;
    };

    
    
    std::vector<NodeUpdateEntry> replacements;

    DISALLOW_COPY_AND_ASSIGN(UnfoldShortCircuitAST);
};

#endif  
