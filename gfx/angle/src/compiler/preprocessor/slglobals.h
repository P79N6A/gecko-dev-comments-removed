



















































#if !defined(__SLGLOBALS_H)
#define __SLGLOBALS_H 1

typedef struct CPPStruct_Rec CPPStruct;

extern CPPStruct *cpp;

#undef  CPPC_DEBUG_THE_COMPILER
#if defined(_DEBUG)
#define CPPC_DEBUG_THE_COMPILER 1
#endif

#undef  CPPC_ENABLE_TOOLS
#define CPPC_ENABLE_TOOLS 1

#include "compiler/preprocessor/memory.h"
#include "compiler/preprocessor/atom.h"
#include "compiler/preprocessor/scanner.h"
#include "compiler/preprocessor/cpp.h"
#include "compiler/preprocessor/tokens.h"
#include "compiler/preprocessor/symbols.h"
#include "compiler/preprocessor/compile.h"
#if !defined(NO_PARSER)
#include "compiler/preprocessor/parser.h"
#endif

#if !defined(NULL)
#define NULL 0
#endif

#endif 


    

