





































#ifndef _MDB_
#define _MDB_ 1

#include "nscore.h"
#include "nsISupports.h"



typedef unsigned char  mdb_u1;  
typedef unsigned short mdb_u2;  
typedef short          mdb_i2;  
typedef PRUint32       mdb_u4;  
typedef PRInt32        mdb_i4;  
typedef PRWord         mdb_ip;  

typedef mdb_u1 mdb_bool;  


#define mdbBool_kTrue  ((mdb_bool) 1) /* actually any nonzero means true */
#define mdbBool_kFalse ((mdb_bool) 0) /* only zero means false */

typedef mdb_u4 mdb_id;    
typedef mdb_id mdb_rid;          
typedef mdb_id mdb_tid;          
typedef mdb_u4 mdb_token; 
typedef mdb_token mdb_scope;     
typedef mdb_token mdb_kind;      
typedef mdb_token mdb_column;    
typedef mdb_token mdb_cscode;    
typedef mdb_u4 mdb_seed;  
typedef mdb_u4 mdb_count; 
typedef mdb_u4 mdb_size;  
typedef mdb_u4 mdb_fill;  
typedef mdb_u4 mdb_more;  

#define mdbId_kNone ((mdb_id) -1) /* never a valid Mork object ID */

typedef mdb_u4 mdb_percent; 

typedef mdb_u1 mdb_priority; 


typedef nsresult mdb_err;   


typedef mdb_i4 mdb_pos; 

#define mdbPos_kBeforeFirst ((mdb_pos) -1) /* any negative is before zero */


typedef mdb_i4 mdb_order; 

typedef mdb_order (* mdbAny_Order)(const void* inA, const void* inB, 
  const void* inClosure);





#ifndef mdbScopeStringSet_typedef
typedef struct mdbScopeStringSet mdbScopeStringSet;
#define mdbScopeStringSet_typedef 1
#endif















#ifndef mdbScopeStringSet_struct
#define mdbScopeStringSet_struct 1
struct mdbScopeStringSet { 
  
  mdb_count     mScopeStringSet_Count;    
  const char**  mScopeStringSet_Strings;  
};
#endif 

#ifndef mdbOpenPolicy_typedef
typedef struct mdbOpenPolicy mdbOpenPolicy;
#define mdbOpenPolicy_typedef 1
#endif

#ifndef mdbOpenPolicy_struct
#define mdbOpenPolicy_struct 1
struct mdbOpenPolicy { 
  mdbScopeStringSet  mOpenPolicy_ScopePlan; 
  mdb_bool           mOpenPolicy_MaxLazy;   
  mdb_bool           mOpenPolicy_MinMemory; 
};
#endif 

#ifndef mdbTokenSet_typedef
typedef struct mdbTokenSet mdbTokenSet;
#define mdbTokenSet_typedef 1
#endif

#ifndef mdbTokenSet_struct
#define mdbTokenSet_struct 1
struct mdbTokenSet { 
  mdb_count   mTokenSet_Count;   
  mdb_fill    mTokenSet_Fill;    
  mdb_more    mTokenSet_More;    
  mdb_token*  mTokenSet_Tokens;  
};
#endif 

#ifndef mdbUsagePolicy_typedef
typedef struct mdbUsagePolicy mdbUsagePolicy;
#define mdbUsagePolicy_typedef 1
#endif






#ifndef mdbUsagePolicy_struct
#define mdbUsagePolicy_struct 1
struct mdbUsagePolicy { 
  mdbTokenSet  mUsagePolicy_ScopePlan; 
  mdb_bool     mUsagePolicy_MaxLazy;   
  mdb_bool     mUsagePolicy_MinMemory; 
};
#endif 

#ifndef mdbOid_typedef
typedef struct mdbOid mdbOid;
#define mdbOid_typedef 1
#endif

#ifndef mdbOid_struct
#define mdbOid_struct 1
struct mdbOid { 
  mdb_scope   mOid_Scope;  
  mdb_id      mOid_Id;     
};
#endif 

#ifndef mdbRange_typedef
typedef struct mdbRange mdbRange;
#define mdbRange_typedef 1
#endif

#ifndef mdbRange_struct
#define mdbRange_struct 1
struct mdbRange { 
  mdb_pos   mRange_FirstPos;  
  mdb_pos   mRange_LastPos;   
};
#endif 

#ifndef mdbColumnSet_typedef
typedef struct mdbColumnSet mdbColumnSet;
#define mdbColumnSet_typedef 1
#endif

#ifndef mdbColumnSet_struct
#define mdbColumnSet_struct 1
struct mdbColumnSet { 
  mdb_count    mColumnSet_Count;    
  mdb_column*  mColumnSet_Columns;  
};
#endif 

#ifndef mdbYarn_typedef
typedef struct mdbYarn mdbYarn;
#define mdbYarn_typedef 1
#endif

#ifdef MDB_BEGIN_C_LINKAGE_define
#define MDB_BEGIN_C_LINKAGE_define 1
#define MDB_BEGIN_C_LINKAGE extern "C" {
#define MDB_END_C_LINKAGE }
#endif 




























typedef void (* mdbYarn_mGrow)(mdbYarn* self, mdb_size inNewSize);




























































