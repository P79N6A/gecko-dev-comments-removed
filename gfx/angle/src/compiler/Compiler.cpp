





#include "compiler/BuiltInFunctionEmulator.h"
#include "compiler/DetectRecursion.h"
#include "compiler/ForLoopUnroll.h"
#include "compiler/Initialize.h"
#include "compiler/InitializeParseContext.h"
#include "compiler/MapLongVariableNames.h"
#include "compiler/ParseHelper.h"
#include "compiler/RenameFunction.h"
#include "compiler/ShHandle.h"
#include "compiler/ValidateLimitations.h"
#include "compiler/VariablePacker.h"
#include "compiler/depgraph/DependencyGraph.h"
#include "compiler/depgraph/DependencyGraphOutput.h"
#include "compiler/timing/RestrictFragmentShaderTiming.h"
#include "compiler/timing/RestrictVertexShaderTiming.h"

bool isWebGLBasedSpec(ShShaderSpec spec)
{
     return spec == SH_WEBGL_SPEC || spec == SH_CSS_SHADERS_SPEC;
}

namespace {
bool InitializeSymbolTable(
    const TBuiltInStrings& builtInStrings,
    ShShaderType type, ShShaderSpec spec, const ShBuiltInResources& resources,
    TInfoSink& infoSink, TSymbolTable& symbolTable)
{
    TIntermediate intermediate(infoSink);
    TExtensionBehavior extBehavior;
    InitExtensionBehavior(resources, extBehavior);
    
    
    TParseContext parseContext(symbolTable, extBehavior, intermediate, type, spec, 0, false, NULL, infoSink);

    GlobalParseContext = &parseContext;

    assert(symbolTable.isEmpty());       
    
    
    
    
    
    
    
    
    symbolTable.push();

    for (TBuiltInStrings::const_iterator i = builtInStrings.begin(); i != builtInStrings.end(); ++i)
    {
        const char* builtInShaders = i->c_str();
        int builtInLengths = static_cast<int>(i->size());
        if (builtInLengths <= 0)
          continue;

        if (PaParseStrings(1, &builtInShaders, &builtInLengths, &parseContext) != 0)
        {
            infoSink.info.message(EPrefixInternalError, "Unable to parse built-ins");
            return false;
        }
    }

    IdentifyBuiltIns(type, spec, resources, symbolTable);

    return true;
}

class TScopedPoolAllocator {
public:
    TScopedPoolAllocator(TPoolAllocator* allocator, bool pushPop)
        : mAllocator(allocator), mPushPopAllocator(pushPop) {
        if (mPushPopAllocator) mAllocator->push();
        SetGlobalPoolAllocator(mAllocator);
    }
    ~TScopedPoolAllocator() {
        SetGlobalPoolAllocator(NULL);
        if (mPushPopAllocator) mAllocator->pop();
    }

private:
    TPoolAllocator* mAllocator;
    bool mPushPopAllocator;
};
}  

TShHandleBase::TShHandleBase() {
    allocator.push();
    SetGlobalPoolAllocator(&allocator);
}

TShHandleBase::~TShHandleBase() {
    SetGlobalPoolAllocator(NULL);
    allocator.popAll();
}

TCompiler::TCompiler(ShShaderType type, ShShaderSpec spec)
    : shaderType(type),
      shaderSpec(spec),
      builtInFunctionEmulator(type)
{
    longNameMap = LongNameMap::GetInstance();
}

TCompiler::~TCompiler()
{
    ASSERT(longNameMap);
    longNameMap->Release();
}

bool TCompiler::Init(const ShBuiltInResources& resources)
{
    maxUniformVectors = (shaderType == SH_VERTEX_SHADER) ?
        resources.MaxVertexUniformVectors :
        resources.MaxFragmentUniformVectors;
    TScopedPoolAllocator scopedAlloc(&allocator, false);

    
    if (!InitBuiltInSymbolTable(resources))
        return false;
    InitExtensionBehavior(resources, extensionBehavior);

    hashFunction = resources.HashFunction;

    return true;
}

