














#ifndef _SQLITEINT_H_
#define _SQLITEINT_H_




#ifdef SQLITE_EXTRA
# include "sqliteExtra.h"
#endif








#if !defined(NDEBUG) && !defined(SQLITE_DEBUG) 
# define NDEBUG 1
#endif
















#ifndef SQLITE_DISABLE_LFS
# define _LARGE_FILE       1
# ifndef _FILE_OFFSET_BITS
#   define _FILE_OFFSET_BITS 64
# endif
# define _LARGEFILE_SOURCE 1
#endif

#include "sqlite3.h"
#include "hash.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>





#ifdef SQLITE_OMIT_FLOATING_POINT
# define double sqlite_int64
# define LONGDOUBLE_TYPE sqlite_int64
# ifndef SQLITE_BIG_DBL
#   define SQLITE_BIG_DBL (0x7fffffffffffffff)
# endif
# define SQLITE_OMIT_DATETIME_FUNCS 1
# define SQLITE_OMIT_TRACE 1
#endif
#ifndef SQLITE_BIG_DBL
# define SQLITE_BIG_DBL (1e99)
#endif








#ifdef SQLITE_DEFAULT_CACHE_SIZE
# define MAX_PAGES SQLITE_DEFAULT_CACHE_SIZE
#else
# define MAX_PAGES   2000
#endif
#ifdef SQLITE_DEFAULT_TEMP_CACHE_SIZE
# define TEMP_PAGES SQLITE_DEFAULT_TEMP_CACHE_SIZE
#else
# define TEMP_PAGES   500
#endif






#ifdef SQLITE_OMIT_TEMPDB
#define OMIT_TEMPDB 1
#else
#define OMIT_TEMPDB 0
#endif













#define NULL_DISTINCT_FOR_UNIQUE 1








#define MAX_ATTACHED 10




#define SQLITE_MAX_VARIABLE_NUMBER 999







#define SQLITE_MAX_FILE_FORMAT 4
#ifndef SQLITE_DEFAULT_FILE_FORMAT
# define SQLITE_DEFAULT_FILE_FORMAT 4
#endif





#ifndef TEMP_STORE
# define TEMP_STORE 1
#endif





#ifndef offsetof
#define offsetof(STRUCTURE,FIELD) ((int)((char*)&((STRUCTURE*)0)->FIELD))
#endif





#if 'A' == '\301'
# define SQLITE_EBCDIC 1
#else
# define SQLITE_ASCII 1
#endif








#ifndef UINT32_TYPE
# define UINT32_TYPE unsigned int
#endif
#ifndef UINT16_TYPE
# define UINT16_TYPE unsigned short int
#endif
#ifndef INT16_TYPE
# define INT16_TYPE short int
#endif
#ifndef UINT8_TYPE
# define UINT8_TYPE unsigned char
#endif
#ifndef INT8_TYPE
# define INT8_TYPE signed char
#endif
#ifndef LONGDOUBLE_TYPE
# define LONGDOUBLE_TYPE long double
#endif
typedef sqlite_int64 i64;          
typedef sqlite_uint64 u64;         
typedef UINT32_TYPE u32;           
typedef UINT16_TYPE u16;           
typedef INT16_TYPE i16;            
typedef UINT8_TYPE u8;             
typedef UINT8_TYPE i8;             





extern const int sqlite3one;
#define SQLITE_BIGENDIAN    (*(char *)(&sqlite3one)==0)
#define SQLITE_LITTLEENDIAN (*(char *)(&sqlite3one)==1)










typedef struct BusyHandler BusyHandler;
struct BusyHandler {
  int (*xFunc)(void *,int);  
  void *pArg;                
  int nBusy;                 
};





#include "vdbe.h"
#include "btree.h"
#include "pager.h"

#ifdef SQLITE_MEMDEBUG




extern int sqlite3_nMalloc;      
extern int sqlite3_nFree;        
extern int sqlite3_iMallocFail;  
extern int sqlite3_iMallocReset; 

extern void *sqlite3_pFirst;         
extern int sqlite3_nMaxAlloc;        
extern int sqlite3_mallocDisallowed; 
extern int sqlite3_isFail;           
extern const char *sqlite3_zFile;    
extern int sqlite3_iLine;            

#define ENTER_MALLOC (sqlite3_zFile = __FILE__, sqlite3_iLine = __LINE__)
#define sqliteMalloc(x)          (ENTER_MALLOC, sqlite3Malloc(x,1))
#define sqliteMallocRaw(x)       (ENTER_MALLOC, sqlite3MallocRaw(x,1))
#define sqliteRealloc(x,y)       (ENTER_MALLOC, sqlite3Realloc(x,y))
#define sqliteStrDup(x)          (ENTER_MALLOC, sqlite3StrDup(x))
#define sqliteStrNDup(x,y)       (ENTER_MALLOC, sqlite3StrNDup(x,y))
#define sqliteReallocOrFree(x,y) (ENTER_MALLOC, sqlite3ReallocOrFree(x,y))

#else

#define ENTER_MALLOC 0
#define sqliteMalloc(x)          sqlite3Malloc(x,1)
#define sqliteMallocRaw(x)       sqlite3MallocRaw(x,1)
#define sqliteRealloc(x,y)       sqlite3Realloc(x,y)
#define sqliteStrDup(x)          sqlite3StrDup(x)
#define sqliteStrNDup(x,y)       sqlite3StrNDup(x,y)
#define sqliteReallocOrFree(x,y) sqlite3ReallocOrFree(x,y)