#ifndef mdbYarn_struct
#define mdbYarn_struct 1
struct mdbYarn { 
  void*         mYarn_Buf;   
  mdb_fill      mYarn_Fill;  
  mdb_size      mYarn_Size;  
  mdb_more      mYarn_More;  
  mdb_cscode    mYarn_Form;  
  mdbYarn_mGrow mYarn_Grow;  
  
  
  
};
#endif 




class nsIMdbEnv;
class nsIMdbObject;
class nsIMdbErrorHook;
class nsIMdbCompare;
class nsIMdbThumb;
class nsIMdbFactory;
class nsIMdbFile;
class nsIMdbPort;
class nsIMdbStore;
class nsIMdbCursor;
class nsIMdbPortTableCursor;
class nsIMdbCollection;
class nsIMdbTable;
class nsIMdbTableRowCursor;
class nsIMdbRow;
class nsIMdbRowCellCursor;
class nsIMdbBlob;
class nsIMdbCell;
class nsIMdbSorting;



















#define NS_IMDBOBJECT_IID_STR "5533ea4b-14c3-4bef-ac60-22f9e9a49084"

#define NS_IMDBOBJECT_IID \
{0x5533ea4b, 0x14c3, 0x4bef, \
{ 0xac, 0x60, 0x22, 0xf9, 0xe9, 0xa4, 0x90, 0x84}}

class nsIMdbObject : public nsISupports { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBOBJECT_IID)


  
  NS_IMETHOD IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly) = 0;
  
  

  
  NS_IMETHOD GetMdbFactory(nsIMdbEnv* ev, nsIMdbFactory** acqFactory) = 0; 
  

  
  NS_IMETHOD GetWeakRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount) = 0;  
  NS_IMETHOD GetStrongRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount) = 0;

  NS_IMETHOD AddWeakRef(nsIMdbEnv* ev) = 0;
  NS_IMETHOD AddStrongRef(nsIMdbEnv* ev) = 0;

  NS_IMETHOD CutWeakRef(nsIMdbEnv* ev) = 0;
  NS_IMETHOD CutStrongRef(nsIMdbEnv* ev) = 0;
  
  NS_IMETHOD CloseMdbObject(nsIMdbEnv* ev) = 0; 
  NS_IMETHOD IsOpenMdbObject(nsIMdbEnv* ev, mdb_bool* outOpen) = 0;
  
  

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbObject, NS_IMDBOBJECT_IID)













class nsIMdbErrorHook : public nsISupports{ 
public:


  NS_IMETHOD OnErrorString(nsIMdbEnv* ev, const char* inAscii) = 0;
  NS_IMETHOD OnErrorYarn(nsIMdbEnv* ev, const mdbYarn* inYarn) = 0;



  NS_IMETHOD OnWarningString(nsIMdbEnv* ev, const char* inAscii) = 0;
  NS_IMETHOD OnWarningYarn(nsIMdbEnv* ev, const mdbYarn* inYarn) = 0;



  NS_IMETHOD OnAbortHintString(nsIMdbEnv* ev, const char* inAscii) = 0;
  NS_IMETHOD OnAbortHintYarn(nsIMdbEnv* ev, const mdbYarn* inYarn) = 0;

};








class nsIMdbCompare { 
public:


  NS_IMETHOD Order(nsIMdbEnv* ev,      
    const mdbYarn* inFirst,   
    const mdbYarn* inSecond,  
    mdb_order* outOrder) = 0; 
    
  NS_IMETHOD AddStrongRef(nsIMdbEnv* ev) = 0; 
  NS_IMETHOD CutStrongRef(nsIMdbEnv* ev) = 0; 

  
};












class nsIMdbHeap { 
public:

  NS_IMETHOD Alloc(nsIMdbEnv* ev, 
    mdb_size inSize,        
    void** outBlock) = 0;   
    
  NS_IMETHOD Free(nsIMdbEnv* ev, 
    void* ioBlock) = 0;     
    
  NS_IMETHOD HeapAddStrongRef(nsIMdbEnv* ev) = 0;
  NS_IMETHOD HeapCutStrongRef(nsIMdbEnv* ev) = 0;
    

};




class nsIMdbCPlusHeap { 
public:

  NS_IMETHOD Alloc(nsIMdbEnv* ev, 
    mdb_size inSize,   
    void** outBlock);  
    
  NS_IMETHOD Free(nsIMdbEnv* ev, 
    void* inBlock);
    
  NS_IMETHOD HeapAddStrongRef(nsIMdbEnv* ev);
  NS_IMETHOD HeapCutStrongRef(nsIMdbEnv* ev);

};





#define NS_IMDBTHUMB_IID_STR "6d3ad7c1-a809-4e74-8577-49fa9a4562fa"

#define NS_IMDBTHUMB_IID \
{0x6d3ad7c1, 0xa809, 0x4e74, \
{ 0x85, 0x77, 0x49, 0xfa, 0x9a, 0x45, 0x62, 0xfa}}


class nsIMdbThumb : public nsISupports { 
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBTHUMB_IID)


  NS_IMETHOD GetProgress(nsIMdbEnv* ev,
    mdb_count* outTotal,    
    mdb_count* outCurrent,  
    mdb_bool* outDone,      
    mdb_bool* outBroken     
  ) = 0;
  
  NS_IMETHOD DoMore(nsIMdbEnv* ev,
    mdb_count* outTotal,    
    mdb_count* outCurrent,  
    mdb_bool* outDone,      
    mdb_bool* outBroken     
  ) = 0;
  
  NS_IMETHOD CancelAndBreakThumb( 
    nsIMdbEnv* ev) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbThumb, NS_IMDBTHUMB_IID)































