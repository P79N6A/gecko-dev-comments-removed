





#include "compiler/InitializeDll.h"

#include "compiler/InitializeGlobals.h"
#include "compiler/InitializeParseContext.h"
#include "compiler/osinclude.h"

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
