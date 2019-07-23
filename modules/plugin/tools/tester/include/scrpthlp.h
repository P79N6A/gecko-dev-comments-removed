




































#ifndef __SCRPTHLP_H__
#define __SCRPTHLP_H__

#include "script.h"

ScriptItemStruct * makeScriptItemStruct(NPAPI_Action action = action_invalid, 
                                        DWORD dw1 = DEFAULT_DWARG_VALUE, 
                                        DWORD dw2 = DEFAULT_DWARG_VALUE, 
                                        DWORD dw3 = DEFAULT_DWARG_VALUE, 
                                        DWORD dw4 = DEFAULT_DWARG_VALUE, 
                                        DWORD dw5 = DEFAULT_DWARG_VALUE, 
                                        DWORD dw6 = DEFAULT_DWARG_VALUE, 
                                        DWORD dw7 = DEFAULT_DWARG_VALUE, 
                                        DWORD dwDelay = 0L);

#endif 
