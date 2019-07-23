





















#define keyToInt(X)   (X)
#define intToKey(X)   (X)







extern char *sqlite3OpcodeNames[];






typedef struct VdbeOp Op;




typedef unsigned char Bool;
















struct Cursor {
  BtCursor *pCursor;    
  int iDb;              
  i64 lastRowid;        
  i64 nextRowid;        
  Bool zeroed;          
  Bool rowidIsValid;    
  Bool atFirst;         
  Bool useRandomRowid;  
  Bool nullRow;         
  Bool nextRowidValid;  
  Bool pseudoTable;     
  Bool deferredMoveto;  
  Bool isTable;         
  Bool isIndex;         
  u8 bogusIncrKey;      
  i64 movetoTarget;     
  Btree *pBt;           
  int nData;            
  char *pData;          
  i64 iKey;             
  u8 *pIncrKey;         
  KeyInfo *pKeyInfo;    
  int nField;           
  i64 seqCount;         

  




  int cacheStatus;      
  int payloadSize;      
  u32 *aType;           
  u32 *aOffset;         
  u8 *aRow;             
};
typedef struct Cursor Cursor;






#define NBFS 32




#define CACHE_STALE 0












struct Mem {
  i64 i;              
  double r;           
  char *z;            
  int n;              
  u16 flags;          
  u8  type;           
  u8  enc;            
  void (*xDel)(void *);  
  char zShort[NBFS];  
};
typedef struct Mem Mem;
















#define MEM_Null      0x0001   /* Value is NULL */
#define MEM_Str       0x0002   /* Value is a string */
#define MEM_Int       0x0004   /* Value is an integer */
#define MEM_Real      0x0008   /* Value is a real number */
#define MEM_Blob      0x0010   /* Value is a BLOB */






#define MEM_Term      0x0020   /* String rep is nul terminated */
#define MEM_Dyn       0x0040   /* Need to call sqliteFree() on Mem.z */
#define MEM_Static    0x0080   /* Mem.z points to a static string */
#define MEM_Ephem     0x0100   /* Mem.z points to an ephemeral string */
#define MEM_Short     0x0200   /* Mem.z points to Mem.zShort */
#define MEM_Agg       0x0400   /* Mem.z points to an agg function context */











struct VdbeFunc {
  FuncDef *pFunc;               
  int nAux;                     
  struct AuxData {
    void *pAux;                   
    void (*xDelete)(void *);      
  } apAux[1];                   
};
typedef struct VdbeFunc VdbeFunc;














struct sqlite3_context {
  FuncDef *pFunc;       
  VdbeFunc *pVdbeFunc;  
  Mem s;                
  Mem *pMem;            
  u8 isError;           
  CollSeq *pColl;       
};







typedef struct Set Set;
struct Set {
  Hash hash;             
  HashElem *prev;        
};





typedef struct FifoPage FifoPage;
struct FifoPage {
  int nSlot;         
  int iWrite;        
  int iRead;         
  FifoPage *pNext;   
  i64 aSlot[1];      
};







typedef struct Fifo Fifo;
struct Fifo {
  int nEntry;         
  FifoPage *pFirst;   
  FifoPage *pLast;    
};










typedef struct Context Context;
struct Context {
  i64 lastRowid;    
  int nChange;      
  Fifo sFifo;       
};








struct Vdbe {
  sqlite3 *db;        
  Vdbe *pPrev,*pNext; 
  FILE *trace;        
  int nOp;            
  int nOpAlloc;       
  Op *aOp;            
  int nLabel;         
  int nLabelAlloc;    
  int *aLabel;        
  Mem *aStack;        
  Mem *pTos;          
  Mem **apArg;        
  Mem *aColName;      
  int nCursor;        
  Cursor **apCsr;     
  int nVar;           
  Mem *aVar;          
  char **azVar;       
  int okVar;          
  int magic;              
  int nMem;               
  Mem *aMem;              
  int nCallback;          
  int cacheCtr;           
  Fifo sFifo;             
  int contextStackTop;    
  int contextStackDepth;  
  Context *contextStack;  
  int pc;                 
  int rc;                 
  unsigned uniqueCnt;     
  int errorAction;        
  int inTempTrans;        
  int returnStack[100];   
  int returnDepth;        
  int nResColumn;         
  char **azResColumn;      
  int popStack;           
  char *zErrMsg;          
  u8 resOnStack;          
  u8 explain;             
  u8 changeCntOn;         
  u8 aborted;             
  u8 expired;             
  u8 minWriteFileFormat;  
  int nChange;            
  i64 startTime;          
#ifdef SQLITE_SSE
  int fetchId;          
  int lru;              
#endif
};




