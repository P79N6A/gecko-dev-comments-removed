





#include "angle_gl.h"
#include "compiler/translator/BuiltInFunctionEmulator.h"
#include "compiler/translator/SymbolTable.h"

namespace {






const char* kFunctionEmulationVertexSource[] = {
    "#error no emulation for cos(float)",
    "#error no emulation for cos(vec2)",
    "#error no emulation for cos(vec3)",
    "#error no emulation for cos(vec4)",

    "#define webgl_distance_emu(x, y) ((x) >= (y) ? (x) - (y) : (y) - (x))",
    "#error no emulation for distance(vec2, vec2)",
    "#error no emulation for distance(vec3, vec3)",
    "#error no emulation for distance(vec4, vec4)",

    "#define webgl_dot_emu(x, y) ((x) * (y))",
    "#error no emulation for dot(vec2, vec2)",
    "#error no emulation for dot(vec3, vec3)",
    "#error no emulation for dot(vec4, vec4)",

    "#define webgl_length_emu(x) ((x) >= 0.0 ? (x) : -(x))",
    "#error no emulation for length(vec2)",
    "#error no emulation for length(vec3)",
    "#error no emulation for length(vec4)",

    "#define webgl_normalize_emu(x) ((x) == 0.0 ? 0.0 : ((x) > 0.0 ? 1.0 : -1.0))",
    "#error no emulation for normalize(vec2)",
    "#error no emulation for normalize(vec3)",
    "#error no emulation for normalize(vec4)",

    "#define webgl_reflect_emu(I, N) ((I) - 2.0 * (N) * (I) * (N))",
    "#error no emulation for reflect(vec2, vec2)",
    "#error no emulation for reflect(vec3, vec3)",
    "#error no emulation for reflect(vec4, vec4)",

    
    "#define webgl_faceforward_emu(N, I, Nref) (((Nref) * (I) < 0.0) ? (N) : -(N))",
    "#error no emulation for faceforward(vec2, vec2, vec2)",
    "#error no emulation for faceforward(vec3, vec3, vec3)",
    "#error no emulation for faceforward(vec4, vec4, vec4)"
};

const char* kFunctionEmulationFragmentSource[] = {
    "webgl_emu_precision float webgl_cos_emu(webgl_emu_precision float a) { return cos(a); }",
    "webgl_emu_precision vec2 webgl_cos_emu(webgl_emu_precision vec2 a) { return cos(a); }",
    "webgl_emu_precision vec3 webgl_cos_emu(webgl_emu_precision vec3 a) { return cos(a); }",
    "webgl_emu_precision vec4 webgl_cos_emu(webgl_emu_precision vec4 a) { return cos(a); }",

    "#define webgl_distance_emu(x, y) ((x) >= (y) ? (x) - (y) : (y) - (x))",
    "#error no emulation for distance(vec2, vec2)",
    "#error no emulation for distance(vec3, vec3)",
    "#error no emulation for distance(vec4, vec4)",

    "#define webgl_dot_emu(x, y) ((x) * (y))",
    "#error no emulation for dot(vec2, vec2)",
    "#error no emulation for dot(vec3, vec3)",
    "#error no emulation for dot(vec4, vec4)",

    "#define webgl_length_emu(x) ((x) >= 0.0 ? (x) : -(x))",
    "#error no emulation for length(vec2)",
    "#error no emulation for length(vec3)",
    "#error no emulation for length(vec4)",

    "#define webgl_normalize_emu(x) ((x) == 0.0 ? 0.0 : ((x) > 0.0 ? 1.0 : -1.0))",
    "#error no emulation for normalize(vec2)",
    "#error no emulation for normalize(vec3)",
    "#error no emulation for normalize(vec4)",

    "#define webgl_reflect_emu(I, N) ((I) - 2.0 * (N) * (I) * (N))",
    "#error no emulation for reflect(vec2, vec2)",
    "#error no emulation for reflect(vec3, vec3)",
    "#error no emulation for reflect(vec4, vec4)",

    
    "#define webgl_faceforward_emu(N, I, Nref) (((Nref) * (I) < 0.0) ? (N) : -(N))",
    "#error no emulation for faceforward(vec2, vec2, vec2)",
    "#error no emulation for faceforward(vec3, vec3, vec3)",
    "#error no emulation for faceforward(vec4, vec4, vec4)"
};

const bool kFunctionEmulationVertexMask[] = {
#if defined(__APPLE__)
    
    false, 
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
#else
    
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
#endif
    false  
};

const bool kFunctionEmulationFragmentMask[] = {
#if defined(__APPLE__)
    
    true,  
    true,  
    true,  
    true,  
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
    true,  
    false, 
    false, 
    false, 
#else
    
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
    false, 
#endif
    false  
};

class BuiltInFunctionEmulationMarker : public TIntermTraverser {
public:
    BuiltInFunctionEmulationMarker(BuiltInFunctionEmulator& emulator)
        : mEmulator(emulator)
    {
    }