#endif

#define sqliteFree(x)          sqlite3FreeX(x)
#define sqliteAllocSize(x)     sqlite3AllocSize(x)






struct ThreadData {
  int dummy;               

#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  int nSoftHeapLimit;      
  int nAlloc;              
  Pager *pPager;           
#endif

#ifndef SQLITE_OMIT_SHARED_CACHE
  u8 useSharedData;        
  BtShared *pBtree;        
#endif
};






#define MASTER_NAME       "sqlite_master"
#define TEMP_MASTER_NAME  "sqlite_temp_master"




#define MASTER_ROOT       1




#define SCHEMA_TABLE(x)  ((!OMIT_TEMPDB)&&(x==1)?TEMP_MASTER_NAME:MASTER_NAME)





#define ArraySize(X)    (sizeof(X)/sizeof(X[0]))




typedef struct AggInfo AggInfo;
typedef struct AuthContext AuthContext;
typedef struct CollSeq CollSeq;
typedef struct Column Column;
typedef struct Db Db;
typedef struct Schema Schema;
typedef struct Expr Expr;
typedef struct ExprList ExprList;
typedef struct FKey FKey;
typedef struct FuncDef FuncDef;
typedef struct IdList IdList;
typedef struct Index Index;
typedef struct KeyClass KeyClass;
typedef struct KeyInfo KeyInfo;
typedef struct NameContext NameContext;
typedef struct Parse Parse;
typedef struct Select Select;
typedef struct SrcList SrcList;
typedef struct ThreadData ThreadData;
typedef struct Table Table;
typedef struct TableLock TableLock;
typedef struct Token Token;
typedef struct TriggerStack TriggerStack;
typedef struct TriggerStep TriggerStep;
typedef struct Trigger Trigger;
typedef struct WhereInfo WhereInfo;
typedef struct WhereLevel WhereLevel;








struct Db {
  char *zName;         
  Btree *pBt;          
  u8 inTrans;          
  u8 safety_level;     
  void *pAux;               
  void (*xFreeAux)(void*);  
  Schema *pSchema;     
};




struct Schema {
  int schema_cookie;   
  Hash tblHash;        
  Hash idxHash;        
  Hash trigHash;       
  Hash aFKey;          
  Table *pSeqTab;      
  u8 file_format;      
  u8 enc;              
  u16 flags;           
  int cache_size;      
};





#define DbHasProperty(D,I,P)     (((D)->aDb[I].pSchema->flags&(P))==(P))
#define DbHasAnyProperty(D,I,P)  (((D)->aDb[I].pSchema->flags&(P))!=0)
#define DbSetProperty(D,I,P)     (D)->aDb[I].pSchema->flags|=(P)
#define DbClearProperty(D,I,P)   (D)->aDb[I].pSchema->flags&=~(P)











#define DB_SchemaLoaded    0x0001  /* The schema has been loaded */
#define DB_UnresetViews    0x0002  /* Some views have defined column names */
#define DB_Empty           0x0004  /* The file is empty (length 0 bytes) */

#define SQLITE_UTF16NATIVE (SQLITE_BIGENDIAN?SQLITE_UTF16BE:SQLITE_UTF16LE)



























struct sqlite3 {
  int nDb;                      
  Db *aDb;                      
  int flags;                    
  int errCode;                  
  u8 autoCommit;                
  u8 temp_store;                
  int nTable;                   
  CollSeq *pDfltColl;           
  i64 lastRowid;                
  i64 priorNewRowid;            
  int magic;                    
  int nChange;                  
  int nTotalChange;             
  struct sqlite3InitInfo {      
    int iDb;                    
    int newTnum;                
    u8 busy;                    
  } init;
  struct Vdbe *pVdbe;           
  int activeVdbeCnt;            
  void (*xTrace)(void*,const char*);        
  void *pTraceArg;                          
  void (*xProfile)(void*,const char*,u64);  
  void *pProfileArg;                        
  void *pCommitArg;                    
  int (*xCommitCallback)(void*);    
  void *pRollbackArg;                  
  void (*xRollbackCallback)(void*); 
  void *pUpdateArg;
  void (*xUpdateCallback)(void*,int, const char*,const char*,sqlite_int64);
  void(*xCollNeeded)(void*,sqlite3*,int eTextRep,const char*);
  void(*xCollNeeded16)(void*,sqlite3*,int eTextRep,const void*);
  void *pCollNeededArg;
  sqlite3_value *pErr;          
  char *zErrMsg;                
  char *zErrMsg16;              
#ifndef SQLITE_OMIT_AUTHORIZATION
  int (*xAuth)(void*,int,const char*,const char*,const char*,const char*);
                                
  void *pAuthArg;               
#endif
#ifndef SQLITE_OMIT_PROGRESS_CALLBACK
  int (*xProgress)(void *);     
  void *pProgressArg;           
  int nProgressOps;             
#endif
#ifndef SQLITE_OMIT_GLOBALRECOVER
  sqlite3 *pNext;               
#endif
  Hash aFunc;                   
  Hash aCollSeq;                
  BusyHandler busyHandler;      
  int busyTimeout;             
  Db aDbStatic[2];              
#ifdef SQLITE_SSE
  sqlite3_stmt *pFetch;         
#endif
};




#define ENC(db) ((db)->aDb[0].pSchema->enc)