#define NS_IMDBENV_IID_STR "a765e46b-efb6-41e6-b75b-c5d6bd710594"

#define NS_IMDBENV_IID \
{0xa765e46b, 0xefb6, 0x41e6, \
{ 0xb7, 0x5b, 0xc5, 0xd6, 0xbd, 0x71, 0x05, 0x94}}

class nsIMdbEnv : public nsISupports { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBENV_IID)


  
  NS_IMETHOD GetErrorCount(mdb_count* outCount,
    mdb_bool* outShouldAbort) = 0;
  NS_IMETHOD GetWarningCount(mdb_count* outCount,
    mdb_bool* outShouldAbort) = 0;
  
  NS_IMETHOD GetEnvBeVerbose(mdb_bool* outBeVerbose) = 0;
  NS_IMETHOD SetEnvBeVerbose(mdb_bool inBeVerbose) = 0;
  
  NS_IMETHOD GetDoTrace(mdb_bool* outDoTrace) = 0;
  NS_IMETHOD SetDoTrace(mdb_bool inDoTrace) = 0;
  
  NS_IMETHOD GetAutoClear(mdb_bool* outAutoClear) = 0;
  NS_IMETHOD SetAutoClear(mdb_bool inAutoClear) = 0;
  
  NS_IMETHOD GetErrorHook(nsIMdbErrorHook** acqErrorHook) = 0;
  NS_IMETHOD SetErrorHook(
    nsIMdbErrorHook* ioErrorHook) = 0; 
  
  NS_IMETHOD GetHeap(nsIMdbHeap** acqHeap) = 0;
  NS_IMETHOD SetHeap(
    nsIMdbHeap* ioHeap) = 0; 
  
  
  NS_IMETHOD ClearErrors() = 0; 
  NS_IMETHOD ClearWarnings() = 0; 
  NS_IMETHOD ClearErrorsAndWarnings() = 0; 

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbEnv, NS_IMDBENV_IID)
















































#define NS_IMDBFACTORY_IID_STR "2b80395c-b91e-4990-b1a7-023e99ab14e9"

#define NS_IMDBFACTORY_IID \
{0xf04aa4ab, 0x1fe, 0x4115, \
{ 0xa4, 0xa5, 0x68, 0x19, 0xdf, 0xf1, 0x10, 0x3d}}


class nsIMdbFactory : public nsISupports { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBFACTORY_IID)


  
  NS_IMETHOD OpenOldFile(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath,
    mdb_bool inFrozen, nsIMdbFile** acqFile) = 0;
  
  
  
  
  
  

  NS_IMETHOD CreateNewFile(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath,
    nsIMdbFile** acqFile) = 0;
  
  
  
  
  
  
  

  
  NS_IMETHOD MakeEnv(nsIMdbHeap* ioHeap, nsIMdbEnv** acqEnv) = 0; 
  
  

  
  NS_IMETHOD MakeHeap(nsIMdbEnv* ev, nsIMdbHeap** acqHeap) = 0; 
  

  
  NS_IMETHOD MakeCompare(nsIMdbEnv* ev, nsIMdbCompare** acqCompare) = 0; 
  

  
  NS_IMETHOD MakeRow(nsIMdbEnv* ev, nsIMdbHeap* ioHeap, nsIMdbRow** acqRow) = 0; 
  
  
  
  
  NS_IMETHOD CanOpenFilePort(
    nsIMdbEnv* ev, 
    
    
    nsIMdbFile* ioFile, 
    mdb_bool* outCanOpen, 
    mdbYarn* outFormatVersion) = 0; 
    
  NS_IMETHOD OpenFilePort(
    nsIMdbEnv* ev, 
    nsIMdbHeap* ioHeap, 
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy, 
    nsIMdbThumb** acqThumb) = 0; 
  
  

  NS_IMETHOD ThumbToOpenPort( 
    nsIMdbEnv* ev, 
    nsIMdbThumb* ioThumb, 
    nsIMdbPort** acqPort) = 0; 
  
  
  
  NS_IMETHOD CanOpenFileStore(
    nsIMdbEnv* ev, 
    
    
    nsIMdbFile* ioFile, 
    mdb_bool* outCanOpenAsStore, 
    mdb_bool* outCanOpenAsPort, 
    mdbYarn* outFormatVersion) = 0; 
    
  NS_IMETHOD OpenFileStore( 
    nsIMdbEnv* ev, 
    nsIMdbHeap* ioHeap, 
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy, 
    nsIMdbThumb** acqThumb) = 0; 
  
  
    
  NS_IMETHOD
  ThumbToOpenStore( 
    nsIMdbEnv* ev, 
    nsIMdbThumb* ioThumb, 
    nsIMdbStore** acqStore) = 0; 
  
  NS_IMETHOD CreateNewFileStore( 
    nsIMdbEnv* ev, 
    nsIMdbHeap* ioHeap, 
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy, 
    nsIMdbStore** acqStore) = 0; 
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbFactory, NS_IMDBFACTORY_IID)

extern "C" nsIMdbFactory* MakeMdbFactory(); 














































#define NS_IMDBFILE_IID_STR "f04aa4ab-1fe7-4115-a4a5-6819dff1103d"

