



















































#if !defined(__CPP_H)
#define __CPP_H 1

#include "compiler/preprocessor/parser.h"
#include "compiler/preprocessor/tokens.h"

int InitCPP(void);
int FinalCPP(void);
int  readCPPline(yystypepp * yylvalpp);
int MacroExpand(int atom, yystypepp * yylvalpp);
int ChkCorrectElseNesting(void);

typedef struct MacroSymbol {
    int argc;
    int *args;
    TokenStream *body;
    unsigned busy:1;
    unsigned undef:1;
} MacroSymbol;

void FreeMacro(MacroSymbol *);
void PredefineIntMacro(const char *name, int value);

void  CPPDebugLogMsg(const char *msg);      
void  CPPShInfoLogMsg(const char*);         
void  CPPWarningToInfoLog(const char *msg); 
void  HandlePragma(const char**, int numTokens);  
void  ResetTString(void);                   
void  CPPErrorToInfoLog(char*);             
void  StoreStr(char*);                      
void  SetLineNumber(int);                   
void  SetStringNumber(int);                 
int   GetLineNumber(void);                  
int   GetStringNumber(void);                
const char* GetStrfromTStr(void);           
void  updateExtensionBehavior(const char* extName, const char* behavior);
int   FreeCPP(void);

#endif 