#define SQLITE_VdbeTrace      0x00000001  /* True to trace VDBE execution */
#define SQLITE_Interrupt      0x00000004  /* Cancel current operation */
#define SQLITE_InTrans        0x00000008  /* True if in a transaction */
#define SQLITE_InternChanges  0x00000010  /* Uncommitted Hash table changes */
#define SQLITE_FullColNames   0x00000020  /* Show full column names on SELECT */
#define SQLITE_ShortColNames  0x00000040  /* Show short columns names */
#define SQLITE_CountRows      0x00000080  /* Count rows changed by INSERT, */
                                          
                                          
#define SQLITE_NullCallback   0x00000100  /* Invoke the callback once if the */
                                          
#define SQLITE_SqlTrace       0x00000200  /* Debug print SQL as it executes */
#define SQLITE_VdbeListing    0x00000400  /* Debug listings of VDBE programs */
#define SQLITE_WriteSchema    0x00000800  /* OK to update SQLITE_MASTER */
#define SQLITE_NoReadlock     0x00001000  /* Readlocks are omitted when 
                                          ** accessing read-only databases */
#define SQLITE_IgnoreChecks   0x00002000  /* Do not enforce check constraints */
#define SQLITE_ReadUncommitted 0x00004000  /* For shared-cache mode */
#define SQLITE_LegacyFileFmt  0x00008000  /* Create new databases in format 1 */
#define SQLITE_FullFSync      0x00010000  /* Use full fsync on the backend */






#define SQLITE_MAGIC_OPEN     0xa029a697  /* Database is open */
#define SQLITE_MAGIC_CLOSED   0x9f3c2d33  /* Database is closed */
#define SQLITE_MAGIC_BUSY     0xf03b7906  /* Database currently in use */
#define SQLITE_MAGIC_ERROR    0xb5357930  /* An SQLITE_MISUSE error occurred */







struct FuncDef {
  i16 nArg;            
  u8 iPrefEnc;         
  u8 needCollSeq;      
  u8 flags;            
  void *pUserData;     
  FuncDef *pNext;      
  void (*xFunc)(sqlite3_context*,int,sqlite3_value**); 
  void (*xStep)(sqlite3_context*,int,sqlite3_value**); 
  void (*xFinalize)(sqlite3_context*);                
  char zName[1];       
};




#define SQLITE_FUNC_LIKE   0x01  /* Candidate for the LIKE optimization */
#define SQLITE_FUNC_CASE   0x02  /* Case-sensitive LIKE-type function */





struct Column {
  char *zName;     
  Expr *pDflt;     
  char *zType;     
  char *zColl;     
  u8 notNull;      
  u8 isPrimKey;    
  char affinity;   
};






















struct CollSeq {
  char *zName;         
  u8 enc;              
  u8 type;             
  void *pUser;         
  int (*xCmp)(void*,int, const void*, int, const void*);
};




#define SQLITE_COLL_BINARY  1  /* The default memcmp() collating sequence */
#define SQLITE_COLL_NOCASE  2  /* The built-in NOCASE collating sequence */
#define SQLITE_COLL_REVERSE 3  /* The built-in REVERSE collating sequence */
#define SQLITE_COLL_USER    0  /* Any other user-defined collating sequence */




#define SQLITE_SO_ASC       0  /* Sort in ascending order */
#define SQLITE_SO_DESC      1  /* Sort in ascending order */















#define SQLITE_AFF_TEXT     'a'
#define SQLITE_AFF_NONE     'b'
#define SQLITE_AFF_NUMERIC  'c'
#define SQLITE_AFF_INTEGER  'd'
#define SQLITE_AFF_REAL     'e'

#define sqlite3IsNumericAffinity(X)  ((X)>=SQLITE_AFF_NUMERIC)































struct Table {
  char *zName;     
  int nCol;        
  Column *aCol;    
  int iPKey;       
  Index *pIndex;   
  int tnum;        
  Select *pSelect; 
  u8 readOnly;     
  u8 isTransient;  
  u8 hasPrimKey;   
  u8 keyConf;      
  u8 autoInc;      
  int nRef;          
  Trigger *pTrigger; 
  FKey *pFKey;       
  char *zColAff;     
#ifndef SQLITE_OMIT_CHECK
  Expr *pCheck;      
#endif
#ifndef SQLITE_OMIT_ALTERTABLE
  int addColOffset;  
#endif
  Schema *pSchema;
};


























struct FKey {
  Table *pFrom;     
  FKey *pNextFrom;  
  char *zTo;        
  FKey *pNextTo;    
  int nCol;         
  struct sColMap {  
    int iFrom;         
    char *zCol;        
  } *aCol;          
  u8 isDeferred;    
  u8 updateConf;    
  u8 deleteConf;    
  u8 insertConf;    
};


























#define OE_None     0   /* There is no constraint to check */
#define OE_Rollback 1   /* Fail the operation and rollback the transaction */
#define OE_Abort    2   /* Back out changes but do no rollback transaction */
#define OE_Fail     3   /* Stop the operation but leave all prior changes */
#define OE_Ignore   4   /* Ignore the error. Do not do the INSERT or UPDATE */
#define OE_Replace  5   /* Delete existing record, then do INSERT or UPDATE */

#define OE_Restrict 6   /* OE_Abort for IMMEDIATE, OE_Rollback for DEFERRED */
#define OE_SetNull  7   /* Set the foreign key value to NULL */
#define OE_SetDflt  8   /* Set the foreign key value to its default */
#define OE_Cascade  9   /* Cascade the changes */

#define OE_Default  99  /* Do whatever the default action is */











