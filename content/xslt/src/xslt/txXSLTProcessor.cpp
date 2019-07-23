





































#include "txXSLTProcessor.h"
#include "txInstructions.h"
#include "txAtoms.h"
#include "txLog.h"
#include "txStylesheetCompileHandlers.h"
#include "txStylesheetCompiler.h"
#include "txExecutionState.h"
#include "txExprResult.h"
#ifdef TX_EXE
#include "txHTMLOutput.h"
#endif

TX_LG_IMPL


MBool
txXSLTProcessor::init()
{
    TX_LG_CREATE;

#ifdef TX_EXE
    if (!txStandaloneNamespaceManager::init())
        return MB_FALSE;

    if (NS_FAILED(txHTMLOutput::init())) {
        return MB_FALSE;
    }

    txXMLAtoms::init();
    txXPathAtoms::init();
    txXSLTAtoms::init();
    txHTMLAtoms::init();
#endif
    
    if (!txHandlerTable::init())
        return MB_FALSE;

    extern PRBool TX_InitEXSLTFunction();
    if (!TX_InitEXSLTFunction())
        return MB_FALSE;

    return MB_TRUE;
}


void
txXSLTProcessor::shutdown()
{
#ifdef TX_EXE
    txStandaloneNamespaceManager::shutdown();
    txHTMLOutput::shutdown();
#endif

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