    virtual bool visitUnary(Visit visit, TIntermUnary* node)
    {
        if (visit == PreVisit) {
            bool needToEmulate = mEmulator.SetFunctionCalled(
                node->getOp(), node->getOperand()->getType());
            if (needToEmulate)
                node->setUseEmulatedFunction();
        }
        return true;
    }

    virtual bool visitAggregate(Visit visit, TIntermAggregate* node)
    {
        if (visit == PreVisit) {
            
            
            switch (node->getOp()) {
                case EOpLessThan:
                case EOpGreaterThan:
                case EOpLessThanEqual:
                case EOpGreaterThanEqual:
                case EOpVectorEqual:
                case EOpVectorNotEqual:
                case EOpMod:
                case EOpPow:
                case EOpAtan:
                case EOpMin:
                case EOpMax:
                case EOpClamp:
                case EOpMix:
                case EOpStep:
                case EOpSmoothStep:
                case EOpDistance:
                case EOpDot:
                case EOpCross:
                case EOpFaceForward:
                case EOpReflect:
                case EOpRefract:
                case EOpMul:
                    break;
                default:
                    return true;
            };
            const TIntermSequence& sequence = *(node->getSequence());
            bool needToEmulate = false;

            if (sequence.size() == 2) {
                TIntermTyped* param1 = sequence[0]->getAsTyped();
                TIntermTyped* param2 = sequence[1]->getAsTyped();
                if (!param1 || !param2)
                    return true;
                needToEmulate = mEmulator.SetFunctionCalled(
                    node->getOp(), param1->getType(), param2->getType());
            } else if (sequence.size() == 3) {
                TIntermTyped* param1 = sequence[0]->getAsTyped();
                TIntermTyped* param2 = sequence[1]->getAsTyped();
                TIntermTyped* param3 = sequence[2]->getAsTyped();
                if (!param1 || !param2 || !param3)
                    return true;
                needToEmulate = mEmulator.SetFunctionCalled(
                    node->getOp(), param1->getType(), param2->getType(), param3->getType());
            } else {
                return true;
            }
            if (needToEmulate)
                node->setUseEmulatedFunction();
        }
        return true;
    }

private:
    BuiltInFunctionEmulator& mEmulator;
};

}  

BuiltInFunctionEmulator::BuiltInFunctionEmulator(sh::GLenum shaderType)
{
    if (shaderType == GL_FRAGMENT_SHADER) {
        mFunctionMask = kFunctionEmulationFragmentMask;
        mFunctionSource = kFunctionEmulationFragmentSource;
    } else {
        mFunctionMask = kFunctionEmulationVertexMask;
        mFunctionSource = kFunctionEmulationVertexSource;
    }
}

bool BuiltInFunctionEmulator::SetFunctionCalled(
    TOperator op, const TType& param)
{
    TBuiltInFunction function = IdentifyFunction(op, param);
    return SetFunctionCalled(function);
}

bool BuiltInFunctionEmulator::SetFunctionCalled(
    TOperator op, const TType& param1, const TType& param2)
{
    TBuiltInFunction function = IdentifyFunction(op, param1, param2);
    return SetFunctionCalled(function);
}

bool BuiltInFunctionEmulator::SetFunctionCalled(
    TOperator op, const TType& param1, const TType& param2, const TType& param3)
{
    TBuiltInFunction function = IdentifyFunction(op, param1, param2, param3);
    return SetFunctionCalled(function);
}

