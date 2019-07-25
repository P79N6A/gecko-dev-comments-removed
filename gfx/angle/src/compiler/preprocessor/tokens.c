














































#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "compiler/compilerdebug.h"
#include "compiler/preprocessor/slglobals.h"
#include "compiler/util.h"












static char *idstr(const char *fstr, MemoryPool *pool)
{
    size_t len;
    char *str, *t;
    const char *f;

    len = strlen(fstr);
    if (!pool)
        str = (char *) malloc(len + 1);
    else
        str = (char *) mem_Alloc(pool, len + 1);
    
    for (f=fstr, t=str; *f; f++) {
        if (isalnum(*f)) *t++ = *f;
        else if (*f == '.' || *f == '/') *t++ = '_';
    }
    *t = 0;
    return str;
} 







static TokenBlock *lNewBlock(TokenStream *fTok, MemoryPool *pool)
{
    TokenBlock *lBlock;

    if (!pool)
        lBlock = (TokenBlock *) malloc(sizeof(TokenBlock) + 256);
    else
        lBlock = (TokenBlock *) mem_Alloc(pool, sizeof(TokenBlock) + 256);
    lBlock->count = 0;
    lBlock->current = 0;
    lBlock->data = (unsigned char *) lBlock + sizeof(TokenBlock);
    lBlock->max = 256;
    lBlock->next = NULL;
    if (fTok->head) {
        fTok->current->next = lBlock;
    } else {
        fTok->head = lBlock;
    }
    fTok->current = lBlock;
    return lBlock;
} 






static void lAddByte(TokenStream *fTok, unsigned char fVal)
{
    TokenBlock *lBlock;
    lBlock = fTok->current;
    if (lBlock->count >= lBlock->max)
        lBlock = lNewBlock(fTok, 0);
    lBlock->data[lBlock->count++] = fVal;
} 








static int lReadByte(TokenStream *pTok)
{
    TokenBlock *lBlock;
    int lval = -1;

    lBlock = pTok->current;
    if (lBlock) {
        if (lBlock->current >= lBlock->count) {
            lBlock = lBlock->next;
            if (lBlock)
                lBlock->current = 0;
            pTok->current = lBlock;
        }
        if (lBlock)
            lval = lBlock->data[lBlock->current++];
    }
    return lval;
} 








TokenStream *NewTokenStream(const char *name, MemoryPool *pool)
{
    TokenStream *pTok;

    if (!pool)
        pTok = (TokenStream *) malloc(sizeof(TokenStream));
    else
        pTok = (TokenStream*)mem_Alloc(pool, sizeof(TokenStream));
    pTok->next = NULL;
    pTok->name = idstr(name, pool);
    pTok->head = NULL;
    pTok->current = NULL;
    lNewBlock(pTok, pool);
    return pTok;
} 






void DeleteTokenStream(TokenStream *pTok)
{
    TokenBlock *pBlock, *nBlock;

    if (pTok) {
        pBlock = pTok->head;
        while (pBlock) {
            nBlock = pBlock->next;
            free(pBlock);
            pBlock = nBlock;
        }
        if (pTok->name)
            free(pTok->name);
        free(pTok);
    }
} 






void RecordToken(TokenStream *pTok, int token, yystypepp * yylvalpp)
{
    const char *s;
    char *str=NULL;

    if (token > 256)
        lAddByte(pTok, (unsigned char)((token & 0x7f) + 0x80));
    else
        lAddByte(pTok, (unsigned char)(token & 0x7f));
    switch (token) {
    case CPP_IDENTIFIER:
    case CPP_TYPEIDENTIFIER:
    case CPP_STRCONSTANT:
        s = GetAtomString(atable, yylvalpp->sc_ident);
        while (*s)
            lAddByte(pTok, (unsigned char) *s++);
        lAddByte(pTok, 0);
        break;
    case CPP_FLOATCONSTANT:
    case CPP_INTCONSTANT:
         str=yylvalpp->symbol_name;
         while (*str){
            lAddByte(pTok, (unsigned char) *str++);
         }
         lAddByte(pTok, 0);
         break;
    case '(':
        lAddByte(pTok, (unsigned char)(yylvalpp->sc_int ? 1 : 0));
    default:
        break;
    }
} 






void RewindTokenStream(TokenStream *pTok)
{
    if (pTok->head) {
        pTok->current = pTok->head;
        pTok->current->current = 0;
    }
} 






