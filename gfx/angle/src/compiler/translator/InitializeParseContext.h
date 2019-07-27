





#ifndef __INITIALIZE_PARSE_CONTEXT_INCLUDED_
#define __INITIALIZE_PARSE_CONTEXT_INCLUDED_

bool InitializeParseContextIndex();
void FreeParseContextIndex();

struct TParseContext;
extern void SetGlobalParseContext(TParseContext* context);
extern TParseContext* GetGlobalParseContext();

#endif 
