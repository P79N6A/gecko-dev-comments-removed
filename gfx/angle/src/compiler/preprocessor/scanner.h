














































#if !defined(__SCANNER_H)
#define __SCANNER_H 1

#include "compiler/preprocessor/length_limits.h"
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
int InitScannerInput(CPPStruct *cpp, int count, const char* const string[], const int length[]);
int check_EOF(int);              
void CPPErrorToInfoLog(const char *);   
void SetLineNumber(int);
void SetStringNumber(int);
void IncLineNumber(void);
void DecLineNumber(void);
int FreeScanner(void);                 
#endif 

