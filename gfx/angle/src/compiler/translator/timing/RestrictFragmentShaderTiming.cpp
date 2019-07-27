





#include "compiler/translator/InfoSink.h"
#include "compiler/translator/ParseContext.h"
#include "compiler/translator/depgraph/DependencyGraphOutput.h"
#include "compiler/translator/timing/RestrictFragmentShaderTiming.h"

RestrictFragmentShaderTiming::RestrictFragmentShaderTiming(TInfoSinkBase& sink)
    : mSink(sink)
    , mNumErrors(0)
{
    
    mSamplingOps.insert("texture2D(s21;vf2;f1;");
    mSamplingOps.insert("texture2DProj(s21;vf3;f1;");
    mSamplingOps.insert("texture2DProj(s21;vf4;f1;");
    mSamplingOps.insert("textureCube(sC1;vf3;f1;");
    
    mSamplingOps.insert("texture2D(s21;vf2;");
    mSamplingOps.insert("texture2DProj(s21;vf3;");
    mSamplingOps.insert("texture2DProj(s21;vf4;");
    mSamplingOps.insert("textureCube(sC1;vf3;");
    
    mSamplingOps.insert("texture2D(1;vf2;");
    mSamplingOps.insert("texture2DProj(1;vf3;");
    mSamplingOps.insert("texture2DProj(1;vf4;");
    
    mSamplingOps.insert("texture2DRect(1;vf2;");
    mSamplingOps.insert("texture2DRectProj(1;vf3;");
    mSamplingOps.insert("texture2DRectProj(1;vf4;");
    
    mSamplingOps.insert("texture2DLodEXT(1;vf2;f1;");
    mSamplingOps.insert("texture2DProjLodEXT(1;vf3;f1;");
    mSamplingOps.insert("texture2DProjLodEXT(1;vf4;f1;");
    mSamplingOps.insert("textureCubeLodEXT(1;vf4;f1;");
    mSamplingOps.insert("texture2DGradEXT(1;vf2;vf2;vf2;");
    mSamplingOps.insert("texture2DProjGradEXT(1;vf3;vf2;vf2;");
    mSamplingOps.insert("texture2DProjGradEXT(1;vf4;vf2;vf2;");
    mSamplingOps.insert("textureCubeGradEXT(1;vf3;vf3;vf3;");
}



void RestrictFragmentShaderTiming::enforceRestrictions(const TDependencyGraph& graph)
{
    mNumErrors = 0;

    
    
    validateUserDefinedFunctionCallUsage(graph);

    
    
    for (TGraphSymbolVector::const_iterator iter = graph.beginSamplerSymbols();
         iter != graph.endSamplerSymbols();
         ++iter)
    {
        TGraphSymbol* samplerSymbol = *iter;
        clearVisited();
        samplerSymbol->traverse(this);
    }
}

void RestrictFragmentShaderTiming::validateUserDefinedFunctionCallUsage(const TDependencyGraph& graph)
{
    for (TFunctionCallVector::const_iterator iter = graph.beginUserDefinedFunctionCalls();
         iter != graph.endUserDefinedFunctionCalls();
         ++iter)
    {
        TGraphFunctionCall* functionCall = *iter;
        beginError(functionCall->getIntermFunctionCall());
        mSink << "A call to a user defined function is not permitted.\n";
    }
}

void RestrictFragmentShaderTiming::beginError(const TIntermNode* node)
{
    ++mNumErrors;
    mSink.prefix(EPrefixError);
    mSink.location(node->getLine());
}

bool RestrictFragmentShaderTiming::isSamplingOp(const TIntermAggregate* intermFunctionCall) const
{
    return !intermFunctionCall->isUserDefined() &&
           mSamplingOps.find(intermFunctionCall->getName()) != mSamplingOps.end();
}

void RestrictFragmentShaderTiming::visitArgument(TGraphArgument* parameter)
{
    
    
    
    if (isSamplingOp(parameter->getIntermFunctionCall())) {
        switch (parameter->getArgumentNumber()) {
            case 1:
                
                beginError(parameter->getIntermFunctionCall());
                mSink << "An expression dependent on a sampler is not permitted to be the"
                      << " coordinate argument of a sampling operation.\n";
                break;
            case 2:
                
                beginError(parameter->getIntermFunctionCall());
                mSink << "An expression dependent on a sampler is not permitted to be the"
                      << " bias argument of a sampling operation.\n";
                break;
            default:
                
                break;
        }
    }
}

void RestrictFragmentShaderTiming::visitSelection(TGraphSelection* selection)
{
    beginError(selection->getIntermSelection());
    mSink << "An expression dependent on a sampler is not permitted in a conditional statement.\n";
}

void RestrictFragmentShaderTiming::visitLoop(TGraphLoop* loop)
{
    beginError(loop->getIntermLoop());
    mSink << "An expression dependent on a sampler is not permitted in a loop condition.\n";
}

void RestrictFragmentShaderTiming::visitLogicalOp(TGraphLogicalOp* logicalOp)
{
    beginError(logicalOp->getIntermLogicalOp());
    mSink << "An expression dependent on a sampler is not permitted on the left hand side of a logical "
          << logicalOp->getOpString()
          << " operator.\n";
}