#define NS_IMDBFILE_IID \
{0xf04aa4ab, 0x1fe, 0x4115, \
{ 0xa4, 0xa5, 0x68, 0x19, 0xdf, 0xf1, 0x10, 0x3d}}

class nsIMdbFile : public nsISupports { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBFILE_IID)


  
  NS_IMETHOD Tell(nsIMdbEnv* ev, mdb_pos* outPos) const = 0;
  NS_IMETHOD Seek(nsIMdbEnv* ev, mdb_pos inPos, mdb_pos *outPos) = 0;
  NS_IMETHOD Eof(nsIMdbEnv* ev, mdb_pos* outPos) = 0;
  

  
  NS_IMETHOD Read(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_size* outActualSize) = 0;
  NS_IMETHOD Get(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_pos inPos, mdb_size* outActualSize) = 0;
  
    
  
  NS_IMETHOD  Write(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_size* outActualSize) = 0;
  NS_IMETHOD  Put(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_pos inPos, mdb_size* outActualSize) = 0;
  NS_IMETHOD  Flush(nsIMdbEnv* ev) = 0;
  
    
  
  NS_IMETHOD  Path(nsIMdbEnv* ev, mdbYarn* outFilePath) = 0;
  
    
  
  NS_IMETHOD  Steal(nsIMdbEnv* ev, nsIMdbFile* ioThief) = 0;
  NS_IMETHOD  Thief(nsIMdbEnv* ev, nsIMdbFile** acqThief) = 0;
  

  
  NS_IMETHOD BecomeTrunk(nsIMdbEnv* ev) = 0;
  
  
  
  
  
  
  

  NS_IMETHOD AcquireBud(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    nsIMdbFile** acqBud) = 0; 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbFile, NS_IMDBFILE_IID)



























































class nsIMdbPort : public nsISupports {
public:



  
  NS_IMETHOD GetIsPortReadonly(nsIMdbEnv* ev, mdb_bool* outBool) = 0;
  NS_IMETHOD GetIsStore(nsIMdbEnv* ev, mdb_bool* outBool) = 0;
  NS_IMETHOD GetIsStoreAndDirty(nsIMdbEnv* ev, mdb_bool* outBool) = 0;

  NS_IMETHOD GetUsagePolicy(nsIMdbEnv* ev, 
    mdbUsagePolicy* ioUsagePolicy) = 0;

  NS_IMETHOD SetUsagePolicy(nsIMdbEnv* ev, 
    const mdbUsagePolicy* inUsagePolicy) = 0;
  

  
  NS_IMETHOD IdleMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size* outEstimatedBytesFreed) = 0; 

  NS_IMETHOD SessionMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size inDesiredBytesFreed, 
    mdb_size* outEstimatedBytesFreed) = 0; 

  NS_IMETHOD PanicMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size* outEstimatedBytesFreed) = 0; 
  

  
  NS_IMETHOD GetPortFilePath(
    nsIMdbEnv* ev, 
    mdbYarn* outFilePath, 
    mdbYarn* outFormatVersion) = 0; 
    
  NS_IMETHOD GetPortFile(
    nsIMdbEnv* ev, 
    nsIMdbFile** acqFile) = 0; 
  

  
  NS_IMETHOD BestExportFormat( 
    nsIMdbEnv* ev, 
    mdbYarn* outFormatVersion) = 0; 

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  NS_IMETHOD
  CanExportToFormat( 
    nsIMdbEnv* ev, 
    const char* inFormatVersion, 
    mdb_bool* outCanExport) = 0; 

  NS_IMETHOD ExportToFormat( 
    nsIMdbEnv* ev, 
    
    nsIMdbFile* ioFile, 
    const char* inFormatVersion, 
    nsIMdbThumb** acqThumb) = 0; 
  
  

  

  
  NS_IMETHOD TokenToString( 
    nsIMdbEnv* ev, 
    mdb_token inToken, 
    mdbYarn* outTokenName) = 0; 
  
  NS_IMETHOD StringToToken( 
    nsIMdbEnv* ev, 
    const char* inTokenName, 
    mdb_token* outToken) = 0; 
    
  
  
  
  

  NS_IMETHOD QueryToken( 
    nsIMdbEnv* ev, 
    const char* inTokenName, 
    mdb_token* outToken) = 0; 
  
  
  
  

  

  
  NS_IMETHOD HasRow( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_bool* outHasRow) = 0; 

  NS_IMETHOD GetRowRefCount( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_count* outRefCount) = 0; 
    
  NS_IMETHOD GetRow( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    nsIMdbRow** acqRow) = 0; 
    
  
  
  
  
  

  NS_IMETHOD FindRow(nsIMdbEnv* ev, 
    mdb_scope inRowScope,   
    mdb_column inColumn,   
    const mdbYarn* inTargetCellValue, 
    mdbOid* outRowOid, 
    nsIMdbRow** acqRow) = 0; 
                             
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  NS_IMETHOD HasTable( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_bool* outHasTable) = 0; 
    
  NS_IMETHOD GetTable( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    nsIMdbTable** acqTable) = 0; 
  
  NS_IMETHOD HasTableKind( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    mdb_kind inTableKind, 
    mdb_count* outTableCount, 
    mdb_bool* outSupportsTable) = 0; 
    
  
  
  
  
 
  
  
  
  
  
  
  
  
    
  NS_IMETHOD GetTableKind( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope,      
    mdb_kind inTableKind,      
    mdb_count* outTableCount, 
    mdb_bool* outMustBeUnique, 
    nsIMdbTable** acqTable) = 0;       
    
  NS_IMETHOD
  GetPortTableCursor( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    mdb_kind inTableKind, 
    nsIMdbPortTableCursor** acqCursor) = 0; 
  


  

  NS_IMETHOD ShouldCompress( 
    nsIMdbEnv* ev, 
    mdb_percent inPercentWaste, 
    mdb_percent* outActualWaste, 
    mdb_bool* outShould) = 0; 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


};


































































