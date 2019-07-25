





































#include "txXSLTProcessor.h"
#include "txInstructions.h"
#include "nsGkAtoms.h"
#include "txLog.h"
#include "txStylesheetCompileHandlers.h"
#include "txStylesheetCompiler.h"
#include "txExecutionState.h"
#include "txExprResult.h"

TX_LG_IMPL


MBool
txXSLTProcessor::init()
{
    TX_LG_CREATE;

    if (!txHandlerTable::init())
        return MB_FALSE;

    extern bool TX_InitEXSLTFunction();
    if (!TX_InitEXSLTFunction())
        return MB_FALSE;

    return MB_TRUE;
}


void
txXSLTProcessor::shutdown()
{
    txStylesheetCompilerState::shutdown();
    txHandlerTable::shutdown();
}



nsresult
txXSLTProcessor::execute(txExecutionState& aEs)
{
    nsresult rv = NS_OK;
    txInstruction* instr;
    while ((instr = aEs.getNextInstruction())) {
        rv = instr->execute(aEs);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}