struct KeyInfo {
  u8 enc;             
  u8 incrKey;         
  int nField;         
  u8 *aSortOrder;     
  CollSeq *aColl[1];  
};



























struct Index {
  char *zName;     
  int nColumn;     
  int *aiColumn;   
  unsigned *aiRowEst; 
  Table *pTable;   
  int tnum;        
  u8 onError;      
  u8 autoIndex;    
  char *zColAff;   
  Index *pNext;    
  Schema *pSchema; 
  u8 *aSortOrder;  
  char **azColl;   
};









struct Token {
  const unsigned char *z; 
  unsigned dyn  : 1;      
  unsigned n    : 31;     
};














struct AggInfo {
  u8 directMode;          

  u8 useSortingIdx;       

  int sortingIdx;         
  ExprList *pGroupBy;     
  int nSortingColumn;     
  struct AggInfo_col {    
    int iTable;              
    int iColumn;             
    int iSorterColumn;       
    int iMem;                
    Expr *pExpr;             
  } *aCol;
  int nColumn;            
  int nColumnAlloc;       
  int nAccumulator;       


  struct AggInfo_func {   
    Expr *pExpr;             
    FuncDef *pFunc;          
    int iMem;                
    int iDistinct;           
  } *aFunc;
  int nFunc;              
  int nFuncAlloc;         
};















































struct Expr {
  u8 op;                 
  char affinity;         
  u8 flags;              
  CollSeq *pColl;        
  Expr *pLeft, *pRight;  
  ExprList *pList;       

  Token token;           
  Token span;            
  int iTable, iColumn;   

  AggInfo *pAggInfo;     
  int iAgg;              
  int iRightJoinTable;   
  Select *pSelect;       

  Table *pTab;           
  Schema *pSchema;
};




#define EP_FromJoin     0x01  /* Originated in ON or USING clause of a join */
#define EP_Agg          0x02  /* Contains one or more aggregate functions */
#define EP_Resolved     0x04  /* IDs have been resolved to COLUMNs */
#define EP_Error        0x08  /* Expression contains one or more errors */
#define EP_Distinct     0x10  /* Aggregate function with DISTINCT keyword */
#define EP_VarSelect    0x20  /* pSelect is correlated, not constant */
#define EP_Dequoted     0x40  /* True if the string has been dequoted */





#define ExprHasProperty(E,P)     (((E)->flags&(P))==(P))
#define ExprHasAnyProperty(E,P)  (((E)->flags&(P))!=0)
#define ExprSetProperty(E,P)     (E)->flags|=(P)
#define ExprClearProperty(E,P)   (E)->flags&=~(P)









struct ExprList {
  int nExpr;             
  int nAlloc;            
  int iECursor;          
  struct ExprList_item {
    Expr *pExpr;           
    char *zName;           
    u8 sortOrder;          
    u8 isAgg;              
    u8 done;               
  } *a;                  
};
















struct IdList {
  struct IdList_item {
    char *zName;      
    int idx;          
  } *a;
  int nId;         
  int nAlloc;      
};




typedef unsigned int Bitmask;












struct SrcList {
  i16 nSrc;        
  i16 nAlloc;      
  struct SrcList_item {
    char *zDatabase;  
    char *zName;      
    char *zAlias;     
    Table *pTab;      
    Select *pSelect;  
    u8 isPopulated;   
    u8 jointype;      
    i16 iCursor;      
    Expr *pOn;        
    IdList *pUsing;   
    Bitmask colUsed;  
  } a[1];             
};




#define JT_INNER     0x0001    /* Any kind of inner or cross join */
#define JT_CROSS     0x0002    /* Explicit use of the CROSS keyword */
#define JT_NATURAL   0x0004    /* True for a "natural" join */
#define JT_LEFT      0x0008    /* Left outer join */
#define JT_RIGHT     0x0010    /* Right outer join */
#define JT_OUTER     0x0020    /* The "OUTER" keyword is present */
#define JT_ERROR     0x0040    /* unknown or unsupported join type */







struct WhereLevel {
  int iFrom;            
  int flags;            
  int iMem;             
  int iLeftJoin;        
  Index *pIdx;          
  int iTabCur;          
  int iIdxCur;          
  int brk;              
  int cont;             
  int top;              
  int op, p1, p2;       
  int nEq;              
  int nIn;              
  int *aInLoop;         
};








struct WhereInfo {
  Parse *pParse;
  SrcList *pTabList;   
  int iTop;            
  int iContinue;       
  int iBreak;          
  int nLevel;          
  WhereLevel a[1];     
};






















struct NameContext {
  Parse *pParse;       
  SrcList *pSrcList;   
  ExprList *pEList;    
  int nRef;            
  int nErr;            
  u8 allowAgg;         
  u8 hasAgg;           
  u8 isCheck;          
  int nDepth;          
  AggInfo *pAggInfo;   
  NameContext *pNext;  
};





















struct Select {
  ExprList *pEList;      
  u8 op;                 
  u8 isDistinct;         
  u8 isResolved;         
  u8 isAgg;              
  u8 usesVirt;           
  u8 disallowOrderBy;    
  SrcList *pSrc;         
  Expr *pWhere;          
  ExprList *pGroupBy;    
  Expr *pHaving;         
  ExprList *pOrderBy;    
  Select *pPrior;        
  Select *pRightmost;    
  Expr *pLimit;          
  Expr *pOffset;         
  int iLimit, iOffset;   
  int addrOpenVirt[3];   
};




