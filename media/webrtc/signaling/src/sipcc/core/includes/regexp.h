






































#ifndef __REGEXP_H__
#define __REGEXP_H__







#define NSUBEXP  10
struct regexp_ {
    char *startp[NSUBEXP];
    char *endp[NSUBEXP];
    char regflag;       
    char regstart;      
    char reganch;       
    char regatom;       
    char *regmust;      
    int regmlen;        
    char program[1];    
};

typedef struct regexp_ regexp;






#define MAGIC    0234




#define REG_NOSUB 0x02   /* Don't do sub-string matches */



typedef enum {
    REG_NO_MATCH = 0,
    REG_MATCHED,
    REG_MAYBE
} regval;











extern char *reg(int, int *);
extern char *regatom(int *);
extern char *regbranch(int *);
extern char *regnode(char);
extern char *regpiece(int *);
extern char *regprop(char *);
extern int regexec(regexp *, char *);
extern int regmatch(char *);
extern int regrepeat(char *);
extern int regtry(regexp *, char *);
extern regexp *regcomp(char *, char);
extern void regc(char);
extern void regdump(regexp *);
extern void regerror(char *);
extern void reginsert(char, char *);
extern void regoptail(char *, char *);
extern void regtail(char *, char *);
extern regval regexecstring(regexp *, char *);




extern int regsub(regexp *, char *, char *, int);

#endif 
