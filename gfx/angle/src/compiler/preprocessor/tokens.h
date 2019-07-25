



















































#if !defined(__TOKENS_H)
#define __TOKENS_H 1

#include "compiler/preprocessor/parser.h"

#define EOF_SY (-1)

typedef struct TokenBlock_Rec TokenBlock;

typedef struct TokenStream_Rec {
    struct TokenStream_Rec *next;
    char *name;
    TokenBlock *head;
    TokenBlock *current;
} TokenStream;

struct TokenBlock_Rec {
    TokenBlock *next;
    int current;
    int count;
    int max;
    unsigned char *data;
};

extern TokenStream stdlib_cpp_stream;


TokenStream *NewTokenStream(const char *name, MemoryPool *pool);
void DeleteTokenStream(TokenStream *pTok); 
void RecordToken(TokenStream *pTok, int token, yystypepp * yylvalpp);
void RewindTokenStream(TokenStream *pTok);
int ReadToken(TokenStream *pTok, yystypepp * yylvalpp);
int ReadFromTokenStream(TokenStream *pTok, int name, int (*final)(CPPStruct *));
void UngetToken(int, yystypepp * yylvalpp);

#if defined(CPPC_ENABLE_TOOLS)

void DumpTokenStream(FILE *, TokenStream *, yystypepp * yylvalpp);

#endif 

#endif 
