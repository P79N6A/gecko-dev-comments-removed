





#include "compiler/VersionGLSL.h"

static const int GLSL_VERSION_110 = 110;
static const int GLSL_VERSION_120 = 120;


















TVersionGLSL::TVersionGLSL(ShShaderType type)
    : mShaderType(type),
      mVersion(GLSL_VERSION_110)
{
}

void TVersionGLSL::visitSymbol(TIntermSymbol* node)
{
    ASSERT(mShaderType == SH_FRAGMENT_SHADER);

    if (node->getSymbol() == "gl_PointCoord")
        updateVersion(GLSL_VERSION_120);
}

void TVersionGLSL::visitConstantUnion(TIntermConstantUnion*)
{
    ASSERT(mShaderType == SH_FRAGMENT_SHADER);
}

bool TVersionGLSL::visitBinary(Visit, TIntermBinary*)
{
    ASSERT(mShaderType == SH_FRAGMENT_SHADER);
    return true;
}

bool TVersionGLSL::visitUnary(Visit, TIntermUnary*)
{
    ASSERT(mShaderType == SH_FRAGMENT_SHADER);
    return true;
}

bool TVersionGLSL::visitSelection(Visit, TIntermSelection*)
{
    ASSERT(mShaderType == SH_FRAGMENT_SHADER);
    return true;
}

bool TVersionGLSL::visitAggregate(Visit, TIntermAggregate* node)
{
    
    
    bool visitChildren = mShaderType == SH_FRAGMENT_SHADER ? true : false;

    switch (node->getOp()) {
      case EOpSequence:
        
        visitChildren = true;
        break;
      case EOpDeclaration: {
        const TIntermSequence& sequence = node->getSequence();
        TQualifier qualifier = sequence.front()->getAsTyped()->getQualifier();
        if ((qualifier == EvqInvariantVaryingIn) ||
            (qualifier == EvqInvariantVaryingOut)) {
            updateVersion(GLSL_VERSION_120);
        }
        break;
      }
      default: break;
    }

    return visitChildren;
}

bool TVersionGLSL::visitLoop(Visit, TIntermLoop*)
{
    ASSERT(mShaderType == SH_FRAGMENT_SHADER);
    return true;
}

bool TVersionGLSL::visitBranch(Visit, TIntermBranch*)
{
    ASSERT(mShaderType == SH_FRAGMENT_SHADER);
    return true;
}

void TVersionGLSL::updateVersion(int version)
{
    mVersion = std::max(version, mVersion);
}

