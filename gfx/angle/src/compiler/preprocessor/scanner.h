














































#if !defined(__SCANNER_H)
#define __SCANNER_H 1


#define MAX_SYMBOL_NAME_LEN 256
#define MAX_STRING_LEN 511

#include "compiler/preprocessor/parser.h"



typedef struct SourceLoc_Rec {
    unsigned short file, line;
} SourceLoc;

int yylex_CPP(char* buf, int maxSize);

typedef struct InputSrc {
    struct InputSrc	*prev;
    int			(*scan)(struct InputSrc *, yystypepp *);
    int			(*getch)(struct InputSrc *, yystypepp *);
    void		(*ungetch)(struct InputSrc *, int, yystypepp *);
    int			name;  
    int			line;
} InputSrc;

int InitScanner(CPPStruct *cpp);   
int ScanFromString(const char *);      
int check_EOF(int);              
void CPPErrorToInfoLog(char *);   
void SetLineNumber(int);
void SetStringNumber(int);
void IncLineNumber(void);
void DecLineNumber(void);
int FreeScanner(void);                 
#endif 

