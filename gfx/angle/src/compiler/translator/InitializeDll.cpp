





#include "compiler/translator/InitializeDll.h"

#include "compiler/translator/InitializeGlobals.h"
#include "compiler/translator/InitializeParseContext.h"
#include "compiler/translator/osinclude.h"

bool InitProcess()
{
    if (!InitializePoolIndex()) {
        assert(0 && "InitProcess(): Failed to initalize global pool");
        return false;
    }

    if (!InitializeParseContextIndex()) {
        assert(0 && "InitProcess(): Failed to initalize parse context");
        return false;
    }

    return true;
}

void DetachProcess()
{
    FreeParseContextIndex();
    FreePoolIndex();
}
