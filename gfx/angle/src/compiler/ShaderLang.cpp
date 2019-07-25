










#include "GLSLANG/ShaderLang.h"

#include "compiler/Initialize.h"
#include "compiler/InitializeDll.h"
#include "compiler/ParseHelper.h"
#include "compiler/ShHandle.h"
#include "compiler/SymbolTable.h"

static bool InitializeSymbolTable(
        const TBuiltInStrings& builtInStrings,
        EShLanguage language, EShSpec spec, const TBuiltInResource& resources,
        TInfoSink& infoSink, TSymbolTable& symbolTable)
{
    TIntermediate intermediate(infoSink);
    TParseContext parseContext(symbolTable, intermediate, language, spec, infoSink);

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
        const char* builtInShaders[1];
        int builtInLengths[1];

        builtInShaders[0] = (*i).c_str();
        builtInLengths[0] = (int) (*i).size();

        if (PaParseStrings(const_cast<char**>(builtInShaders), builtInLengths, 1, parseContext) != 0)
        {
            infoSink.info.message(EPrefixInternalError, "Unable to parse built-ins");
            return false;
        }
    }

    IdentifyBuiltIns(language, spec, resources, symbolTable);

    FinalizePreprocessor();

    return true;
}

static bool GenerateBuiltInSymbolTable(
        EShLanguage language, EShSpec spec, const TBuiltInResource& resources,
        TInfoSink& infoSink, TSymbolTable& symbolTable)
{
    TBuiltIns builtIns;

    builtIns.initialize(language, spec, resources);
    return InitializeSymbolTable(builtIns.getBuiltInStrings(), language, spec, resources, infoSink, symbolTable);
}










int ShInitialize()
{
    if (!InitProcess())
        return 0;

    return 1;
}





ShHandle ShConstructCompiler(EShLanguage language, EShSpec spec, const TBuiltInResource* resources)
{
    if (!InitThread())
        return 0;

    TShHandleBase* base = static_cast<TShHandleBase*>(ConstructCompiler(language, spec));
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return 0;

    
    if (!GenerateBuiltInSymbolTable(language, spec, *resources, compiler->getInfoSink(), compiler->getSymbolTable())) {
        ShDestruct(base);
        return 0;
    }

    return reinterpret_cast<void*>(base);
}

void ShDestruct(ShHandle handle)
{
    if (handle == 0)
        return;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);

    if (base->getAsCompiler())
        DeleteCompiler(base->getAsCompiler());
}




int ShFinalize()
{
    if (!DetachProcess())
        return 0;

    return 1;
}








int ShCompile(
    const ShHandle handle,
    const char* const shaderStrings[],
    const int numStrings,
    const EShOptimizationLevel optLevel,
    int debugOptions
    )
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = reinterpret_cast<TShHandleBase*>(handle);
    TCompiler* compiler = base->getAsCompiler();
    if (compiler == 0)
        return 0;
    
    GlobalPoolAllocator.push();
    TInfoSink& infoSink = compiler->getInfoSink();
    infoSink.info.erase();
    infoSink.debug.erase();
    infoSink.obj.erase();

    if (numStrings == 0)
        return 1;

    TIntermediate intermediate(infoSink);
    TSymbolTable& symbolTable = compiler->getSymbolTable();

    TParseContext parseContext(symbolTable, intermediate, compiler->getLanguage(), compiler->getSpec(), infoSink);
    parseContext.initializeExtensionBehavior();
    GlobalParseContext = &parseContext;
 
    setInitialState();

    InitPreprocessor();
    
    
    
    
    
    bool success = true;

    symbolTable.push();
    if (!symbolTable.atGlobalLevel())
        parseContext.infoSink.info.message(EPrefixInternalError, "Wrong symbol table level");

    int ret = PaParseStrings(const_cast<char**>(shaderStrings), 0, numStrings, parseContext);
    if (ret)
        success = false;

    if (success && parseContext.treeRoot) {
        if (optLevel == EShOptNoGeneration)
            parseContext.infoSink.info.message(EPrefixNone, "No errors.  No code generation was requested.");
        else {
            success = intermediate.postProcess(parseContext.treeRoot, parseContext.language);

            if (success) {

                if (debugOptions & EDebugOpIntermediate)
                    intermediate.outputTree(parseContext.treeRoot);

                
                
                
                if (!compiler->compile(parseContext.treeRoot))
                    success = false;
            }
        }
    } else if (!success) {
        parseContext.infoSink.info.prefix(EPrefixError);
        parseContext.infoSink.info << parseContext.numErrors << " compilation errors.  No code generated.\n\n";
        success = false;
        if (debugOptions & EDebugOpIntermediate)
            intermediate.outputTree(parseContext.treeRoot);
    } else if (!parseContext.treeRoot) {
        parseContext.error(1, "Unexpected end of file.", "", "");
        parseContext.infoSink.info << parseContext.numErrors << " compilation errors.  No code generated.\n\n";
        success = false;
        if (debugOptions & EDebugOpIntermediate)
            intermediate.outputTree(parseContext.treeRoot);
    }

    intermediate.remove(parseContext.treeRoot);

    
    
    
    
    while (!symbolTable.atBuiltInLevel())
        symbolTable.pop();

    FinalizePreprocessor();
    
    
    
    GlobalPoolAllocator.pop();

    return success ? 1 : 0;
}




const char* ShGetInfoLog(const ShHandle handle)
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TInfoSink* infoSink = 0;

    if (base->getAsCompiler())
        infoSink = &(base->getAsCompiler()->getInfoSink());

    infoSink->info << infoSink->debug.c_str();
    return infoSink->info.c_str();
}




const char* ShGetObjectCode(const ShHandle handle)
{
    if (!InitThread())
        return 0;

    if (handle == 0)
        return 0;

    TShHandleBase* base = static_cast<TShHandleBase*>(handle);
    TInfoSink* infoSink;

    if (base->getAsCompiler())
        infoSink = &(base->getAsCompiler()->getInfoSink());

    return infoSink->obj.c_str();
}