#define SRT_Union        1  /* Store result as keys in an index */
#define SRT_Except       2  /* Remove result from a UNION index */
#define SRT_Discard      3  /* Do not save the results anywhere */


#define IgnorableOrderby(X) (X<=SRT_Discard)

#define SRT_Callback     4  /* Invoke a callback with each row of result */
#define SRT_Mem          5  /* Store result in a memory cell */
#define SRT_Set          6  /* Store non-null results as keys in an index */
#define SRT_Table        7  /* Store result as data with an automatic rowid */
#define SRT_VirtualTab   8  /* Create virtual table and store like SRT_Table */
#define SRT_Subroutine   9  /* Call a subroutine to handle results */
#define SRT_Exists      10  /* Store 1 if the result is not empty */

















struct Parse {
  sqlite3 *db;         
  int rc;              
  char *zErrMsg;       
  Vdbe *pVdbe;         
  u8 colNamesSet;      
  u8 nameClash;        
  u8 checkSchema;      
  u8 nested;           
  int nErr;            
  int nTab;            
  int nMem;            
  int nSet;            
  int ckOffset;        
  u32 writeMask;       
  u32 cookieMask;      
  int cookieGoto;      
  int cookieValue[MAX_ATTACHED+2];  
#ifndef SQLITE_OMIT_SHARED_CACHE
  int nTableLock;        
  TableLock *aTableLock; 
#endif

  


  int nVar;            
  int nVarExpr;        
  int nVarExprAlloc;   
  Expr **apVarExpr;    
  u8 explain;          
  Token sErrToken;     
  Token sNameToken;    
  Token sLastToken;    
  const char *zSql;    
  const char *zTail;   
  Table *pNewTable;    
  Trigger *pNewTrigger;     
  TriggerStack *trigStack;  
  const char *zAuthContext; 
};





struct AuthContext {
  const char *zAuthContext;   
  Parse *pParse;              
};




#define OPFLAG_NCHANGE   1    /* Set to update db->nChange */
#define OPFLAG_LASTROWID 2    /* Set to update db->lastRowid */
#define OPFLAG_ISUPDATE  4    /* This OP_Insert is an sql UPDATE */
















struct Trigger {
  char *name;             
  char *table;            
  u8 op;                  
  u8 tr_tm;               
  Expr *pWhen;            
  IdList *pColumns;       

  int foreach;            
  Token nameToken;        
  Schema *pSchema;        
  Schema *pTabSchema;     
  TriggerStep *step_list; 
  Trigger *pNext;         
};








#define TRIGGER_BEFORE  1
#define TRIGGER_AFTER   2







































struct TriggerStep {
  int op;              
  int orconf;          
  Trigger *pTrig;      

  Select *pSelect;     

  Token target;        
  Expr *pWhere;        
  ExprList *pExprList; 

  IdList *pIdList;     
  TriggerStep *pNext;  
  TriggerStep *pLast;  
};




























struct TriggerStack {
  Table *pTab;         
  int newIdx;          
  int oldIdx;          
  int orconf;          
  int ignoreJump;      
  Trigger *pTrigger;   
  TriggerStack *pNext; 
};






typedef struct DbFixer DbFixer;
struct DbFixer {
  Parse *pParse;      
  const char *zDb;    
  const char *zType;  
  const Token *pName; 
};





typedef struct {
  sqlite3 *db;        
  char **pzErrMsg;    
} InitData;






extern int sqlite3_always_code_trigger_setup;







#ifdef SQLITE_DEBUG
  extern int sqlite3Corrupt(void);
# define SQLITE_CORRUPT_BKPT sqlite3Corrupt()
#else
# define SQLITE_CORRUPT_BKPT SQLITE_CORRUPT
#endif




int sqlite3StrICmp(const char *, const char *);
int sqlite3StrNICmp(const char *, const char *, int);
int sqlite3HashNoCase(const char *, int);
int sqlite3IsNumber(const char*, int*, u8);
int sqlite3Compare(const char *, const char *);
int sqlite3SortCompare(const char *, const char *);
void sqlite3RealToSortable(double r, char *);

void *sqlite3Malloc(int,int);
void *sqlite3MallocRaw(int,int);
void sqlite3Free(void*);
void *sqlite3Realloc(void*,int);
char *sqlite3StrDup(const char*);
char *sqlite3StrNDup(const char*, int);
# define sqlite3CheckMemory(a,b)
void sqlite3ReallocOrFree(void**,int);
void sqlite3FreeX(void*);
void *sqlite3MallocX(int);
int sqlite3AllocSize(void *);

