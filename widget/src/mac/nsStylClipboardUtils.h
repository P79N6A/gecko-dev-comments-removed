




































#ifndef nsStylClipboardUtils_h___
#define nsStylClipboardUtils_h___

#include "prtypes.h"
#include "nscore.h"
#include <MacTypes.h>

nsresult CreateStylFromScriptRuns(ScriptCodeRun *scriptCodeRuns,
                                  ItemCount scriptRunOutLen,
                                  char **stylData,
                                  PRInt32 *stylLen);

#endif 