#define VDBE_MAGIC_INIT     0x26bceaa5    /* Building a VDBE program */
#define VDBE_MAGIC_RUN      0xbdf20da3    /* VDBE is ready to execute */
#define VDBE_MAGIC_HALT     0x519c2973    /* VDBE has completed execution */
#define VDBE_MAGIC_DEAD     0xb606c3c8    /* The VDBE has been deallocated */




void sqlite3VdbeFreeCursor(Cursor*);
void sqliteVdbePopStack(Vdbe*,int);
int sqlite3VdbeCursorMoveto(Cursor*);
#if defined(SQLITE_DEBUG) || defined(VDBE_PROFILE)
void sqlite3VdbePrintOp(FILE*, int, Op*);
#endif
#ifdef SQLITE_DEBUG
void sqlite3VdbePrintSql(Vdbe*);
#endif
int sqlite3VdbeSerialTypeLen(u32);
u32 sqlite3VdbeSerialType(Mem*, int);
int sqlite3VdbeSerialPut(unsigned char*, Mem*, int);
int sqlite3VdbeSerialGet(const unsigned char*, u32, Mem*);
void sqlite3VdbeDeleteAuxData(VdbeFunc*, int);

int sqlite2BtreeKeyCompare(BtCursor *, const void *, int, int, int *);
int sqlite3VdbeIdxKeyCompare(Cursor*, int , const unsigned char*, int*);
int sqlite3VdbeIdxRowid(BtCursor *, i64 *);
int sqlite3MemCompare(const Mem*, const Mem*, const CollSeq*);
int sqlite3VdbeRecordCompare(void*,int,const void*,int, const void*);
int sqlite3VdbeIdxRowidLen(const u8*);
int sqlite3VdbeExec(Vdbe*);
int sqlite3VdbeList(Vdbe*);
int sqlite3VdbeHalt(Vdbe*);
int sqlite3VdbeChangeEncoding(Mem *, int);
int sqlite3VdbeMemCopy(Mem*, const Mem*);
void sqlite3VdbeMemShallowCopy(Mem*, const Mem*, int);
int sqlite3VdbeMemMove(Mem*, Mem*);
int sqlite3VdbeMemNulTerminate(Mem*);
int sqlite3VdbeMemSetStr(Mem*, const char*, int, u8, void(*)(void*));
void sqlite3VdbeMemSetInt64(Mem*, i64);
void sqlite3VdbeMemSetDouble(Mem*, double);
void sqlite3VdbeMemSetNull(Mem*);
int sqlite3VdbeMemMakeWriteable(Mem*);
int sqlite3VdbeMemDynamicify(Mem*);
int sqlite3VdbeMemStringify(Mem*, int);
i64 sqlite3VdbeIntValue(Mem*);
int sqlite3VdbeMemIntegerify(Mem*);
double sqlite3VdbeRealValue(Mem*);
void sqlite3VdbeIntegerAffinity(Mem*);
int sqlite3VdbeMemRealify(Mem*);
int sqlite3VdbeMemNumerify(Mem*);
int sqlite3VdbeMemFromBtree(BtCursor*,int,int,int,Mem*);
void sqlite3VdbeMemRelease(Mem *p);
int sqlite3VdbeMemFinalize(Mem*, FuncDef*);
#ifndef NDEBUG
void sqlite3VdbeMemSanity(Mem*);
int sqlite3VdbeOpcodeNoPush(u8);
#endif
int sqlite3VdbeMemTranslate(Mem*, u8);
void sqlite3VdbeMemPrettyPrint(Mem *pMem, char *zBuf);
int sqlite3VdbeMemHandleBom(Mem *pMem);
void sqlite3VdbeFifoInit(Fifo*);
int sqlite3VdbeFifoPush(Fifo*, i64);
int sqlite3VdbeFifoPop(Fifo*, i64*);
void sqlite3VdbeFifoClear(Fifo*);
