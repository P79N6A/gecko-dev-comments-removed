





#include "compiler/Initialize.h"
#include "compiler/ParseHelper.h"
#include "compiler/ShHandle.h"

static bool InitializeSymbolTable(
        const TBuiltInStrings& builtInStrings,
        ShShaderType type, ShShaderSpec spec, const ShBuiltInResources& resources,
        TInfoSink& infoSink, TSymbolTable& symbolTable)
{
    TIntermediate intermediate(infoSink);
    TExtensionBehavior extBehavior;
    TParseContext parseContext(symbolTable, extBehavior, intermediate, type, spec, infoSink);

    GlobalParseContext = &parseContext;

    setInitialState();

    assert(symbolTable.isEmpty());       
    
    
    
    
    
    
    
    
    symbolTable.push();
    
    
    if (InitPreprocessor())
    {
        infoSink.info.message(EPrefixInternalError,  "Unable to intialize the Preprocessor");
        return false;
    }

    for (TBuiltInStrings::const_iterator i = builtInStrings.begin(); i != builtInStrings.end(); ++i)
    {
        const char* builtInShaders = i->c_str();
        int builtInLengths = static_cast<int>(i->size());
        if (builtInLengths <= 0)
          continue;

        if (PaParseStrings(&builtInShaders, &builtInLengths, 1, parseContext) != 0)
        {
            infoSink.info.message(EPrefixInternalError, "Unable to parse built-ins");
            return false;
        }
    }

    IdentifyBuiltIns(type, spec, resources, symbolTable);

    FinalizePreprocessor();

    return true;
}

static void DefineExtensionMacros(const TExtensionBehavior& extBehavior)
{
    for (TExtensionBehavior::const_iterator iter = extBehavior.begin();
         iter != extBehavior.end(); ++iter) {
        PredefineIntMacro(iter->first.c_str(), 1);
    }
}

TCompiler::TCompiler(ShShaderType type, ShShaderSpec spec)
    : shaderType(type),
      shaderSpec(spec) 
{
}

TCompiler::~TCompiler()
{
}

bool TCompiler::Init(const ShBuiltInResources& resources)
{
    
    if (!InitBuiltInSymbolTable(resources))
        return false;

    InitExtensionBehavior(resources, extensionBehavior);
    return true;
}

bool TCompiler::compile(const char* const shaderStrings[],
                        const int numStrings,
                        int compileOptions)
{
    clearResults();

    if (numStrings == 0)
        return true;

    TIntermediate intermediate(infoSink);
    TParseContext parseContext(symbolTable, extensionBehavior, intermediate,
                               shaderType, shaderSpec, infoSink);
    GlobalParseContext = &parseContext;
    setInitialState();

    
    InitPreprocessor();
    DefineExtensionMacros(extensionBehavior);

    
    
    symbolTable.push();
    if (!symbolTable.atGlobalLevel())
        infoSink.info.message(EPrefixInternalError, "Wrong symbol table level");

    
    bool success =
        (PaParseStrings(shaderStrings, 0, numStrings, parseContext) == 0) &&
        (parseContext.treeRoot != NULL);
    if (success) {
        success = intermediate.postProcess(parseContext.treeRoot);

        if (success && (compileOptions & SH_INTERMEDIATE_TREE))
            intermediate.outputTree(parseContext.treeRoot);

        if (success && (compileOptions & SH_OBJECT_CODE))
            translate(parseContext.treeRoot);

        if (success && (compileOptions & SH_ATTRIBUTES_UNIFORMS))
            collectAttribsUniforms(parseContext.treeRoot);
    }

    
    intermediate.remove(parseContext.treeRoot);
    
    
    while (!symbolTable.atBuiltInLevel())
        symbolTable.pop();
    FinalizePreprocessor();

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
}

void TCompiler::collectAttribsUniforms(TIntermNode* root)
{
    CollectAttribsUniforms collect(attribs, uniforms);
    root->traverse(&collect);
}