#define NS_IMDBSTORE_IID_STR "726618d3-f15b-49b9-9f4a-efcc9db53d0d"

#define NS_IMDBSTORE_IID \
{0x726618d3, 0xf15b, 0x49b9, \
{0x9f, 0x4a, 0xef, 0xcc, 0x9d, 0xb5, 0x3d, 0x0d}}

class nsIMdbStore : public nsIMdbPort {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBSTORE_IID)



  
  NS_IMETHOD NewTable( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope,    
    mdb_kind inTableKind,    
    mdb_bool inMustBeUnique, 
    const mdbOid* inOptionalMetaRowOid, 
    nsIMdbTable** acqTable) = 0;     
    
  NS_IMETHOD NewTableWithOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,   
    mdb_kind inTableKind,    
    mdb_bool inMustBeUnique, 
    const mdbOid* inOptionalMetaRowOid, 
    nsIMdbTable** acqTable) = 0;     
  

  
  NS_IMETHOD RowScopeHasAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned) = 0; 

  NS_IMETHOD SetCallerAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned) = 0; 

  NS_IMETHOD SetStoreAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned) = 0; 
  

  
  NS_IMETHOD NewRowWithOid(nsIMdbEnv* ev, 
    const mdbOid* inOid,   
    nsIMdbRow** acqRow) = 0; 

  NS_IMETHOD NewRow(nsIMdbEnv* ev, 
    mdb_scope inRowScope,   
    nsIMdbRow** acqRow) = 0; 
  
  

  

  
  NS_IMETHOD ImportContent( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    nsIMdbPort* ioPort, 
    nsIMdbThumb** acqThumb) = 0; 
  
  

  NS_IMETHOD ImportFile( 
    nsIMdbEnv* ev, 
    nsIMdbFile* ioFile, 
    nsIMdbThumb** acqThumb) = 0; 
  
  
  

  
  NS_IMETHOD
  ShareAtomColumnsHint( 
    nsIMdbEnv* ev, 
    mdb_scope inScopeHint, 
    const mdbColumnSet* inColumnSet) = 0; 

  NS_IMETHOD
  AvoidAtomColumnsHint( 
    nsIMdbEnv* ev, 
    const mdbColumnSet* inColumnSet) = 0; 
  

  
  NS_IMETHOD SmallCommit( 
    nsIMdbEnv* ev) = 0; 
  
  NS_IMETHOD LargeCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb) = 0; 
  
  
  
  

  NS_IMETHOD SessionCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb) = 0; 
  
  
  
  

  NS_IMETHOD
  CompressCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb) = 0; 
  
  
  
  
  
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbStore, NS_IMDBSTORE_IID)
























#define NS_IMDBCURSOR_IID_STR "a0c37337-6ebc-474c-90db-e65ea0b850aa"

#define NS_IMDBCURSOR_IID \
{0xa0c37337, 0x6ebc, 0x474c, \
{0x90, 0xdb, 0xe6, 0x5e, 0xa0, 0xb8, 0x50, 0xaa}}

class nsIMdbCursor  : public nsISupports  { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBCURSOR_IID)


  
  NS_IMETHOD GetCount(nsIMdbEnv* ev, mdb_count* outCount) = 0; 
  NS_IMETHOD GetSeed(nsIMdbEnv* ev, mdb_seed* outSeed) = 0;    
  
  NS_IMETHOD SetPos(nsIMdbEnv* ev, mdb_pos inPos) = 0;   
  NS_IMETHOD GetPos(nsIMdbEnv* ev, mdb_pos* outPos) = 0;
  
  NS_IMETHOD SetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool inFail) = 0;
  NS_IMETHOD GetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool* outFail) = 0;
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbCursor, NS_IMDBCURSOR_IID)

#define NS_IMDBPORTTABLECURSOR_IID_STR = "f181a41e-933d-49b3-af93-20d3634b8b78"

#define NS_IMDBPORTTABLECURSOR_IID \
{0xf181a41e, 0x933d, 0x49b3, \
{0xaf, 0x93, 0x20, 0xd3, 0x63, 0x4b, 0x8b, 0x78}}








class nsIMdbPortTableCursor : public nsISupports { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBPORTTABLECURSOR_IID)


  
  NS_IMETHOD SetPort(nsIMdbEnv* ev, nsIMdbPort* ioPort) = 0; 
  NS_IMETHOD GetPort(nsIMdbEnv* ev, nsIMdbPort** acqPort) = 0;
  
  NS_IMETHOD SetRowScope(nsIMdbEnv* ev, 
    mdb_scope inRowScope) = 0;
  NS_IMETHOD GetRowScope(nsIMdbEnv* ev, mdb_scope* outRowScope) = 0; 
  
    
  NS_IMETHOD SetTableKind(nsIMdbEnv* ev, 
    mdb_kind inTableKind) = 0;
  NS_IMETHOD GetTableKind(nsIMdbEnv* ev, mdb_kind* outTableKind) = 0;
  
  

  
  NS_IMETHOD NextTable( 
    nsIMdbEnv* ev, 
    nsIMdbTable** acqTable) = 0; 
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbPortTableCursor,
                              NS_IMDBPORTTABLECURSOR_IID)













































