char *sqlite3MPrintf(const char*, ...);
char *sqlite3VMPrintf(const char*, va_list);
void sqlite3DebugPrintf(const char*, ...);
void *sqlite3TextToPtr(const char*);
void sqlite3SetString(char **, ...);
void sqlite3ErrorMsg(Parse*, const char*, ...);
void sqlite3ErrorClear(Parse*);
void sqlite3Dequote(char*);
void sqlite3DequoteExpr(Expr*);
int sqlite3KeywordCode(const unsigned char*, int);
int sqlite3RunParser(Parse*, const char*, char **);
void sqlite3FinishCoding(Parse*);
Expr *sqlite3Expr(int, Expr*, Expr*, const Token*);
Expr *sqlite3RegisterExpr(Parse*,Token*);
Expr *sqlite3ExprAnd(Expr*, Expr*);
void sqlite3ExprSpan(Expr*,Token*,Token*);
Expr *sqlite3ExprFunction(ExprList*, Token*);
void sqlite3ExprAssignVarNumber(Parse*, Expr*);
void sqlite3ExprDelete(Expr*);
ExprList *sqlite3ExprListAppend(ExprList*,Expr*,Token*);
void sqlite3ExprListDelete(ExprList*);
int sqlite3Init(sqlite3*, char**);
int sqlite3InitCallback(void*, int, char**, char**);
void sqlite3Pragma(Parse*,Token*,Token*,Token*,int);
void sqlite3ResetInternalSchema(sqlite3*, int);
void sqlite3BeginParse(Parse*,int);
void sqlite3RollbackInternalChanges(sqlite3*);
void sqlite3CommitInternalChanges(sqlite3*);
Table *sqlite3ResultSetOfSelect(Parse*,char*,Select*);
void sqlite3OpenMasterTable(Parse *, int);
void sqlite3StartTable(Parse*,Token*,Token*,int,int,int);
void sqlite3AddColumn(Parse*,Token*);
void sqlite3AddNotNull(Parse*, int);
void sqlite3AddPrimaryKey(Parse*, ExprList*, int, int, int);
void sqlite3AddCheckConstraint(Parse*, Expr*);
void sqlite3AddColumnType(Parse*,Token*);
void sqlite3AddDefaultValue(Parse*,Expr*);
void sqlite3AddCollateType(Parse*, const char*, int);
void sqlite3EndTable(Parse*,Token*,Token*,Select*);

#ifndef SQLITE_OMIT_VIEW
  void sqlite3CreateView(Parse*,Token*,Token*,Token*,Select*,int);
  int sqlite3ViewGetColumnNames(Parse*,Table*);
#else
# define sqlite3ViewGetColumnNames(A,B) 0
#endif

void sqlite3DropTable(Parse*, SrcList*, int, int);
void sqlite3DeleteTable(sqlite3*, Table*);
void sqlite3Insert(Parse*, SrcList*, ExprList*, Select*, IdList*, int);
int sqlite3ArrayAllocate(void**,int,int);
IdList *sqlite3IdListAppend(IdList*, Token*);
int sqlite3IdListIndex(IdList*,const char*);
SrcList *sqlite3SrcListAppend(SrcList*, Token*, Token*);
void sqlite3SrcListAddAlias(SrcList*, Token*);
void sqlite3SrcListAssignCursors(Parse*, SrcList*);
void sqlite3IdListDelete(IdList*);
void sqlite3SrcListDelete(SrcList*);
void sqlite3CreateIndex(Parse*,Token*,Token*,SrcList*,ExprList*,int,Token*,
                        Token*, int, int);
void sqlite3DropIndex(Parse*, SrcList*, int);
void sqlite3AddKeyType(Vdbe*, ExprList*);
void sqlite3AddIdxKeyType(Vdbe*, Index*);
int sqlite3Select(Parse*, Select*, int, int, Select*, int, int*, char *aff);
Select *sqlite3SelectNew(ExprList*,SrcList*,Expr*,ExprList*,Expr*,ExprList*,
                        int,Expr*,Expr*);
void sqlite3SelectDelete(Select*);
void sqlite3SelectUnbind(Select*);
Table *sqlite3SrcListLookup(Parse*, SrcList*);
int sqlite3IsReadOnly(Parse*, Table*, int);
void sqlite3OpenTable(Parse*, int iCur, int iDb, Table*, int);
void sqlite3DeleteFrom(Parse*, SrcList*, Expr*);
void sqlite3Update(Parse*, SrcList*, ExprList*, Expr*, int);
WhereInfo *sqlite3WhereBegin(Parse*, SrcList*, Expr*, ExprList**);
void sqlite3WhereEnd(WhereInfo*);
void sqlite3ExprCode(Parse*, Expr*);
void sqlite3ExprCodeAndCache(Parse*, Expr*);
int sqlite3ExprCodeExprList(Parse*, ExprList*);
void sqlite3ExprIfTrue(Parse*, Expr*, int, int);
void sqlite3ExprIfFalse(Parse*, Expr*, int, int);
void sqlite3NextedParse(Parse*, const char*, ...);
Table *sqlite3FindTable(sqlite3*,const char*, const char*);
Table *sqlite3LocateTable(Parse*,const char*, const char*);
Index *sqlite3FindIndex(sqlite3*,const char*, const char*);
void sqlite3UnlinkAndDeleteTable(sqlite3*,int,const char*);
void sqlite3UnlinkAndDeleteIndex(sqlite3*,int,const char*);
void sqlite3Vacuum(Parse*);
int sqlite3RunVacuum(char**, sqlite3*);
char *sqlite3NameFromToken(Token*);
int sqlite3ExprCheck(Parse*, Expr*, int, int*);
int sqlite3ExprCompare(Expr*, Expr*);
int sqliteFuncId(Token*);
int sqlite3ExprResolveNames(NameContext *, Expr *);
int sqlite3ExprAnalyzeAggregates(NameContext*, Expr*);
int sqlite3ExprAnalyzeAggList(NameContext*,ExprList*);
Vdbe *sqlite3GetVdbe(Parse*);
void sqlite3Randomness(int, void*);
void sqlite3RollbackAll(sqlite3*);
void sqlite3CodeVerifySchema(Parse*, int);
void sqlite3BeginTransaction(Parse*, int);
void sqlite3CommitTransaction(Parse*);
void sqlite3RollbackTransaction(Parse*);
int sqlite3ExprIsConstant(Expr*);
int sqlite3ExprIsConstantOrFunction(Expr*);
int sqlite3ExprIsInteger(Expr*, int*);
int sqlite3IsRowid(const char*);
void sqlite3GenerateRowDelete(sqlite3*, Vdbe*, Table*, int, int);
void sqlite3GenerateRowIndexDelete(Vdbe*, Table*, int, char*);
void sqlite3GenerateIndexKey(Vdbe*, Index*, int);
void sqlite3GenerateConstraintChecks(Parse*,Table*,int,char*,int,int,int,int);
void sqlite3CompleteInsertion(Parse*, Table*, int, char*, int, int, int);
void sqlite3OpenTableAndIndices(Parse*, Table*, int, int);
void sqlite3BeginWriteOperation(Parse*, int, int);
Expr *sqlite3ExprDup(Expr*);
void sqlite3TokenCopy(Token*, Token*);
ExprList *sqlite3ExprListDup(ExprList*);
SrcList *sqlite3SrcListDup(SrcList*);
IdList *sqlite3IdListDup(IdList*);
Select *sqlite3SelectDup(Select*);
FuncDef *sqlite3FindFunction(sqlite3*,const char*,int,int,u8,int);
void sqlite3RegisterBuiltinFunctions(sqlite3*);
void sqlite3RegisterDateTimeFunctions(sqlite3*);
int sqlite3SafetyOn(sqlite3*);
int sqlite3SafetyOff(sqlite3*);
int sqlite3SafetyCheck(sqlite3*);
void sqlite3ChangeCookie(sqlite3*, Vdbe*, int);

