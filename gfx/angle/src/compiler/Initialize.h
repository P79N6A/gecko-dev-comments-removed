





#ifndef _INITIALIZE_INCLUDED_
#define _INITIALIZE_INCLUDED_

#include "GLSLANG/ResourceLimits.h"

#include "compiler/Common.h"
#include "compiler/ShHandle.h"
#include "compiler/SymbolTable.h"

typedef TVector<TString> TBuiltInStrings;

class TBuiltIns {
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)

    void initialize(EShLanguage language, EShSpec spec, const TBuiltInResource& resources);
    const TBuiltInStrings& getBuiltInStrings() { return builtInStrings; }

protected:
    TBuiltInStrings builtInStrings;
};

void IdentifyBuiltIns(EShLanguage language, EShSpec spec, const TBuiltInResource& resources,
                      TSymbolTable& symbolTable);

extern "C" int InitPreprocessor(void);
extern "C" int FinalizePreprocessor(void);

#endif 