int ReadToken(TokenStream *pTok, yystypepp * yylvalpp)
{
    char symbol_name[MAX_SYMBOL_NAME_LEN + 1];
    char string_val[MAX_STRING_LEN + 1];
    int ltoken, len;
    char ch;

    ltoken = lReadByte(pTok);
    if (ltoken >= 0) {
        if (ltoken > 127)
            ltoken += 128;
        switch (ltoken) {
        case CPP_IDENTIFIER:
        case CPP_TYPEIDENTIFIER:
            len = 0;
            ch = lReadByte(pTok);
            while ((ch >= 'a' && ch <= 'z') ||
                     (ch >= 'A' && ch <= 'Z') ||
                     (ch >= '0' && ch <= '9') ||
                     ch == '_')
            {
                if (len < MAX_SYMBOL_NAME_LEN) {
                    symbol_name[len++] = ch;
                    ch = lReadByte(pTok);
                }
            }
            symbol_name[len] = '\0';
            assert(ch == '\0');
            yylvalpp->sc_ident = LookUpAddString(atable, symbol_name);
            return CPP_IDENTIFIER;
            break;
        case CPP_STRCONSTANT:
            len = 0;
            while ((ch = lReadByte(pTok)) != 0)
                if (len < MAX_STRING_LEN)
                    string_val[len++] = ch;
            string_val[len] = '\0';
            yylvalpp->sc_ident = LookUpAddString(atable, string_val);
            break;
        case CPP_FLOATCONSTANT:
            len = 0;
            ch = lReadByte(pTok);
            while ((ch >= '0' && ch <= '9')||(ch=='e'||ch=='E'||ch=='.')||(ch=='+'||ch=='-'))
            {
                if (len < MAX_SYMBOL_NAME_LEN) {
                    symbol_name[len++] = ch;
                    ch = lReadByte(pTok);
                }
            }
            symbol_name[len] = '\0';
            assert(ch == '\0');
            strcpy(yylvalpp->symbol_name,symbol_name);
            yylvalpp->sc_fval=(float)atof_dot(yylvalpp->symbol_name);
            break;
        case CPP_INTCONSTANT:
            len = 0;
            ch = lReadByte(pTok);
            while ((ch >= '0' && ch <= '9'))
            {
                if (len < MAX_SYMBOL_NAME_LEN) {
                    symbol_name[len++] = ch;
                    ch = lReadByte(pTok);
                }
            }
            symbol_name[len] = '\0';
            assert(ch == '\0');
            strcpy(yylvalpp->symbol_name,symbol_name);
            yylvalpp->sc_int=atoi(yylvalpp->symbol_name);
            break;
        case '(':
            yylvalpp->sc_int = lReadByte(pTok);
            break;
        }
        return ltoken;
    }
    return EOF_SY;
} 

typedef struct TokenInputSrc {
    InputSrc            base;
    TokenStream         *tokens;
    int                 (*final)(CPPStruct *);
} TokenInputSrc;

static int scan_token(TokenInputSrc *in, yystypepp * yylvalpp)
{
    int token = ReadToken(in->tokens, yylvalpp);
    int (*final)(CPPStruct *);
    cpp->tokenLoc->file = cpp->currentInput->name;
    cpp->tokenLoc->line = cpp->currentInput->line;
    if (token == '\n') {
        in->base.line++;
        return token;
    }
    if (token > 0) return token;
    cpp->currentInput = in->base.prev;
    final = in->final;
    free(in);
    if (final && !final(cpp)) return -1;
    return cpp->currentInput->scan(cpp->currentInput, yylvalpp);
}

int ReadFromTokenStream(TokenStream *ts, int name, int (*final)(CPPStruct *))
{
    TokenInputSrc *in = malloc(sizeof(TokenInputSrc));
    memset(in, 0, sizeof(TokenInputSrc));
    in->base.name = name;
    in->base.prev = cpp->currentInput;
    in->base.scan = (int (*)(InputSrc *, yystypepp *))scan_token;
    in->base.line = 1;
    in->tokens = ts;
    in->final = final;
    RewindTokenStream(ts);
    cpp->currentInput = &in->base;
    return 1;
}

typedef struct UngotToken {
    InputSrc    base;
    int         token;
    yystypepp     lval;
} UngotToken;

static int reget_token(UngotToken *t, yystypepp * yylvalpp)
{
    int token = t->token;
    *yylvalpp = t->lval;
    cpp->currentInput = t->base.prev;
    free(t);
    return token;
}

void UngetToken(int token, yystypepp * yylvalpp) {
    UngotToken *t = malloc(sizeof(UngotToken));
    memset(t, 0, sizeof(UngotToken));
    t->token = token;
    t->lval = *yylvalpp;
    t->base.scan = (void *)reget_token;
    t->base.prev = cpp->currentInput;
    t->base.name = cpp->currentInput->name;
    t->base.line = cpp->currentInput->line;
    cpp->currentInput = &t->base;
}


void DumpTokenStream(FILE *fp, TokenStream *s, yystypepp * yylvalpp) {
    int token;
    char str[100];

    if (fp == 0) fp = stdout;
    RewindTokenStream(s);
    while ((token = ReadToken(s, yylvalpp)) > 0) {
        switch (token) {
        case CPP_IDENTIFIER:
        case CPP_TYPEIDENTIFIER:
            sprintf(str, "%s ", GetAtomString(atable, yylvalpp->sc_ident));
            break;
        case CPP_STRCONSTANT:
            sprintf(str, "\"%s\"", GetAtomString(atable, yylvalpp->sc_ident));
            break;
        case CPP_FLOATCONSTANT:
            
            break;
        case CPP_INTCONSTANT:
            
            break;
        default:
            if (token >= 127)
                sprintf(str, "%s ", GetAtomString(atable, token));
            else
                sprintf(str, "%c", token);
            break;
        }
        CPPDebugLogMsg(str);
    }
}