#ifndef SQLITE_OMIT_TRIGGER
  void sqlite3BeginTrigger(Parse*, Token*,Token*,int,int,IdList*,SrcList*,
                           int,Expr*,int);
  void sqlite3FinishTrigger(Parse*, TriggerStep*, Token*);
  void sqlite3DropTrigger(Parse*, SrcList*);
  void sqlite3DropTriggerPtr(Parse*, Trigger*);
  int sqlite3TriggersExist(Parse*, Table*, int, ExprList*);
  int sqlite3CodeRowTrigger(Parse*, int, ExprList*, int, Table *, int, int, 
                           int, int);
  void sqliteViewTriggers(Parse*, Table*, Expr*, int, ExprList*);
  void sqlite3DeleteTriggerStep(TriggerStep*);
  TriggerStep *sqlite3TriggerSelectStep(Select*);
  TriggerStep *sqlite3TriggerInsertStep(Token*, IdList*, ExprList*,Select*,int);
  TriggerStep *sqlite3TriggerUpdateStep(Token*, ExprList*, Expr*, int);
  TriggerStep *sqlite3TriggerDeleteStep(Token*, Expr*);
  void sqlite3DeleteTrigger(Trigger*);
  void sqlite3UnlinkAndDeleteTrigger(sqlite3*,int,const char*);
#else
# define sqlite3TriggersExist(A,B,C,D,E,F) 0
# define sqlite3DeleteTrigger(A)
# define sqlite3DropTriggerPtr(A,B)
# define sqlite3UnlinkAndDeleteTrigger(A,B,C)
# define sqlite3CodeRowTrigger(A,B,C,D,E,F,G,H,I) 0
#endif

int sqlite3JoinType(Parse*, Token*, Token*, Token*);
void sqlite3CreateForeignKey(Parse*, ExprList*, Token*, ExprList*, int);
void sqlite3DeferForeignKey(Parse*, int);
#ifndef SQLITE_OMIT_AUTHORIZATION
  void sqlite3AuthRead(Parse*,Expr*,SrcList*);
  int sqlite3AuthCheck(Parse*,int, const char*, const char*, const char*);
  void sqlite3AuthContextPush(Parse*, AuthContext*, const char*);
  void sqlite3AuthContextPop(AuthContext*);
#else
# define sqlite3AuthRead(a,b,c)
# define sqlite3AuthCheck(a,b,c,d,e)    SQLITE_OK
# define sqlite3AuthContextPush(a,b,c)
# define sqlite3AuthContextPop(a)  ((void)(a))
#endif
void sqlite3Attach(Parse*, Expr*, Expr*, Expr*);
void sqlite3Detach(Parse*, Expr*);
int sqlite3BtreeFactory(const sqlite3 *db, const char *zFilename,
                       int omitJournal, int nCache, Btree **ppBtree);