class nsIMdbCollection : public nsISupports { 
public:



  
  NS_IMETHOD GetSeed(nsIMdbEnv* ev,
    mdb_seed* outSeed) = 0;    
  NS_IMETHOD GetCount(nsIMdbEnv* ev,
    mdb_count* outCount) = 0; 

  NS_IMETHOD GetPort(nsIMdbEnv* ev,
    nsIMdbPort** acqPort) = 0; 
  

  
  NS_IMETHOD GetCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inMemberPos, 
    nsIMdbCursor** acqCursor) = 0; 
  

  
  NS_IMETHOD GetOid(nsIMdbEnv* ev,
    mdbOid* outOid) = 0; 
  NS_IMETHOD BecomeContent(nsIMdbEnv* ev,
    const mdbOid* inOid) = 0; 
  

  
  NS_IMETHOD DropActivity( 
    nsIMdbEnv* ev) = 0;
  


};
















































































#define NS_IMDBTABLE_IID_STR = "fe11bc98-d02b-4128-9fac-87042fdf9639"

#define NS_IMDBTABLE_IID \
{0xfe11bc98, 0xd02b, 0x4128, \
{0x9f, 0xac, 0x87, 0x04, 0x2f, 0xdf, 0x96, 0x39}}

class nsIMdbTable : public nsIMdbCollection { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBTABLE_IID)


  
  NS_IMETHOD SetTablePriority(nsIMdbEnv* ev, mdb_priority inPrio) = 0;
  NS_IMETHOD GetTablePriority(nsIMdbEnv* ev, mdb_priority* outPrio) = 0;
  
  NS_IMETHOD GetTableBeVerbose(nsIMdbEnv* ev, mdb_bool* outBeVerbose) = 0;
  NS_IMETHOD SetTableBeVerbose(nsIMdbEnv* ev, mdb_bool inBeVerbose) = 0;
  
  NS_IMETHOD GetTableIsUnique(nsIMdbEnv* ev, mdb_bool* outIsUnique) = 0;
  
  NS_IMETHOD GetTableKind(nsIMdbEnv* ev, mdb_kind* outTableKind) = 0;
  NS_IMETHOD GetRowScope(nsIMdbEnv* ev, mdb_scope* outRowScope) = 0;
  
  NS_IMETHOD GetMetaRow(
    nsIMdbEnv* ev, 
    const mdbOid* inOptionalMetaRowOid, 
    mdbOid* outOid, 
    nsIMdbRow** acqRow) = 0; 
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  


  
  NS_IMETHOD GetTableRowCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbTableRowCursor** acqCursor) = 0; 
  

  
  NS_IMETHOD PosToOid( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    mdbOid* outOid) = 0; 

  NS_IMETHOD OidToPos( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid, 
    mdb_pos* outPos) = 0; 
    
  NS_IMETHOD PosToRow( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbRow** acqRow) = 0; 
    
  NS_IMETHOD RowToPos( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow, 
    mdb_pos* outPos) = 0; 
  

  
  NS_IMETHOD AddOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid) = 0; 

  NS_IMETHOD HasOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid, 
    mdb_bool* outHasOid) = 0; 

  NS_IMETHOD CutOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid) = 0; 
  

  
  NS_IMETHOD NewRow( 
    nsIMdbEnv* ev, 
    mdbOid* ioOid, 
    nsIMdbRow** acqRow) = 0; 

  NS_IMETHOD AddRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow) = 0; 

  NS_IMETHOD HasRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow, 
    mdb_bool* outHasRow) = 0; 

  NS_IMETHOD CutRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow) = 0; 

  NS_IMETHOD CutAllRows( 
    nsIMdbEnv* ev) = 0; 
  

  
  NS_IMETHOD SearchColumnsHint( 
    nsIMdbEnv* ev, 
    const mdbColumnSet* inColumnSet) = 0; 
    
  NS_IMETHOD SortColumnsHint( 
    nsIMdbEnv* ev, 
    const mdbColumnSet* inColumnSet) = 0; 
    
  NS_IMETHOD StartBatchChangeHint( 
    nsIMdbEnv* ev, 
    const void* inLabel) = 0; 
    
    
    
    
  NS_IMETHOD EndBatchChangeHint( 
    nsIMdbEnv* ev, 
    const void* inLabel) = 0; 
    
    
    
    
    
    
    
    
    
  

  
  NS_IMETHOD FindRowMatches( 
    nsIMdbEnv* ev, 
    const mdbYarn* inPrefix, 
    nsIMdbTableRowCursor** acqCursor) = 0; 
    
  NS_IMETHOD GetSearchColumns( 
    nsIMdbEnv* ev, 
    mdb_count* outCount, 
    mdbColumnSet* outColSet) = 0; 
    
    
    
    
    
    
    
    
    
    
    
    
  

  
  
  

  NS_IMETHOD
  CanSortColumn( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdb_bool* outCanSort) = 0; 
    
  NS_IMETHOD GetSorting( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbSorting** acqSorting) = 0; 
    
  NS_IMETHOD SetSearchSorting( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbSorting* ioSorting) = 0; 
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  

  
  
  
  NS_IMETHOD MoveOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_pos inHintFromPos, 
    mdb_pos inToPos,       
    mdb_pos* outActualPos) = 0; 

  NS_IMETHOD MoveRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow,  
    mdb_pos inHintFromPos, 
    mdb_pos inToPos,       
    mdb_pos* outActualPos) = 0; 
  
  
  
  NS_IMETHOD AddIndex( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbThumb** acqThumb) = 0; 
  
  
  
  NS_IMETHOD CutIndex( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbThumb** acqThumb) = 0; 
  
  
  
  NS_IMETHOD HasIndex( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdb_bool* outHasIndex) = 0; 

  
  NS_IMETHOD EnableIndexOnSort( 
    nsIMdbEnv* ev, 
    mdb_column inColumn) = 0; 
  
  NS_IMETHOD QueryIndexOnSort( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdb_bool* outIndexOnSort) = 0; 
  
  NS_IMETHOD DisableIndexOnSort( 
    nsIMdbEnv* ev, 
    mdb_column inColumn) = 0; 
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbTable, NS_IMDBTABLE_IID)






