bool TCompiler::compile(const char* const shaderStrings[],
                        const int numStrings,
                        int compileOptions)
{
    TScopedPoolAllocator scopedAlloc(&allocator, true);
    clearResults();

    if (numStrings == 0)
        return true;

    
    if (isWebGLBasedSpec(shaderSpec))
        compileOptions |= SH_VALIDATE_LOOP_INDEXING;

    
    const char* sourcePath = NULL;
    int firstSource = 0;
    if (compileOptions & SH_SOURCE_PATH)
    {
        sourcePath = shaderStrings[0];
        ++firstSource;
    }

    TIntermediate intermediate(infoSink);
    TParseContext parseContext(symbolTable, extensionBehavior, intermediate,
                               shaderType, shaderSpec, compileOptions, true,
                               sourcePath, infoSink);
    GlobalParseContext = &parseContext;

    
    
    symbolTable.push();
    if (!symbolTable.atGlobalLevel())
        infoSink.info.message(EPrefixInternalError, "Wrong symbol table level");

    
    bool success =
        (PaParseStrings(numStrings - firstSource, &shaderStrings[firstSource], NULL, &parseContext) == 0) &&
        (parseContext.treeRoot != NULL);
    if (success) {
        TIntermNode* root = parseContext.treeRoot;
        success = intermediate.postProcess(root);

        if (success)
            success = detectRecursion(root);

        if (success && (compileOptions & SH_VALIDATE_LOOP_INDEXING))
            success = validateLimitations(root);

        if (success && (compileOptions & SH_TIMING_RESTRICTIONS))
            success = enforceTimingRestrictions(root, (compileOptions & SH_DEPENDENCY_GRAPH) != 0);

        if (success && shaderSpec == SH_CSS_SHADERS_SPEC)
            rewriteCSSShader(root);

        
        if (success && (compileOptions & SH_UNROLL_FOR_LOOP_WITH_INTEGER_INDEX))
            ForLoopUnroll::MarkForLoopsWithIntegerIndicesForUnrolling(root);

        
        if (success && (compileOptions & SH_EMULATE_BUILT_IN_FUNCTIONS))
            builtInFunctionEmulator.MarkBuiltInFunctionsForEmulation(root);

        
        
        
        
        if (success && (compileOptions & SH_MAP_LONG_VARIABLE_NAMES) && hashFunction == NULL)
            mapLongVariableNames(root);

        if (success && (compileOptions & SH_ATTRIBUTES_UNIFORMS)) {
            collectAttribsUniforms(root);
            if (compileOptions & SH_ENFORCE_PACKING_RESTRICTIONS) {
                success = enforcePackingRestrictions();
                if (!success) {
                    infoSink.info.message(EPrefixError, "too many uniforms");
                }
            }
        }

        if (success && (compileOptions & SH_INTERMEDIATE_TREE))
            intermediate.outputTree(root);

        if (success && (compileOptions & SH_OBJECT_CODE))
            translate(root);
    }

    
    intermediate.remove(parseContext.treeRoot);
    
    
    while (!symbolTable.atBuiltInLevel())
        symbolTable.pop();

    return success;
}

bool TCompiler::InitBuiltInSymbolTable(const ShBuiltInResources& resources)
{
    TBuiltIns builtIns;

    builtIns.initialize(shaderType, shaderSpec, resources);
    return InitializeSymbolTable(builtIns.getBuiltInStrings(),
        shaderType, shaderSpec, resources, infoSink, symbolTable);
}

void TCompiler::clearResults()
{
    infoSink.info.erase();
    infoSink.obj.erase();
    infoSink.debug.erase();

    attribs.clear();
    uniforms.clear();

    builtInFunctionEmulator.Cleanup();

    nameMap.clear();
}

bool TCompiler::detectRecursion(TIntermNode* root)
{
    DetectRecursion detect;
    root->traverse(&detect);
    switch (detect.detectRecursion()) {
        case DetectRecursion::kErrorNone:
            return true;
        case DetectRecursion::kErrorMissingMain:
            infoSink.info.message(EPrefixError, "Missing main()");
            return false;
        case DetectRecursion::kErrorRecursion:
            infoSink.info.message(EPrefixError, "Function recursion detected");
            return false;
        default:
            UNREACHABLE();
            return false;
    }
}

void TCompiler::rewriteCSSShader(TIntermNode* root)
{
    RenameFunction renamer("main(", "css_main(");
    root->traverse(&renamer);
}

bool TCompiler::validateLimitations(TIntermNode* root) {
    ValidateLimitations validate(shaderType, infoSink.info);
    root->traverse(&validate);
    return validate.numErrors() == 0;
}

bool TCompiler::enforceTimingRestrictions(TIntermNode* root, bool outputGraph)
{
    if (shaderSpec != SH_WEBGL_SPEC) {
        infoSink.info << "Timing restrictions must be enforced under the WebGL spec.";
        return false;
    }

    if (shaderType == SH_FRAGMENT_SHADER) {
        TDependencyGraph graph(root);

        
        bool success = enforceFragmentShaderTimingRestrictions(graph);
        
        
        if (outputGraph) {
            TDependencyGraphOutput output(infoSink.info);
            output.outputAllSpanningTrees(graph);
        }
        
        return success;
    }
    else {
        return enforceVertexShaderTimingRestrictions(root);
    }
}

bool TCompiler::enforceFragmentShaderTimingRestrictions(const TDependencyGraph& graph)
{
    RestrictFragmentShaderTiming restrictor(infoSink.info);
    restrictor.enforceRestrictions(graph);
    return restrictor.numErrors() == 0;
}

bool TCompiler::enforceVertexShaderTimingRestrictions(TIntermNode* root)
{
    RestrictVertexShaderTiming restrictor(infoSink.info);
    restrictor.enforceRestrictions(root);
    return restrictor.numErrors() == 0;
}

void TCompiler::collectAttribsUniforms(TIntermNode* root)
{
    CollectAttribsUniforms collect(attribs, uniforms, hashFunction);
    root->traverse(&collect);
}

bool TCompiler::enforcePackingRestrictions()
{
    VariablePacker packer;
    return packer.CheckVariablesWithinPackingLimits(maxUniformVectors, uniforms);
}

void TCompiler::mapLongVariableNames(TIntermNode* root)
{
    ASSERT(longNameMap);
    MapLongVariableNames map(longNameMap);
    root->traverse(&map);
}

int TCompiler::getMappedNameMaxLength() const
{
    return MAX_SHORTENED_IDENTIFIER_SIZE + 1;
}

const TExtensionBehavior& TCompiler::getExtensionBehavior() const
{
    return extensionBehavior;
}

const BuiltInFunctionEmulator& TCompiler::getBuiltInFunctionEmulator() const
{
    return builtInFunctionEmulator;
}
