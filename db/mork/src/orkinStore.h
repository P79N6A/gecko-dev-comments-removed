




































#ifndef _ORKINSTORE_
#define _ORKINSTORE_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif

#ifndef _MORKStore_
#include "morkStore.h"
#endif



#define morkMagic_kStore 0x53746F72 /* ascii 'Stor' */
 


class orkinStore : public morkHandle, public nsIMdbStore { 


public: 
  
  virtual ~orkinStore(); 
  
protected: 
  orkinStore(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkStore* ioObject); 
    
  

private: 
  orkinStore(const morkHandle& other);
  orkinStore& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
 
public: 

  static orkinStore* MakeStore(morkEnv* ev, morkStore* ioObject);

public: 

  morkEnv* CanUseStore(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: 
  mork_bool IsOrkinStore() const
  { return mHandle_Magic == morkMagic_kStore; }

  mork_bool IsOrkinStoreHandle() const
  { return this->IsHandle() && this->IsOrkinStore(); }

  NS_DECL_ISUPPORTS


  
  NS_IMETHOD IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly);
  
  

  
  NS_IMETHOD GetMdbFactory(nsIMdbEnv* ev, nsIMdbFactory** acqFactory); 
  

  
  NS_IMETHOD GetWeakRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount);  
  NS_IMETHOD GetStrongRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount);

  NS_IMETHOD AddWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD AddStrongRef(nsIMdbEnv* ev);

  NS_IMETHOD CutWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD CutStrongRef(nsIMdbEnv* ev);
  
  NS_IMETHOD CloseMdbObject(nsIMdbEnv* ev); 
  NS_IMETHOD IsOpenMdbObject(nsIMdbEnv* ev, mdb_bool* outOpen);
  
  




  
  NS_IMETHOD GetIsPortReadonly(nsIMdbEnv* ev, mdb_bool* outBool);
  NS_IMETHOD GetIsStore(nsIMdbEnv* ev, mdb_bool* outBool);
  NS_IMETHOD GetIsStoreAndDirty(nsIMdbEnv* ev, mdb_bool* outBool);

  NS_IMETHOD GetUsagePolicy(nsIMdbEnv* ev, 
    mdbUsagePolicy* ioUsagePolicy);

  NS_IMETHOD SetUsagePolicy(nsIMdbEnv* ev, 
    const mdbUsagePolicy* inUsagePolicy);
  

  
  NS_IMETHOD IdleMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size* outEstimatedBytesFreed); 

  NS_IMETHOD SessionMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size inDesiredBytesFreed, 
    mdb_size* outEstimatedBytesFreed); 

  NS_IMETHOD PanicMemoryPurge( 
    nsIMdbEnv* ev, 
    mdb_size* outEstimatedBytesFreed); 
  

  
  NS_IMETHOD GetPortFilePath(
    nsIMdbEnv* ev, 
    mdbYarn* outFilePath, 
    mdbYarn* outFormatVersion); 

  NS_IMETHOD GetPortFile(
    nsIMdbEnv* ev, 
    nsIMdbFile** acqFile); 
  

  
  NS_IMETHOD BestExportFormat( 
    nsIMdbEnv* ev, 
    mdbYarn* outFormatVersion); 

  NS_IMETHOD
  CanExportToFormat( 
    nsIMdbEnv* ev, 
    const char* inFormatVersion, 
    mdb_bool* outCanExport); 

  NS_IMETHOD ExportToFormat( 
    nsIMdbEnv* ev, 
    
    nsIMdbFile* ioFile, 
    const char* inFormatVersion, 
    nsIMdbThumb** acqThumb); 
  
  

  

  
  NS_IMETHOD TokenToString( 
    nsIMdbEnv* ev, 
    mdb_token inToken, 
    mdbYarn* outTokenName); 
  
  NS_IMETHOD StringToToken( 
    nsIMdbEnv* ev, 
    const char* inTokenName, 
    mdb_token* outToken); 
    
  
  
  
  

  NS_IMETHOD QueryToken( 
    nsIMdbEnv* ev, 
    const char* inTokenName, 
    mdb_token* outToken); 
  
  
  
  

  

  
  NS_IMETHOD HasRow( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_bool* outHasRow); 

  NS_IMETHOD GetRowRefCount( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_count* outRefCount); 
    
  NS_IMETHOD GetRow( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    nsIMdbRow** acqRow); 

  NS_IMETHOD FindRow(nsIMdbEnv* ev, 
    mdb_scope inRowScope,   
    mdb_column inColumn,   
    const mdbYarn* inTargetCellValue, 
    mdbOid* outRowOid, 
    nsIMdbRow** acqRow); 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  NS_IMETHOD HasTable( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    mdb_bool* outHasTable); 
    
  NS_IMETHOD GetTable( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,  
    nsIMdbTable** acqTable); 
  
  NS_IMETHOD HasTableKind( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    mdb_kind inTableKind, 
    mdb_count* outTableCount, 
    mdb_bool* outSupportsTable); 
        
  NS_IMETHOD GetTableKind( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope,      
    mdb_kind inTableKind,      
    mdb_count* outTableCount, 
    mdb_bool* outMustBeUnique, 
    nsIMdbTable** acqTable);       
    
  NS_IMETHOD
  GetPortTableCursor( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    mdb_kind inTableKind, 
    nsIMdbPortTableCursor** acqCursor); 
  


  

  NS_IMETHOD ShouldCompress( 
    nsIMdbEnv* ev, 
    mdb_percent inPercentWaste, 
    mdb_percent* outActualWaste, 
    mdb_bool* outShould); 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  





  
  NS_IMETHOD NewTable( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope,    
    mdb_kind inTableKind,    
    mdb_bool inMustBeUnique, 
    const mdbOid* inOptionalMetaRowOid, 
    nsIMdbTable** acqTable);     
    
  NS_IMETHOD NewTableWithOid( 
    nsIMdbEnv* ev, 
    const mdbOid* inOid,   
    mdb_kind inTableKind,    
    mdb_bool inMustBeUnique, 
    const mdbOid* inOptionalMetaRowOid, 
    nsIMdbTable** acqTable);     
  

  
  NS_IMETHOD RowScopeHasAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned); 

  NS_IMETHOD SetCallerAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned); 

  NS_IMETHOD SetStoreAssignedIds(nsIMdbEnv* ev,
    mdb_scope inRowScope,   
    mdb_bool* outCallerAssigned, 
    mdb_bool* outStoreAssigned); 
  

  
  NS_IMETHOD NewRowWithOid(nsIMdbEnv* ev, 
    const mdbOid* inOid,   
    nsIMdbRow** acqRow); 

  NS_IMETHOD NewRow(nsIMdbEnv* ev, 
    mdb_scope inRowScope,   
    nsIMdbRow** acqRow); 
  
  
  

  
  NS_IMETHOD ImportContent( 
    nsIMdbEnv* ev, 
    mdb_scope inRowScope, 
    nsIMdbPort* ioPort, 
    nsIMdbThumb** acqThumb); 
  
  

  NS_IMETHOD ImportFile( 
    nsIMdbEnv* ev, 
    nsIMdbFile* ioFile, 
    nsIMdbThumb** acqThumb); 
  
  
  

  
  NS_IMETHOD
  ShareAtomColumnsHint( 
    nsIMdbEnv* ev, 
    mdb_scope inScopeHint, 
    const mdbColumnSet* inColumnSet); 

  NS_IMETHOD
  AvoidAtomColumnsHint( 
    nsIMdbEnv* ev, 
    const mdbColumnSet* inColumnSet); 
  

  
  NS_IMETHOD SmallCommit( 
    nsIMdbEnv* ev); 
  
  NS_IMETHOD LargeCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb); 
  
  
  
  

  NS_IMETHOD SessionCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb); 
  
  
  
  

  NS_IMETHOD
  CompressCommit( 
    nsIMdbEnv* ev, 
    nsIMdbThumb** acqThumb); 
  
  
  
  
  


};
 



#endif 
