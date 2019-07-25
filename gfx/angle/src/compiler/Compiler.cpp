





#include "compiler/BuiltInFunctionEmulator.h"
#include "compiler/DetectRecursion.h"
#include "compiler/ForLoopUnroll.h"
#include "compiler/Initialize.h"
#include "compiler/ParseHelper.h"
#include "compiler/ShHandle.h"
#include "compiler/ValidateLimitations.h"
#include "compiler/MapLongVariableNames.h"

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
}

TCompiler::~TCompiler()
{
}

bool TCompiler::Init(const ShBuiltInResources& resources)
{
    TScopedPoolAllocator scopedAlloc(&allocator, false);

    
    if (!InitBuiltInSymbolTable(resources))
        return false;
    InitExtensionBehavior(resources, extensionBehavior);

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

    
    if (shaderSpec == SH_WEBGL_SPEC)
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

        
        if (success && (compileOptions & SH_UNROLL_FOR_LOOP_WITH_INTEGER_INDEX))
            ForLoopUnroll::MarkForLoopsWithIntegerIndicesForUnrolling(root);

        
        if (success && (compileOptions & SH_EMULATE_BUILT_IN_FUNCTIONS))
            builtInFunctionEmulator.MarkBuiltInFunctionsForEmulation(root);

        
        
        
        if (success && (compileOptions & SH_MAP_LONG_VARIABLE_NAMES))
            mapLongVariableNames(root);

        if (success && (compileOptions & SH_ATTRIBUTES_UNIFORMS))
            collectAttribsUniforms(root);

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

bool TCompiler::validateLimitations(TIntermNode* root) {
    ValidateLimitations validate(shaderType, infoSink.info);
    root->traverse(&validate);
    return validate.numErrors() == 0;
}

void TCompiler::collectAttribsUniforms(TIntermNode* root)
{
    CollectAttribsUniforms collect(attribs, uniforms);
    root->traverse(&collect);
}

void TCompiler::mapLongVariableNames(TIntermNode* root)
{
    MapLongVariableNames map(varyingLongNameMap);
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