bool BuiltInFunctionEmulator::SetFunctionCalled(
    BuiltInFunctionEmulator::TBuiltInFunction function) {
    if (function == TFunctionUnknown || mFunctionMask[function] == false)
        return false;
    for (size_t i = 0; i < mFunctions.size(); ++i) {
        if (mFunctions[i] == function)
            return true;
    }
    mFunctions.push_back(function);
    return true;
}

void BuiltInFunctionEmulator::OutputEmulatedFunctionDefinition(
    TInfoSinkBase& out, bool withPrecision) const
{
    if (mFunctions.size() == 0)
        return;
    out << "// BEGIN: Generated code for built-in function emulation\n\n";
    if (withPrecision) {
        out << "#if defined(GL_FRAGMENT_PRECISION_HIGH)\n"
            << "#define webgl_emu_precision highp\n"
            << "#else\n"
            << "#define webgl_emu_precision mediump\n"
            << "#endif\n\n";
    } else {
        out << "#define webgl_emu_precision\n\n";
    }
    for (size_t i = 0; i < mFunctions.size(); ++i) {
        out << mFunctionSource[mFunctions[i]] << "\n\n";
    }
    out << "// END: Generated code for built-in function emulation\n\n";
}

BuiltInFunctionEmulator::TBuiltInFunction
BuiltInFunctionEmulator::IdentifyFunction(
    TOperator op, const TType& param)
{
    if (param.getNominalSize() > 4 || param.getSecondarySize() > 4)
        return TFunctionUnknown;
    unsigned int function = TFunctionUnknown;
    switch (op) {
        case EOpCos:
            function = TFunctionCos1;
            break;
        case EOpLength:
            function = TFunctionLength1;
            break;
        case EOpNormalize:
            function = TFunctionNormalize1;
            break;
        default:
            break;
    }
    if (function == TFunctionUnknown)
        return TFunctionUnknown;
    if (param.isVector())
        function += param.getNominalSize() - 1;
    return static_cast<TBuiltInFunction>(function);
}

BuiltInFunctionEmulator::TBuiltInFunction
BuiltInFunctionEmulator::IdentifyFunction(
    TOperator op, const TType& param1, const TType& param2)
{
    
    
    if (param1.getNominalSize()     != param2.getNominalSize()   ||
        param1.getSecondarySize()   != param2.getSecondarySize() ||
        param1.getNominalSize() > 4 || param1.getSecondarySize() > 4)
        return TFunctionUnknown;

    unsigned int function = TFunctionUnknown;
    switch (op) {
        case EOpDistance:
            function = TFunctionDistance1_1;
            break;
        case EOpDot:
            function = TFunctionDot1_1;
            break;
        case EOpReflect:
            function = TFunctionReflect1_1;
            break;
        default:
            break;
    }
    if (function == TFunctionUnknown)
        return TFunctionUnknown;
    if (param1.isVector())
        function += param1.getNominalSize() - 1;
    return static_cast<TBuiltInFunction>(function);
}

BuiltInFunctionEmulator::TBuiltInFunction
BuiltInFunctionEmulator::IdentifyFunction(
    TOperator op, const TType& param1, const TType& param2, const TType& param3)
{
    
    
    if (param1.isVector() != param2.isVector() ||
        param2.isVector() != param3.isVector() ||
        param1.getNominalSize() != param2.getNominalSize() ||
        param2.getNominalSize() != param3.getNominalSize() ||
        param1.getNominalSize() > 4)
        return TFunctionUnknown;

    unsigned int function = TFunctionUnknown;
    switch (op) {
        case EOpFaceForward:
            function = TFunctionFaceForward1_1_1;
            break;
        default:
            break;
    }
    if (function == TFunctionUnknown)
        return TFunctionUnknown;
    if (param1.isVector())
        function += param1.getNominalSize() - 1;
    return static_cast<TBuiltInFunction>(function);
}

void BuiltInFunctionEmulator::MarkBuiltInFunctionsForEmulation(
    TIntermNode* root)
{
    ASSERT(root);

    BuiltInFunctionEmulationMarker marker(*this);
    root->traverse(&marker);
}

void BuiltInFunctionEmulator::Cleanup()
{
    mFunctions.clear();
}


TString BuiltInFunctionEmulator::GetEmulatedFunctionName(
    const TString& name)
{
    ASSERT(name[name.length() - 1] == '(');
    return "webgl_" + name.substr(0, name.length() - 1) + "_emu(";
}

