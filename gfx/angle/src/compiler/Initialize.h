





#ifndef _INITIALIZE_INCLUDED_
#define _INITIALIZE_INCLUDED_

#include "compiler/Common.h"
#include "compiler/ShHandle.h"
#include "compiler/SymbolTable.h"

typedef TVector<TString> TBuiltInStrings;

class TBuiltIns {
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)

    void initialize(ShShaderType type, ShShaderSpec spec,
                    const ShBuiltInResources& resources);
    const TBuiltInStrings& getBuiltInStrings() { return builtInStrings; }

protected:
    TBuiltInStrings builtInStrings;
};

void IdentifyBuiltIns(ShShaderType type, ShShaderSpec spec,
                      const ShBuiltInResources& resources,
                      TSymbolTable& symbolTable);

void InitExtensionBehavior(const ShBuiltInResources& resources,
                           TExtensionBehavior& extensionBehavior);

extern "C" int InitPreprocessor(void);
extern "C" int FinalizePreprocessor(void);
extern "C" void PredefineIntMacro(const char *name, int value);

#endif 