class nsIMdbSorting : public nsIMdbObject { 
public:


  
  
  
  
  NS_IMETHOD GetTable(nsIMdbEnv* ev, nsIMdbTable** acqTable) = 0;
  NS_IMETHOD GetSortColumn( 
    nsIMdbEnv* ev, 
    mdb_column* outColumn) = 0; 

  NS_IMETHOD SetNewCompare(nsIMdbEnv* ev,
    nsIMdbCompare* ioNewCompare) = 0;
    
    
    
    
    
    

  NS_IMETHOD GetOldCompare(nsIMdbEnv* ev,
    nsIMdbCompare** acqOldCompare) = 0;
    
    
    
    
    
    
  
  

  
  NS_IMETHOD GetSortingRowCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbTableRowCursor** acqCursor) = 0; 
    
  

  
  NS_IMETHOD PosToOid( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    mdbOid* outOid) = 0; 
    
  NS_IMETHOD PosToRow( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbRow** acqRow) = 0; 
  


};


























#define NS_IMDBTABLEROWCURSOR_IID_STR = "4f325dad-0385-4b62-a992-c914ab93587e"

#define NS_IMDBTABLEROWCURSOR_IID \
{0x4f325dad, 0x0385, 0x4b62, \
{0xa9, 0x92, 0xc9, 0x14, 0xab, 0x93, 0x58, 0x7e}}



class nsIMdbTableRowCursor : public nsISupports { 
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBTABLEROWCURSOR_IID)



  
  
  
  
  NS_IMETHOD GetTable(nsIMdbEnv* ev, nsIMdbTable** acqTable) = 0;
  

  
  NS_IMETHOD CanHaveDupRowMembers(nsIMdbEnv* ev, 
    mdb_bool* outCanHaveDups) = 0;
    
  NS_IMETHOD MakeUniqueCursor( 
    nsIMdbEnv* ev, 
    nsIMdbTableRowCursor** acqCursor) = 0;    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  

  
  NS_IMETHOD NextRowOid( 
    nsIMdbEnv* ev, 
    mdbOid* outOid, 
    mdb_pos* outRowPos) = 0;    
  

  
  NS_IMETHOD NextRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow, 
    mdb_pos* outRowPos) = 0;    

  NS_IMETHOD PrevRowOid( 
    nsIMdbEnv* ev, 
    mdbOid* outOid, 
    mdb_pos* outRowPos) = 0;    
  

  
  NS_IMETHOD PrevRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow, 
    mdb_pos* outRowPos) = 0;    

  

  
  
  
  
  
  
  
  
  
  
  
  
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbTableRowCursor, NS_IMDBTABLEROWCURSOR_IID)





#define NS_IMDBROW_IID_STR "271e8d6e-183a-40e3-9f18-36913b4c7853"


#define NS_IMDBROW_IID \
{0x271e8d6e, 0x183a, 0x40e3, \
{0x9f, 0x18, 0x36, 0x91, 0x3b, 0x4c, 0x78, 0x53}}


class nsIMdbRow : public nsIMdbCollection { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBROW_IID)


  
  NS_IMETHOD GetRowCellCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inCellPos, 
    nsIMdbRowCellCursor** acqCursor) = 0; 
  

  
  NS_IMETHOD AddColumn( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    const mdbYarn* inYarn) = 0; 

  NS_IMETHOD CutColumn( 
    nsIMdbEnv* ev, 
    mdb_column inColumn) = 0; 

  NS_IMETHOD CutAllColumns( 
    nsIMdbEnv* ev) = 0; 
  

  
  NS_IMETHOD NewCell( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbCell** acqCell) = 0; 
    
  NS_IMETHOD AddCell( 
    nsIMdbEnv* ev, 
    const nsIMdbCell* inCell) = 0; 
    
  NS_IMETHOD GetCell( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbCell** acqCell) = 0; 
    
  NS_IMETHOD EmptyAllCells( 
    nsIMdbEnv* ev) = 0; 
  

  
  NS_IMETHOD AddRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioSourceRow) = 0; 
    
  NS_IMETHOD SetRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioSourceRow) = 0; 
  

  
  NS_IMETHOD SetCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    const mdbYarn* inYarn) = 0;   
  
  
  NS_IMETHOD GetCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdbYarn* outYarn) = 0;  
  
  
  NS_IMETHOD AliasCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdbYarn* outYarn) = 0; 
  
  NS_IMETHOD NextCellYarn(nsIMdbEnv* ev, 
    mdb_column* ioColumn, 
    mdbYarn* outYarn) = 0;  
  
  
  
  
  
  
  
  
  
  
  
  

  NS_IMETHOD SeekCellYarn( 
    nsIMdbEnv* ev, 
    mdb_pos inPos, 
    mdb_column* outColumn, 
    mdbYarn* outYarn) = 0; 
  
  
  
  

  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbRow, NS_IMDBROW_IID)











