





#ifndef __INITIALIZE_PARSE_CONTEXT_INCLUDED_
#define __INITIALIZE_PARSE_CONTEXT_INCLUDED_

bool InitializeParseContextIndex();
bool FreeParseContextIndex();

bool InitializeGlobalParseContext();
bool FreeParseContext();

struct TParseContext;
typedef TParseContext* TParseContextPointer;
extern TParseContextPointer& GetGlobalParseContext();
#define GlobalParseContext GetGlobalParseContext()

typedef struct TThreadParseContextRec
{
    TParseContext *lpGlobalParseContext;
} TThreadParseContext;

#endif 
