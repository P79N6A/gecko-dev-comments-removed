


















#ifndef _SQLITE_VDBE_H_
#define _SQLITE_VDBE_H_
#include <stdio.h>






typedef struct Vdbe Vdbe;






struct VdbeOp {
  u8 opcode;          
  int p1;             
  int p2;             
  char *p3;           
  int p3type;         
#ifdef VDBE_PROFILE
  int cnt;            
  long long cycles;   
#endif
};
typedef struct VdbeOp VdbeOp;





struct VdbeOpList {
  u8 opcode;          
  signed char p1;     
  short int p2;       
  char *p3;           
};
typedef struct VdbeOpList VdbeOpList;




#define P3_NOTUSED    0   /* The P3 parameter is not used */
#define P3_DYNAMIC  (-1)  /* Pointer to a string obtained from sqliteMalloc() */
#define P3_STATIC   (-2)  /* Pointer to a static string */
#define P3_COLLSEQ  (-4)  /* P3 is a pointer to a CollSeq structure */
#define P3_FUNCDEF  (-5)  /* P3 is a pointer to a FuncDef structure */
#define P3_KEYINFO  (-6)  /* P3 is a pointer to a KeyInfo structure */
#define P3_VDBEFUNC (-7)  /* P3 is a pointer to a VdbeFunc structure */
#define P3_MEM      (-8)  /* P3 is a pointer to a Mem*    structure */
#define P3_TRANSIENT (-9)  /* P3 is a pointer to a transient string */








#define P3_KEYINFO_HANDOFF (-9)





#define COLNAME_NAME     0
#define COLNAME_DECLTYPE 1
#define COLNAME_DATABASE 2
#define COLNAME_TABLE    3
#define COLNAME_COLUMN   4
#define COLNAME_N        5      /* Number of COLNAME_xxx symbols */







#define ADDR(X)  (-1-(X))





#include "opcodes.h"





Vdbe *sqlite3VdbeCreate(sqlite3*);
void sqlite3VdbeCreateCallback(Vdbe*, int*);
int sqlite3VdbeAddOp(Vdbe*,int,int,int);
int sqlite3VdbeOp3(Vdbe*,int,int,int,const char *zP3,int);
int sqlite3VdbeAddOpList(Vdbe*, int nOp, VdbeOpList const *aOp);
void sqlite3VdbeChangeP1(Vdbe*, int addr, int P1);
void sqlite3VdbeChangeP2(Vdbe*, int addr, int P2);
void sqlite3VdbeJumpHere(Vdbe*, int addr);
void sqlite3VdbeChangeToNoop(Vdbe*, int addr, int N);
void sqlite3VdbeChangeP3(Vdbe*, int addr, const char *zP1, int N);
VdbeOp *sqlite3VdbeGetOp(Vdbe*, int);
int sqlite3VdbeMakeLabel(Vdbe*);
void sqlite3VdbeDelete(Vdbe*);
void sqlite3VdbeMakeReady(Vdbe*,int,int,int,int);
int sqlite3VdbeFinalize(Vdbe*);
void sqlite3VdbeResolveLabel(Vdbe*, int);
int sqlite3VdbeCurrentAddr(Vdbe*);
void sqlite3VdbeTrace(Vdbe*,FILE*);
int sqlite3VdbeReset(Vdbe*);
int sqliteVdbeSetVariables(Vdbe*,int,const char**);
void sqlite3VdbeSetNumCols(Vdbe*,int);
int sqlite3VdbeSetColName(Vdbe*, int, int, const char *, int);
void sqlite3VdbeCountChanges(Vdbe*);
sqlite3 *sqlite3VdbeDb(Vdbe*);

#ifndef NDEBUG
  void sqlite3VdbeComment(Vdbe*, const char*, ...);
# define VdbeComment(X)  sqlite3VdbeComment X
#else
# define VdbeComment(X)
#endif

#endif