#define NS_IMDBROWCELLCURSOR_IID_STR "b33371a7-5d63-4d10-85a8-e44dffe75c28"


#define NS_IMDBROWCELLCURSOR_IID \
{0x271e8d6e, 0x5d63, 0x4d10 , \
{0x85, 0xa8, 0xe4, 0x4d, 0xff, 0xe7, 0x5c, 0x28}}


class nsIMdbRowCellCursor : public nsISupports{ 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBROWCELLCURSOR_IID)


  
  NS_IMETHOD SetRow(nsIMdbEnv* ev, nsIMdbRow* ioRow) = 0; 
  NS_IMETHOD GetRow(nsIMdbEnv* ev, nsIMdbRow** acqRow) = 0;
  

  
  NS_IMETHOD MakeCell( 
    nsIMdbEnv* ev, 
    mdb_column* outColumn, 
    mdb_pos* outPos, 
    nsIMdbCell** acqCell) = 0; 
  

  
  NS_IMETHOD SeekCell( 
    nsIMdbEnv* ev, 
    mdb_pos inPos, 
    mdb_column* outColumn, 
    nsIMdbCell** acqCell) = 0; 
  

  
  NS_IMETHOD NextCell( 
    nsIMdbEnv* ev, 
    nsIMdbCell** acqCell, 
    mdb_column* outColumn, 
    mdb_pos* outPos) = 0; 
    
  NS_IMETHOD PickNextCell( 
    nsIMdbEnv* ev, 
    nsIMdbCell* ioCell, 
    const mdbColumnSet* inFilterSet, 
    mdb_column* outColumn, 
    mdb_pos* outPos) = 0; 

  
  
  
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbRowCellCursor, NS_IMDBROWCELLCURSOR_IID)





class nsIMdbBlob : public nsISupports { 
public:



  
  NS_IMETHOD SetBlob(nsIMdbEnv* ev,
    nsIMdbBlob* ioBlob) = 0; 
  
  
  NS_IMETHOD ClearBlob( 
    nsIMdbEnv* ev) = 0;
  
  
  NS_IMETHOD GetBlobFill(nsIMdbEnv* ev,
    mdb_fill* outFill) = 0;  
  
  
  
  NS_IMETHOD SetYarn(nsIMdbEnv* ev, 
    const mdbYarn* inYarn) = 0;   
  
  
  NS_IMETHOD GetYarn(nsIMdbEnv* ev, 
    mdbYarn* outYarn) = 0;  
  
  
  NS_IMETHOD AliasYarn(nsIMdbEnv* ev, 
    mdbYarn* outYarn) = 0; 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


};






























#define NS_IMDBCELL_IID \
{0xa3b62f71, 0xa181, 0x4a91, \
{0xb6, 0x6b, 0x27, 0x10, 0x9b, 0x88, 0x98, 0x35}}

#define NS_IMDBCELL_IID_STR = "a3b62f71-a181-4a91-b66b-27109b889835"

class nsIMdbCell : public nsIMdbBlob { 
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMDBTABLEROWCURSOR_IID)


  
  NS_IMETHOD SetColumn(nsIMdbEnv* ev, mdb_column inColumn) = 0; 
  NS_IMETHOD GetColumn(nsIMdbEnv* ev, mdb_column* outColumn) = 0;
  
  NS_IMETHOD GetCellInfo(  
    nsIMdbEnv* ev, 
    mdb_column* outColumn,           
    mdb_fill*   outBlobFill,         
    mdbOid*     outChildOid,         
    mdb_bool*   outIsRowChild) = 0;  

  
  
  
  NS_IMETHOD GetRow(nsIMdbEnv* ev, 
    nsIMdbRow** acqRow) = 0;
  NS_IMETHOD GetPort(nsIMdbEnv* ev, 
    nsIMdbPort** acqPort) = 0;
  

  
  NS_IMETHOD HasAnyChild( 
    nsIMdbEnv* ev,
    mdbOid* outOid,  
    mdb_bool* outIsRow) = 0; 

  NS_IMETHOD GetAnyChild( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow, 
    nsIMdbTable** acqTable) = 0; 


  NS_IMETHOD SetChildRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow) = 0; 

  NS_IMETHOD GetChildRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow) = 0; 


  NS_IMETHOD SetChildTable( 
    nsIMdbEnv* ev, 
    nsIMdbTable* inTable) = 0; 

  NS_IMETHOD GetChildTable( 
    nsIMdbEnv* ev, 
    nsIMdbTable** acqTable) = 0; 
  


};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMdbCell, NS_IMDBTABLEROWCURSOR_IID)





#endif 

