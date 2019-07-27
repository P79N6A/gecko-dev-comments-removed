









#include "compiler/translator/UnfoldShortCircuit.h"

#include "compiler/translator/InfoSink.h"
#include "compiler/translator/OutputHLSL.h"
#include "compiler/translator/UtilsHLSL.h"

namespace sh
{
UnfoldShortCircuit::UnfoldShortCircuit(TParseContext &context, OutputHLSL *outputHLSL) : mContext(context), mOutputHLSL(outputHLSL)
{
    mTemporaryIndex = 0;
}

void UnfoldShortCircuit::traverse(TIntermNode *node)
{
    int rewindIndex = mTemporaryIndex;
    node->traverse(this);
    mTemporaryIndex = rewindIndex;
}

bool UnfoldShortCircuit::visitBinary(Visit visit, TIntermBinary *node)
{
    TInfoSinkBase &out = mOutputHLSL->getBodyStream();

    
    
    
    if (!node->getRight()->hasSideEffects())
    {
        return true;
    }

    switch (node->getOp())
    {
      case EOpLogicalOr:
        
        
        {
            int i = mTemporaryIndex;

            out << "bool s" << i << ";\n";

            out << "{\n";

            mTemporaryIndex = i + 1;
            node->getLeft()->traverse(this);
            out << "s" << i << " = ";
            mTemporaryIndex = i + 1;
            node->getLeft()->traverse(mOutputHLSL);
            out << ";\n";
            out << "if (!s" << i << ")\n"
                   "{\n";
            mTemporaryIndex = i + 1;
            node->getRight()->traverse(this);
            out << "    s" << i << " = ";
            mTemporaryIndex = i + 1;
            node->getRight()->traverse(mOutputHLSL);
            out << ";\n"
                   "}\n";

            out << "}\n";

            mTemporaryIndex = i + 1;
        }
        return false;
      case EOpLogicalAnd:
        
        
        {
            int i = mTemporaryIndex;

            out << "bool s" << i << ";\n";

            out << "{\n";

            mTemporaryIndex = i + 1;
            node->getLeft()->traverse(this);
            out << "s" << i << " = ";
            mTemporaryIndex = i + 1;
            node->getLeft()->traverse(mOutputHLSL);
            out << ";\n";
            out << "if (s" << i << ")\n"
                   "{\n";
            mTemporaryIndex = i + 1;
            node->getRight()->traverse(this);
            out << "    s" << i << " = ";
            mTemporaryIndex = i + 1;
            node->getRight()->traverse(mOutputHLSL);
            out << ";\n"
                   "}\n";

            out << "}\n";

            mTemporaryIndex = i + 1;
        }
        return false;
      default:
        return true;
    }
}

bool UnfoldShortCircuit::visitSelection(Visit visit, TIntermSelection *node)
{
    TInfoSinkBase &out = mOutputHLSL->getBodyStream();

    
    if (node->usesTernaryOperator())
    {
        int i = mTemporaryIndex;

        out << TypeString(node->getType()) << " s" << i << ";\n";

        out << "{\n";

        mTemporaryIndex = i + 1;
        node->getCondition()->traverse(this);
        out << "if (";
        mTemporaryIndex = i + 1;
        node->getCondition()->traverse(mOutputHLSL);
        out << ")\n"
               "{\n";
        mTemporaryIndex = i + 1;
        node->getTrueBlock()->traverse(this);
        out << "    s" << i << " = ";
        mTemporaryIndex = i + 1;
        node->getTrueBlock()->traverse(mOutputHLSL);
        out << ";\n"
               "}\n"
               "else\n"
               "{\n";
        mTemporaryIndex = i + 1;
        node->getFalseBlock()->traverse(this);
        out << "    s" << i << " = ";
        mTemporaryIndex = i + 1;
        node->getFalseBlock()->traverse(mOutputHLSL);
        out << ";\n"
               "}\n";

        out << "}\n";

        mTemporaryIndex = i + 1;
    }

    return false;
}

bool UnfoldShortCircuit::visitLoop(Visit visit, TIntermLoop *node)
{
    int rewindIndex = mTemporaryIndex;

    if (node->getInit())
    {
        node->getInit()->traverse(this);
    }
    
    if (node->getCondition())
    {
        node->getCondition()->traverse(this);
    }

    if (node->getExpression())
    {
        node->getExpression()->traverse(this);
    }

    mTemporaryIndex = rewindIndex;

    return false;
}

int UnfoldShortCircuit::getNextTemporaryIndex()
{
    return mTemporaryIndex++;
}
}