int sqlite3FixInit(DbFixer*, Parse*, int, const char*, const Token*);
int sqlite3FixSrcList(DbFixer*, SrcList*);
int sqlite3FixSelect(DbFixer*, Select*);
int sqlite3FixExpr(DbFixer*, Expr*);
int sqlite3FixExprList(DbFixer*, ExprList*);
int sqlite3FixTriggerStep(DbFixer*, TriggerStep*);
int sqlite3AtoF(const char *z, double*);
char *sqlite3_snprintf(int,char*,const char*,...);
int sqlite3GetInt32(const char *, int*);
int sqlite3FitsIn64Bits(const char *);
int sqlite3utf16ByteLen(const void *pData, int nChar);
int sqlite3utf8CharLen(const char *pData, int nByte);
int sqlite3ReadUtf8(const unsigned char *);
int sqlite3PutVarint(unsigned char *, u64);
int sqlite3GetVarint(const unsigned char *, u64 *);
int sqlite3GetVarint32(const unsigned char *, u32 *);
int sqlite3VarintLen(u64 v);
void sqlite3IndexAffinityStr(Vdbe *, Index *);
void sqlite3TableAffinityStr(Vdbe *, Table *);
char sqlite3CompareAffinity(Expr *pExpr, char aff2);
int sqlite3IndexAffinityOk(Expr *pExpr, char idx_affinity);
char sqlite3ExprAffinity(Expr *pExpr);
int sqlite3atoi64(const char*, i64*);
void sqlite3Error(sqlite3*, int, const char*,...);
void *sqlite3HexToBlob(const char *z);
int sqlite3TwoPartName(Parse *, Token *, Token *, Token **);
const char *sqlite3ErrStr(int);
int sqlite3ReadUniChar(const char *zStr, int *pOffset, u8 *pEnc, int fold);
int sqlite3ReadSchema(Parse *pParse);
CollSeq *sqlite3FindCollSeq(sqlite3*,u8 enc, const char *,int,int);
CollSeq *sqlite3LocateCollSeq(Parse *pParse, const char *zName, int nName);
CollSeq *sqlite3ExprCollSeq(Parse *pParse, Expr *pExpr);
int sqlite3CheckCollSeq(Parse *, CollSeq *);
int sqlite3CheckIndexCollSeq(Parse *, Index *);
int sqlite3CheckObjectName(Parse *, const char *);
void sqlite3VdbeSetChanges(sqlite3 *, int);
void sqlite3utf16Substr(sqlite3_context *,int,sqlite3_value **);

const void *sqlite3ValueText(sqlite3_value*, u8);
int sqlite3ValueBytes(sqlite3_value*, u8);
void sqlite3ValueSetStr(sqlite3_value*, int, const void *,u8, void(*)(void*));
void sqlite3ValueFree(sqlite3_value*);
sqlite3_value *sqlite3ValueNew(void);
char *sqlite3utf16to8(const void*, int);
int sqlite3ValueFromExpr(Expr *, u8, u8, sqlite3_value **);
void sqlite3ValueApplyAffinity(sqlite3_value *, u8, u8);
extern const unsigned char sqlite3UpperToLower[];
void sqlite3RootPageMoved(Db*, int, int);
void sqlite3Reindex(Parse*, Token*, Token*);
void sqlite3AlterFunctions(sqlite3*);
void sqlite3AlterRenameTable(Parse*, SrcList*, Token*);
int sqlite3GetToken(const unsigned char *, int *);
void sqlite3NestedParse(Parse*, const char*, ...);
void sqlite3ExpirePreparedStatements(sqlite3*);
void sqlite3CodeSubselect(Parse *, Expr *);
int sqlite3SelectResolve(Parse *, Select *, NameContext *);
void sqlite3ColumnDefault(Vdbe *, Table *, int);
void sqlite3AlterFinishAddColumn(Parse *, Token *);
void sqlite3AlterBeginAddColumn(Parse *, SrcList *);
const char *sqlite3TestErrorName(int);
CollSeq *sqlite3GetCollSeq(sqlite3*, CollSeq *, const char *, int);
char sqlite3AffinityType(const Token*);
void sqlite3Analyze(Parse*, Token*, Token*);
int sqlite3InvokeBusyHandler(BusyHandler*);
int sqlite3FindDb(sqlite3*, Token*);
void sqlite3AnalysisLoad(sqlite3*,int iDB);
void sqlite3DefaultRowEst(Index*);
void sqlite3RegisterLikeFunctions(sqlite3*, int);
int sqlite3IsLikeFunction(sqlite3*,Expr*,int*,char*);
ThreadData *sqlite3ThreadData(void);
const ThreadData *sqlite3ThreadDataReadOnly(void);
void sqlite3ReleaseThreadData(void);
void sqlite3AttachFunctions(sqlite3 *);
void sqlite3MinimumFileFormat(Parse*, int, int);
void sqlite3SchemaFree(void *);
Schema *sqlite3SchemaGet(Btree *);
int sqlite3SchemaToIndex(sqlite3 *db, Schema *);
KeyInfo *sqlite3IndexKeyinfo(Parse *, Index *);
int sqlite3CreateFunc(sqlite3 *, const char *, int, int, void *, 
  void (*)(sqlite3_context*,int,sqlite3_value **),
  void (*)(sqlite3_context*,int,sqlite3_value **), void (*)(sqlite3_context*));
int sqlite3ApiExit(sqlite3 *db, int);
int sqlite3MallocFailed(void);
void sqlite3FailedMalloc(void);
void sqlite3AbortOtherActiveVdbes(sqlite3 *, Vdbe *);
int sqlite3OpenTempDatabase(Parse *);

#ifndef SQLITE_OMIT_SHARED_CACHE
  void sqlite3TableLock(Parse *, int, int, u8, const char *);
#else
  #define sqlite3TableLock(v,w,x,y,z)
#endif

#ifdef SQLITE_MEMDEBUG
  void sqlite3MallocDisallow(void);
  void sqlite3MallocAllow(void);
  int sqlite3TestMallocFail(void);
#else
  #define sqlite3TestMallocFail() 0
  #define sqlite3MallocDisallow()
  #define sqlite3MallocAllow()
#endif

#ifdef SQLITE_ENABLE_MEMORY_MANAGEMENT
  void *sqlite3ThreadSafeMalloc(int);
  void sqlite3ThreadSafeFree(void *);
#else
  #define sqlite3ThreadSafeMalloc sqlite3MallocX
  #define sqlite3ThreadSafeFree sqlite3FreeX
#endif

#ifdef SQLITE_SSE
#include "sseInt.h"
#endif

#endif
