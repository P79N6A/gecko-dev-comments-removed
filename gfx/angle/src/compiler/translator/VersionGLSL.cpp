





#include "compiler/translator/VersionGLSL.h"

static const int GLSL_VERSION_110 = 110;
static const int GLSL_VERSION_120 = 120;



























TVersionGLSL::TVersionGLSL(ShShaderType type)
    : mVersion(GLSL_VERSION_110)
{
}

void TVersionGLSL::visitSymbol(TIntermSymbol* node)
{
    if (node->getSymbol() == "gl_PointCoord")
        updateVersion(GLSL_VERSION_120);
}

void TVersionGLSL::visitConstantUnion(TIntermConstantUnion*)
{
}

bool TVersionGLSL::visitBinary(Visit, TIntermBinary*)
{
    return true;
}

bool TVersionGLSL::visitUnary(Visit, TIntermUnary*)
{
    return true;
}

bool TVersionGLSL::visitSelection(Visit, TIntermSelection*)
{
    return true;
}

bool TVersionGLSL::visitAggregate(Visit, TIntermAggregate* node)
{
    bool visitChildren = true;

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
      case EOpParameters: {
        const TIntermSequence& params = node->getSequence();
        for (TIntermSequence::const_iterator iter = params.begin();
             iter != params.end(); ++iter)
        {
            const TIntermTyped* param = (*iter)->getAsTyped();
            if (param->isArray())
            {
                TQualifier qualifier = param->getQualifier();
                if ((qualifier == EvqOut) || (qualifier ==  EvqInOut))
                {
                    updateVersion(GLSL_VERSION_120);
                    break;
                }
            }
        }
        
        visitChildren = false;
        break;
      }
      case EOpConstructMat2:
      case EOpConstructMat3:
      case EOpConstructMat4: {
        const TIntermSequence& sequence = node->getSequence();
        if (sequence.size() == 1) {
          TIntermTyped* typed = sequence.front()->getAsTyped();
          if (typed && typed->isMatrix()) {
            updateVersion(GLSL_VERSION_120);
          }
        }
        break;
      }

      default: break;
    }

    return visitChildren;
}

bool TVersionGLSL::visitLoop(Visit, TIntermLoop*)
{
    return true;
}

bool TVersionGLSL::visitBranch(Visit, TIntermBranch*)
{
    return true;
}

void TVersionGLSL::updateVersion(int version)
{
    mVersion = std::max(version, mVersion);
}

